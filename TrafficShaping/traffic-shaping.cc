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
#include "unrestrained-link.hh"
#include "token-bucket.hh"
#include "cbr-link.hh"
#include "schedule-link.hh"
#include "trace-link.hh"
#include "rate-schedule.hh"
#include<iostream>
#include<vector>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <sys/signalfd.h>
#include <signal.h>
#include <sys/time.h>
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

int recv_packet(int socket,uint8_t* frame,uint64_t *rx_ts) {
     /* data structure to receive timestamp, source address, and payload */
     struct msghdr header;
     struct iovec msg_iovec;
   
     const int BUF_SIZE = 2048;
   
     char msg_control[ BUF_SIZE ];
     header.msg_name = NULL;
     header.msg_namelen = 0;
     msg_iovec.iov_base = frame;
     msg_iovec.iov_len = BUF_SIZE;
     header.msg_iov = &msg_iovec;
     header.msg_iovlen = 1;
     header.msg_control = msg_control;
     header.msg_controllen = BUF_SIZE;
     header.msg_flags = 0;
   
     ssize_t received_len = recvmsg( socket, &header, 0 );
     if ( received_len < 0 ) {
       perror( "recvmsg" );
       exit( 1 );
     }
   
     if ( received_len > BUF_SIZE ) {
       fprintf( stderr, "Received oversize datagram (size %d) and limit is %d\n",
       static_cast<int>( received_len ), BUF_SIZE );
       exit( 1 );
     }
   
     /* verify presence of timestamp */
     struct cmsghdr *ts_hdr = CMSG_FIRSTHDR( &header );
     assert( ts_hdr );
     assert( ts_hdr->cmsg_level == SOL_SOCKET );
     assert( ts_hdr->cmsg_type == SO_TIMESTAMPNS );

     /* recv packet and time stamp */
     struct timespec ts=*(struct timespec *)CMSG_DATA( ts_hdr );
     *rx_ts=1.e9*ts.tv_sec+ts.tv_nsec;

     return received_len;
}

