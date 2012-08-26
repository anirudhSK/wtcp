#include"unrestrained-link.hh"
void UnrestrainedLink::tick() {
   /* print stats alone */
   uint64_t ts_now=Link::timestamp(); 
   Link::print_stats(ts_now);
}
int UnrestrainedLink::recv(uint8_t* ether_frame,uint16_t size) {
     Payload p(ether_frame,size); /* */ 
       /* no need to traffic shape, send packet right away. That way tick will always find an empty queue */
     Link::send_pkt(p); 
     return 0;
}

UnrestrainedLink::UnrestrainedLink(int fd, bool output_enable, std::string link_name)  : 
      Link::Link(fd,output_enable,link_name) {
}
