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
#include <string.h>
// ANIRUDH: Added stuff 
#include <pthread.h>
#include <errno.h>
struct senderdata {
  int datagram_count;
  int queue_len;
  int secTimeStamp;
  int usecTimeStamp;
};
void* receiver(void* args) {
 /* receiver for ACKS */
 int sock=*((int *)(args));
 while(1) {
    /* Setup for using recvmsg */
    struct msghdr header;
    struct iovec msg_iovec;
    char msg_payload[ 2048 ];
    char msg_control[ 2048 ];

    header.msg_name = NULL;
    header.msg_namelen = 0;
    msg_iovec.iov_base = msg_payload;
    msg_iovec.iov_len = 2048;
    header.msg_iov = &msg_iovec;
    header.msg_iovlen = 1;
    header.msg_control = msg_control;
    header.msg_controllen = 4096;
    header.msg_flags = 0;
    /* Use recvmsg for getting time stamp */ 
    ssize_t ret = recvmsg( sock, &header, 0 );
    if ( ret < 0 ) {
      perror("recvmsg");
      exit( 1 );
    }

    /* timestamp data in msg_control , to retireve current timestamp */ 
    struct cmsghdr *ts_hdr = CMSG_FIRSTHDR( &header );
    assert( ts_hdr );
    assert( ts_hdr->cmsg_level == SOL_SOCKET );
    assert( ts_hdr->cmsg_type == SO_TIMESTAMP );
    struct timeval *ts = (struct timeval *)CMSG_DATA( ts_hdr );

    /* get payload of ACK, which has the echoed timestamp of the original msg.  */
    struct senderdata data;
    assert( ret == sizeof( data ) );
    memcpy( &data, msg_payload, ret );

    /* compute RTT */
    long int deltaus=(ts->tv_sec-data.secTimeStamp)*1000000 + (ts->tv_usec-data.usecTimeStamp)  ;
    printf("RTT estimate is %ld microseconds from datagram # %d \n",deltaus,data.datagram_count); 
 } 
}
int main( int argc, char *argv[] )
{
  /* Command line argument handling */ 
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
  /* ask for timestamps */
  int ts_opt = 1;
  if ( setsockopt( sock, SOL_SOCKET, SO_TIMESTAMP, &ts_opt, sizeof( ts_opt ) ) < 0 ) {
    perror("setsockopt" );
    exit( 1 );
  }
  /* bind socket to port */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons( port );
  addr.sin_addr.s_addr = INADDR_ANY;

  if ( bind( sock, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
    perror( "bind" );
    exit( 1 );
  }
  /* start receiver thread to get ACKs */
  pthread_t thr;
  if(pthread_create(&thr, NULL, &receiver, &sock)) // pass sock's address as the void* ptr 
  {
      printf("Could not create thread\n");
      return -1;
  }

  /* wait for initial datagram , to det. client's NAT end point */
  printf("Waiting for new client \n");
  socklen_t addrlen = sizeof( addr );
  if ( recvfrom( sock, NULL, 0, 0, (struct sockaddr *)&addr, &addrlen ) < 0 ) {
    perror( "recvfrom" );
    exit( 1 );
  }

  fprintf( stderr, "Received datagram from %s:%d\n",
	   inet_ntoa( addr.sin_addr ),
	   ntohs( addr.sin_port ) );

  int datagram_count = 0;
  int queue_len = -1;
  struct timeval timeStamp;

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
    gettimeofday(&timeStamp,NULL); // ANIRUDH: get timeStamp
    data.secTimeStamp=timeStamp.tv_sec;  // Maybe a few microseconds here and there could be off. 
    data.usecTimeStamp=timeStamp.tv_usec;
//    printf("Sent out data of size %d bytes \n",sizeof(data));
    if ( sendto( sock, &data, sizeof( data ),
		 0, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
      perror( "sendto" );
      exit( 1 );
    }
    datagram_count++;
    if(datagram_count==1000) {
      printf("Have sent 1000 data grams now returning to listen for connection \n");
      sleep(10);            // sleep 5 seconds waiting for the receiver thread to catch all the ack packets that may ever be sent. 
      break ; // hopefully client waits long enough before starting a new connection. , Can't loopback to listenForConn cause it messes things up. 
    }
  }
  return 0;
}