int bind_to_if(const char* if_name) {
  /* Bind to interface, return a socket 
     code that actually works and doesn't get random packets from other interfaces */
  int socket_fd;
  /* Create a packet socket, that binds to eth0 by default. Receive all packets using ETH_P_ALL */
  if ((socket_fd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }
  /* Now get interface details */ 
  struct sockaddr_ll sll;
  struct ifreq ifr;
  bzero(&sll, sizeof(sll));
  bzero(&ifr, sizeof(ifr));
  /* First Get the Interface Index */
  strncpy((char *)ifr.ifr_name, if_name, IFNAMSIZ);
  if((ioctl(socket_fd, SIOCGIFINDEX, &ifr)) == -1) {
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

  if((bind(socket_fd, (struct sockaddr *)&sll, sizeof(sll)))== -1) {
    perror("Error binding raw socket to interface\n");
    exit(-1);
  }
  /* Set timestamp option for socket */
  int ts_opt = 1;
  if ( setsockopt( socket_fd, SOL_SOCKET, SO_TIMESTAMPNS, &ts_opt, sizeof( ts_opt ) )
       < 0 ) {
    perror( "setsockopt" );
    exit( 1 );
  }
 return socket_fd;
}

int main(int argc,char** argv) {
  /* command line handling */
  std::string ingress,egress;
  uint8_t client_mac[6]; /* read from command line */

  /* ingress and egress Links */ 
  Link *uplink,*downlink;

  /* variable decl */
  int ingress_socket,egress_socket,recv_bytes;

  /* log kernel timestamp */
  uint64_t rx_ts;
  /* declare the supported options. */
  po::options_description desc("Allowed options");
  desc.add_options()
      ("ingress",po::value<std::string>(), "ingress interface name")
      ("egress" ,po::value<std::string>(), "egress interface name")
      ("client-mac" ,po::value<std::string>(), "client's MAC address")
      ("uplink-rate" ,po::value<float>(), "uplink rate in bits per sec")
      ("downlink-rate" ,po::value<float>(), "downlink rate in bits per sec")
      ("uplink-schedule" ,po::value<std::string>(), "filename with uplink rate schedule")
      ("downlink-schedule" ,po::value<std::string>(), "filename with downlink rate schedule")
      ("uplink-trace" ,po::value<std::string>(), "filename with uplink trace")
      ("downlink-trace" ,po::value<std::string>(), "filename with downlink trace")
      ("help" ,"produce help message")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    
  
  if (vm.count("help")) {
      std::cout << desc << "\n";
      return 1;
  }
  if (vm.count("ingress")&& vm.count("egress") && vm.count("client-mac") && vm.count("uplink-rate") && vm.count("downlink-rate")) {
    ingress=vm["ingress"].as<std::string>();
    egress=vm["egress"].as<std::string>();
    float uplink_rate=vm["uplink-rate"].as<float>();
    float downlink_rate=vm["downlink-rate"].as<float>();
    read_mac_addr(vm["client-mac"].as<std::string>().c_str(),client_mac);
    /* bind to ingress and egress */
    ingress_socket=bind_to_if(ingress.c_str()); 
    egress_socket=bind_to_if(egress.c_str()); 
    /* uplink and downlink */
    std::cout<<"CBR link here \n";
    if(downlink_rate<0) downlink=new UnrestrainedLink (ingress_socket,true,"downlink");
    else                downlink=new CbrLink(ingress_socket,true,"downlink",downlink_rate);
    if(uplink_rate<0) uplink=new UnrestrainedLink (egress_socket,true,"uplink");
    else              uplink=new CbrLink(egress_socket,true,"uplink",uplink_rate);
  } 
  else if (vm.count("ingress")&& vm.count("egress") && vm.count("client-mac") && vm.count("uplink-schedule") && vm.count("downlink-schedule")) {
    ingress=vm["ingress"].as<std::string>();
    egress=vm["egress"].as<std::string>();
    std::string uplink_schedule=vm["uplink-schedule"].as<std::string>();
    std::string downlink_schedule=vm["downlink-schedule"].as<std::string>();
    read_mac_addr(vm["client-mac"].as<std::string>().c_str(),client_mac);
    /* bind to ingress and egress */
    ingress_socket=bind_to_if(ingress.c_str()); 
    egress_socket=bind_to_if(egress.c_str()); 
    /* uplink and downlink */
    std::cout<<"Reading from a schedule here \n";
    downlink=new ScheduleLink(ingress_socket,true,"downlink",downlink_schedule);
    uplink=new ScheduleLink(egress_socket,true,"uplink",uplink_schedule);
  }
  else if (vm.count("ingress")&& vm.count("egress") && vm.count("client-mac") && vm.count("uplink-trace") && vm.count("downlink-trace")) {
    ingress=vm["ingress"].as<std::string>();
    egress=vm["egress"].as<std::string>();
    std::string uplink_trace=vm["uplink-trace"].as<std::string>();
    std::string downlink_trace=vm["downlink-trace"].as<std::string>();
    read_mac_addr(vm["client-mac"].as<std::string>().c_str(),client_mac);
    /* bind to ingress and egress */
    ingress_socket=bind_to_if(ingress.c_str()); 
    egress_socket=bind_to_if(egress.c_str()); 
    /* uplink and downlink */
    std::cout<<"Reading from a trace here \n";
    downlink=new TraceLink(ingress_socket,true,"downlink",downlink_trace);
    uplink=new TraceLink(egress_socket,true,"uplink",uplink_trace);
  }

  else {
    std::cout << desc << "\n";
    return 1;
  }
   
  
  /* Allocate memory for the buffers */
  uint8_t* ether_frame = (uint8_t *) malloc (IP_MAXPACKET);
  if (ether_frame == NULL) {
    fprintf (stderr, "ERROR: Cannot allocate memory for array 'ether_frame'.\n");
    exit (EXIT_FAILURE);
  }

  /* Zero out all bytes */
  memset (ether_frame, 0, IP_MAXPACKET);

  /* 1000Hz timer */
  /* UNIX signal data structures */
  sigset_t mask;
  int sfd;
  sigemptyset(&mask);
  sigaddset(&mask, SIGALRM);

  /* 1ms timer */ 
  struct itimerval it_val,old_val;
  it_val.it_interval.tv_sec=0;
  it_val.it_interval.tv_usec=1000;
  it_val.it_value.tv_sec=0;
  it_val.it_value.tv_usec=1000;
  setitimer(ITIMER_REAL,&it_val,&old_val);

  /* block signals so that they aren't handled by default*/
  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
      perror("sigprocmask");

  /* create signal fd */ 
  sfd = signalfd(-1, &mask, 0);
  if (sfd == -1)
      perror("signalfd");

  /* create a poll structure for ingress and egress */
  struct pollfd poll_fds[ 3 ];
  poll_fds[ 0 ].fd = ingress_socket;
  poll_fds[ 0 ].events = POLLIN;

  poll_fds[ 1 ].fd = egress_socket;
  poll_fds[ 1 ].events = POLLIN;

  poll_fds[ 2 ].fd = sfd;
  poll_fds[ 2 ].events = POLLIN;

  struct timespec timeout;
  timeout.tv_sec=0;
  timeout.tv_nsec=0;

  while(1) {
    /* send packets if possible */ 
    /* poll both ingress and egress sockets */ 
    poll( poll_fds, 3, 0 ); 
    /* First poll timer */
    if ( poll_fds[ 2 ].revents & POLLIN ) {
           uint8_t buf[1000];
           if( (read(poll_fds [ 2 ].fd,buf,sizeof(buf))) < 0) {
             perror("signalfd read \n");
             exit(-1);
           }
         /* send packets if possible */ 
       uplink->tick(); 
       downlink->tick();
    }

    /* from ingress socket to egress */  
    if ( poll_fds[ 0 ].revents & POLLIN ) {
      recv_bytes = recv_packet(ingress_socket, ether_frame,&rx_ts) ; 
      /* parse src mac */
      uint8_t* src_mac=(uint8_t*)(ether_frame +6); 
      if (  check_mac_addr(src_mac,client_mac) )   {
#ifdef DEBUG
           printf("Received packet of %d bytes on ingress from client \n",recv_bytes);   
#endif
           uplink->recv(ether_frame,recv_bytes,rx_ts);
      }
    }
    /* egress to ingress */ 
    if ( poll_fds[ 1 ].revents & POLLIN ) {
      recv_bytes = recv_packet(egress_socket, ether_frame,&rx_ts)  ;
      /* parse dst mac */
      uint8_t* dst_mac=(uint8_t*)(ether_frame);
      if ( check_mac_addr(dst_mac,client_mac) || check_mac_addr(dst_mac,bcast) )   {
#ifdef DEBUG
          if (check_mac_addr(dst_mac,client_mac)) printf("Received packet of %d bytes on egress to client \n",recv_bytes);   
          else if (check_mac_addr(dst_mac,bcast)) printf("Received packet of %d bytes on egress to broadcast \n",recv_bytes);   
#endif
          downlink->recv(ether_frame,recv_bytes,rx_ts);
      }
    }
    std::cout.flush();
  }
  return 0;
}
