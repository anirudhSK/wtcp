/* Implements a tail drop buffer of a specified size on a link */ 
#ifndef SCHEDULE_LINK_HH
#define SCHEDULE_LINK_HH
#include "token-bucket.hh"
class ScheduleLink: public TokenBucketLink {
   public : 
     ScheduleLink(int fd,bool output_enable,std::string link_name,std::string schedule_file);
     void check_current_rate(uint64_t current_ts) ; 
     virtual void update_token_count(uint64_t current_ts,long double drain);
   private : 
     RateSchedule rate_schedule;     /* schedule for link rates */  
};   
#endif
