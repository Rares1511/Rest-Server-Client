#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string.h>
#include <poll.h>
#include <queue>

#include "helpers.h"

#define REGISTER "register"
#define LOGIN "login"
#define ENTER_LIBRARY "enter_library"
#define GET_BOOKS "get_books"
#define GET_BOOK "get_book"
#define ADD_BOOK "add_book"
#define DELETE_BOOK "delete_book"
#define LOGOUT "logout"
#define EXIT "exit"
#define HELP "help"
#define REMINDER_LOGIN "reminder_login"

#define REGISTER_PATH "/api/v1/tema/auth/register"
#define LOGIN_PATH "/api/v1/tema/auth/login"
#define ENTER_LIBRARY_PATH "/api/v1/tema/library/access"
#define GET_BOOKS_PATH "/api/v1/tema/library/books"
#define GET_BOOK_PATH "/api/v1/tema/library/books/%s"
#define ADD_BOOK_PATH "/api/v1/tema/library/books"
#define DELETE_BOOK_PATH "/api/v1/tema/library/books/%s"
#define LOGOUT_PATH "/api/v1/tema/auth/logout"
#define DUMMY_PATH "/api/v1/dummy"

#define POST "POST"
#define GET "GET"
#define DELETE "DELETE"

using namespace std;

int server_fd;
bool logged;
queue < char* > tasks;
char *cookie = NULL, *JWT = NULL, *user = NULL, *pass = NULL;
time_t current_time;

char* compute_request ( char* type, char* url, vector < char* > headers, char* payload ) {
    char *message = ( char* ) malloc ( BUFSIZ ); 
    char line[BUFSIZ];
    memset ( message, 0, BUFSIZ );

    sprintf ( line, "%s %s HTTP/1.1", type, url );
    compute_message ( message, line );

    sprintf ( line, "Host: %s:%d", SERVER_IP, SERVER_PORT );
    compute_message ( message, line );

    for ( int i = 0; i < headers.size ( ); i++ )
        compute_message ( message, headers[i] );

    if ( strcmp ( payload, "" ) )
        compute_message ( message, ( char* ) "Content-Type: application/json" );
    
    sprintf ( line, "Content-Length: %ld", strlen ( payload ) );
    compute_message ( message, line );
    compute_message ( message, ( char* ) "" );
    compute_message ( message, payload );

    compute_message ( message, ( char* ) "" );
    return message;
}

void execute_reminder_login ( ) {
    vector < char* > headers;
    char payload[BUFSIZ];
    tasks.push ( ( char* ) REMINDER_LOGIN );
    sprintf ( payload, "{\"username\":\"%s\",\"password\":\"%s\"}", user, pass );
    send_to_server ( server_fd, compute_request ( ( char* ) POST, ( char* ) LOGIN_PATH, headers, payload ) );
    current_time = time ( NULL );
}

void read_data ( char* text, char* data ) {
    printf ( "%s=", text );
    fgets ( data, LINELEN, stdin );
    if ( data[strlen ( data ) - 1] == '\n' )
        data[strlen ( data ) - 1] = 0;
    if ( time ( NULL ) - current_time > 5 ) {
        server_fd = reopen_connection ( server_fd );
        current_time = time ( NULL );
        if ( logged )
            execute_reminder_login ( );
    }
}

void execute_register ( ) {
    if ( logged ) { printf ( "You are already logged in, logout before registering another account!\n" ); return; }
    tasks.push ( ( char* ) REGISTER );
    vector < char* > headers;
    char username[LINELEN], password[LINELEN], payload[2 * LINELEN + 100];
    read_data ( ( char* ) "username", username );
    read_data ( ( char* ) "password", password );
    sprintf ( payload, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password );
    int rc = send_to_server ( server_fd, compute_request ( ( char* ) POST, ( char* ) REGISTER_PATH, headers, payload ) );
    DIE ( rc < 0, "sending register" );
    current_time = time ( NULL );
}

void execute_login ( ) {
    if ( logged ) { printf ( "You are already logged in!\n" ); return; }
    tasks.push ( ( char* ) LOGIN );
    vector < char* > headers;
    char username[BUFSIZ], password[BUFSIZ], payload[2 * BUFSIZ + 100];
    bool correct_credentials = false;
    while ( !correct_credentials ) {
        correct_credentials = true;
        read_data ( ( char* ) "username", username );
        if ( strstr ( username, " " ) ) {
            correct_credentials = false;
            printf ( "Username can't contain spaces!\n" );
            continue;
        }
        read_data ( ( char* ) "password", password );
        if ( strstr ( password, " " ) ) {
            correct_credentials = false;
            printf ( "Password can't contain spaces!\n" );
            continue;
        }
    }
    user = strdup ( username );
    pass = strdup ( password );
    sprintf ( payload, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password );
    int rc = send_to_server ( server_fd, compute_request ( ( char* ) POST, ( char* ) LOGIN_PATH, headers, payload ) );
    DIE ( rc < 0, "sending login" );
    current_time = time ( NULL );
}

