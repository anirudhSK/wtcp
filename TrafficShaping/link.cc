#include "link.hh"
#include <sys/time.h>
#include<assert.h>
#include<sys/socket.h>
Link::Link(double rate,int fd)
 : pkt_queue(),
   pkt_queue_occupancy(0),
   byte_queue_occupancy(0),
   next_transmission(-1),
   last_token_update(Link::timestamp()),
   token_count(0),
   BUFFER_SIZE_BYTES(1000000),
   BURST_SIZE(1500), /* 1 packet */
   link_socket(fd),
   link_rate(rate) {

}

int Link::dequeue() {
  if(pkt_queue.empty()) return -1;  /* underflow */
  Payload p=pkt_queue.front();
  pkt_queue_occupancy--;
  byte_queue_occupancy=byte_queue_occupancy-p.size; 
  pkt_queue.pop();
  return 0; 
}

int Link::enqueue(Payload p) {
   if(byte_queue_occupancy+p.size>BUFFER_SIZE_BYTES) return -1; /* overflow */
   pkt_queue.push(p); 
   byte_queue_occupancy=byte_queue_occupancy+p.size; 
   pkt_queue_occupancy++;
   return 0; 
}

int Link::recv(uint8_t* ether_frame,uint16_t size) {
     Payload p(ether_frame,size); /* */ 
     return enqueue(p); 
}

void Link::tick() {
   uint64_t ts_now=Link::timestamp(); 
   /* compare against last_token_update */
   uint64_t elapsed = ts_now - last_token_update ;
   /* get new count */
   uint32_t new_token_count=token_count+elapsed*link_rate*1.e-9; 
   update_token_count(ts_now,new_token_count);
   /* Can I send pkts right away ? */ 
   if(!pkt_queue.empty()) { 
     Payload head=pkt_queue.front();
     while(token_count>=head.size || !pkt_queue.empty() ) { 
        ts_now=Link::timestamp();     
        send_pkt(head);
        pkt_queue.pop(); /* remove the head */ 
        elapsed = ts_now - last_token_update ;
        new_token_count=token_count-head.size+elapsed*link_rate; 
        update_token_count(ts_now,new_token_count);
        head=pkt_queue.front();
     }
     /* if there are packets wait till tokens accumulate in the future */ 
     if(!pkt_queue.empty())  {
      uint32_t requiredTokens = head.size-token_count; 
      uint64_t wait_time_ns = (1.e9*requiredTokens) / link_rate ;  
      next_transmission=wait_time_ns+ts_now;
     }
   }
   else next_transmission = -1 ;
}

void Link::update_token_count(uint64_t current_ts,uint32_t new_count) {
      /* maintain invariant */ 
      last_token_update=current_ts; 
      token_count=(new_count > BURST_SIZE) ? BURST_SIZE : new_count;
}

void Link::send_pkt(Payload p)  {
   int sent_bytes;
   if ((sent_bytes = send(link_socket,p.pkt_data,p.size,MSG_TRUNC))<0) {
               perror("send() on egress failed:");
               exit(EXIT_FAILURE);
   }
}

int Link::wait_time_ns( void ) const
{
  return next_transmission - Link::timestamp();
}

uint64_t Link::timestamp( void )
{
  struct timespec ts;
  if ( clock_gettime( CLOCK_REALTIME, &ts ) < 0 ) {
    perror( "clock_gettime" );
    exit( 1 );
  }
  uint64_t ret = ts.tv_sec * 1000000000 + ts.tv_nsec;
  return ret;
}
