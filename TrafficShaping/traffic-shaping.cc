#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdlib.h>
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <errno.h>            // errno, perror()
#include <stdio.h>           // close()
#include <string.h>          // strcpy, memset(), and memcpy()
#include <net/if.h>          // struct ifreq
#include <sys/ioctl.h>       // for SIOCGIFINDEX
#include <assert.h>
#include <poll.h>            // poll.h
#include "link.hh"
#include "rate-schedule.hh"
#define DEBUG
#include<iostream>
#include<vector>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
namespace po = boost::program_options;
uint8_t bcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};

int check_mac_addr(uint8_t m1[6],uint8_t m2[6]) {
       /* check if two mac addr match */
      return ((m1[0]==m2[0])&&(m1[1]==m2[1])&&(m1[2]==m2[2])&&(m1[3]==m2[3])&&(m1[4]==m2[4])&&(m1[5]==m2[5])) ;
}

void read_mac_addr(const char* mac_str,uint8_t* client_mac) {  
     sscanf(mac_str, "%hx:%hx:%hx:%hx:%hx:%hx", (short unsigned int *)&client_mac[0], 
                                                (short unsigned int *)&client_mac[1],
                                                (short unsigned int *)&client_mac[2],
                                                (short unsigned int *)&client_mac[3], 
                                                (short unsigned int *)&client_mac[4], 
                                                (short unsigned int *)&client_mac[5]);
}

int recv_packet(int socket,uint8_t* frame) {
     /* recv_packet with all the error handling */ 
     int recv_bytes;
     while ((recv_bytes = recv (socket, frame, IP_MAXPACKET, MSG_TRUNC)) < 0) {
          if (errno == EINTR) {
            memset (frame, 0, IP_MAXPACKET);
            continue;  // Something weird happened, but let's try again.
          } else {
            perror ("recv() failed:");
            exit (EXIT_FAILURE);
          }
      }
     return recv_bytes;
}

void bind_to_if(int socket,const char* if_name) {
  /* Bind to interface, 
     code that actually works and doesn't get random packets from other interfaces */
  struct sockaddr_ll sll;
  struct ifreq ifr;
  bzero(&sll, sizeof(sll));
  bzero(&ifr, sizeof(ifr));
  /* First Get the Interface Index */
  strncpy((char *)ifr.ifr_name, if_name, IFNAMSIZ);
  if((ioctl(socket, SIOCGIFINDEX, &ifr)) == -1) {
   perror("Error getting Interface index !\n");
   exit(-1);
  }
  else {
   printf("Successfully bound to %s \n",if_name);
  }
  /* Bind our raw socket to this interface */
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = ifr.ifr_ifindex;
  sll.sll_protocol = htons(ETH_P_ALL);

  if((bind(socket, (struct sockaddr *)&sll, sizeof(sll)))== -1) {
    perror("Error binding raw socket to interface\n");
    exit(-1);
  }
}

