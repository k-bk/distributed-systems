#ifndef TOKENRING_CLIENT_H
#define TOKENRING_CLIENT_H

#define TCP 0
#define UDP 1
#define TOKEN_FREE 0
#define TOKEN_INIT 1
#define TOKEN_MSG  2
#define MAX_CONNECTIONS 10
#define MAX_DATA_SIZE 500
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
    char type;
    char message [63];
} token;


typedef struct token_init
{
    char from_ID [32];
    char to_ID [32];   // should be empty here
    char type;
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


#endif
