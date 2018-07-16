#include <string.h>
#include <unistd.h>

#include "jsmn.h"
#include "dnsconfig.h"
#include "utils.h"

#define TOKEN_SIZE           1024
#define START_SIZE           32
#define MAX_NUMBER_OF_TOKENS 100000*2

static int init_config(dnsconfig_t *config, char *content, jsmntok_t *tokens, int count) {
    if (config == NULL)  return null_config;
    if (content == NULL) return null_content_error;
    if (tokens == NULL)  return null_tokens_error;

    char key[TOKEN_SIZE];
    char value[TOKEN_SIZE];

    for (size_t i = 1; i < count; i += 2) {
        get_pair(tokens, content, i, key, value);
#ifdef DEBUG
        fprintf(stderr, "%s : %s\n", key, value);
#endif
    }
    return 0;
}

int get_pair(jsmntok_t *tokens, char *content, size_t index, char *key, char *value) {
    if (tokens == NULL)  return null_tokens_error;
    if (content == NULL) return null_content_error;
    if (key == NULL)     return null_key_error;
    if (value == NULL)   return null_value_error;
    
    jsmntok_t json_key   = tokens[index];
    jsmntok_t json_value = tokens[index+1];

    size_t key_len   = json_key.end   - json_key.start;
    size_t value_len = json_value.end - json_value.start;

    strncpy(key,   content+json_key.start,   key_len);
    strncpy(value, content+json_value.start, value_len);

    key  [key_len]   = '\x00';
    value[value_len] = '\x00';
    return 0;
}

int parse_config(dnsconfig_t *config, char *filename) {
    if (config == NULL)   return null_config;
    if (filename == NULL) return null_filename;
    
    char *content = NULL;
    size_t size       = START_SIZE;
    size_t exact_size = 0;

    if (access(filename, F_OK) == -1) fatal("[-] No such config file\n");
    if ((exact_size = read_file(filename, &content, &size)) < 0) fatal("[-] Error while reading config\n");

    jsmn_parser jparser;
    jsmntok_t   tokens[MAX_NUMBER_OF_TOKENS];
    jsmn_init(&jparser);

    int count = jsmn_parse(&jparser, content, exact_size, tokens, sizeof(tokens) / sizeof(jsmntok_t));

    /* Error handling*/
    if (count < 0) return failed_parse_json;
    if (count < 1 || tokens[0].type != JSMN_OBJECT) return top_elem_not_object;

    init_config(config, content, tokens, count);
    
    return 0;
}