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
  if(argc < 3)  {
     printf("Usage: packet-forwarder ingress-interface egress-interface \n");
     exit(EXIT_FAILURE);
  }
  else {
     strcpy(ingress,argv[1]);
     strcpy(egress,argv[2]);
  }
    
  /* variable decl */
  int recv_socket,send_socket,recv_bytes,sent_bytes,rc;

  /* Allocate memory for the buffers */
  unsigned char* ether_frame = (unsigned char *) malloc (IP_MAXPACKET);
  if (ether_frame == NULL) {
    fprintf (stderr, "ERROR: Cannot allocate memory for array 'ether_frame'.\n");
    exit (EXIT_FAILURE);
  }

  /* Zero out all bytes */
  memset (ether_frame, 0, IP_MAXPACKET);

  /* Create a packet socket, that binds to eth0 by default. Receive all packets using ETH_P_ALL */
  if ((recv_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* bind to interface eth0 */
  bind_to_if(recv_socket,ingress); 

  /* Create another packet socket to send packets to. In some sense we are a dumb software repeater */
  if ((send_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* bind to interface eth1 */
  bind_to_if(send_socket,egress); 

  /* receive in a tight loop, use MSG_TRUNC to get actual msg length */
  while(1) {
    if ((recv_bytes = recv (recv_socket, ether_frame, IP_MAXPACKET, MSG_TRUNC)) < 0) {
        if (errno == EINTR) {
          memset (ether_frame, 0, IP_MAXPACKET);
          continue;  // Something weird happened, but let's try again.
        } else {
          perror ("recv() failed:");
          exit (EXIT_FAILURE);
        }
    }
    else { 
          printf("Packet received with %d bytes  \n",recv_bytes);
    }
    if ((sent_bytes = send(send_socket,ether_frame,recv_bytes,MSG_TRUNC))<0) {
         perror("send() failed:");
         exit(EXIT_FAILURE);
    }
    else {
         printf("Sent out %d bytes \n",sent_bytes);
    }
  }
  return 0;
}
