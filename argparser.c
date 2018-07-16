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
                              { L_VALID,    LOW_VALID },
                              { H_VALID,    HIGH_VALID },
                              { L_NONVALID, LOW_NONVALID },
                              { H_NONVALID, HIGH_NONVALID },
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
    fprintf(stderr, "           low-valid:     low sized (UDP) valid packets\n");
    fprintf(stderr, "           high-valid:    high sized (TCP) valid packets\n");
    fprintf(stderr, "           low-nonvalid:  low sized (UDP) non-valid (to be blocked) packets\n");
    fprintf(stderr, "           high-nonvalid: high sized (TCP) non-valid (to be blocked) packets\n");
    fprintf(stderr, "           any: random packets generating\n");
    fprintf(stderr, "       --wcount: number of workers (100 by default)\n");
    fprintf(stderr, "       --conf: file with dnstress configuration\n");
    
    exit(1);
}

void parse_args(int argc, char **argv, dnsconfig_t *config) {
    char *ptr        = NULL;
    char *configfile = "dnstress.json";

    for (size_t i = 1; i < argc; i++) {
        switch(get_value(argv[i])) {
            case V_MODE:
                i++;
                if (i >= argc) fatal("[-] Error in command line arguments\n");
                switch(get_value(argv[i])) {
                    case LOW_VALID:
                        config->mode = LOW_VALID;
                        break;
                    case LOW_NONVALID:
                        config->mode = LOW_NONVALID;
                        break;
                    case HIGH_VALID:
                        config->mode = HIGH_VALID;
                        break;
                    case HIGH_NONVALID:
                        config->mode = HIGH_NONVALID;
                        break;
                    case SHUFFLE:
                        config->mode = SHUFFLE;
                        break;
                    default:
                        fatal("[-] Invalid mode parameter\n");
                        break;
                }
                break;
            case V_WCOUNT:
                i++;
                if (i >= argc) fatal("[-] Error in command line arguments\n");
                if (is_negative_int(argv[i])) fatal("[-] Workers count have to be a positive number\n");
                size_t ret = strtoull(argv[i], &ptr, 10);
                if (ret == 0) fatal("[-] Workers count have to be an int\n");
                else 
                    config->wcount = max(ret, MIN_WCOUNT);
                break;
            case V_HELP:
                usage();
                break;
            case V_CONF:
                i++;
                if (i >= argc) fatal("[-] Error in command line arguments\n");
                config->configfile = argv[i];
                break;
            default:
                fatal("[-] Unexpected parameter\n");
                break;
        }
    }
    if (config->wcount == 0) config->wcount = DEFAULT_COUNT;
    if (config->mode   == 0) config->mode   = LOW_VALID;
    if (config->configfile == NULL) config->configfile = configfile;
}