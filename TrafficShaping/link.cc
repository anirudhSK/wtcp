#include "link.hh"
#include <sys/time.h>
#include<assert.h>
#include<sys/socket.h>
#include<iostream>
Link::Link(RateSchedule t_rate_schedule,int fd,bool t_output_enable,std::string t_link_name,bool t_unrestrained)
 : pkt_queue(),
   byte_queue_occupancy(0),
   next_transmission(-1),
   last_token_update(Link::timestamp()),  /* token count is in bytes */
   token_count(0),
   BUFFER_SIZE_BYTES(1000000000),
   BURST_SIZE(1600), /* 1 packet */
   link_socket(fd),
   total_bytes(0),
   begin_time(Link::timestamp()),
   last_stat_update(0),
   last_stat_bytes(0), 
   output_enable(t_output_enable),
   link_name(t_link_name) ,
   unrestrained(t_unrestrained),
   rate_schedule(t_rate_schedule),
   link_rate(rate_schedule.current_rate), /* bits per second */
   pkt_queue_occupancy(0)  {
   std::cout<<link_name<<" starting rate is"<<link_rate<<std::endl;
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
     if(unrestrained) { 
       /* no need to traffic shape, send packet right away. That way tick will always find an empty queue */
       send_pkt(p); 
       return 0;
     } 
     return enqueue(p); 
}

void Link::print_stats(uint64_t ts_now){
  if(output_enable) {
   if(ts_now>last_stat_update+1e9)  {/* 1 second ago */
          std::cout<<link_name<<" at time " <<ts_now<<" , queue is " <<byte_queue_occupancy<<" , "<<" @ "<<(float)(8*(total_bytes-last_stat_bytes)*1e9)/(ts_now-last_stat_update)<<" bits per sec "<<token_count<<" tokens \n";
          last_stat_update=ts_now;
          last_stat_bytes=total_bytes;
   }
  }
}

void Link::check_current_rate(uint64_t ts) {
      if((ts-begin_time)>=(uint64_t)rate_schedule.next_timestamp*1e9) { /* nano secs to secs conversion */ 
        assert((!rate_schedule.rate_list.empty()));  /* schedule list can't be empty */
        link_rate=std::get<1>(rate_schedule.rate_list.front()); 
        std::cout<<link_name<<" switched to new rate "<<link_rate<<" at time "<<ts<<" \n";
        rate_schedule.rate_list.pop_front(); 
        if(!rate_schedule.rate_list.empty()) {
          rate_schedule.next_timestamp=std::get<0>(rate_schedule.rate_list.front());
        }
        else {
          rate_schedule.next_timestamp=(uint16_t)-1;
        }
      }
}
void Link::tick() {

   uint64_t ts_now=Link::timestamp(); 
   print_stats(ts_now);
   if(unrestrained) return; /* Don't do anything */
   /* compare against last_token_update */
   uint64_t elapsed = ts_now - last_token_update ;
   /* get new count */
   long double new_token_count=token_count+elapsed*(link_rate/8.0)*1.e-9; /* token count in bytes */
   update_token_count(ts_now,new_token_count);
   check_current_rate(ts_now);
   /* Can I send pkts right away ? */ 
   if(!pkt_queue.empty()) { 
     Payload head=pkt_queue.front();
     while(token_count>=head.size && head.size > 0) {
        send_pkt(head);
        dequeue();
        ts_now=Link::timestamp();  
        elapsed = ts_now - last_token_update ;
        new_token_count=token_count-head.size+elapsed*(link_rate/8.0)*1.e-9; 
        update_token_count(ts_now,new_token_count);
        if(pkt_queue.empty()) head.size=-1;
        else  head=pkt_queue.front();
     }
     /* if there are packets wait till tokens accumulate in the future */ 
     if(!pkt_queue.empty())  {
      long double requiredTokens = head.size-token_count; 
      uint64_t wait_time_ns = (1.e9*requiredTokens) / (link_rate/8.0) ;  
      next_transmission=wait_time_ns+ts_now;
     }
     else next_transmission = (uint64_t)-1;
   }
   else next_transmission = (uint64_t)-1 ;
}

void Link::update_token_count(uint64_t current_ts,long double new_count) {
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
   total_bytes=total_bytes+sent_bytes;
}

uint64_t Link::wait_time_ns( void ) const
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
