#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "client.h"

#define TCP 0
#define UDP 1
#define MAX_CONNECTIONS 10
#define MAX_DATA_SIZE 500
#define err( cond, args... ) if ( cond ) { \
    fprintf( stderr, args ); exit( EXIT_FAILURE ); }


int main( const int argc, const char** argv )
{
    int sockfd;
    int new_fd;
    int numbytes;
    char buffer [MAX_DATA_SIZE + 1];
    input in;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];
    
    parse_command_line( argc, argv, &in );

    printf( "%s: opening socket...\n", in.ID );
    sockfd = new_socket( in.neighbour_IP, in.local_port );

    printf( "%s: waiting for connections...\n", in.ID );

    while ( 1 )
    {
        sin_size = sizeof their_addr;
        new_fd = accept( 
                sockfd, ( struct sockaddr* )&their_addr, &sin_size );
        if ( new_fd == -1 )
        {
            perror( "accept" );
            continue;
        }

        inet_ntop( their_addr.ss_family,
                get_in_addr( ( struct sockaddr* )&their_addr ),
                s, sizeof s);
        printf( "%s: got connection from %s\n", in.ID, s );
        if ( !fork() )
        {
            close( sockfd );
            if ( send( new_fd, "Hello, world!", 13, 0 ) == -1 )
                perror( "send" );
            close( new_fd );
            exit( 0 );
        }
        close( new_fd );
    }

    printf( "%s: received '%s'\n", in.ID, buffer );

    printf( "%s: closing socket...\n", in.ID );
    close_socket( sockfd );
    return 0;
}


int new_socket( const char* host_IP, const char* host_port )
{
    struct addrinfo* srvinfo;
    struct addrinfo* p;
    int yes = 1;
    int sockfd;

    srvinfo = get_address_info( host_IP, host_port );

    // bind to the first result possible
    for ( p = srvinfo; p != NULL; p = p->ai_next ) 
    {
        sockfd = socket( p->ai_family, p->ai_socktype, p->ai_protocol );
        if ( sockfd == -1 ) 
        {
            perror( "new_socket: socket" );
            continue;
        }

        if ( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, 
                    &yes, sizeof yes ) )
            perror( "new_socket: setsockopt" );

        if ( bind( sockfd, p->ai_addr, p->ai_addrlen ) == -1 ) 
        {
            close( sockfd );
            perror( "new_socket: bind" );
            continue;
        }

        break;
    }
    err ( p == NULL, "new_socket: failed to bind\n" );
    err ( listen( sockfd, MAX_CONNECTIONS ) == -1, "new_socket: listen\n" );

    freeaddrinfo( srvinfo );

    set_sigaction();

    return sockfd;
}


struct addrinfo* get_address_info( const char* host_IP, const char* host_port ) 
{
    struct addrinfo hints;
    struct addrinfo* srvinfo;
    int status;

    memset( &hints, 0, sizeof hints );
    hints.ai_family = AF_UNSPEC;        // both v4 and v6 work
    hints.ai_socktype = SOCK_STREAM;
    if ( host_IP == NULL || strlen( host_IP ) == 0 )
    {
        hints.ai_flags = AI_PASSIVE;        // fill in my IP for me
        host_IP = NULL;
    }

    status = getaddrinfo( host_IP, host_port, &hints, &srvinfo );
    err( status != 0, "getaddrinfo: %s\n", gai_strerror( status ) );

    return srvinfo;
}


void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while ( waitpid( -1, NULL, WNOHANG ) > 0 );
    errno = saved_errno;
}


void set_sigaction()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = SA_RESTART;
    err( sigaction( SIGCHLD, &sa, NULL ) == -1, "set_sigaction\n" );
}


void receive( int sockfd, token* buffer )
{
    int numbytes;
    numbytes = recv( sockfd, buffer, MAX_DATA_SIZE, 0 );
    if ( numbytes == -1 )
    {
        perror( "main: receive\n" );
    }
    buffer[numbytes] = '\0';
}


void send( int sockfd,  )
{
}


void close_socket( int socket )
{
    close( socket );
}


void* get_in_addr( struct sockaddr* sa )
{
    if ( sa->sa_family == AF_INET ) 
    {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*) sa;
        return &(ipv4->sin_addr);
    } 
    struct sockaddr_in6* ipv6 = (struct sockaddr_in6*) sa;
    return &(ipv6->sin6_addr);
}


void parse_command_line( const int argc, const char** argv, input* in )
{
    err( argc != 7, "usage: uID uPort neighIP neighPort hasToken tcp|udp\n" );
    err( strlen( argv[1] ) >= 64, "userID: too long (max 64).\n" );

    strcpy( in->ID, argv[1] );
    strcpy( in->local_port, argv[2] );
    strcpy( in->neighbour_port, argv[3] );

    if ( strcasecmp( argv[5], "true" ) == 0 )  
        in->has_token = 1;
    else if ( strcasecmp( argv[5], "false" ) == 0 ) 
        in->has_token = 0;

    if ( strcasecmp( argv[6], "tcp" ) == 0 ) 
        in->protocol = TCP;
    else if ( strcasecmp( argv[6], "udp" ) == 0 ) 
        in->protocol = UDP;
}
