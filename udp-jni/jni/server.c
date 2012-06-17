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
int pktdist = CBR; 
struct senderdata {
  int datagram_count;
  int queue_len;
  int secs;
  int us;
  int padding[PKT_PADDING];
};

int main( int argc, char *argv[] )
{
  srand(0);
  if ( argc != 5 ) {
    fprintf( stderr, "Usage: %s PORT <DIST: 1-CBR, 2-POISSON, 3-SQUARE> <DUTY CYCLE>  <ARRIVAL RATE> \n", argv[ 0 ] );
    exit( 1 );
  }

  int port = atoi( argv[ 1 ] );
  int pktdist = atoi(argv[2]);
  double dutyCycle=atof(argv[3]);
  double arrivalRate=atof(argv[4]);
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
    if ( (nrTx=sendto( sock, &data, sizeof( data ),
		 0, (sockaddr *)&addr, sizeof( addr ) )) < 0 ) {
      perror( "sendto" );
      exit( 1 );
    }
    else {
//     printf("Sent %d bytes at time %ld.%06ld \n",nrTx,timestamp.tv_sec,timestamp.tv_usec);
     ;
    }
    datagram_count++;
    if (pktdist == CBR) {    
      usleep(1000000/arrivalRate);
     } 
    else if (pktdist == POISSON) {
       float u= rand()/(float)RAND_MAX;
//       printf("Sleeping for time %f \n",-log(u)*1000000/arrivalRate);
       usleep(-log(u)*1000000/arrivalRate);
    }
    else if (pktdist == SQUARE ) {
         if(numPackets >= ACTIVE_PACKETS) {
             float activePeriod=(ACTIVE_PACKETS*1000000*dutyCycle)/arrivalRate;
             float dormantPeriod=(activePeriod*(1-dutyCycle))/dutyCycle;
             numPackets=0;
             usleep((int)dormantPeriod);
             printf("Sleeping for a dormantPeriod %f microseconds \n",dormantPeriod);
         }
         else {
             numPackets++;
             usleep((int)((1000000*dutyCycle)/arrivalRate));
//             printf("Sleeping for an interim period %d microseconds \n",(int)((1000000*DUTY_CYCLE)/arrivalRate));
         }
    }       
     
    }

    /*
    if ( (datagram_count % 500) == 0 ) {
      sleep( 1 );
    }
    */
  

  return 0;
}
