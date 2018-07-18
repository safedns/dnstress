#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
    open_file_error     = -1,
    realloc_failed      = -2,
    failed_parse_json   = -3,
    top_elem_not_object = -4,
    null_config         = -5,
    null_filename       = -6,
    null_content_size   = -7,
    null_content        = -8

} utils_error_t;

int get_addrfamily(char *addr);

void * xmalloc(size_t size);
void * xmalloc_0(size_t size);

bool is_negative_int(char *str);
bool is_file(char *str); 

/* reads file and writes its data to content variable */
int  read_file(const char *filename, char **content, size_t *content_size);

void log_info(const char *msg, ...);
void fatal(const char *msg, ...); /* TODO: replace it with a more meaningful function */

#endif