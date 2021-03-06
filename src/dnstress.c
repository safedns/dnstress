/**
 * dnstress is a tool for generating DNS traffic in
 * different modes for DNS servers. In other words,
 * this is a stress-test perfomance tool. 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <alloca.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#include "dnstress.h"
#include "argparser.h"
#include "utils.h"
#include "log.h"
#include "proc.h"

#define MAX_UDP_SERVANTS 20
#define MAX_TCP_SERVANTS 20
#define MAX_WORKERS      1000

#define MAX_OPEN_FD 1000000

#define PROJNAME "dnstress"

/* subsidiary structure for communicating between 
 * workers and master process
 */
struct __mst {
    struct event_base *evb;
    pid_t *pids;
    size_t pids_count;
    
    struct process_pipes *pipes;

    struct rstats_t *stats;
};

static void
send_stats_worker(const struct rstats_t *stats,
    const struct process_pipes *pipes, const size_t worker_id)
{
    int fd = pipes[worker_id].proc_fd[WORKER_PROC_FD];

    ssize_t wrote = proc_transmit_rstats(fd, stats);
 
    log_info("proc-worker: %zu | wrote %ld bytes to master", worker_id, wrote);
}

static void
recv_stats_master(const evutil_socket_t fd, short events, void *arg)
{
    struct rstats_t *stats   = arg;
    struct rstats_t *r_stats = stats_create(stats->config);

    ssize_t recv = proc_obtain_rstats(fd, r_stats);

    log_info("master: received %ld bytes from a worker", recv);

    stats_update_stats(stats, r_stats);    
    stats_free(r_stats);
}

static void
dnstress_workers_disable(const struct dnstress_t *dnstress)
{
    for (size_t i = 0; i < dnstress->workers_count; i++) {
        worker_deactivate(&dnstress->workers[i]);
    }
}

bool exit_msg_printed = false;

static void
dnstress_evb_close(const struct dnstress_t *dnstress)
{
    struct timeval tv;
    
    dnstress_workers_disable(dnstress);

    if (!exit_msg_printed) {
        fprintf(stderr, "proc-worker: %zu | Please, wait! dnstress is closing... (~5sec)\n",
            dnstress->proc_worker_id);
        exit_msg_printed = true;
    }

    timerclear(&tv);
    tv.tv_sec  = 5;
    event_base_loopexit(dnstress->evb, &tv);
}

static void
pworker_signal_cb(evutil_socket_t signal, short events, void *arg)
{
    /* we don't want to exit because of this signal */
    if (signal == SIGPIPE)
        return;

    log_info("proc-worker: got fatal signal %d", signal);

    struct dnstress_t *dnstress = (struct dnstress_t *) arg;
    dnstress_evb_close(dnstress);
}

static void
pworker_pipe_cb(evutil_socket_t fd, short events, void *arg)
{
    log_info("proc-worker: EOF on a master pipe, terminating");
    
    struct dnstress_t *dnstress = (struct dnstress_t *) arg;
    dnstress_evb_close(dnstress);
}

static void
pworker(struct dnsconfig_t *config, const int proc_worker_id,
    const struct process_pipes *pipes)
{
    signal(SIGPIPE, SIG_IGN);

    if (config == NULL)
        fatal("%s: null pointer to config", __func__);
    if (pipes == NULL)
        fatal("%s: null pointer to pipes", __func__);

    struct dnstress_t *dnstress = NULL;
    struct rlimit lim;

    for (size_t i = 0; i < config->workers_count; i++) {
        close(pipes[i].proc_fd[MASTER_PROC_FD]);
        if (i != proc_worker_id)
            close(pipes[i].proc_fd[WORKER_PROC_FD]);
    }

    lim.rlim_cur = MAX_OPEN_FD;
    lim.rlim_max = MAX_OPEN_FD;

    if (setrlimit(RLIMIT_NOFILE, &lim) < 0)
        fatal("%s: failed to set rlimit", __func__);

    dnstress = dnstress_create(config,
        pipes[proc_worker_id].proc_fd[WORKER_PROC_FD], proc_worker_id);

    log_info("proc-worker: %d | dnstress created", proc_worker_id);
    log_info("proc-worker: %d | dnstress running", proc_worker_id);
    fprintf(stderr, "proc-worker: %d | starting dnstress...\n", proc_worker_id);

    dnstress_run(dnstress);

    send_stats_worker(dnstress->stats, pipes, proc_worker_id);
    
    if (dnstress_free(dnstress) < 0)
        fatal("%s: failed to free dnstress", __func__);

    log_info("proc-worker: %d  | dnstress closing", proc_worker_id);

    exit(0);
}

