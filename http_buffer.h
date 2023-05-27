#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <iostream>

using namespace std;

typedef struct {
    char *data;
    size_t header_end;
    size_t content_length;
    size_t size;
} http_buffer;

http_buffer *http_buffer_init ( );
void http_buffer_destroy ( http_buffer* );
int http_buffer_is_empty ( http_buffer* );
void http_buffer_add ( http_buffer*, const char*, size_t );
int http_buffer_find ( http_buffer*, const char*, size_t );