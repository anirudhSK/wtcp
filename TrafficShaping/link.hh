/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef LINK_HH
#define LINK_HH
#include "payload.hh"
#include <queue>             
class Link {
  private : 
    std::queue <Payload> pkt_queue; 
    uint32_t pkt_queue_occupancy ;  /* # packets in queue */
    uint32_t byte_queue_occupancy ; /* # bytes   in queue */
    uint64_t next_transmission;
    uint64_t last_token_update;     /* time at which token count was last updated */
    uint64_t token_count ;        
    const uint64_t BUFFER_SIZE_BYTES;
    const uint32_t BURST_SIZE;
    int      link_socket;           /* file descriptor of the link */ 

 public : 
    double link_rate ;     /* current link rate */ 
   
    Link(double rate, int fd);  
    int enqueue(Payload p); 
    int dequeue(); 
    void tick() ; 
    void update_token_count(uint64_t current_ts,uint32_t new_count); 
    void send_pkt(Payload p);
    int recv(uint8_t* ether_frame,uint16_t size) ; 
    static uint64_t timestamp(void) ; 
    int wait_time_ns( void ) const ;
};   
#endif
