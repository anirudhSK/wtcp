#ifndef RATE_SCHEDULE
#define RATE_SCHEDULE
#include<list>
#include<tuple>
#include<string>
#include<iostream>
#include<fstream>
#include<assert.h>
class RateSchedule {
   /* a schedule of rates for the experiment. 
      Could either be synthetic or trace driven */
   private :
     std::string file_name;
     
   public:
     uint16_t next_timestamp;
     uint32_t current_rate; 
     std::list <std::tuple<uint16_t,uint32_t>> rate_list; 

     RateSchedule();     
     RateSchedule(uint32_t link_rate); /* Set single link rate for entire experiment */
     RateSchedule(std::string file_name) ; /* Read synthetic schedule from file */
};
#endif
