/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef LINK_HH
#define LINK_HH
#include "payload.hh"
#include <queue>             
#define DEBUG
class Link {
  private : 
    std::queue <Payload> pkt_queue; 
    uint32_t pkt_queue_occupancy ;  /* # packets in queue */
    uint32_t byte_queue_occupancy ; /* # bytes   in queue */
    uint64_t next_transmission;
    uint64_t last_token_update;     /* time at which token count was last updated */
    long double token_count ;        
    const uint64_t BUFFER_SIZE_BYTES;
    const uint32_t BURST_SIZE;
    int      link_socket;           /* file descriptor of the link */ 

    uint64_t total_bytes;           /* total bytes ever sent on this link */
    uint64_t begin_time;            /* time stamp when link went "on" */ 
    uint64_t last_stat_update;      /* last time stats were printed */
    uint64_t last_stat_bytes;       /* number of bytes printed last time */

    bool output_enable;             /* enable or disable stat printing */
    std::string link_name;          /* Use this while printing stats */
 public : 
    double link_rate ;     /* current link rate */ 
   
    Link(double rate, int fd,bool t_output_enable,std::string t_link_name);  
    int enqueue(Payload p); 
    int dequeue(); 
    void tick() ; 
    void update_token_count(uint64_t current_ts,long double new_count); 
    void send_pkt(Payload p);
    int recv(uint8_t* ether_frame,uint16_t size) ; 
    static uint64_t timestamp(void) ; 
    int wait_time_ns( void ) const ;
    void print_stats(uint64_t ts_now);
};   
#endif
