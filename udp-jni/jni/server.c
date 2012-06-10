#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct senderdata {
  int datagram_count;
  int queue_len;
};

int main( int argc, char *argv[] )
{
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s PORT\n", argv[ 0 ] );
    exit( 1 );
  }

  int port = atoi( argv[ 1 ] );

  if ( port <= 0 ) {
    fprintf( stderr, "%s: Bad port %s\n", argv[ 0 ], argv[ 1 ] );
    exit( 1 );
  }

  /* create socket */
  int sock = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( sock < 0 ) {
    perror( "socket" );
    exit( 1 );
  }

  /* bond socket to port */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons( port );
  addr.sin_addr.s_addr = INADDR_ANY;

  if ( bind( sock, (sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
    perror( "bind" );
    exit( 1 );
  }

  /* wait for initial datagram */
  socklen_t addrlen = sizeof( addr );
  if ( recvfrom( sock, NULL, 0, 0, (sockaddr *)&addr, &addrlen ) < 0 ) {
    perror( "recvfrom" );
    exit( 1 );
  }

  fprintf( stderr, "Received datagram from %s:%d\n",
	   inet_ntoa( addr.sin_addr ),
	   ntohs( addr.sin_port ) );

  int datagram_count = 0;
  int queue_len = -1;

  while ( 1 ) {
    /* pause if necessary */
    while ( 1 ) {
      if ( ioctl( sock, TIOCOUTQ, &queue_len ) < 0 ) {
	perror( "ioctl" );
	exit( 1 );
      }

      if ( queue_len > 4096 ) {
	assert( usleep( 1 ) == 0 );
      } else {
	break;
      }
    }

    /* send datagrams to target */
    struct senderdata data;
    data.datagram_count = datagram_count;
    data.queue_len = queue_len;

    if ( sendto( sock, &data, sizeof( data ),
		 0, (sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
      perror( "sendto" );
      exit( 1 );
    }
    datagram_count++;

    /*
    if ( (datagram_count % 500) == 0 ) {
      sleep( 1 );
    }
    */
  }

  return 0;
}
