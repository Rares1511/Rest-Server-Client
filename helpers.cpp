#include "helpers.h"

void DIE ( bool cond, const char *msg ) { if ( cond ) { perror ( msg ); exit ( 0 ); } }

http_buffer *receive_from_server ( int sockfd ) {
    char response[BUFSIZ];
    http_buffer *http_buffer = http_buffer_init ( );
    bool http_204 = false;

    do {
        int bytes = recv ( sockfd, response, BUFSIZ, 0 );
        DIE ( bytes < 0, "reading response from socket" );

        if ( bytes == 0 ) {
            return NULL;
        }

        http_buffer_add ( http_buffer, response, ( size_t ) bytes );
        
        http_buffer->header_end = http_buffer_find ( http_buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE );
        http_204 = ( http_buffer_find ( http_buffer, HTTP_204, HTTP_204_SIZE ) != -1 );

        if ( http_buffer->header_end >= 0 ) {
            http_buffer->header_end += HEADER_TERMINATOR_SIZE;
            if ( http_204 )
                break;
            
            int content_length_start = http_buffer_find ( http_buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE );
            
            if ( content_length_start < 0 )
                continue;           

            content_length_start += CONTENT_LENGTH_SIZE;
            http_buffer->content_length = strtol ( http_buffer->data + content_length_start, NULL, 10 );
            break;
        }
    } while ( 1 );

    size_t total = http_buffer->content_length + http_buffer->header_end;
    
    while ( http_buffer->size < total ) {
        int bytes = recv ( sockfd, response, BUFSIZ, 0 );
        DIE ( bytes < 0, "reading response from socket" );

        if ( bytes == 0 )
            break;

        http_buffer_add ( http_buffer, response, ( size_t ) bytes );
    }
    http_buffer_add ( http_buffer, "", 1 );
    return http_buffer;
}

char* get_json_value ( char* json_string, char* key ) {
    char* start = strstr ( json_string, key ), *value;
    int count = 0;
    if ( start == NULL ) return NULL;
    start = strstr ( start, ":" ) + 1;
    while ( start[count] != ',' && start[count] != '}' ) count++;
    value = ( char* ) malloc ( count + 1 );
    strncpy ( value, start, count );
    value[count] = 0;
    if ( value[strlen ( value ) - 1] == '\"' ) value[strlen ( value ) - 1] = '\0';
    if ( value[0] == '\"' ) return  ( value + 1 );
    return value;
}

char* get_header_value ( char* data, char* header, char* sub_header ) {
    char *start = strstr ( data, header );
    char* end = strstr ( start, "\r" );
    if ( start == NULL ) return NULL;
    if ( sub_header == NULL ) {
        start += strlen ( header ) + 1;
        if ( start[0] == ' ' )
            start++;
        char* value = ( char* ) malloc ( end - start );
        strncpy ( value, start, end - start );
        return value;
    }
    start = strstr ( start, sub_header );
    start = strstr ( start, "=" );
    start++;
    end = strstr ( start, ";" );
    char* value = ( char* ) malloc ( end - start + 1 );
    strncpy ( value, start, end - start );
    return value;
}

int open_connection ( ) {
    struct sockaddr_in address;

	int server_fd = socket ( AF_INET, SOCK_STREAM, 0 );
    DIE ( server_fd < 0, "socket" );

    memset ( &address, 0, sizeof ( address ) );
	address.sin_family = AF_INET;
	address.sin_port = htons ( SERVER_PORT );
    address.sin_addr.s_addr = inet_addr ( ( char* ) SERVER_IP );
    int rc = connect ( server_fd, ( struct sockaddr* ) &address, sizeof ( address ) );
    DIE ( rc < 0, "connect" );

    return server_fd;
}

void close_connection ( int socketfd ) { close ( socketfd ); }

int reopen_connection ( int sockfd ) {
    close_connection ( sockfd );
    return open_connection ( );
}

void read_line ( char* buffer ) {
    fgets ( buffer, BUFSIZ, stdin );
    sscanf ( buffer, "%s\n", buffer );
}

void compute_message ( char* msg, char* line ) {
    strcat ( msg, line );
    strcat ( msg, "\r\n" );
}

void read_credentials ( char* username, char* password ) {
    printf ( "username=" );
    read_line ( username );
    printf ( "password=" );
    read_line ( password );
}

int send_to_server ( int sockfd, char *message ) {
    int bytes, sent = 0;
    int total = strlen ( message );

    do {
        bytes = send ( sockfd, message + sent, total - sent, 0 );
        DIE ( bytes < 0, "writing message to socket" );

        if ( bytes == 0 )
            break;

        sent += bytes;
    } while ( sent < total );

    return sent - total;
}