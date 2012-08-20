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
char bcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};

int check_mac_addr(char m1[6],char m2[6]) {
       /* check if two mac addr match */
      return ((m1[0]==m2[0])&&(m1[1]==m2[1])&&(m1[2]==m2[2])&&(m1[3]==m2[3])&&(m1[4]==m2[4])&&(m1[5]==m2[5])) ;
}

int recv_packet(int socket,char* frame) {
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
   printf("Succesffully bound to %s \n",if_name);
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
  char client_mac[6]; /* read from command line */
  if(argc < 4)  {
     printf("Usage: packet-forwarder ingress-interface egress-interface client-mac \n");
     exit(EXIT_FAILURE);
  }
  else {
     strcpy(ingress,argv[1]);
     strcpy(egress,argv[2]);
     sscanf(argv[3], "%x:%x:%x:%x:%x:%x", &client_mac[0], &client_mac[1], &client_mac[2], &client_mac[3], &client_mac[4], &client_mac[5]);
  }
    
  /* variable decl */
  int ingress_socket,egress_socket,recv_bytes,sent_bytes,rc;

  /* Allocate memory for the buffers */
  unsigned char* ether_frame = (unsigned char *) malloc (IP_MAXPACKET);
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

  while(1) {
    /* poll both ingress and egress sockets */ 
    ppoll( poll_fds, 2, NULL, NULL ); 
    
    /* from ingress socket to egress */  
    if ( poll_fds[ 0 ].revents & POLLIN ) {
      recv_bytes = recv_packet(ingress_socket, ether_frame) ; 
      /* parse src mac */
      char* src_mac=(char*)(ether_frame +6); 
      if (  check_mac_addr(src_mac,client_mac) )   {
#ifdef DEBUG
             printf("Received packet of %d bytes on ingress from client \n",recv_bytes);   
#endif
             if ((sent_bytes = send(egress_socket,ether_frame,recv_bytes,MSG_TRUNC))<0) {
               perror("send() on egress failed:");
               exit(EXIT_FAILURE);
             }
      }
    }
    
    /* egress to ingress */ 
    else if ( poll_fds[ 1 ].revents & POLLIN ) {
      recv_bytes = recv_packet(egress_socket, ether_frame)  ;
      /* parse dst mac */
      char* dst_mac=(char*)(ether_frame);
      if ( check_mac_addr(dst_mac,client_mac) || check_mac_addr(dst_mac,bcast) )   {
#ifdef DEBUG
          if      (check_mac_addr(dst_mac,client_mac)) printf("Received packet of %d bytes on egress to client \n",recv_bytes);   
          else if (check_mac_addr(dst_mac,bcast)) printf("Received packet of %d bytes on egress to broadcast \n",recv_bytes);   
#endif
          if ((sent_bytes = send(ingress_socket,ether_frame,recv_bytes,MSG_TRUNC))<0) {
            perror("send() on ingress failed:");
            exit(EXIT_FAILURE);
          }
      }
    }
  }
  return 0;
}
