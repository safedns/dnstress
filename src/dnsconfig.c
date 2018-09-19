#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <bsd/stdlib.h>

#include "jsmn.h"
#include "dnsconfig.h"
#include "utils.h"
#include "network.h"
#include "log.h"

static void
init_domain(struct dnsconfig_t *config, size_t index,
    char * key, char *value)
{
    char *ptr = NULL;
    
    if (strcmp(key, DOM_NAME) == 0) {
        config->queries[index].dname = strdup(value);
    } else if (strcmp(key, DOM_BLOCKED) == 0) {
        config->queries[index].blocked = strtol(value, &ptr, 10);
    } else if (strcmp(key, DOM_TYPE) == 0) {
        config->queries[index].type = strdup(value);
    } else
        fatal("unknown domain's field");
}

static void
validate_config(struct dnsconfig_t *config)
{
    size_t valid = 0, nonvalid = 0;

    for (size_t i = 0; i < config->queries_count; i++) {
        if (config->queries[i].blocked)
            nonvalid++;
        else
            valid++;
    }
    
    switch (config->mode) {
        case UDP_VALID:
        case TCP_VALID:
            if (valid == 0)
                fatal("valid mode with no valid domains");
            break;
        case UDP_NONVALID:
        case TCP_NONVALID:
            if (nonvalid == 0)
                fatal("nonvalid mode with no non-valid domains");
            break;
        default:
            break;
    }
}

const char *
parse_sockaddr(struct sockaddr_storage *sock, char *__host, const in_port_t __port)
{
	const char *errstr = NULL;

    char *host     = strdup(__host);
    char *beg_host = host;

    if (host == NULL)
        fatal("%s: failed to strdup host", __func__);
    
    size_t n_colon  = 0;
    size_t host_len = strlen(host);

    in_port_t port = __port;

    /* count colons to determine address type */
    for (size_t i = 0; i < host_len; i++)
        if (host[i] == ':') n_colon++;

    if (n_colon <= 1) {
        /* IPv4 case */

        char *colon = NULL;
        struct sockaddr_in *sin = (struct sockaddr_in *) sock;

        if ((colon = strchr(host, ':')) != NULL) {
            *colon++ = '\0';

            port = strtonum(colon, 0, USHRT_MAX, &errstr);
            if (errstr != NULL)
                return errstr;
        }

        if (inet_pton(AF_INET, host, &(sin->sin_addr)) != 1)
            return "Failed to parse IPv4 address";

        sin->sin_family = AF_INET;
        sin->sin_port   = htons(port);
    } else {
        /* IPv6 case */
        
        char * closed_bracket = NULL;
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sock;
        
        if (host[0] == '[') {
            if ((closed_bracket = strchr(host, ']')) != NULL)
                *closed_bracket++ = '\x00';
            else
                return "Incorrect IPv6 address: unclosed brackets";
            
            host++;
            closed_bracket++;

            port = strtonum(closed_bracket, 0, USHRT_MAX, &errstr);
            if (errstr != NULL)
                return errstr;
        }

        if (inet_pton(AF_INET6, host, &(sin6->sin6_addr)) != 1)
            return "Failed to parse IPv6 address";
        
        sin6->sin6_family = AF_INET6;
        sin6->sin6_port   = htons(port);
    }

    free(beg_host);

	return NULL;
}

static void
process_addrs(struct dnsconfig_t *config, jsmntok_t * tokens, 
    char *content, char *obj, size_t *index)
{
    
    size_t __index = *index;
    
    struct sockaddr_storage sstor;

    if (tokens[__index].type != JSMN_ARRAY) 
        fatal("error in the config json file: array expected");
    
    size_t addrs_len = tokens[__index].size;

    log_info("number of addresses in conf: %zu", addrs_len);
    
    config->addrs = xmalloc_0(addrs_len * sizeof(struct _saddr));
    config->addrs_count = addrs_len;

    for (size_t j = 0; j < addrs_len; j++) {
        const char * errmsg = NULL;
        
        __index++;
        get_object(tokens, content, __index, obj);
        
        memset(&sstor, 0, sizeof(sstor));

        if ((errmsg = parse_sockaddr(&sstor, obj, DNS_PORT)) != NULL)
            fatal("%s: failed to parse IP address: %s", __func__, errmsg);
        
        switch (sstor.ss_family) {
            case AF_INET:
                config->addrs[j].len = sizeof(struct sockaddr_in);
                break;
            case AF_INET6:
                config->addrs[j].len = sizeof(struct sockaddr_in6);
                break;
            default:
                fatal("unknown address family");
        }
        
        memcpy(&(config->addrs[j].addr), &sstor, sizeof(sstor));

        config->addrs[j].id   = j;
        config->addrs[j].repr = strdup(obj);
    }
    *index = __index;
}

