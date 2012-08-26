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
          std::cout<<link_name<<" at time " <<ts_now<<" , queue is " <<byte_queue_occupancy<<" , "<<" @ "<<(float)(8*(total_bytes-last_stat_bytes)*1e9)/(ts_now-last_stat_update)<<" bits per sec "<<"\n";
          last_stat_update=ts_now;
          last_stat_bytes=total_bytes;
   }
  }
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
   pkt_queue_occupancy(0) {

}