static void
master_close_workers(struct __mst *mst)
{
    int status = 0;
    
    for (size_t i = 0; i < mst->pids_count; i++) {
        /* FIXME: infinite waiting while killing a child process */
        if (kill(mst->pids[i], SIGINT) < 0)
            log_warn("failed to kill child process");
        waitpid(mst->pids[i], &status, 0);
    }
}

static void
master_signal_cb(evutil_socket_t signal, short events, void *arg)
{
    struct __mst *mst = (struct __mst *) arg;
	pid_t pid;

    log_warn("master: got signal: %d", signal);

	switch (signal) {
	    case SIGTERM:
	    case SIGINT:
		    break;
	    case SIGCHLD:
            do {
			    pid = waitpid(WAIT_ANY, NULL, WNOHANG);
			    if (pid <= 0)
				    continue;
		    } while (pid > 0 || (pid == -1 && errno == EINTR));
		    break;
	    default:
		    fatal("unexpected signal received");
		    break;
	}

    master_close_workers(mst);
	event_base_loopbreak(mst->evb);
}

static void
master(struct dnsconfig_t *config, struct process_pipes *pipes,
    pid_t *pids, const size_t wcount)
{
    signal(SIGPIPE, SIG_IGN);

    struct __mst *mst = alloca(sizeof(struct __mst));
    memset(mst, 0, sizeof(struct __mst));
    
    struct event_base *evb   = NULL;
    struct event *ev_sigint  = NULL;
    struct event *ev_sigterm = NULL;
    struct event *ev_sigchld = NULL;
    
    struct event *ev_pipe[10];

    struct rstats_t *stats = stats_create(config);

    if (stats == NULL)
        fatal("%s: failed to create stats", __func__);

    if ((evb = event_base_new()) == NULL)
        fatal("failed to create master event base");

    mst->evb        = evb;
    mst->pids       = pids;
    mst->pids_count = wcount;
    mst->stats      = stats;
    mst->pipes      = pipes;
    
    if ((ev_sigint = event_new(evb, SIGINT, 
        EV_SIGNAL | EV_PERSIST, master_signal_cb, mst)) == NULL)
        fatal("failed to create SIGINT master event");
    
    if ((ev_sigterm = event_new(evb, SIGTERM,
        EV_SIGNAL | EV_PERSIST, master_signal_cb, mst)) == NULL)
        fatal("failed to create SIGTERM master event");

    if ((ev_sigchld = event_new(evb, SIGCHLD,
        EV_SIGNAL | EV_PERSIST, master_signal_cb, mst)) == NULL)
        fatal("failed to create SIGCHLD master event");

    for (size_t i = 0; i < wcount; i++) {
        if ((ev_pipe[i] = event_new(evb, pipes[i].proc_fd[MASTER_PROC_FD],
            EV_READ | EV_PERSIST, recv_stats_master, stats)) == NULL)
		    fatal("failed to create master pipe event");
    }

    if (event_add(ev_sigint, NULL) < 0)
        fatal("failed to add SIGINT master event");

    if (event_add(ev_sigterm, NULL) < 0)
        fatal("failed to add SIGTERM master event");

    if (event_add(ev_sigchld, NULL) < 0)
        fatal("failed to add SIGCHLD master event");

    for (size_t i = 0; i < wcount; i++) {
        if (event_add(ev_pipe[i], NULL) < 0)
            fatal("failed to add pipe event");
    }

    if (event_base_dispatch(evb) < 0)
        fatal("fatal to dispatch master event base");

    /* receive stats from workers if master was terminated */
    for (size_t i = 0; i < wcount; i++)
        recv_stats_master(pipes[i].proc_fd[MASTER_PROC_FD], 0, stats);

    print_stats(stats);
    
    stats_free(stats);

    event_free(ev_sigint);
    event_free(ev_sigterm);
    event_free(ev_sigchld);

    for (size_t i = 0; i < wcount; i++)
        event_free(ev_pipe[i]);

    event_base_free(evb);
}

