#include "http_buffer.h"

http_buffer *http_buffer_init ( ) {
    http_buffer *buffer = ( http_buffer* ) malloc ( sizeof ( http_buffer ) );
    buffer->data = NULL;
    buffer->size = 0;
    buffer->header_end = 0;
    buffer->content_length = 0;
    return buffer;
}

void http_buffer_destroy ( http_buffer *http_buffer ) {
    if ( http_buffer->data != NULL ) {
        free ( http_buffer->data );
        http_buffer->data = NULL;
    }
    http_buffer->size = 0;
}

int http_buffer_is_empty ( http_buffer *http_buffer ) { return http_buffer->data == NULL; }

void http_buffer_add ( http_buffer *http_buffer, const char *data, size_t data_size ) {
    if ( http_buffer->data != NULL )
        http_buffer->data = ( char* ) realloc ( http_buffer->data, ( http_buffer->size + data_size ) * sizeof ( char ) );
    else
        http_buffer->data = ( char* ) calloc ( data_size, sizeof ( char ) );
    memcpy ( http_buffer->data + http_buffer->size, data, data_size );
    http_buffer->size += data_size;
}

int http_buffer_find ( http_buffer *http_buffer, const char *data, size_t data_size ) {
    if ( data_size > http_buffer->size ) return -1;
    size_t last_pos = http_buffer->size - data_size + 1;
    for ( size_t i = 0; i < last_pos; i++ ) {
        size_t j;
        for ( j = 0; j < data_size; j++ )
            if ( tolower ( http_buffer->data[i + j] ) != tolower ( data[j] ) )
                break;
        if (j == data_size)
            return i;
    }
    return -1;
}
