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
// ANIRUDH: Android logging
#include <jni.h>
#include <android/log.h>
#include <errno.h>
#include "parameters.h"
struct senderdata {
  int datagram_count;
  int queue_len;
  int secs;
  int us;
  int padding[PKT_PADDING];
};

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

int mainFunction( int argc, char *argv[] )
{
  /* command line argument handling */ 
  if ( argc != 3 ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "Usage: %s IP PORT\n", argv[ 0 ] );
    exit( 1 );
  }

  char *ip = argv[ 1 ];
  int port = atoi( argv[ 2 ] );

  if ( port <= 0 ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "%s: Bad port %s\n", argv[ 0 ], argv[ 2 ] );
    exit( 1 );
  }

  /* create socket */
  int sock = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( sock < 0 ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "socket :%s \n",strerror(errno) );
    exit( 1 );
  }

  /* associate socket with remote host and port */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons( port );
  if ( !inet_aton( ip, &addr.sin_addr ) ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "%s: Bad IP address %s\n", argv[ 0 ], ip );
    exit( 1 );
  }

  if ( connect( sock, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "connect: %s \n",strerror(errno) );
    exit( 1 );
  }

  /* ask for timestamps */
  int ts_opt = 1;
  if ( setsockopt( sock, SOL_SOCKET, SO_TIMESTAMP, &ts_opt, sizeof( ts_opt ) ) < 0 ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "setsockopt: %s \n",strerror(errno) );
    exit( 1 );
  }

  /* send initial datagram */
  if ( send( sock, NULL, 0, 0 ) < 0 ) {
    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "send: %s \n",strerror(errno) );
    exit( 1 );
  }

  int last_secs = 0, first_secs = -1;
  int datagram_count = 0;

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

    ssize_t ret = recvmsg( sock, &header, 0 );
    if ( ret < 0 ) {
      __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "recvmsg: %s \n",strerror(errno) );
      exit( 1 );
    }

    datagram_count++;

    /* timestamp data in msg_control */
    struct cmsghdr *ts_hdr = CMSG_FIRSTHDR( &header );
    assert( ts_hdr );
    assert( ts_hdr->cmsg_level == SOL_SOCKET );
    assert( ts_hdr->cmsg_type == SO_TIMESTAMP );
    struct timeval *ts = (struct timeval *)CMSG_DATA( ts_hdr );

    /* get sender data */
    struct senderdata data;
//    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "Just before asserting , ret is %d, sizeof is %d \n , sizeof timeval is %d , sizeof sentdata is %d \n",ret,sizeof(data),sizeof(struct timeval),sizeof(struct senderdata) );
    assert( ret == sizeof( data ) );
//    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING", "Just after asserting \n");
    memcpy( &data, msg_payload, ret );

    if ( datagram_count > 10000 ) {
      return 0;
    }

    if ( first_secs == -1 ) {
      first_secs = ts->tv_sec;
    }

    __android_log_print(ANDROID_LOG_DEBUG,"UDP-TIMING",
            "%d %d %.9f\n",
	    data.datagram_count,
	    data.queue_len,
	    (ts->tv_sec - data.secs) + .000001 * (ts->tv_usec - data.us) );

  }
  return 0;
}
