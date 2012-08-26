#ifndef TOKEN_BUCKET_HH
#define TOKEN_BUCKET_HH
#include "link.hh"
class TokenBucketLink: public Link {
  private : 
    uint64_t last_token_update;     /* time at which token count was last updated */
    long double token_count ;        
    const uint32_t BURST_SIZE;
    double link_rate;
 public : 
 
    virtual void tick() ;           /*virtual cause this was taken from Link*/
    virtual int recv(uint8_t* ether_frame,uint16_t size) ; /* Same here */
    
    virtual void update_token_count(uint64_t current_ts)=0;/* defined in the derived class depending on how it chooses to update count */ 
    TokenBucketLink(int fd,bool t_output_enable,std::string t_link_name,double t_link_rate);
};
#endif
