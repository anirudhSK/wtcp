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
#define DEBUG
#include<iostream>
uint8_t bcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};

int check_mac_addr(uint8_t m1[6],uint8_t m2[6]) {
       /* check if two mac addr match */
      return ((m1[0]==m2[0])&&(m1[1]==m2[1])&&(m1[2]==m2[2])&&(m1[3]==m2[3])&&(m1[4]==m2[4])&&(m1[5]==m2[5])) ;
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

void bind_to_if(int socket,char* if_name) {
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
  char ingress[10];
  char egress[10];
  uint8_t client_mac[6]; /* read from command line */
  float uplink_rate ; 
  float downlink_rate;
  if(argc < 6)  {
     printf("Usage: packet-forwarder ingress-interface egress-interface client-mac uplink-rate downlink-rate \n");
     exit(EXIT_FAILURE);
  }
  else {
     strcpy(ingress,argv[1]);
     strcpy(egress,argv[2]);
     sscanf(argv[3], "%hx:%hx:%hx:%hx:%hx:%hx", (short unsigned int *)&client_mac[0], 
                                                (short unsigned int *)&client_mac[1],
                                                (short unsigned int *)&client_mac[2],
                                                (short unsigned int *)&client_mac[3], 
                                                (short unsigned int *)&client_mac[4], 
                                                (short unsigned int *)&client_mac[5]);
      uplink_rate=atof(argv[4])/8.0;
      downlink_rate=atof(argv[5])/8.0;          /* rates are in bits per sec */
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
  bind_to_if(ingress_socket,ingress); 

  /* Create another packet socket for the egress . In some sense we are a dumb software repeater */
  if ((egress_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* bind to egress */
  bind_to_if(egress_socket,egress); 

  /* create a poll structure for ingress and egress */
  struct pollfd poll_fds[ 2 ];
  poll_fds[ 0 ].fd = ingress_socket;
  poll_fds[ 0 ].events = POLLIN;

  poll_fds[ 1 ].fd = egress_socket;
  poll_fds[ 1 ].events = POLLIN;

  /* Ingress and egress Links */ 
  Link uplink(uplink_rate,egress_socket,true,"uplink"); /* bytes per second */ 
  Link downlink(downlink_rate,ingress_socket,true,"downlink");
  while(1) {
    /* send packets if possible */ 
    uplink.tick(); 
    downlink.tick();
    /* set timeouts */ 
    struct timespec timeout;
    uint64_t next_transmission_delay = std::min( uplink.wait_time_ns(), downlink.wait_time_ns() );
#ifdef DEBUG
//    std::cout<<"Waiting "<<next_transmission_delay<<" ns in ppoll "<<std::endl;
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
