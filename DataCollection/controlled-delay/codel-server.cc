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
  if(argc<3) {
   std::cout<<"Usage: ./codel-server local_ip interface \n";
   exit(1);
  }
  /* get details from cmd line */ 
  std::string local_ip((const char*)argv[1]);
  std::string interface((const char*)argv[2]);

  /* Create to two ethernet sockets for data and feedback */
  Socket data_socket,feedback_socket;

  data_socket.bind( Socket::Address( local_ip, 9000 ) );
  data_socket.bind_to_device( interface );

  feedback_socket.bind( Socket::Address( local_ip, 9001 ) );
  feedback_socket.bind_to_device( interface );

  /* Figure out the NAT addresses of each of the three LTE sockets */
  uint32_t local_id = (int) getpid() ^ rand(); 
  uint32_t remote_id;
  Socket::Address target_data( get_nat_addr( data_socket, local_id, &remote_id ) );
  Socket::Address target_feedback( get_nat_addr( feedback_socket, local_id, &remote_id ) );

  fprintf( stderr, "LTE data endpoint = %s\n", target_data.str().c_str() );
  fprintf( stderr, "LTE feedback endpoint = %s\n", target_feedback.str().c_str() );

  std::cout<<"Local ID "<<local_id<<" remote id "<<remote_id<<"\n";

  DelayServoSender downlink_sender("DOWN-TX",data_socket,feedback_socket,target_data,local_id); 
  DelayServoReceiver uplink_receiver("UP-RX",data_socket,feedback_socket,target_feedback,remote_id);

  while ( 1 ) {
    fflush( NULL );

    /* possibly send packet */
    downlink_sender.tick();
    uplink_receiver.tick(); 
    /* wait for incoming packet OR expiry of timer */
    struct pollfd poll_fds[ 2 ];
    poll_fds[ 0 ].fd = data_socket.get_sock();
    poll_fds[ 0 ].events = POLLIN;

    poll_fds[ 1 ].fd = feedback_socket.get_sock();
    poll_fds[ 1 ].events = POLLIN;

    struct timespec timeout;
    uint64_t next_transmission_delay = std::min( uplink_receiver.wait_time_ns(), downlink_sender.wait_time_ns()  );
    timeout.tv_sec = next_transmission_delay / 1000000000;
    timeout.tv_nsec = next_transmission_delay % 1000000000;
    ppoll( poll_fds, 2, &timeout, NULL );

    if ( poll_fds[ 0 ].revents & POLLIN ) {
      Socket::Packet incoming( data_socket.recv() );
      uint32_t* pkt_id=(uint32_t *)(incoming.payload.data());
      Payload *contents = (Payload *) incoming.payload.data();
      if(*pkt_id==remote_id) { /* this is data */
       contents->recv_timestamp = incoming.timestamp;
       uplink_receiver.recv(contents);
      }
    }
    else if ( poll_fds[ 1 ].revents & POLLIN ) {
      Socket::Packet incoming( feedback_socket.recv() );
      uint32_t* pkt_id=(uint32_t *)(incoming.payload.data());
      if (*pkt_id==local_id) {/* this is feedback */  
       Feedback *feedback = (Feedback *) incoming.payload.data();
       downlink_sender.recv(feedback);
      }
    }
  }
}
