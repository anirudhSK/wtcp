#include <string>
#include <vector>
#include <poll.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "delay-servo.hh"
#include <iostream>
using namespace std;

uint32_t punch_nat_hole ( const Socket & lte_socket, const Socket::Address & server_address ,  uint32_t local_id ) {
  /* 3 way handshake to punch hole, ACK-NAT it and exchange IDs */
  uint32_t remote_id;
  while(1) {
    std::cout<<"Still to get an ACK-NAT, trying to punch a hole again \n";
    char id_str[20]; 
    /* TODO : Fix this. Setting this to even 10 seems to create a buffer overflow under O3 */ 
    sprintf(id_str,"%d",local_id);
    string hole_punch( "HOLE-PUNCH");
    string id_string(id_str);
    std::cout<<"Sending Hole punch string "<<hole_punch+id_string<<"\n";
    lte_socket.send( Socket::Packet( server_address , hole_punch+id_string ) );

    struct pollfd poll_fds[ 1 ];
    poll_fds[ 0 ].fd = lte_socket.get_sock();
    poll_fds[ 0 ].events = POLLIN;

    struct timespec timeout;
    timeout.tv_sec =  1 ;
    timeout.tv_nsec = 0 ;
    ppoll( poll_fds, 1, &timeout, NULL );

    /* receive and ack the ACK-NAT */
    if ( poll_fds[ 0 ].revents & POLLIN ) {
      Socket::Packet ack(lte_socket.recv());
      if ( ack.payload.substr(0,7) == "ACK-NAT" ) {
        remote_id=atoi(ack.payload.substr(7).c_str());
        std::cout<<"Successfully punched a hole and received an ACK-NAT from the server. Ack it \n";
        lte_socket.send( Socket::Packet( server_address , "ACK-ACK" ) );
        break;
      }
    }
  }
  return remote_id; 
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
  uint32_t local_id = (int) getpid() ^ rand(); 
  /* To avoid confusing old zombies */ 
  uint32_t remote_id=punch_nat_hole(lte_socket,server_address,local_id); 

  std::cout<<"Local ID "<<local_id<<" remote id "<<remote_id<<"\n";

  DelayServoSender uplink_sender("UP-TX",lte_socket,server_address,local_id);
  DelayServoReceiver downlink_receiver("DOWN-RX",lte_socket,server_address,remote_id);
}
