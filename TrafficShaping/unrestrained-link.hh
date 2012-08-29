/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef UNRESTRAINED_LINK_HH
#define UNRESTRAINED_LINK_HH
#include "link.hh"
class UnrestrainedLink: public Link {
   public : 
     UnrestrainedLink(int fd,bool output_enable,std::string link_name);
     virtual int recv(uint8_t* ether_frame,uint16_t size);
     virtual void tick();
};   
#endif
