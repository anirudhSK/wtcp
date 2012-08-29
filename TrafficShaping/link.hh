/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef LINK_HH
#define LINK_HH
#include "payload.hh"
#include "rate-schedule.hh"
#include <queue>             
class Link {
  private : 
    /* Queues and buffers */
    uint32_t byte_queue_occupancy ; /* # bytes   in queue */
    uint64_t BUFFER_SIZE_BYTES;
    int      link_socket;           /* file descriptor of the link */ 

    /* Statistics */
    uint64_t total_bytes;           /* total bytes ever sent on this link */
    uint64_t last_stat_update;      /* last time stats were printed */
    uint64_t last_stat_bytes;       /* number of bytes printed last time */

    /* monitoring */ 
    bool output_enable;             /* enable or disable stat printing */
 protected :
    std::string link_name;          /* Use this while printing stats */
    std::queue <Payload> pkt_queue; 
    uint64_t next_transmission;
    uint64_t begin_time;            /* time stamp when link went "on" */ 

 public : 
    uint32_t pkt_queue_occupancy ;  /* # packets in queue */
 
    Link(int fd,bool t_output_enable,std::string t_link_name);
    int enqueue(Payload p); 
    int dequeue(); 
    void send_pkt(Payload p);
    static uint64_t timestamp(void) ; 
    uint64_t wait_time_ns( void ) const ;
    void print_stats(uint64_t ts_now); 
 
    virtual void tick() = 0;
    virtual int recv(uint8_t* ether_frame,uint16_t size) = 0;
    virtual ~Link();
};   
#endif
