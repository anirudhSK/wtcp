#include <string>
#include <vector>
#include <poll.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "delay-servo.hh"

using namespace std;

void punch_nat_hole (const Socket & local, const Socket::Address & server_address) {
  char buf[ 10 ];
  for ( int i = 0; i < 10; i++ ) {
    buf[ i ] = rand() % 256;
  }
  while(1) {
    /* till you get an ACK   */
    std::cout<<"Still to get an ACK, trying to punch a hole again \n";
    string hole_punch( "HOLE-PUNCH" );
    sender.send( Socket::Packet( server_address , hole_punch ) );

    struct pollfd poll_fds[ 1 ];
    poll_fds[ 0 ].fd = local.fd();
    poll_fds[ 0 ].events = POLLIN;

    struct timespec timeout;
    timeout.tv_sec =  1 ;
    timeout.tv_nsec = 0 ;
    ppoll( poll_fds, 1, &timeout, NULL );

    /* receive and ack the packet */
    if ( poll_fds[ 0 ].revents & POLLIN ) {
      Socket::Packet ack(local.recv());
      if ( ack.payload == "ACK-NAT" ) {
        std::cout<<"Successfully punched a hole and received an ACK from the server. \n";
        break;
      }
    }
  }
}

double hread( uint64_t in )
{
  return (double) in / 1.e9;
}

int main( int argc , char* argv[] ) {
  
  if(argc<3) {
   std::cout<<"Usage: ./codel-client local_ip server_ip \n";
   exit(1);
  }
 
  /* get details from cmd line */ 
  std::string local_ip((const char*)argv[1]);
  std::string server_ip((const char*)argv[2]);
  Socket::Address server_address( server_ip , 9000 );

  /* Create and bind LTE socket on USB0 tethered to the phone */
  Socket lte_socket;

  lte_socket.bind( Socket::Address( local_ip, 9001 ) );
  lte_socket.bind_to_device( "usb0" );

  /* Keep sending packets to the server until he acks that he got your packet. Otherwise nothing can run */
  punch_nat_hole(lte_socket,server_address); 
////
////  /* Now do the actual codel routine */
////  DelayServo uplink( "UP  ", lte_socket, server_ip);
////
////  while ( 1 ) {
////    fflush( NULL );
////
////    /* possibly send packet on the uplink */
////    uplink.tick();
////    
////    /* wait for incoming packet OR expiry of timer */
////    struct pollfd poll_fds[ 1 ];
////    poll_fds[ 0 ].fd = uplink.fd();
////    poll_fds[ 0 ].events = POLLIN;
////
////    struct timespec timeout;
////    uint64_t next_transmission_delay = uplink.wait_time_ns();
////    timeout.tv_sec = next_transmission_delay / 1000000000;
////    timeout.tv_nsec = next_transmission_delay % 1000000000;
////    ppoll( poll_fds, 1, &timeout, NULL );
////
////    /* recv and ack the packet coming in on the downlink 
////       recv acks coming in on the downlink for your own packets
////       and update stats */
////    if ( poll_fds[ 0 ].revents & POLLIN ) {
////      uplink.recv();
////    }
////  }
}