int main(int argc,char** argv) {
  /* command line handling */
  std::string ingress,egress,uplink_schedule,downlink_schedule;
  uint8_t client_mac[6]; /* read from command line */
  float uplink_rate=1; 
  float downlink_rate=1;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
      ("ingress",po::value<std::string>(), "ingress interface name")
      ("egress" ,po::value<std::string>(), "egress interface name")
      ("client-mac" ,po::value<std::string>(), "client's MAC address")
      ("uplink-rate" ,po::value<float>(), "uplink rate in bits per sec")
      ("downlink-rate" ,po::value<float>(), "downlink rate in bits per sec")
      ("uplink-schedule" ,po::value<std::string>(), "filename with uplink rate schedule")
      ("downlink-schedule" ,po::value<std::string>(), "filename with downlink rate schedule")
      ("help" ,"produce help message")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    
  
  if (vm.count("help")) {
      std::cout << desc << "\n";
      return 1;
  }
  RateSchedule uplink_rate_schedule,downlink_rate_schedule; 
  if (vm.count("ingress")&& vm.count("egress") && vm.count("client-mac") && vm.count("uplink-rate") && vm.count("downlink-rate")) {
    ingress=vm["ingress"].as<std::string>();
    egress=vm["egress"].as<std::string>();
    uplink_rate=vm["uplink-rate"].as<float>();
    downlink_rate=vm["downlink-rate"].as<float>();
    read_mac_addr(vm["client-mac"].as<std::string>().c_str(),client_mac);
    /* Ingress and egress rate schedules */
    uplink_rate_schedule=RateSchedule (uplink_rate);
    downlink_rate_schedule=RateSchedule(downlink_rate);
  } 
  else if (vm.count("ingress")&& vm.count("egress") && vm.count("client-mac") && vm.count("uplink-schedule") && vm.count("downlink-schedule")) {
    ingress=vm["ingress"].as<std::string>();
    egress=vm["egress"].as<std::string>();
    uplink_schedule=vm["uplink-schedule"].as<std::string>();
    downlink_schedule=vm["downlink-schedule"].as<std::string>();
    read_mac_addr(vm["client-mac"].as<std::string>().c_str(),client_mac);
    /* Schedules as files */
    uplink_rate_schedule=RateSchedule (uplink_schedule);
    downlink_rate_schedule=RateSchedule(downlink_schedule);
  }

  else {
    std::cout << desc << "\n";
    return 1;
  }
   
  
  /* variable decl */
  int ingress_socket,egress_socket,recv_bytes;

  /* Allocate memory for the buffers */
  uint8_t* ether_frame = (uint8_t *) malloc (IP_MAXPACKET);
  if (ether_frame == NULL) {
    fprintf (stderr, "ERROR: Cannot allocate memory for array 'ether_frame'.\n");
    exit (EXIT_FAILURE);
  }

  /* Zero out all bytes */
  memset (ether_frame, 0, IP_MAXPACKET);

  /* Create a packet socket, that binds to eth0 by default. Receive all packets using ETH_P_ALL */
  if ((ingress_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* bind to ingress */
  bind_to_if(ingress_socket,ingress.c_str()); 

  /* Create another packet socket for the egress . In some sense we are a dumb software repeater */
  if ((egress_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* bind to egress */
  bind_to_if(egress_socket,egress.c_str()); 

  /* create a poll structure for ingress and egress */
  struct pollfd poll_fds[ 2 ];
  poll_fds[ 0 ].fd = ingress_socket;
  poll_fds[ 0 ].events = POLLIN;

  poll_fds[ 1 ].fd = egress_socket;
  poll_fds[ 1 ].events = POLLIN;

  /* Ingress and egress Links */ 
  Link uplink(uplink_rate_schedule,egress_socket,true,"uplink",uplink_rate<0);         /* handles the case were uplink_rate<0 by setting , by default it's 1, so this is false */
  Link downlink(downlink_rate_schedule,ingress_socket,true,"downlink",downlink_rate<0);
  while(1) {
    /* send packets if possible */ 
    uplink.tick(); 
    downlink.tick();
    /* set timeouts */ 
    struct timespec timeout;
    uint64_t next_transmission_delay = std::min( uplink.wait_time_ns(), downlink.wait_time_ns() );
#ifdef DEBUG
//    std::cout<<"Waiting "<<next_transmission_delay<<" ns in ppoll queues at uplink : "<<uplink.pkt_queue_occupancy<<std::endl<<std::endl;
#endif
    timeout.tv_sec = next_transmission_delay / 1000000000;
    timeout.tv_nsec = next_transmission_delay % 1000000000;    
    /* poll both ingress and egress sockets */ 
    ppoll( poll_fds, 2, &timeout, NULL );  
    /* from ingress socket to egress */  
    if ( poll_fds[ 0 ].revents & POLLIN ) {
      recv_bytes = recv_packet(ingress_socket, ether_frame) ; 
      /* parse src mac */
      uint8_t* src_mac=(uint8_t*)(ether_frame +6); 
      if (  check_mac_addr(src_mac,client_mac) )   {
#ifdef DEBUG
//           printf("Received packet of %d bytes on ingress from client \n",recv_bytes);   
#endif
           uplink.recv(ether_frame,recv_bytes);
      }
    }
    /* egress to ingress */ 
    else if ( poll_fds[ 1 ].revents & POLLIN ) {
      recv_bytes = recv_packet(egress_socket, ether_frame)  ;
      /* parse dst mac */
      uint8_t* dst_mac=(uint8_t*)(ether_frame);
      if ( check_mac_addr(dst_mac,client_mac) || check_mac_addr(dst_mac,bcast) )   {
#ifdef DEBUG
//          if (check_mac_addr(dst_mac,client_mac)) printf("Received packet of %d bytes on egress to client \n",recv_bytes);   
//          else if (check_mac_addr(dst_mac,bcast)) printf("Received packet of %d bytes on egress to broadcast \n",recv_bytes);   
#endif
          downlink.recv(ether_frame,recv_bytes);
      }
    }
  }
  return 0;
}
