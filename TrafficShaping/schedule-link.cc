#include"schedule-link.hh"
#include"rate-schedule.hh"
ScheduleLink::ScheduleLink(int fd,bool t_output_enable,std::string t_link_name,std::string schedule_file) :
   TokenBucketLink(fd,t_output_enable,t_link_name,0),
   rate_schedule(RateSchedule (schedule_file)) {
   link_rate=rate_schedule.current_rate;
   std::cout<<link_name<<" starting rate is"<<link_rate<<std::endl;
}

void ScheduleLink::check_current_rate(uint64_t ts) {
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

void ScheduleLink::update_token_count(uint64_t current_ts,long double drain) {
     check_current_rate(current_ts);
     /* check to see if you need to update rate */ 
     uint64_t elapsed = current_ts - last_token_update ;
     /* get new count */
     long double new_token_count=token_count+elapsed*(link_rate/8.0)*1.e-9-drain; 
     /* new token count in bytes */
     last_token_update=current_ts;
     /* change the last_token_update variable */ 
     token_count=(new_token_count > BURST_SIZE) ? BURST_SIZE : new_token_count;
     /* limit token_count to BURST_SIZE */
}
