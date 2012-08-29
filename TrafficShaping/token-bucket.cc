#include"token-bucket.hh"
int TokenBucketLink::recv(uint8_t* ether_frame,uint16_t size,uint64_t rx_ts) {
     Payload p(ether_frame,size,rx_ts); /* */ 
     return enqueue(p); 
}

void TokenBucketLink::tick() {
   uint64_t ts_now=Link::timestamp(); 
   Link::print_stats(ts_now);
   update_token_count(ts_now,0);
   /* Can I send pkts right away ? */ 
   if(!pkt_queue.empty()) { 
     Payload head=pkt_queue.front();
     while(token_count>=head.size && head.size > 0) {
        ts_now=send_pkt(head);
        dequeue();
        head.sent_ts=ts_now; // get sent_ts
        latency_estimator.add_latency_sample(head);
        update_token_count(ts_now,head.size);
        if(pkt_queue.empty()) head.size=-1;
        else  head=pkt_queue.front();
     }
     /* if there are packets wait till tokens accumulate in the future */ 
   #ifndef BUSY_WAIT
     if(!pkt_queue.empty())  {
      long double requiredTokens = head.size-token_count; 
      uint64_t wait_time_ns = (1.e9*requiredTokens) / (link_rate/8.0) ;  
      next_transmission=wait_time_ns+ts_now;
     }
     else next_transmission = (uint64_t)-1;
   #endif
   }
   #ifndef BUSY_WAIT
   else next_transmission = (uint64_t)-1 ;
   #endif
}

TokenBucketLink::TokenBucketLink(int fd,bool t_output_enable,std::string t_link_name,double t_link_rate) :
     Link(fd,t_output_enable,t_link_name) ,
     last_token_update(Link::timestamp()),
     token_count(0),
     BURST_SIZE(1600), /* 1 packet */
     link_rate(t_link_rate) {


}