static void
process_workers(struct dnsconfig_t *config, jsmntok_t * tokens, 
    char *content, char *obj, size_t *index)
{

    char *ptr = NULL;

    if (tokens[*index].type != JSMN_PRIMITIVE)
        fatal("error in the config file: primitive expected");

    get_object(tokens, content, *index, obj);
    
    if (is_negative_int(obj))
        fatal("workers count have to be a positive number");
    
    
    size_t wcount = strtoull(obj, &ptr, 10);
    config->workers_count = wcount;
    if (wcount >= MAX_WCOUNT)
        config->workers_count = MAX_WCOUNT;
    if (wcount == 0)
        config->workers_count = 1;
}

static void
process_domains(struct dnsconfig_t *config, jsmntok_t *tokens,
    char *content, char *obj, size_t *index)
{

    size_t __index = *index;
    
    if (tokens[__index].type != JSMN_ARRAY) /* TODO: make errors more meaningful */
        fatal("error in the config file: array expected");

    size_t domains_count = tokens[__index++].size;
    config->queries = xmalloc_0(sizeof(struct query_t) * domains_count);
    config->queries_count = domains_count;

    /* iterate through domain structures */
    for (size_t i = 0; i < domains_count; i++, __index++) {
        get_object(tokens, content, __index, obj);

        if (tokens[__index].type != JSMN_OBJECT)
            fatal("error in the config file: object expected");
        
        /* iterate through domain structure itself */
        size_t domain_size = tokens[__index++].size;
        for (size_t j = 0; j < domain_size; j++, __index += 2) {
            if (tokens[__index].type != JSMN_STRING)
                fatal("error in the config file: string expected");

            char key[MAX_TOKEN_SIZE];
            char value[MAX_TOKEN_SIZE];

            get_object(tokens, content, __index, key);
            get_object(tokens, content, __index+1, value);

            init_domain(config, i, key, value);
        }
        __index--;
    }
    *index = __index;
}

static int
init_config(struct dnsconfig_t *config, char *content,
    jsmntok_t *tokens, int count)
{
    if (config == NULL)  return null_config;
    if (content == NULL) return null_content_error;
    if (tokens == NULL)  return null_tokens_error;

    char obj[MAX_TOKEN_SIZE];

    for (size_t i = 1; i < count; i++) {
        get_object(tokens, content, i, obj);
        if (tokens[i].type == JSMN_STRING && strcmp(obj, ADDRS_TOK) == 0) {
            i++;
            process_addrs(config, tokens, content, obj, &i);
        } else if (tokens[i].type == JSMN_STRING && strcmp(obj, WORKERS_TOK) == 0) {
            i++;
            process_workers(config, tokens, content, obj, &i);
        } else if (tokens[i].type == JSMN_STRING && strcmp(obj, DOMAINS_TOK) == 0) {
            i++;
            process_domains(config, tokens, content, obj, &i);
        } else
            fatal("unexpected token in the config file");
    }
    return 0;
}

struct dnsconfig_t *
dnsconfig_create(void)
{
    struct dnsconfig_t * config = xmalloc_0(sizeof(struct dnsconfig_t));
    return config;
}

void
dnsconfig_free(struct dnsconfig_t *config)
{
    if (config == NULL) return;

    for (size_t i = 0; i < config->addrs_count; i++) {
        free(config->addrs[i].repr);
    }

    for (size_t i = 0; i < config->queries_count; i++) {
        free(config->queries[i].dname);
        free(config->queries[i].type);
    }

    free(config->addrs);
    free(config->queries);
    
    config->configfile = NULL;
    config->addrs      = NULL;
    config->queries    = NULL;

    free(config);
}

int
parse_config(struct dnsconfig_t *config, char *filename)
{
    if (config == NULL)   return null_config;
    if (filename == NULL) return null_filename;
    
    size_t size       = START_SIZE;
    size_t exact_size = 0;
    char *content     = (char *) xmalloc_0(size * sizeof(char));

    if (access(filename, F_OK) == -1)
        fatal("no such config file");
    if ((exact_size = read_file(filename, &content, &size)) < 0)
        fatal("error while reading config");

    jsmn_parser jparser;
    jsmntok_t   tokens[MAX_NUMBER_OF_TOKENS];
    jsmn_init(&jparser);

    int count = jsmn_parse(&jparser, content, exact_size, tokens, sizeof(tokens) / sizeof(jsmntok_t));

    /* Error handling*/
    if (count == JSMN_ERROR_PART) fatal("add extra empty line at the end of dnsconfig");
    if (count < 0) return failed_parse_json;
    if (count < 1 || tokens[0].type != JSMN_OBJECT) return top_elem_not_object;

    log_info("started config loading");
    
    init_config(config, content, tokens, count);
    validate_config(config);
    
    log_info("config loaded");

    free(content);
    
    return 0;
}