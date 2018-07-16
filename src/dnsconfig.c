#include <string.h>
#include <unistd.h>

#include "jsmn.h"
#include "dnsconfig.h"
#include "utils.h"
#include "network.h"

static int init_config(dnsconfig_t *config, char *content, jsmntok_t *tokens, int count) {
    if (config == NULL)  return null_config;
    if (content == NULL) return null_content_error;
    if (tokens == NULL)  return null_tokens_error;

    char obj[MAX_TOKEN_SIZE];
    struct sockaddr_in  sock4;
    struct sockaddr_in6 sock6;

    for (size_t i = 1; i < count; i++) {
        get_object(tokens, content, i, obj);
        if (tokens[i].type == JSMN_STRING && strcmp(obj, ADDRS_TOK) == 0) {
            i++;
            if (tokens[i].type != JSMN_ARRAY) fatal("[-] Error in configuration json file");
            size_t addrs_len = tokens[i].size;
            
            config->addrs = xmalloc_0(addrs_len * sizeof(struct sockaddr_storage));
            config->addrs_count = addrs_len;

            for (size_t j = 0; j < addrs_len; j++) {
                i++;
                get_object(tokens, content, i, obj);
                switch (get_addrfamily(obj)) {
                    case AF_INET:
                        memset(&sock4, 0, sizeof(sock4));
                        sock4.sin_family = AF_INET;
                        inet_pton(AF_INET, obj, &(sock4.sin_addr));
                        memcpy(&(config->addrs[j]), &sock4, sizeof(sock4));
                        break;
                    case AF_INET6:
                        memset(&sock6, 0, sizeof(sock6));
                        sock6.sin6_family = AF_INET6;
                        inet_pton(AF_INET, obj, &(sock6.sin6_addr));
                        memcpy(&(config->addrs[j]), &sock6, sizeof(sock6));
                        break;
                    default:
                        fatal("[-] Unknown address family");
                }
            }
        }
    }
    return 0;
}

dnsconfig_t * dnsconfig_create(void) {
    dnsconfig_t * config = xmalloc_0(sizeof(dnsconfig_t));
    return config;
}

void dnsconfig_free(dnsconfig_t * config) {
    if (config->addrs != NULL) free(config->addrs);
    free(config);
}

int parse_config(dnsconfig_t *config, char *filename) {
    if (config == NULL)   return null_config;
    if (filename == NULL) return null_filename;
    
    size_t size       = START_SIZE;
    size_t exact_size = 0;
    char *content     = (char *) xmalloc_0(size * sizeof(char));

    if (access(filename, F_OK) == -1) fatal("[-] No such config file");
    if ((exact_size = read_file(filename, &content, &size)) < 0) fatal("[-] Error while reading config");

    jsmn_parser jparser;
    jsmntok_t   tokens[MAX_NUMBER_OF_TOKENS];
    jsmn_init(&jparser);

    int count = jsmn_parse(&jparser, content, exact_size, tokens, sizeof(tokens) / sizeof(jsmntok_t));

    /* Error handling*/
    if (count == JSMN_ERROR_PART) fatal("[-] Add extra empty line at the end of dnsconfig");
    if (count < 0) return failed_parse_json;
    if (count < 1 || tokens[0].type != JSMN_OBJECT) return top_elem_not_object;

    log_info("[~] Started config loading");
    init_config(config, content, tokens, count);
    log_info("[+] Config loaded");

    free(content);
    
    return 0;
}