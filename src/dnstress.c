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

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#include "dnstress.h"
#include "argparser.h"
#include "utils.h"

#define MAX_SENDERS 1
#define MAX_OPEN_FD 1000000

static void worker_signal(evutil_socket_t signal, short events, void *arg) {
    struct dnstress_t *dnstress = (struct dnstress_t *) arg;
    event_base_loopbreak(dnstress->evb);

    /* TODO: PRINT RESULTS */
}

static void worker_pipe(evutil_socket_t fd, short events, void *arg) {
    struct dnstress_t *dnstress = (struct dnstress_t *) arg;
    event_base_loopbreak(dnstress->evb);
}

static void master_signal(evutil_socket_t signal, short events, void *arg) {
    struct event_base *evb = (struct event_base *) arg;
	pid_t pid;

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
		    fatal("[-] Unexpected signal");
		    break;
	}
	event_base_loopbreak(evb);
}

static void worker(struct dnsconfig_t *config, int worker_id, struct process_pipes *pipes) {
    struct dnstress_t *dnstress = NULL;
    struct rlimit lim;

    for (size_t i = 0; i < config->workers_count; i++) {
        close(pipes[i].proc_fd[MASTER_PROC_FD]);
        if (i != worker_id)
            close(pipes[i].proc_fd[WORKER_PROC_FD]);
    }

    lim.rlim_cur = MAX_OPEN_FD;
    lim.rlim_max = MAX_OPEN_FD;

    if (setrlimit(RLIMIT_NOFILE, &lim) < 0)
        fatal("[-] Failed to set rlimit");

    dnstress = dnstress_create(config, pipes[worker_id].proc_fd[WORKER_PROC_FD]);
    
    dnstress_run(dnstress);
    dnstress_free(dnstress);

    exit(0);
}

static void master(void) {
    struct event_base *evb   = NULL;
    struct event *ev_sigint  = NULL;
    struct event *ev_sigterm = NULL;
    struct event *ev_sigchld = NULL;

    if ((evb = event_base_new()) == NULL)
        fatal("Failed to create master event base");
    
    if ((ev_sigint = event_new(evb, SIGINT, 
        EV_SIGNAL | EV_PERSIST, master_signal, evb)) == NULL)
        fatal("[-] Failed to create SIGINT master event");
    
    if ((ev_sigterm = event_new(evb, SIGTERM,
        EV_SIGNAL | EV_PERSIST, master_signal, evb)) == NULL)
        fatal("[-] Failed to create SIGTERM master event");

    if ((ev_sigchld = event_new(evb, SIGCHLD,
        EV_SIGNAL | EV_PERSIST, master_signal, evb)) == NULL)
        fatal("[-] Failed to create SIGCHLD master event");

    if (event_add(ev_sigint, NULL) < 0)
        fatal("[-] Failed to add SIGINT master event");

    if (event_add(ev_sigterm, NULL) < 0)
        fatal("[-] Failed to add SIGTERM master event");

    if (event_add(ev_sigchld, NULL) < 0)
        fatal("[-] Failed to add SIGCHLD master event");

    if (event_base_dispatch(evb) < 0)
        fatal("[-] Fatal to dispatch master event base");
    
    event_free(ev_sigint);
    event_free(ev_sigterm);
    event_free(ev_sigchld);

    event_base_free(evb);
}

struct dnstress_t * dnstress_create(dnsconfig_t *config, int fd) {
    struct dnstress_t *dnstress = xmalloc_0(sizeof(struct dnstress_t));
    
    dnstress->config        = config;
    dnstress->workers_count = config->addrs_count;
    dnstress->max_senders   = MAX_SENDERS;

    dnstress->workers = xmalloc_0(sizeof(struct worker_t) * dnstress->workers_count);

    if ((dnstress->evb = event_base_new()) == NULL)
		fatal("[-] Failed to create event base");
    
    if ((dnstress->ev_sigint = event_new(dnstress->evb, SIGINT,
	    EV_SIGNAL | EV_PERSIST, worker_signal, dnstress)) == NULL)
		fatal("[-] Failed to create SIGINT signal event");

	if ((dnstress->ev_sigterm = event_new(dnstress->evb, SIGTERM,
	    EV_SIGNAL | EV_PERSIST, worker_signal, dnstress)) == NULL)
		fatal("[-] Failed to create SIGTERM signal event");

    if ((dnstress->ev_pipe = event_new(dnstress->evb, fd, 
        EV_READ | EV_PERSIST, worker_pipe, dnstress)) == NULL)
        fatal("[-] Failed to create pipe event");

    if (event_add(dnstress->ev_pipe, NULL) < 0)
        fatal("[-] Failed to add pipe event");

    for (size_t i = 0; i < dnstress->workers_count; i++)
        worker_init(dnstress, i);

    if ((dnstress->pool = thread_pool_init(MAX_THREADS, QUEUE_SIZE)) == NULL)
        fatal("[-] Failed to create thread pool");

    return dnstress;
}

int dnstress_run(struct dnstress_t *dnstress) {
    if (event_add(dnstress->ev_sigint, NULL) < 0)
        fatal("[-] Failed to add SIGINT dnstress event");

    if (event_add(dnstress->ev_sigterm, NULL) < 0)
        fatal("[-] Failed to add SIGTERM dnstress event");

    /* put all works in a queue */
    for (size_t i = 0; i < dnstress->workers_count; i++) {
        thread_pool_add(dnstress->pool, &worker_run, &(dnstress->workers[i]));
    }

    if (event_base_dispatch(dnstress->evb) < 0)
		fatal("[-] Failed to dispatch dnstress event base");
    
    return 0;
}

int dnstress_free(struct dnstress_t *dnstress) {
    if (dnstress == NULL) return 0;

    dnsconfig_free(dnstress->config);
    
    for (size_t i = 0; i < dnstress->workers_count; i++)
        worker_clear(&(dnstress->workers[i]));
    
    event_base_free(dnstress->evb);
    thread_pool_kill(dnstress->pool, complete_shutdown);

    free(dnstress->workers);
    free(dnstress);
    
    return 0;
}

int main(int argc, char **argv) {
    dnsconfig_t *config = NULL;

    config = dnsconfig_create();

    parse_args(argc, argv, config);
    
    if (parse_config(config, config->configfile) < 0)
        fatal("[-] Error while parsing config");

    struct process_pipes *pipes = xmalloc_0(sizeof(struct process_pipes) * config->workers_count);

    for (size_t i = 0; i < config->workers_count; i++)
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipes->proc_fd) < 0)
            fatal("[-] Failed to create socketpair");

    for (size_t i = 0; i < config->workers_count; i++) {
        pid_t pid = fork();
        switch (pid) {
            case -1:
                fatal("[-] Failed to fork a process");
                break;
            case 0:
                worker(config, i, pipes);
                break;
            default:
                close(pipes[i].proc_fd[WORKER_PROC_FD]);
        }
    }

    master();
    
    free(pipes);

    return 0;
}