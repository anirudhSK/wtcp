#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "parameters.h"
// For select 
/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
struct senderdata {
  int datagram_count;
  int queue_len;
  int secs;
  int us;
  int padding[PKT_PADDING];
};

#define PC 1

// ANIRUDH: Android logging
#ifndef PC
#include <jni.h>
#include <android/log.h>
#endif

#ifndef PC
JNIEXPORT void JNICALL Java_com_example_hellojni_HelloJni_runClient( JNIEnv* env, jobject thiz,jstring destIp, jstring port)
{
   char** argv=(char**)malloc(sizeof(char*)*10); // ANIRUDH: Mod here
   argv[0]="client";
   argv[1] = (*env)->GetStringUTFChars(env, destIp, 0);
   argv[2] = (*env)->GetStringUTFChars(env, port, 0);
 //  argv[1]="128.30.66.123";
 //  argv[2]="1025" ;
   mainFunction(3,argv);
}
#endif

#ifdef PC
int main( int argc, char *argv[] )
#else
int mainFunction( int argc, char *argv[] )
#endif
{
  /* Get current timeStamp */
  struct timeval timeStamp;
  gettimeofday(&timeStamp,NULL);
  
  /* Create log file */
  char fileName[100];
#ifdef PC
  sprintf(fileName,"../../logs/%d-logsPORTS%d-PADDING-%d.txt", (long)timeStamp.tv_sec,NUM_CONN,PKT_PADDING);
#else
  sprintf(fileName,"/mnt/sdcard/Logs/%d-logsPORTS%d-PADDING-%d.txt",timeStamp.tv_sec,NUM_CONN,PKT_PADDING);
#endif

  /* Open file for logging */
  FILE * logFileHandle;
  logFileHandle=fopen(fileName,"w");

  /* command line argument handling */ 
  if ( (argc < 3) || (argc > 4 )) {
#ifndef PC
    fprintf(logFileHandle, "Usage: %s IP PORT\n", argv[ 0 ] );
#else
    fprintf(stderr, "Usage: %s server_ip server_port [local_ip] \n", argv[ 0 ] );    
#endif
    exit( 1 );
  }

  char *ip = argv[ 1 ];
  int port = atoi( argv[ 2 ] );

  if ( port <= 0 || port >= 65536 ) {
    fprintf(logFileHandle, "%s: Bad port %s\n", argv[ 0 ], argv[ 2 ] );
    //    printf("%s: Bad port %s\n", argv[ 0 ], argv[ 2 ] );
    exit( 1 );
  }

  /* create an array of 128 sockets to round robin receive the packets */ 
  int socketArray[NUM_CONN];
  struct sockaddr_in srcAddr[NUM_CONN];

  /* associate sockets with remote host and port */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons( port );
  if ( !inet_aton(ip, &addr.sin_addr ) ) {
    fprintf(logFileHandle, "%s: Bad IP address %s\n", argv[ 0 ], ip );
    printf("%s: Bad IP address %s\n", argv[ 0 ], ip );
    exit( 1 );
  }
  /* start of source ports */
  int srcPort=port + 1000;// set it same as destination port for now. 
  int i=0;
  fd_set master;    // master file descriptor list
  FD_ZERO(&master); // clear out the master file descriptor . Very important to get this right, otherwise select has no effect, it's like the select line doesn't exist. 
  for (i=0;i<NUM_CONN;i++) {
    /* create socket */
    socketArray[i] = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( socketArray[i] < 0 ) {
      fprintf(logFileHandle, "socket :%s \n",strerror(errno) );
      exit( 1 );
    }
    /* put socket in non-blocking mode */
    fcntl(socketArray[i], F_SETFL, O_NONBLOCK);
    /* bind socket to specific source port */ 
    srcAddr[i].sin_family = AF_INET;
    srcAddr[i].sin_port = htons(srcPort+i);
    srcAddr[i].sin_addr.s_addr = INADDR_ANY;
    if (argc == 4) {
      printf("using localip %s\n", argv[3]);
      inet_aton(argv[ 3 ], &(srcAddr[i].sin_addr));
    }

    if ( bind( socketArray[i], (struct sockaddr *)&srcAddr[i], sizeof(srcAddr[i]) ) < 0 ) {
      perror( "bind" );
      exit( 1 );
    }
    /* connect to the remote stationery server */
    if (connect( socketArray[i], (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
      fprintf(logFileHandle, "connect: %s \n",strerror(errno) );
      printf("connect: %s \n",strerror(errno) );
      exit( 1 );
    }
    /* ask for timestamps */
    int ts_opt = 1;
    if ( setsockopt( socketArray[i], SOL_SOCKET, SO_TIMESTAMP, &ts_opt, sizeof( ts_opt ) ) < 0 ) {
      fprintf(logFileHandle, "setsockopt: %s \n",strerror(errno) );
      exit( 1 );
    }

    /* send initial datagram */
    if ( send( socketArray[i], NULL, 0, 0 ) < 0 ) {
      fprintf(logFileHandle, "send: %s \n",strerror(errno) );
      printf("send: %s \n",strerror(errno) );
      exit( 1 );
    }
    FD_SET(socketArray[i],&master);
  }
  
  fprintf(logFileHandle,"ports %d parameters PADDING %d\n",NUM_CONN,PKT_PADDING);
  printf("ports %d parameters PADDING %d, \n",NUM_CONN,PKT_PADDING);

  int last_secs = 0, first_secs = -1;
  int datagram_count = 0;

  // pick the maximum file descriptor 
  int fdmax=socketArray[0];
  for (i=1; i<NUM_CONN; i++) 
     if(fdmax<socketArray[i]) fdmax=socketArray[i];
  fdmax=fdmax+1;

  /* receive responses, ie 1000 real quick probe packets */
  while ( 1 ) {
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
    fprintf(logFileHandle,"just before selecting \n");
    int retval=select(fdmax, &master, NULL,NULL,NULL);
    if(retval<0) { 
      fprintf(logFileHandle,"select: %s \n",strerror(errno));
      printf("select: %s \n",strerror(errno));
      exit(1) ;
    }
    //    fprintf(logFileHandle,"just after selecting \n");
    i=0;
    for(i = 0; i < NUM_CONN; i++) {
                    // check for data
        ssize_t ret = recvmsg( socketArray[i], &header, 0 );
        if ( ret < 0 ) {
          if(errno==EWOULDBLOCK) {
            continue; // no data on the socket 
          }
          else {
            fprintf(logFileHandle, "recvmsg: %s \n",strerror(errno) );
            exit( 1 );
          }
        }
        datagram_count++;

        /* timestamp data in msg_control */
        struct cmsghdr *ts_hdr = CMSG_FIRSTHDR( &header );
        assert( ts_hdr );
        assert( ts_hdr->cmsg_level == SOL_SOCKET );
        //        assert( ts_hdr->cmsg_type == SO_TIMESTAMP );
        struct timeval *ts = (struct timeval *)CMSG_DATA( ts_hdr );

        /* get sender data */
        struct senderdata data;
//        fprintf(logFileHandle, "Just before asserting , ret is %d, sizeof is %d \n , sizeof timeval is %d , sizeof sentdata is %d \n",ret,sizeof(data),sizeof(struct timeval),sizeof(struct senderdata) );
        assert( ret == sizeof( data ) );
//        fprintf(logFileHandle, "Just after asserting \n");
        memcpy( &data, msg_payload, ret );

        if ( datagram_count > NUM_PACKETS ) {
          return 0;
        }

        if ( first_secs == -1 ) {
          first_secs = ts->tv_sec;
        }

        fprintf(logFileHandle,
                "Sender Dgm #:%d Q:%d Tx timestamp %f Rx timestamp %f \n",
                data.datagram_count,
                data.queue_len,
                data.secs  + .000001*data.us,
                ts->tv_sec + .000001*ts->tv_usec);
        fprintf(logFileHandle,
                "Received %d dgrams so far , last dgram on socket %d \n",
        	    datagram_count,socketArray[i]);
  }
 } // end of while(1) loop
}
