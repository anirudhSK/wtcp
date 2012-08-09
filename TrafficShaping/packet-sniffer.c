#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdlib.h>
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <errno.h>            // errno, perror()
#include <stdio.h>           // close()
#include <string.h>          // strcpy, memset(), and memcpy()

int main() {
  /* variable decl */
  int packet_socket,recv_bytes;

  /* Allocate memory for the buffers */
  unsigned char* ether_frame = (unsigned char *) malloc (IP_MAXPACKET);
  if (ether_frame == NULL) {
    fprintf (stderr, "ERROR: Cannot allocate memory for array 'ether_frame'.\n");
    exit (EXIT_FAILURE);
  }

  /* Zero out all bytes */
  memset (ether_frame, 0, IP_MAXPACKET);

  /* Create a packet socket, that binds to eth0 by default. Receive all packets using ETH_P_ALL */
  if ((packet_socket = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

  /* receive in a tight loop, use MSG_TRUNC to get actual msg length */
  while(1) {
    if ((recv_bytes = recv (packet_socket, ether_frame, IP_MAXPACKET, MSG_TRUNC)) < 0) {
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
  }
  return 0;
}
