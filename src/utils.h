#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "log.h"
#include "servant.h"

#define EXIT_ON_NULL(x) do { if ((x) == NULL) \
	fatal("%s:%d: fatal: function returned NULL", \
	__FILE__, __LINE__); } while(0)

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

int randint(const size_t up_bound);

void * xmalloc(const size_t size);
void * xmalloc_0(const size_t size);

bool is_negative_int(const char *str);
bool is_file(const char *filename); 

bool is_server_available(const struct _saddr *server);

const char * type2str(const servant_type_t type);

double time_elapsed(clock_t t);

/* reads file and writes its data to content variable */
int read_file(const char *filename, char **content, size_t *content_size);

#endif