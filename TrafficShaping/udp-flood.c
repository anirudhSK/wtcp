#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include<signal.h>
struct senderdata {
  int datagram_count;
  int queue_len;
  int secs;
  int us;
  int padding;
};
uint64_t ts_start=0;
uint64_t total_count=0;
void my_handler (int sig)
{
 struct timeval ts;
 gettimeofday(&ts,NULL);
 uint64_t now=ts.tv_sec*1e6+ts.tv_usec;
 printf("Transfer rate is %f MBytes per sec \n", (float)total_count/(float)(now-ts_start));
 exit(1);
}
int main( int argc, char *argv[] )
{
  int port = 9999;

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

  if ( bind( sock, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
    perror( "bind" );
    exit( 1 );
  }
  /* bind to device */ 
  if ( setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE, argv[1], 4 ) < 0 ) {
    fprintf( stderr, "Error binding to %s\n", argv[1] );
    perror( "setsockopt SO_BINDTODEVICE" );
    exit( 1 );
  }

  /* random address to flood to */ 
  addr.sin_family=AF_INET;
  addr.sin_port=htons(9999);
  if ( !inet_aton("220.181.111.147", &addr.sin_addr ) ) {
    exit( 1 );
  }
  signal(SIGINT, my_handler);

  struct senderdata data;
  data.datagram_count = 10;
  data.queue_len = 10;
  data.secs = 5;
  data.us = 10;
  struct timeval ts;
  total_count=0;
  gettimeofday(&ts,NULL);
  ts_start=ts.tv_sec*1e6 + ts.tv_usec;
  while ( 1 ) {
    /* send datagrams to target */
        if ( (sendto( sock, &data, sizeof( data ),
		 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) )) < 0 ) {
      perror( "sendto" );
      exit( 1 );
    }
    else  {
     total_count=total_count+62;
    }
  }
  return 0;
}
