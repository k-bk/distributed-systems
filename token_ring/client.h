#ifndef TOKENRING_CLIENT_H
#define TOKENRING_CLIENT_H

#define TCP 0
#define UDP 1
#define TOKEN_FREE 3
#define TOKEN_INIT 4
#define TOKEN_MSG  5
#define MAX_CONNECTIONS 10
#define MAX_DATA_SIZE 500
#define MAX_TTL 10
#define MAX_HUNGER 5
#define err( cond, args... ) if ( cond ) { \
    fprintf( stderr, args ); exit( EXIT_FAILURE ); }


typedef struct input
{
    char ID [32];
    char local_port [10];
    char next_IP [32];
    char next_port [10];
    int has_token;
    int protocol;
} input;


typedef struct token
{
    char from_ID [32];
    char to_ID [32];
    int type;
    int TTL;
    char message [60];
} token;


typedef struct token_init
{
    char from_ID [32];
    char to_ID [32];   // should be empty here
    int type;
    int TTL;
    char source_IP [20];
    char source_port [10];
    char new_IP [20];
    char new_port [10];
} token_init;

void parse_command_line( const int argc, const char** argv );

struct addrinfo* get_address_info( const char* host_IP, const char* host_port );
void* get_in_addr( struct sockaddr* sa );
int new_socket( const char* host_IP, const char* host_port );

void token_send( int sockfd, token* message );
void token_queue_send( int sockfd );
void token_receive( int sockfd, token* buffer );
void token_pass( int sockfd, token* message );
void token_print( token* msg );
void token_send_free( int sockfd );

void get_IP_string( struct sockaddr* sa, char* buffer );


#endif
