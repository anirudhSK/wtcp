/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef CBR_LINK_HH
#define CBR_LINK_HH
#include "token-bucket.hh"
#define DEBUG
class CbrLink: public TokenBucketLink {
   public : 
     CbrLink(int fd,bool output_enable,std::string link_name,double link_rate);
     virtual void update_token_count(uint64_t current_ts); 
};   
#endif
