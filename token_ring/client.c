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
#include <pthread.h>

#include "client.h"

token free_message;
token queued_message;
int queue_full = 0;
int queue_repeat = 1;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

input this;

int socket_in;
int socket_out;


void* receive_loop( void* arg )
{
    token received_message;
    token_init* init;
    int hunger = 0;

    while ( 1 ) 
    {
        token_receive( socket_in, &received_message );
        sleep( 1 );

        switch ( received_message.type )
        {
            case TOKEN_INIT:
                init = ( token_init* )&received_message;
                printf( "\n-- INIT --\n" );
                printf( " new: %s:%s\n", init->new_IP, init->new_port );
                printf( " connected to: %s:%s\n", init->source_IP, init->source_port );
                printf( " my neighbour: %s:%s\n", this.next_IP, this.next_port );
                printf( "> " );

                if ( strcmp( this.next_IP, init->source_IP ) == 0 && 
                        strcmp( this.next_port, init->source_port ) == 0 )
                {
                    strcpy( this.next_IP, init->new_IP );
                    strcpy( this.next_port, init->new_port );

                    close( socket_out );
                    socket_out = new_socket( this.next_IP, this.next_port );
                }
                else
                {
                    token_send( socket_out, &received_message );
                }
                break;

            case TOKEN_MSG: 

                if ( queue_full )
                {
                    hunger++;
                }

                if ( strcmp( this.ID, received_message.to_ID ) == 0 ) 
                {

                    printf( "%s <- %s: ", this.ID, received_message.from_ID );
                    printf( "%s\n", received_message.message );
                    printf( "> " );
                    token_queue_send( socket_out );
                    hunger = 0;
                } 
                else if ( strcmp( this.ID, received_message.from_ID ) == 0 )
                {
                    token_send_free( socket_out );
                    hunger = 0;
                }
                else
                {
                    printf( "%s <- ", received_message.to_ID ); 
                    printf( "%s ", received_message.from_ID ); 
                    printf( "(%d)\n", received_message.TTL ); 
                    printf( "> " );

                    if ( hunger >= MAX_HUNGER )
                    {
                        token_send( socket_out, &queued_message );
                        memcpy( &queued_message, &received_message, sizeof( token ) );
                        hunger = 0;
                    }
                    else
                    {
                        token_send( socket_out, &received_message );
                    }
                }
                break;

            case TOKEN_FREE:
                token_queue_send( socket_out );
                break;
        }
    }
}


void* read_input_loop( void* arg )
{
    char message [100];
    char destination [100];
    sleep( 2 );

    while( 1 )
    {
        while ( queue_full ) ;

        printf( "> " );

        if ( scanf( " %d ", &queue_repeat ) == 0 )
            queue_repeat = 1;

        scanf( " %99[^: ] : %99[^\n]", destination, message );
        strcpy( queued_message.from_ID, this.ID );
        strcpy( queued_message.to_ID, destination );
        queued_message.type = TOKEN_MSG;
        queued_message.TTL = MAX_TTL;
        strcpy( queued_message.message, message );

        pthread_mutex_lock( &queue_lock );
        queue_full = 1;
        pthread_mutex_unlock( &queue_lock );
    }
}


int main( const int argc, const char** argv )
{
    setbuf( stdout, NULL ); // disable output buffering

    pthread_t receiver;
    int status;

    parse_command_line( argc, argv );

    printf( "%s: opening input socket...\n", this.ID );
    socket_in = new_socket( "", this.local_port );

    printf( "%s: opening output socket...\n", this.ID );
    socket_out = new_socket( this.next_IP, this.next_port );

    if ( this.has_token )
    {
        token_send( socket_in, &free_message );
    }
    else 
    {
        printf( "No token. I will try to connect\n" );
        token_init init;
        memset( &init, 0, sizeof init );
        init.type = TOKEN_INIT;
        init.TTL = MAX_TTL;
        strcpy( init.from_ID, this.ID );
        strcpy( init.source_IP, this.next_IP );
        strcpy( init.source_port, this.next_port );
        strcpy( init.new_port, this.local_port );
        token_send( socket_out, ( token* )&init );
    }

    status = pthread_create( &receiver, NULL, receive_loop, NULL );
    err( status != 0, "%s: can't create thread\n", this.ID );


    read_input_loop( NULL );

    return 0;
}


int new_socket( const char* IP, const char* port )
{
    struct addrinfo* srvinfo;
    struct addrinfo* p;
    int sockfd;

    srvinfo = get_address_info( IP, port );

    // bind to the first result possible
    for ( p = srvinfo; p != NULL; p = p->ai_next ) 
    {
        sockfd = socket( p->ai_family, p->ai_socktype, p->ai_protocol );
        if ( sockfd == -1 ) 
        {
            perror( "new_socket: socket" );
            continue;
        }

        if ( IP == NULL || strlen( IP ) == 0 ) 
        {
            if ( bind( sockfd, p->ai_addr, p->ai_addrlen ) == -1 ) 
            {
                close( sockfd );
                perror( "new_socket: bind" );
                continue;
            }
        }
        break;
    }
    err ( p == NULL, "new_socket: failed to bind\n" );
    
    freeaddrinfo( srvinfo );

    return sockfd;
}


