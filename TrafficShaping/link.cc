#include "link.hh"
#include <sys/time.h>
#include<assert.h>
#include<sys/socket.h>
#include<iostream>
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

void Link::print_stats(uint64_t ts_now){
  if(output_enable) {
   if(ts_now>last_stat_update+1e9)  {/* 1 second ago */
          double bitrate=(double)(8*(total_bytes-last_stat_bytes)*1e9)/(ts_now-last_stat_update);
          std::cout<<link_name<<" at time " <<ts_now<<" , "<<(ts_now-begin_time)/1.e9<<" seconds since start , queue in seconds is " <<(byte_queue_occupancy*8.0)/bitrate<<" , "<<" @ "<<bitrate<<" bits per sec "<<" queue latency estimate is "<<latency_estimator.get_latency(ts_now)<<" seconds \n";
          last_stat_update=ts_now;
          last_stat_bytes=total_bytes;
   }
  }
}

uint64_t Link::send_pkt(Payload p)  {
   int sent_bytes;
   if ((sent_bytes = send(link_socket,p.pkt_data,p.size,MSG_TRUNC))<0) {
               std::cout<<"Trying to send payload with size "<<p.size<<"\n";
               perror("send() on egress failed:");
               exit(EXIT_FAILURE);
   }
   uint64_t sent_ts=Link::timestamp();
   total_bytes=total_bytes+sent_bytes;
   return sent_ts;
}

uint64_t Link::wait_time_ns( void ) const
{
  uint64_t current_ts=Link::timestamp();
  if(next_transmission<current_ts)  /* time has already passed */ {
   return 0;
  }
  else {                            /* wont return null value */
   return next_transmission - current_ts;
  }
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

Link::~Link() {

}

Link::Link(int fd,bool t_output_enable,std::string t_link_name)
 : byte_queue_occupancy(0),
   BUFFER_SIZE_BYTES(1000000000),
   link_socket(fd),
   total_bytes(0),
   last_stat_update(0),
   last_stat_bytes(0), 
   output_enable(t_output_enable),
   link_name(t_link_name) ,
   pkt_queue(),
   next_transmission(-1),
   begin_time(Link::timestamp()),
   latency_estimator(0,1000),
   pkt_queue_occupancy(0) {

}
