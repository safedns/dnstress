#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <argp.h>

#include "argparser.h"
#include "defines.h"
#include "utils.h"
#include "log.h"

#define V_MODE  1
#define V_HELP  7
#define V_CONF  9
#define V_RPS   10
#define V_LDLVL 11

static m_entity_t table[] = {
    { U_VALID,    UDP_VALID },
    { T_VALID,    TCP_VALID },
    { U_NONVALID, UDP_NONVALID },
    { T_NONVALID, TCP_NONVALID },
    { _SHUFFLE,   SHUFFLE }
 };

#define N_ENTITY (sizeof(table) / sizeof(m_entity_t))

const char *argp_program_version = "dnstress 1.0";

struct arguments {
    int verbose;         /* The -v flag */
    request_mode_t mode; /* mode for traffic generation */
    char *outfile;       /* Argument for -o */
    char *config;        /* configuration file */
    uint32_t rps;        /* requests per second */
    uint16_t ld_lvl;     /* load level */
};

static struct argp_option options[] = {
    { "verbose", 'v', 0,         0, "produce verbose output" },
    { "mode",    'm', "MODE",    0, "mode for traffic generating (low-valid by default)\n"
                                    "  • udp-valid:    UDP valid packets\n"
                                    "  • tcp-valid:    TCP valid packets\n"
                                    "  • udp-nonvalid: UDP non-valid (to be blocked) packets\n"
                                    "  • tcp-nonvalid: TCP non-valid (to be blocked) packets\n"
                                    "  • shuffle: random packets generating\n" },
    { "output",  'o', "OUTFILE", 0, "Output to OUTFILE instead of to standard output" },
    { "config",  'c', "CONFIG",  0, "file with dnstress configuration" },
    { "rps",     'r', "RPS",     0, "number of requests per second" },
    { "ld-lvl",  'l', "LD-LVL",  0, "load level. from 1 to 10" },
    { 0 }
};

static char args_doc[] = "";

static char doc[] = "A program that provides performance stress tests for defined DNS servers.";

static size_t
get_value(const char *key)
{
    for (size_t i = 0; i < N_ENTITY; i++) {
        if (strcmp(key, table[i].key) == 0)
            return table[i].value;
    }
    return -1;
}

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    char *pEnd = NULL;
    
    struct arguments *arguments = state->input;

    switch (key) {
        case 'v':
            arguments->verbose = 1;
            break;
        case 'm':
            switch (get_value(arg)) {
                case UDP_VALID:
                    arguments->mode = UDP_VALID;
                    break;
                case UDP_NONVALID:
                    arguments->mode = UDP_NONVALID;
                    break;
                case TCP_VALID:
                    arguments->mode = TCP_VALID;
                    break;
                case TCP_NONVALID:
                    arguments->mode = TCP_NONVALID;
                    break;
                case SHUFFLE:
                    arguments->mode = SHUFFLE;
                    break;
                default:
                    argp_usage(state);
                    break;
            }
            break;
        case 'o':
            arguments->outfile = arg;
            break;
        case 'c':
            arguments->config = arg;
            break;
        case 'r':
            arguments->rps = strtoul(arg, &pEnd, 10);
            if (arguments->rps <= 0) {
                argp_usage(state);
            }
            break;
        case 'l':
            arguments->ld_lvl = strtoul(arg, &pEnd, 10);
            if (arguments->ld_lvl > 10 || arguments->ld_lvl <= 0) {
                argp_usage(state);
            }
            break;
        case ARGP_KEY_ARG:
            // if (state->arg_num >= 2) {
	        //     argp_usage(state);
            // }
            break;
        case ARGP_KEY_END:
            // if (state->arg_num < 2) {
	        //     argp_usage(state);
	        // }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void
parse_args(const int argc, char **argv,
    struct dnsconfig_t *config)
{
    struct arguments arguments;
    FILE *outstream = NULL;

    arguments.outfile = NULL;
    arguments.config  = "./dnstress.json";
    arguments.mode    = UDP_VALID;
    arguments.rps     = 0;
    arguments.ld_lvl  = 5;
    arguments.verbose = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.outfile)
        outstream = fopen(arguments.outfile, "w");
    else
        outstream = stdout;

    config->configfile = arguments.config;
    config->rps        = arguments.rps;
    config->ld_lvl     = arguments.ld_lvl;
    config->outstream  = outstream;
    config->mode       = arguments.mode;
}
