#include <string>
#include <vector>
#include <poll.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "delay-servo.hh"

using namespace std;

Socket::Address get_nat_addr( const Socket & ethernet_socket )
{
  /* Just block forever till you hear the HOLE-PUNCH msg on your socket */ 
  Socket::Packet received( ethernet_socket.recv() );
  while(received.payload != "HOLE-PUNCH") { 
    received=Socket::Packet( ethernet_socket.recv() );
  }
  /* Be nice and tell the phone so that he knows you know about him */
  string ack( "ACK-NAT" );
  ethernet_socket.send( Socket::Packet( received.addr , ack ) );
  return received.addr;
}

double hread( uint64_t in )
{
  return (double) in / 1.e9;
}

int main( int argc, char* argv[] ) {
  if(argc<2) {
   std::cout<<"Usage: ./codel-server local_ip \n";
   exit(1);
  }
  /* get details from cmd line */ 
  std::string local_ip((const char*)argv[1]);

  Socket::Address ethernet_address( local_ip, 9000 );
  ethernet_socket.bind( ethernet_address );
  ethernet_socket.bind_to_device( "eth0" );

  /* Figure out the NAT addresses of each of the three LTE sockets */
  Socket::Address target( get_nat_addr( ethernet_socket ) );
  fprintf( stderr, "LTE = %s\n", target.str().c_str() );
////
////  DelayServo downlink( "DOWN", ethernet_socket, target );
////
////  while ( 1 ) {
////    fflush( NULL );
////
////    /* possibly send packet on the downlink */
////    downlink.tick();
////    
////    /* wait for incoming packet OR expiry of timer */
////    struct pollfd poll_fds[ 1 ];
////    poll_fds[ 0 ].fd = downlink.fd();
////    poll_fds[ 0 ].events = POLLIN;
////
////    struct timespec timeout;
////    uint64_t next_transmission_delay = downlink.wait_time_ns();
////    timeout.tv_sec = next_transmission_delay / 1000000000;
////    timeout.tv_nsec = next_transmission_delay % 1000000000;
////    ppoll( poll_fds, 1, &timeout, NULL );
////
////    /* recv and ack the packet coming in on the uplink 
////       recv acks coming in on the uplink for your own packets
////       and update stats */
////    if ( poll_fds[ 0 ].revents & POLLIN ) {
////      downlink.recv();
////    }
////  }
}
