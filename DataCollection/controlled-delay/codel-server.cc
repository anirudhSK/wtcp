#include <string>
#include <vector>
#include <poll.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "delay-servo.hh"
#include <iostream>
using namespace std;

Socket::Address get_nat_addr( const Socket & ethernet_socket , uint32_t local_id,uint32_t* remote_id )
{
  std::cout<<"Still to get public end point of client , blocking here \n";
  Socket::Packet received( ethernet_socket.recv() );
  while( received.payload.substr(0,10) != "HOLE-PUNCH" ) { 
    std::cout<<"Still to get public end point of client , blocking here \n";
    received=Socket::Packet( ethernet_socket.recv() );
  }
  /* got public end point and remote ID */
  *remote_id = atoi(received.payload.substr(10).c_str());
  
  /* Be nice and ACK it, Send him local_id as well  */
  while (1) {
    char id_str[20]; 
    /* TODO : Fix this. Setting this to even 10 seems to create a buffer overflow under O3 */ 
    sprintf(id_str,"%d",local_id);
    string ack_nat( "ACK-NAT");
    string id_string(id_str);
    std::cout<<"Sending my local ID "<<ack_nat+id_string<<"\n";
    ethernet_socket.send( Socket::Packet( received.addr , ack_nat+id_string ) );

    struct pollfd poll_fds[ 1 ];
    poll_fds[ 0 ].fd = ethernet_socket.get_sock();
    poll_fds[ 0 ].events = POLLIN;

    struct timespec timeout;
    timeout.tv_sec =  1 ;
    timeout.tv_nsec = 0 ;
    ppoll( poll_fds, 1, &timeout, NULL );

    /* wait for an ACK-ACK from client  */
    if ( poll_fds[ 0 ].revents & POLLIN ) {
      Socket::Packet ack(ethernet_socket.recv());
      if ( ack.payload == "ACK-ACK" ) {
        std::cout<<"Successfully exchanged local ID with client. \n";
        break;
      }
    }
  }
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
  Socket ethernet_socket;
  ethernet_socket.bind( ethernet_address );
  ethernet_socket.bind_to_device( "eth0" );

  /* Figure out the NAT addresses of each of the three LTE sockets */
  uint32_t local_id = (int) getpid() ^ rand(); 
  uint32_t remote_id;
  Socket::Address target( get_nat_addr( ethernet_socket, local_id, &remote_id ) );
  fprintf( stderr, "LTE = %s\n", target.str().c_str() );

  std::cout<<"Local ID "<<local_id<<" remote id "<<remote_id<<"\n";

  DelayServoSender downlink_sender("DOWN-TX",ethernet_socket,target,local_id);

  while ( 1 ) {
    fflush( NULL );

    /* possibly send packet */
    downlink_sender.tick();
    
    /* wait for incoming packet OR expiry of timer */
    struct pollfd poll_fds[ 1 ];
    poll_fds[ 0 ].fd = downlink_sender.fd();
    poll_fds[ 0 ].events = POLLIN;

    struct timespec timeout;
    uint64_t next_transmission_delay = std::min( downlink_sender.wait_time_ns(), (uint64_t)-1);
    timeout.tv_sec = next_transmission_delay / 1000000000;
    timeout.tv_nsec = next_transmission_delay % 1000000000;
    ppoll( poll_fds, 1, &timeout, NULL );

    if ( poll_fds[ 0 ].revents & POLLIN ) {
      Socket::Packet incoming( ethernet_socket.recv() );
      uint32_t* pkt_id=(uint32_t *)(incoming.payload.data());
      if(*pkt_id==local_id) /* this is feedback */  {
       Feedback *feedback = (Feedback *) incoming.payload.data();
       downlink_sender.recv(feedback);
      }
      else if (*pkt_id==remote_id) { /* this is data */
       std::cout<<"Received data \n";
      }
    }
  }
}