struct addrinfo* get_address_info( const char* IP, const char* port ) 
{
    struct addrinfo hints;
    struct addrinfo* srvinfo;
    int status;

    memset( &hints, 0, sizeof hints );
    hints.ai_family = AF_UNSPEC;        // both v4 and v6 work
    hints.ai_socktype = this.protocol;

    if ( IP == NULL || strlen( IP ) == 0 )
    {
        hints.ai_flags = AI_PASSIVE;        // fill in my IP for me
        status = getaddrinfo( NULL, port, &hints, &srvinfo );
    }
    else
    {
        status = getaddrinfo( IP, port, &hints, &srvinfo );
    }

    err( status != 0, "getaddrinfo: %s\n", gai_strerror( status ) );
    return srvinfo;
}


void token_receive( int sockfd, token* buffer )
{
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addrlen = sizeof their_addr;

    numbytes = recvfrom( sockfd, buffer, MAX_DATA_SIZE, 0, 
            ( struct sockaddr* )&their_addr, &addrlen );
    if ( numbytes == -1 )
    {
        perror( "token_receive: recv\n" );
    }

    if ( buffer->type == TOKEN_INIT )
    {
        token_init* init = ( token_init* ) buffer;
        if ( strlen( init->new_IP ) == 0 )
        {
            get_IP_string( ( struct sockaddr* )&their_addr, init->new_IP );
        }
    }
}


void token_queue_send( int sockfd )
{
    if ( queue_full )
    {
        queued_message.TTL = MAX_TTL;
        token_send( sockfd, &queued_message );
        queue_repeat -= 1;
        if ( queue_repeat <= 0 )
        {
            pthread_mutex_lock( &queue_lock );
            queue_full = 0;
            pthread_mutex_unlock( &queue_lock );
        }
    }
    else
    {
        token_send_free( sockfd );
    }
}


void token_send_free( int sockfd )
{
    memset( &free_message, 0, sizeof free_message );
    free_message.type = TOKEN_FREE;

    struct addrinfo* srvinfo;
    srvinfo = get_address_info( this.next_IP, this.next_port );

    sendto( sockfd, &free_message, sizeof( token ), 
            0, srvinfo->ai_addr, srvinfo->ai_addrlen );
}


void token_send( int sockfd, token* message )
{
    struct addrinfo* srvinfo;
    srvinfo = get_address_info( this.next_IP, this.next_port );

    message->TTL -= 1;
    if ( message->TTL <= 0 ) 
    {
        token_send_free( sockfd );
    }
    else
    {
        sendto( sockfd, message, sizeof( token ), 0, 
                srvinfo->ai_addr, srvinfo->ai_addrlen );
    }
}


void token_print( token* msg )
{
    printf( "\n  src: %s\n", msg->from_ID );
    printf( "  dest: %s\n", msg->to_ID );
    if ( msg->type == TOKEN_INIT )
        printf( "  type: init\n" );
    else if ( msg->type == TOKEN_MSG )
        printf( "  type: msg\n" );
    else if ( msg->type == TOKEN_FREE )
        printf( "  type: free\n" );
    else
        printf( "  type: unknown\n" );
    printf( "  TTL: %d\n", msg->TTL );
    printf( "  data: %s\n", msg->message );
    printf( "> " );
}


void get_IP_string( struct sockaddr* sa, char* buffer )
{
    struct sockaddr_storage* addr;
    inet_ntop( sa->sa_family, get_in_addr( sa ), buffer, INET6_ADDRSTRLEN );
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


void parse_command_line( const int argc, const char** argv )
{
    err( argc != 7, "usage: uID uPort neighIP neighPort hasToken tcp|udp\n" );
    err( strlen( argv[1] ) >= 64, "userID: too long (max 64).\n" );

    strcpy( this.ID, argv[1] );
    strcpy( this.local_port, argv[2] );
    strcpy( this.next_IP, argv[3] );
    strcpy( this.next_port, argv[4] );

    if ( strcasecmp( argv[5], "true" ) == 0 )  
        this.has_token = 1;
    else if ( strcasecmp( argv[5], "false" ) == 0 ) 
        this.has_token = 0;

    if ( strcasecmp( argv[6], "tcp" ) == 0 ) 
        this.protocol = SOCK_STREAM;
    else if ( strcasecmp( argv[6], "udp" ) == 0 ) 
        this.protocol = SOCK_DGRAM;
}
