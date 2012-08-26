#include"rate-schedule.hh"
RateSchedule::RateSchedule(std::string t_file_name) :
     file_name(t_file_name) ,
     next_timestamp(-1),
     current_rate(0),
     rate_list(std::list<std::tuple<uint16_t,uint32_t>>()) {
     /* populate rate_list using the file */
     std::ifstream rate_stream (file_name.c_str());
     uint16_t time;
     uint32_t rate;
     /* TODO Check that file actually exists */ 
     while (true) {
       rate_stream>>time>>rate;
       if( rate_stream.eof() ) break;
       rate_list.push_back(std::tuple<uint16_t,uint32_t>(time,rate)); /* time in seconds */ 
     }
     assert(rate_list.size()>=2);
     current_rate=std::get<1>(rate_list.front());
     rate_list.pop_front();
     next_timestamp=std::get<0>(rate_list.front());
}

RateSchedule::RateSchedule(uint32_t link_rate) :
     file_name("") ,
     next_timestamp(-1),
     current_rate(link_rate),
     rate_list(std::list<std::tuple<uint16_t,uint32_t>>()) {
     /* populate rate_list using the file */
     rate_list.push_back(std::tuple<uint16_t,uint32_t>(0,link_rate)); /* time in seconds */ 
}

RateSchedule::RateSchedule() :
     file_name("") ,
     next_timestamp(-1),
     current_rate(-1),
     rate_list(std::list<std::tuple<uint16_t,uint32_t>>()) {
     /* populate rate_list using the file */
}
