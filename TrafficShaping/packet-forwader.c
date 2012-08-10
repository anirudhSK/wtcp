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
char client_mac[6]={0x00,0x22,0x68,0x1c,0xa6,0x22};
char bcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};
int check_mac_addr(char m1[6],char m2[6]) {
       /* check if two mac addr match */
      return ((m1[0]==m2[0])&&(m1[1]==m2[1])&&(m1[2]==m2[2])&&(m1[3]==m2[3])&&(m1[4]==m2[4])&&(m1[5]==m2[5])) ;
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
  char client_position[15];
  if(argc < 4)  {
     printf("Usage: packet-forwarder ingress-interface egress-interface client-position \nSpecify behind-egress or behing-ingress fo client position \n");
     exit(EXIT_FAILURE);
  }
  else {
     strcpy(ingress,argv[1]);
     strcpy(egress,argv[2]);
     strcpy(client_position,argv[3]);
     if((!strcmp(client_position,"behind_ingress")==0 ) && (!strcmp(client_position,"behind_egress")==0)) {
       printf("invalid position \n");
       exit(EXIT_FAILURE);
     }
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

  /* bind to ingress */
  bind_to_if(recv_socket,ingress); 

  /* Create another packet socket to send packets to. In some sense we are a dumb software repeater */
  if ((send_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* bind to egress */
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
    /* parse src and dst mac */
    char* dst_mac=(char*)(ether_frame);
    char* src_mac=(char*)(ether_frame +6); 
   
    /* if client behind egress  : forward packets to   it only if they are broadcast or destined to client */
    if(strcmp("behind_egress",client_position)==0) {
         if ( check_mac_addr(dst_mac,client_mac) || check_mac_addr(dst_mac,bcast) )   {
             if ((sent_bytes = send(send_socket,ether_frame,recv_bytes,MSG_TRUNC))<0) {
               perror("send() failed:");
               exit(EXIT_FAILURE);
             }
         }
    }
    /* if client behind ingress : forward packets from it only if they are from the client.                */
    else if (strcmp("behind_ingress",client_position)==0) {
         if (  check_mac_addr(src_mac,client_mac) )   {
             if ((sent_bytes = send(send_socket,ether_frame,recv_bytes,MSG_TRUNC))<0) {
               perror("send() failed:");
               exit(EXIT_FAILURE);
             }
         }
    }
    else  {
      assert(0); // can't come here 
    }
  }
  return 0;
}


