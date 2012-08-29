/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef TRACE_LINK_HH
#define TRACE_LINK_HH
#include "link.hh"
#include <queue>             
class TraceLink: public Link {
   private : 
    uint64_t last_token_update;    
    /* time at which token count was last updated */
    long double token_count ;        
    const uint32_t BURST_SIZE;
    class PktSchedule {
     /* a schedule of packet arrivals for the experiment. 
          trace driven */
     private :
         std::string file_name;
     public:
         uint64_t next_timestamp;
         uint32_t current_byte_credit; 
         /* list of nanosec,byteCounter tuples */ 
         std::list <std::tuple<uint64_t,uint32_t>> pkt_list; 
         PktSchedule(std::string file_name) ;  
     };
     PktSchedule pkt_schedule;
 public : 
 
    virtual void tick() ;          
    /*virtual cause this was taken from Link*/
    virtual int recv(uint8_t* ether_frame,uint16_t size,uint64_t rx_timestamp) ;
    /* Same here */
    void update_token_count(uint64_t current_ts,long double drained); 
     TraceLink(int fd,bool output_enable,std::string link_name,std::string trace_file);
    uint64_t calc_next_time(uint32_t head_size);
};
#endif
