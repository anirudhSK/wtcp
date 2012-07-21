#include <string>
#include <vector>
#include <poll.h>
#include <assert.h>

#include "socket.hh"
#define min(x,y) (x<y ? x : y)
#define max(x,y) (x>y ? x : y)
using namespace std;
/* global values */
uint64_t stt=0;
double currentPacketRate=1.0; /*current Rate at the sender, start it out at 100 Hz */
long int sendBytes=50;           /* number of bytes to put into packet */ 
/* maintain a vector of packet statistics , only sent_time is used for stt */
struct packetstat {
 uint64_t sent_time;
 int dest;
 bool received;
};
/* stt is written by the rx, packetstat is written by the sender */
const int NUM_PACKETS=10000;
vector<struct packetstat> packetstats( NUM_PACKETS );
const double ALPHA=0.125;
const uint64_t TARGET_TT=250000000; /* 250 ms or 250000 us */
const uint64_t epsilon=5000000 ;    /* 5 ms leeway around that. */


/* send packet from lte to ethernet to punch a NAT hole */
/* Use this packet at the ethernet to infer end point */
Socket::Address getNatAddr( const Socket & sender, const Socket::Address & dest,
			      const Socket & receiver )
{
  char buf[ 10 ];
  for ( int i = 0; i < 10; i++ ) {
    buf[ i ] = rand() % 256;
  }
  string to_send( buf, 10 );
  sender.send( Socket::Packet( dest, to_send ) );
  Socket::Packet received( receiver.recv() );
  if ( received.payload != to_send ) {
    fprintf( stderr, "Bad packet received while getting NAT addresses.\n" );
    exit( 1 );
  }
  return received.addr;
}
/* update smoothed transit time based on new value */ 
uint64_t updateStt(uint64_t tt, uint64_t stt) {
   if (stt==0) return tt;
   else return uint64_t(ALPHA*((double)tt) + (1-ALPHA)*((double)stt)) ;
}
/* check for congestion based on rtt and change sleepTime accordingly */
double checkCongestion(uint64_t stt,double currentRate) {
   if(stt>TARGET_TT*0.75) {
     sendBytes=max(sendBytes/2,50)     ; /* At least send 50 bytes in a packet */
     return max(currentRate/2,1.0); /* Multiplicative decrease, At least send 1Hz for maintaining NAT mapping */ 
   }
   else if(stt<0.75*TARGET_TT)  {
     sendBytes=min(sendBytes+10,1250);   /* Not more than 1250 bytes per packet */
     return min((currentRate+10.0),1500); /*Additive increase in some sense */ /* Not more than 1500 packets per second */
   }
   else return currentRate; /* sit tight */
}
/* lteReceiver */
void* lteReceiver(void* receiverSocket ) {
  Socket lteSocket=*((Socket* )(receiverSocket));
  while(1) {
        Socket::Packet rec = lteSocket.recv();
        //	assert ( rec.payload.size() == sizeof( packets_sent ) );
        uint64_t receivedTs=rec.timestamp;
        uint64_t seq=*(uint64_t *)rec.payload.data();
        uint64_t tt=receivedTs-packetstats[seq].sent_time;
        stt=updateStt(tt,stt);   // update stt
        currentPacketRate=checkCongestion(stt,currentPacketRate); // update packet rate
        printf("Packet %lu ,received at %f ms, delay: %f ms, stt: %f  ms , new rate : %f bytesPerSec \n",seq,
                                                                                           (double)receivedTs/1000000,
                                                                                           (double)(receivedTs-packetstats[seq].sent_time)/1000000,
                                                                                           (double)stt/1000000, 
                                                                                           currentPacketRate*sendBytes);
        
 }
  return NULL;
}
int main() {
    /* Create and bind Ethernet socket */
    Socket ethernetSocket;
    Socket::Address ethernetAddress( "18.251.7.128", 9000 );
    ethernetSocket.bind( ethernetAddress );
    ethernetSocket.bind_to_device( "eth0" );

    /* Create and bind the LTE socket */
    Socket lteSocket;
    lteSocket.bind( Socket::Address( "10.100.1.1", 9001 ) );
    lteSocket.bind_to_device( "usb0" );

    /* get nat end point */
    Socket::Address lteEndPoint(getNatAddr( lteSocket, ethernetAddress, ethernetSocket ) );
    fprintf( stderr, "LTE = %s\n",lteEndPoint.str().c_str() );

    /* Create lteReceiver thread ,and pass it appropriate params  */
    pthread_t lteReceiverThread;
    if(pthread_create(&lteReceiverThread, NULL, &lteReceiver,&lteSocket ))  {
       printf("Could not create receiver thread \n");
    }
//    printf("Just created lteReceiver thread \n");
    /* sending loop ie ethernetSender */
    int numSent =0 ; /* number sent out so far */
    while(numSent < NUM_PACKETS) {
       /* check current stt , for instantaneous feedback */
       usleep(1000000/currentPacketRate); /*currentRate is in packets per second, usleep takes us */
       char *seq_encoded = (char *)&numSent;
       packetstats[ numSent ].sent_time = Socket::timestamp(); /* maintain state to calc stt */
       ethernetSocket.send(Socket::Packet(lteEndPoint, string( seq_encoded, sendBytes) ) );     
       numSent++;
    }
}