static void
dnstress_timeout_cb(evutil_socket_t fd, short events, void *arg)
{
    struct dnstress_t *dnstress = arg;

    log_warn("worker %zu | dnstress: ttl", dnstress->proc_worker_id);

    dnstress_evb_close(dnstress);
}

struct dnstress_t *
dnstress_create(struct dnsconfig_t *config, const int fd,
    const size_t proc_worker_id)
{   
    if (config == NULL)
        fatal("%s: null pointer to config", __func__);
    if (fd < 0)
        fatal("%s: invalid pipe fd", __func__);
    if (proc_worker_id < 0)
        fatal("%s: invalid process worker id", __func__);
    
    struct dnstress_t *dnstress = xmalloc_0(sizeof(struct dnstress_t));

    if (dnstress == NULL)
        fatal("%s: null pointer to dnstress", __func__);
    
    dnstress->config        = config;
    dnstress->stats         = stats_create(config);
    dnstress->workers_count = config->addrs_count;

    dnstress->proc_worker_id = proc_worker_id;
    
    dnstress->max_udp_servants = MAX_UDP_SERVANTS;
    dnstress->max_tcp_servants = MAX_TCP_SERVANTS;

    if (dnstress->stats == NULL)
        fatal("error while creating stats");

    dnstress->workers = xmalloc_0(sizeof(struct worker_t) * dnstress->workers_count);

    if (dnstress->workers == NULL)
        fatal("%s: null pointer to dnstress workers", __func__);

    if ((dnstress->evb = event_base_new()) == NULL)
		fatal("failed to create event base");
    
    if ((dnstress->ev_sigint = event_new(dnstress->evb, SIGINT,
	    EV_SIGNAL | EV_PERSIST, pworker_signal_cb, dnstress)) == NULL)
		fatal("failed to create SIGINT signal event");

	if ((dnstress->ev_sigterm = event_new(dnstress->evb, SIGTERM,
	    EV_SIGNAL | EV_PERSIST, pworker_signal_cb, dnstress)) == NULL)
		fatal("failed to create SIGTERM signal event");
    
    if ((dnstress->ev_sigsegv = event_new(dnstress->evb, SIGSEGV,
	    EV_SIGNAL | EV_PERSIST, pworker_signal_cb, dnstress)) == NULL)
		fatal("failed to create SIGSEGV signal event");

    if ((dnstress->ev_sigpipe = event_new(dnstress->evb, SIGPIPE,
        EV_SIGNAL | EV_PERSIST, pworker_signal_cb, dnstress)) == NULL)
        fatal("failed to create SIGPIPE signal event");

    if ((dnstress->ev_pipe = event_new(dnstress->evb, fd,
        EV_READ | EV_PERSIST, pworker_pipe_cb, dnstress)) == NULL)
        fatal("failed to create pipe event");

    if ((dnstress->ev_timeout = event_new(dnstress->evb, -1,
        EV_PERSIST, dnstress_timeout_cb, dnstress)) == NULL)
		fatal("failed to create timeout timer event");

    if (event_add(dnstress->ev_pipe, NULL) < 0)
        fatal("failed to add pipe event");

    for (size_t i = 0; i < dnstress->workers_count; i++)
        worker_init(dnstress, i);
    
    log_info("proc-worker | workers are configurated");

    if ((dnstress->pool = thread_pool_init(MAX_THREADS, QUEUE_SIZE)) == NULL)
        fatal("failed to create thread pool");

    return dnstress;
}

