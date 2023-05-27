#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "http_buffer.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE ( sizeof ( HEADER_TERMINATOR ) - 1 )
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE ( sizeof ( CONTENT_LENGTH ) - 1 )
#define HTTP_204 "HTTP/1.1 204 No Content"
#define HTTP_204_SIZE ( sizeof ( HTTP_204 ) - 1 )

#define SERVER_PORT 8080
#define SERVER_IP "34.254.242.81"

#define LINELEN 1024

void DIE ( bool, const char* );
http_buffer *receive_from_server ( int );
char* get_json_value ( char*, char* );
char* get_header_value ( char*, char*, char* );
int open_connection ( );
void close_connection ( int );
int reopen_connection ( int );
void read_line ( char* );
void compute_message ( char*, char* );
void read_credentials ( char*, char* );
int send_to_server ( int, char* );