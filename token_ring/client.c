#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define err( cond, args... ) if ( cond ) { \
    fprintf( stderr, args ); exit( EXIT_FAILURE ); }
#define TCP 0
#define UDP 1

typedef struct arg_t
{
    int hasToken;
    int protocol;
    struct in_addr neighbourIP;
    char neighbourPort [10];
    char userPort [10];
    char userID [32];
} arg_t;

typedef struct addrinfo addrinfo;

void parseCommandLine( const int argc, const char** argv, arg_t* input );
int openSocket( addrinfo* info );

int main( const int argc, const char** argv )
{
    arg_t input;
    addrinfo hints;
    addrinfo* srvinfo;
    int status;

    parseCommandLine( argc, argv, &input );

    memset( &hints, 0, sizeof hints );
    hints.ai_family = AF_UNSPEC;        // both v4 and v6 work
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;        // fill in my IP for me

    status = getaddrinfo( NULL, input.userPort, &hints, &srvinfo );
    err( status != 0, "getaddrinfo: %s\n", gai_strerror( status ) );

    openSocket( srvinfo );

    freeaddrinfo( srvinfo );
}

int openSocket( addrinfo* info )
{
    int newSocket;
    void* addr;
    char* ipver;
    char ipstr[INET6_ADDRSTRLEN];

    while ( info->ai_next != NULL ) 
    {
        if ( info->ai_family == AF_INET ) 
        {
            struct sockaddr_in* ipv4 = ( struct sockaddr_in* ) info->ai_addr;
            addr = &( ipv4->sin_addr );
            ipver = "IPv4";
        } 
        else 
        { 
            struct sockaddr_in6* ipv6 = ( struct sockaddr_in6* ) info->ai_addr;
            addr = &( ipv6->sin6_addr );
            ipver = "IPv6";
        }

        // convert the IP to a string
        inet_ntop( info->ai_family, addr, ipstr, sizeof ipstr );
        printf("  %s: %s\n", ipver, ipstr);

        info = info->ai_next;
    }

    int sockfd = socket( info->ai_family, info->ai_socktype, info->ai_protocol );
    bind( sockfd, info->ai_addr, info->ai_addrlen );
    connect( sockfd, info->ai_addr, info->ai_addrlen );
    return sockfd;
}

void parseCommandLine( const int argc, const char** argv, arg_t* input )
{
    err( argc != 7, "usage: uID uPort neighIP neighPort hasToken tcp|udp\n" );
    err( strlen( argv[1] ) >= 64, "userID: too long (max 64).\n" );

    strcpy( input->userID, argv[1] );
    strcpy( input->userPort, argv[2] );
    strcpy( input->neighbourPort, argv[3] );

    if ( strcasecmp( argv[5], "true" ) == 0 )  
        input->hasToken = 1;
    else if ( strcasecmp( argv[5], "false" ) == 0 ) 
        input->hasToken = 0;

    if ( strcasecmp( argv[6], "tcp" ) == 0 ) 
        input->protocol = TCP;
    else if ( strcasecmp( argv[6], "udp" ) == 0 ) 
        input->protocol = UDP;
}