int
dnstress_run(const struct dnstress_t *dnstress)
{
    struct timeval tv;
    
    if (event_add(dnstress->ev_sigint, NULL) < 0)
        fatal("failed to add SIGINT dnstress event");

    if (event_add(dnstress->ev_sigterm, NULL) < 0)
        fatal("failed to add SIGTERM dnstress event");

    if (event_add(dnstress->ev_sigpipe, NULL) < 0)
        fatal("failed to add SIGPIPE dnstress event");

    timerclear(&tv);
    tv.tv_sec = dnstress->config->ttl;
    if (dnstress->config->ttl > 0) {
        if (event_add(dnstress->ev_timeout, &tv) < 0)
            fatal("failed to add timeout timer event");
    }

    // if (event_add(dnstress->ev_sigsegv, NULL) < 0)
    //     fatal("failed to add SIGSEGV dnstress event");

    /* put all works in a queue */
    for (size_t i = 0; i < dnstress->workers_count; i++) {
        thread_pool_add(dnstress->pool, &worker_run, &(dnstress->workers[i]));
    }

    if (event_base_dispatch(dnstress->evb) < 0)
		fatal("failed to dispatch dnstress event base");
    
    return 0;
}

int
dnstress_free(struct dnstress_t *dnstress)
{
    if (dnstress == NULL)
        return DNSTRESS_NULL;

    if (thread_pool_kill(dnstress->pool, complete_shutdown) < 0)
        return THREADPOOL_KILL_ERROR;

    for (size_t i = 0; i < dnstress->workers_count; i++)
        worker_clear(&(dnstress->workers[i]));
    
    event_base_free(dnstress->evb);
    stats_free(dnstress->stats);

    event_free(dnstress->ev_sigint);
    event_free(dnstress->ev_sigterm);
    event_free(dnstress->ev_pipe);

    free(dnstress->workers);

    dnstress->config  = NULL;
    dnstress->workers = NULL;
    dnstress->pool    = NULL;
    dnstress->stats   = NULL;

    free(dnstress);
    
    return 0;
}

int
main(int argc, char **argv)
{
    struct dnsconfig_t *config = NULL;

    log_init(PROJNAME, 0);
    log_info("=================================");

    config = dnsconfig_create();

    if (config == NULL)
        fatal("error while creating dnsconfig");

    parse_args(argc, argv, config);
    
    if (parse_config(config, config->configfile) < 0)
        fatal("error while parsing config");

    struct process_pipes *pipes = xmalloc_0(sizeof(struct process_pipes) * config->workers_count);
    
    pid_t  *pids = alloca(sizeof(pid_t) * config->workers_count);

    for (size_t i = 0; i < config->workers_count; i++)
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipes[i].proc_fd) < 0)
            fatal("failed to create socketpair");

    fprintf(stderr, "Enter Ctrl+C to stop dnstress\n\n");

    for (size_t i = 0; i < config->workers_count; i++) {
        pid_t pid = fork();
        switch (pid) {
            case -1:
                fatal("failed to fork a process");
                break;
            case 0:
                pworker(config, i, pipes);
                break;
            default:
                close(pipes[i].proc_fd[WORKER_PROC_FD]);
                break;
        }
        pids[i] = pid;
    }

    fprintf(stderr, "[+] process workers are starting...\n\n");

    master(config, pipes, pids, config->workers_count);

    free(pipes);
    dnsconfig_free(config);

    return 0;
}