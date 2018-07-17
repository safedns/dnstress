#include <string.h>
#include <unistd.h>

#include "argparser.h"
#include "defines.h"
#include "utils.h"

#define V_MODE        1
#define V_WCOUNT      2
#define V_HELP        7
#define V_CONF        9

static m_entity_t table[] = { { MODE,       V_MODE },
                              { WCOUNT,     V_WCOUNT },
                              { HELP,       V_HELP },
                              { U_VALID,    UDP_VALID },
                              { T_VALID,    TCP_VALID },
                              { U_NONVALID, UDP_NONVALID },
                              { T_NONVALID, TCP_NONVALID },
                              { _SHUFFLE,   SHUFFLE },
                              { CONF,       V_CONF } };

#define N_ENTITY (sizeof(table) / sizeof(m_entity_t))

static size_t get_value(char *key) {
    for (size_t i = 0; i < N_ENTITY; i++) {
        if (strcmp(key, table[i].key) == 0) return table[i].value;
    }
    return -1;
}

void usage(void) {
    fprintf(stderr, "Usage: ./dnstress [--mode m] [--wcount c] [--help]\n");
    fprintf(stderr, "       --mode: mode for traffic generating (low-valid by default). Available modes:\n");
    fprintf(stderr, "           udp-valid:    UDP valid packets\n");
    fprintf(stderr, "           tcp-valid:    TCP valid packets\n");
    fprintf(stderr, "           udp-nonvalid: UDP non-valid (to be blocked) packets\n");
    fprintf(stderr, "           tcp-nonvalid: TCP non-valid (to be blocked) packets\n");
    fprintf(stderr, "           shuffle: random packets generating\n");
    fprintf(stderr, "       --wcount: number of workers (100 by default)\n");
    fprintf(stderr, "       --conf: file with dnstress configuration\n");
    
    exit(1);
}

void parse_args(int argc, char **argv, dnsconfig_t *config) {
    char *ptr        = NULL;
    char *configfile = "./dnstress.json";

    for (size_t i = 1; i < argc; i++) {
        switch (get_value(argv[i])) {
            case V_MODE:
                i++;
                if (i >= argc) fatal("[-] Error in command line arguments");
                switch (get_value(argv[i])) {
                    case UDP_VALID:
                        config->mode = UDP_VALID;
                        break;
                    case UDP_NONVALID:
                        config->mode = UDP_NONVALID;
                        break;
                    case TCP_VALID:
                        config->mode = TCP_VALID;
                        break;
                    case TCP_NONVALID:
                        config->mode = TCP_NONVALID;
                        break;
                    case SHUFFLE:
                        config->mode = SHUFFLE;
                        break;
                    default:
                        fatal("[-] Invalid mode parameter");
                        break;
                }
                break;
            case V_WCOUNT:
                i++;
                if (i >= argc) fatal("[-] Error in command line arguments");
                if (is_negative_int(argv[i])) fatal("[-] Workers count have to be a positive number");
                size_t ret = strtoull(argv[i], &ptr, 10);
                if (ret == 0) fatal("[-] Workers count have to be an int");
                else 
                    config->wcount = max(ret, MIN_WCOUNT);
                break;
            case V_HELP:
                usage();
                break;
            case V_CONF:
                i++;
                if (i >= argc) fatal("[-] Error in command line arguments");
                config->configfile = argv[i];
                break;
            default:
                fatal("[-] Unexpected parameter");
                break;
        }
    }
    if (config->wcount == 0) config->wcount = DEFAULT_COUNT;
    if (config->mode   == 0) config->mode   = UDP_VALID;
    if (config->configfile == NULL) config->configfile = configfile;
}