void execute_enter_library ( ) {
    tasks.push ( ( char* ) ENTER_LIBRARY );
    vector < char* > headers;
    char line[BUFSIZ];
    sprintf ( line, "Cookie: connect.sid=%s", cookie );
    headers.push_back ( line );
    send_to_server ( server_fd, compute_request ( ( char* ) GET, ( char* ) ENTER_LIBRARY_PATH, headers, ( char* ) "" ) );
}

void execute_get_books ( ) {
    tasks.push ( ( char* ) GET_BOOKS );
    vector < char* > headers;
    char line[BUFSIZ];
    sprintf ( line, "Cookie: connect.sid=%s", cookie );
    headers.push_back ( line );
    sprintf ( line, "Authorization: Bearer %s", JWT );
    headers.push_back ( line );
    send_to_server ( server_fd, compute_request ( ( char* ) GET, ( char* ) GET_BOOKS_PATH, headers, ( char* ) "" ) );
    current_time = time ( NULL );
}

void execute_get_book ( ) {
    vector < char* > headers;
    char url[LINELEN + sizeof ( GET_BOOKS_PATH )], line[LINELEN], id[LINELEN];
    read_data ( ( char* ) "id", id );
    tasks.push ( ( char* ) GET_BOOK );
    sprintf ( url, GET_BOOK_PATH, id );
    sprintf ( line, "Authorization: Bearer %s", JWT );
    headers.push_back ( line );
    send_to_server ( server_fd, compute_request ( ( char* ) GET, url, headers, ( char* ) "" ) );
    current_time = time ( NULL );
}

void execute_add_book ( ) {
    tasks.push ( ( char* ) ADD_BOOK );
    vector < char* > headers;
    char title[LINELEN], author[LINELEN], genre[LINELEN], publisher[LINELEN], page_count[LINELEN], payload[BUFSIZ];
    char line[BUFSIZ];

    read_data ( ( char* ) "title", title );
    read_data ( ( char* ) "author", author );
    read_data ( ( char* ) "genre", genre );
    read_data ( ( char* ) "publisher", publisher );
    read_data ( ( char* ) "page_count", page_count );

    sprintf ( payload, "{\"title\":\"%s\",\"author\":\"%s\",\"genre\":\"%s\",\"publisher\":\"%s\",\"page_count\":\"%s\"}",
        title, author, genre, publisher, page_count );
    sprintf ( line, "Authorization: Bearer %s", JWT );
    headers.push_back ( line );
    send_to_server ( server_fd, compute_request ( ( char* ) POST, ( char* ) ADD_BOOK_PATH, headers, payload ) );
    current_time = time ( NULL );
}

void execute_delete_book ( ) {
    vector < char* > headers;
    char id[LINELEN], url[LINELEN + sizeof ( DELETE_BOOK_PATH )], line[LINELEN];
    read_data ( ( char* ) "id", id );
    tasks.push ( ( char* ) DELETE_BOOK );
    sprintf ( url, DELETE_BOOK_PATH, id );
    sprintf ( line, "Authorization: Bearer %s", JWT );
    headers.push_back ( line );
    send_to_server ( server_fd, compute_request ( ( char* ) DELETE, url, headers, ( char* ) "" ) );
    current_time = time ( NULL );
}

void execute_logout ( ) {
    if ( !logged ) { printf ( "You must log in before logging out!\n" ); return; }
    vector < char* > headers;
    char line[LINELEN];
    tasks.push ( ( char* ) LOGOUT );
    cookie = NULL, JWT = NULL;
    sprintf ( line, "Cookie: connect.sid=%s", cookie );
    headers.push_back ( line );
    send_to_server ( server_fd, compute_request ( ( char* ) GET, ( char* ) LOGOUT_PATH, headers, ( char* ) "" ) );
    current_time = time ( NULL );
}

void execute_help ( ) {
    printf ( "register      -> register an account to gain access to the server\n" );
    printf ( "login         -> login with a created account\n" );
    printf ( "enter_library -> gain access to the library\n" );
    printf ( "get_books     -> get a list with all of the books you have access to\n" );
    printf ( "get_book      -> receive extra information about a particular book\n" );
    printf ( "add_book      -> add a book in the server database\n" );
    printf ( "delete_book   -> delete a particular book from the server database\n" );
    printf ( "logout        -> logout from the current account\n" );
    printf ( "exit          -> close connection and client\n" );
}

