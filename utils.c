#include "utils.h"

bool is_negative_int(char *str) {
    char *ptr;
    long long ret = strtoll(str, &ptr, 10);
    
    return ret < 0;
}

bool is_file(char *str) {
    /* TODO: implement this function */
    return false;
}

int read_file(const char *filename, char **content, size_t *content_size) {
    char ch        = '\x00';
    char *rcontent = *content;
    size_t index   = 0;
    FILE *fp       = fopen(filename, "r");

    if (fp == NULL) return open_file_error;

    while ((ch = fgetc(fp)) != EOF) {
        if (index < *content_size) rcontent[index++] = ch;
        else {
            char *buf = realloc(rcontent, (*content_size * 2) * sizeof(char));
            if (buf == NULL) {
                free(rcontent);
                return realloc_failed;
            }
            rcontent = buf;
            *content_size = (*content_size) * 2;
            rcontent[index++] = ch;
        }
    }
    rcontent[index] = '\x00';
    *content = rcontent;
    
    fclose(fp);
    return index-1;
}

void fatal(char *errormsg) {
    fprintf(stderr, errormsg);
    exit(1);
}