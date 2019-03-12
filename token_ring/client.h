#ifndef TOKENRING_CLIENT_H
#define TOKENRING_CLIENT_H

typedef struct input
{
    int has_token;
    int protocol;
    char neighbour_IP [32];
    char neighbour_port [10];
    char local_port [10];
    char ID [32];
} input;


typedef struct token
{
    char t_src [30];
    char t_dest [30];
    char t_type;
    char data [MAX_DATA_SIZE - 62];
}

void parse_command_line( const int argc, const char** argv, input* in );

int new_socket( const char* host_IP, const char* host_port );
void close_socket( int socket );

struct addrinfo* get_address_info( const char* host_IP, const char* host_port );
void set_sigaction();

void* get_in_addr( struct sockaddr* sa );

#endif