void execute_command ( char* command ) {
    if ( !strcmp ( command, REGISTER ) ) { execute_register ( ); return; }
    if ( !strcmp ( command, LOGIN ) ) { execute_login ( ); return; } 
    if ( !strcmp ( command, ENTER_LIBRARY ) ) { execute_enter_library ( ); return; }
    if ( !strcmp ( command, GET_BOOKS ) ) { execute_get_books ( ); return; }
    if ( !strcmp ( command, GET_BOOK ) ) { execute_get_book ( ); return; }
    if ( !strcmp ( command, ADD_BOOK ) ) { execute_add_book ( ); return; }
    if ( !strcmp ( command, DELETE_BOOK ) ) { execute_delete_book ( ); return; }
    if ( !strcmp ( command, LOGOUT ) ) { execute_logout ( ); return; }
    if ( !strcmp ( command, HELP ) ) { execute_help ( ); return; }
    if ( !strcmp ( command, EXIT ) ) { close_connection ( server_fd ); exit ( 0 ); }
    printf ( "unknown command %s\n", command );
    printf ( "Unknown command, for a list of commands type help\n" );
}

void check_register_response ( http_buffer response ) {
    printf ( "Your account has been registered successfully.\n" );
}

void check_login_response ( http_buffer response ) {
    cookie = get_header_value ( response.data, ( char* ) "Set-Cookie", ( char* ) "connect.sid" );
    printf ( "You have been successfully logged in!\n" );
    logged = true;
}

void check_enter_library_response ( http_buffer response ) { 
    JWT = get_json_value ( response.data + response.header_end, ( char* ) "token" );
    printf ( "You've gained access to the library contents!\n" );
}

void check_get_books_response ( http_buffer response ) {
    char* data = ( response.data + response.header_end );
    printf ( "Books: \n" );
    while ( data != NULL ) {
        data++;
        printf ( "  -> Title: %s, id=%s\n", get_json_value ( data, ( char* ) "title" ), get_json_value ( data, ( char* ) "id" ) );
        data = strstr ( data, ",{" );
    }
}

void check_get_book_response ( http_buffer response ) {
    char* data = response.data + response.header_end;
    printf ( "Book:\n" );
    printf ( "  -> title: %s\n", get_json_value ( data, ( char* ) "title" ) );
    printf ( "  -> author: %s\n", get_json_value ( data, ( char* ) "author" ) );
    printf ( "  -> genre: %s\n", get_json_value ( data, ( char* ) "genre" ) );
    printf ( "  -> publisher: %s\n", get_json_value ( data, ( char* ) "publisher" ) );
    printf ( "  -> page_count: %s\n", get_json_value ( data, ( char* ) "page_count" ) );
}

void check_add_book_response ( http_buffer response ) {
    printf ( "Your book has been added successfully!\n" );
}

void check_delete_book_response ( http_buffer response ) {
    printf ( "Book has been successfully deleted!\n" );
}

void check_logout_response ( http_buffer response ) {
    printf ( "You have been successfully logged out!\n" );
    logged = false;
}

void check_response ( char* task, http_buffer response ) {
    char* error_text = get_json_value ( response.data + response.header_end, ( char* ) "error" );
    if ( error_text != NULL ) {
        printf ( "%s\n", error_text );
        return;
    }
    if ( !strcmp ( task, REGISTER ) ) check_register_response ( response );
    if ( !strcmp ( task, LOGIN ) ) check_login_response ( response );
    if ( !strcmp ( task, ENTER_LIBRARY ) ) check_enter_library_response ( response );
    if ( !strcmp ( task, GET_BOOKS ) ) check_get_books_response ( response );
    if ( !strcmp ( task, GET_BOOK ) ) check_get_book_response ( response );
    if ( !strcmp ( task, ADD_BOOK ) ) check_add_book_response ( response );
    if ( !strcmp ( task, DELETE_BOOK ) ) check_delete_book_response ( response );
    if ( !strcmp ( task, LOGOUT ) ) check_logout_response ( response );
}

int main ( ) {

    server_fd = open_connection ( );
    struct pollfd poll_fds[2];
    current_time = time ( NULL );

    poll_fds[0].fd = 0;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = server_fd;
    poll_fds[1].events = POLLIN;

    printf ( "Welcome to the client connected to PCom server located at %s:%d\n", ( char* ) SERVER_IP, SERVER_PORT );
    printf ( "For a list of possible commands type help\n" );

    while ( 420 > 69 ) {

        if ( time ( NULL ) - current_time >= 4 ) {
            vector < char* > headers;
            send_to_server ( server_fd, compute_request ( ( char* ) GET, ( char* ) DUMMY_PATH, headers, ( char* ) "" ) );
            current_time = time ( NULL );
        }

        int rc = poll ( poll_fds, 2, 0.1 );
        DIE ( rc < 0, "poll" );

        if ( poll_fds[0].revents & POLLIN ) {
            char command[BUFSIZ];
            read_line ( command );
            execute_command ( command );
        }
        if ( poll_fds[1].revents & POLLIN ) {
            http_buffer *response;
            response = receive_from_server ( server_fd );
            if ( response == NULL ) {
                server_fd = reopen_connection ( server_fd );
                current_time = time ( NULL );
                if ( logged )
                    execute_reminder_login ( );
                continue;
            }
            if ( !strcmp ( response->data + response->header_end, "Hello there!" ) ) continue;
            check_response ( tasks.front ( ), *response );
            tasks.pop ( );
        }
    }

    return 0;
}