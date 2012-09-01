#include"cbr-link.hh"
void CbrLink::update_token_count(uint64_t current_ts,long double drained) {
     uint64_t elapsed = current_ts - last_token_update ;
     /* get new count */
     long double new_token_count=token_count+elapsed*(link_rate/8.0)*1.e-9-drained; 
     /* new token count in bytes */
     last_token_update=current_ts;
     /* change the last_token_update variable */ 
     token_count=new_token_count;
}

CbrLink::CbrLink(int fd,bool t_output_enable,std::string t_link_name,double t_link_rate)  :
  TokenBucketLink(fd,t_output_enable,t_link_name ,t_link_rate) {
}
