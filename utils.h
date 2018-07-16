#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    open_file_error     = -1,
    realloc_failed      = -2,
    null_tokens_error   = -3,
    null_content_error  = -4,
    null_key_error      = -5,
    null_value_error    = -6,
    failed_parse_json   = -7,
    top_elem_not_object = -8,
    null_config         = -9,
    null_filename       = -10,

} utils_error_t;

bool is_negative_int(char *str);
bool is_file(char *str); 

/* reads file and writes its data to content variable */
int  read_file(const char *filename, char **content, size_t *content_size);

void fatal(char *errormsg); /* TODO: replace it with a more meaningful function */

#endif