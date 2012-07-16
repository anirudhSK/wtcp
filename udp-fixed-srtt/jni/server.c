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
#include "parameters.h"
#include <math.h>
#include <pthread.h>
#include<string.h> /* for memcpy */
int pktdist = CBR; 
const int ALPHA=0.125; /* filter gain for EWMA for srtt */
const int TARGET_RTT=250;/*250 ms target RTT */
int sleepTime=10;          /*10 milliseconds */
const double epsilon= 5 ; /* Allow 5 ms around the srtt */ 
double update_srtt(double srtt,double rtt) {
     return (ALPHA*rtt + (1-ALPHA)*srtt);
}
int checkAgainstTarget(double srtt,int sleepTime) {
     if(srtt > TARGET_RTT + epsilon) /* back off / decrease sending rate / increase sleep time */ return sleepTime*2; 
     else if (srtt < TARGET_RTT - epsilon) /* increase sending rate / decrease sleep time */      return sleepTime-1;
     else return sleepTime ;/* close enough to target, sit tight */ 
}

struct senderdata {
  int datagram_count;
  int queue_len;
  int secs;
  int us;
  int padding[PKT_PADDING];
};

void* receiver(void* args) {
 /* receiver for ACKS */
 int sock=*((int *)(args));
 double srtt=0;
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

    /* get payload of ACK, which has the echoed timestamp of the original msg. */
    struct senderdata data;
    assert( ret == sizeof( data ) );
    memcpy( &data, msg_payload, ret );

    /* compute RTT */
    long int deltaus=(ts->tv_sec-data.secs)*1000000 + (ts->tv_usec-data.us) ;
    printf("RTT estimate is %ld microseconds from datagram # %d \n",deltaus,data.datagram_count);
    /*compute srtt */
    srtt=update_srtt(srtt,(double)deltaus/1000); /*Because the srtt is in ms */
    sleepTime=checkAgainstTarget(srtt,sleepTime);          /* check for congestion */ 
 }
}

int main( int argc, char *argv[] )
{
  srand(0);
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s PORT \n", argv[ 0 ] );
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

  /* start receiver thread to get ACKs */
  pthread_t thr;
  if(pthread_create(&thr, NULL, &receiver, &sock)) // pass sock's address as the void* ptr
  {
      printf("Could not create thread\n");
      return -1;
  }

  /* wait for initial datagram from 128 socket-lets */
  int i=0;
  struct sockaddr_in srcAddr[NUM_CONN];
  for(i=0;i<NUM_CONN;i++) {
   socklen_t addrlen = sizeof( addr );
   if ( recvfrom( sock, NULL, 0, 0, (sockaddr *)&addr, &addrlen ) < 0 ) {
     perror( "recvfrom" );
     exit( 1 );
   }
  fprintf( stderr, "Received datagram from %s:%d\n",
	   inet_ntoa( addr.sin_addr ),
	   ntohs( addr.sin_port ) );

   srcAddr[i]=addr; 
  }


  int datagram_count = 0;
  int queue_len = -1;
  struct timeval timestamp;
  int numPackets=0;
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
    gettimeofday( &timestamp, NULL );
    data.secs = timestamp.tv_sec;
    data.us = timestamp.tv_usec;
    int nrTx=0;
    int currentConnection=datagram_count%NUM_CONN;
    if ( (nrTx=sendto( sock, &data, sizeof( data ),
		 0, (sockaddr *)&srcAddr[currentConnection], sizeof(struct sockaddr_in) )) < 0 ) {
      perror( "sendto" );
      exit( 1 );
    }
    else {
//     printf("Sent %d bytes at time %ld.%06ld on conn %d \n",nrTx,timestamp.tv_sec,timestamp.tv_usec,currentConnection);
     ;
    }
    datagram_count++;
    usleep(1000*sleepTime); /* Used to pace yourself at the sender */
 }
    /*
    if ( (datagram_count % 500) == 0 ) {
      sleep( 1 );
    }
    */
  return 0;
}
