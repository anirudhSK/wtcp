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
  
  if(argc<5) {
   std::cout<<"Usage: ./codel-client local_ip server_ip data_interface feedback_interface \n";
   exit(1);
  }
 
  /* get details from cmd line */ 
  std::string local_ip((const char*)argv[1]);
  std::string server_ip((const char*)argv[2]);
  std::string data_interface((const char*)argv[3]);
  std::string feedback_interface((const char*)argv[3]);

  Socket::Address server_data( server_ip , 9000 );
  Socket::Address server_feedback( server_ip , 9001 );

  /* Create and bind LTE socket on USB0 tethered to the phone */
  Socket data_socket,feedback_socket;

  data_socket.bind( Socket::Address( local_ip, 18000 ) );
  data_socket.bind_to_device( data_interface );

  feedback_socket.bind( Socket::Address( local_ip, 18001 ) );
  feedback_socket.bind_to_device( feedback_interface );

  /* Keep sending packets to the server until he acks that he got your packet. Otherwise nothing can run */
  /* Use a unique to avoid confusing old zombies */ 
  uint32_t local_id = (int) getpid() ^ rand(); 
  uint32_t remote_id=punch_nat_hole(data_socket,server_data,local_id); 
  remote_id=punch_nat_hole(feedback_socket,server_feedback,local_id); 

  std::cout<<"Local ID "<<local_id<<" remote id "<<remote_id<<"\n";

  DelayServoReceiver downlink_receiver("DOWN-RX",data_socket,feedback_socket,server_feedback,remote_id);
  DelayServoSender uplink_sender("UP-TX",data_socket,feedback_socket,server_data,local_id);  

  while ( 1 ) {
    fflush( NULL );

    /* possibly send packet */
    downlink_receiver.tick();
    uplink_sender.tick(); 
    /* wait for incoming packet OR expiry of timer */
    struct pollfd poll_fds[ 2 ];
    poll_fds[ 0 ].fd = data_socket.get_sock();
    poll_fds[ 0 ].events = POLLIN;

    poll_fds[ 1 ].fd = feedback_socket.get_sock();
    poll_fds[ 1 ].events = POLLIN;

    struct timespec timeout;
    uint64_t next_transmission_delay = std::min ( downlink_receiver.wait_time_ns() , uplink_sender.wait_time_ns()  );
    timeout.tv_sec = next_transmission_delay / 1000000000;
    timeout.tv_nsec = next_transmission_delay % 1000000000;
    ppoll( poll_fds, 2, &timeout, NULL );

    if ( poll_fds[ 0 ].revents & POLLIN ) {
      Socket::Packet incoming( data_socket.recv() );
      uint32_t* pkt_id=(uint32_t *)(incoming.payload.data());
      Payload *contents = (Payload *) incoming.payload.data();
      if(*pkt_id==remote_id) { /* this is data */
       contents->recv_timestamp = incoming.timestamp;
       downlink_receiver.recv(contents);
      }
    }
    else if ( poll_fds[ 1 ].revents & POLLIN ) {
      Socket::Packet incoming( feedback_socket.recv() );
      uint32_t* pkt_id=(uint32_t *)(incoming.payload.data());
      if (*pkt_id==local_id) {/* this is feedback */  
       Feedback *feedback = (Feedback *) incoming.payload.data();
       uplink_sender.recv(feedback);
      }
    }
  }
}
