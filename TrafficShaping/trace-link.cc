#include"trace-link.hh"
TraceLink::TraceLink(int fd,bool t_output_enable,std::string t_link_name,std::string trace_file) :
   Link(fd,t_output_enable,t_link_name) ,
   last_token_update(begin_time),
   token_count(0),
   delivery_schedule(DeliverySchedule (trace_file)) {
   token_count=delivery_schedule.current_byte_credit;
   std::cout<<"Starting out with byte credit "<<token_count<<" at time "<<begin_time<<"\n"; 
}

int TraceLink::recv(uint8_t* ether_frame,uint16_t size,uint64_t rx_ts) {
     Payload p(ether_frame,size,rx_ts);
     return enqueue(p); 
}

void TraceLink::tick() {
   uint64_t ts_now=Link::timestamp();
   Link::print_stats(ts_now);
   update_token_count(ts_now);
   /* Can I send pkts right away ? */ 
   if(!pkt_queue.empty()) {
     Payload head=pkt_queue.front();
     while(token_count>=head.size ) {
        ts_now=send_pkt(head);
        dequeue();
        head.sent_ts=ts_now; // get sent_ts
        latency_estimator.add_latency_sample(head);        
        token_count=token_count-head.size;
        if(pkt_queue.empty()) break;
        else  head=pkt_queue.front();
     }
     /* if there are packets wait till tokens accumulate in the future */ 
     if(pkt_queue.empty())  {
       token_count=0;
     }
   }
}

void TraceLink::update_token_count(uint64_t current_ts) {
     /* check if you see a more recent line */
     double current_byte_credit=0 ; /* If there is a new point in the file , then update this */ 
     while((current_ts-begin_time)>=delivery_schedule.next_timestamp) {  
        assert((!delivery_schedule.pkt_delivery_list.empty())); 
        current_byte_credit += std::get<1>(delivery_schedule.pkt_delivery_list.front()); 
      #ifdef DEBUG
        std::cout<<link_name<<" Added credit of "<<current_byte_credit<<" bytes at time "<<current_ts<<" \n";
      #endif
        /* House keeping on pkt schedule */
        delivery_schedule.pkt_delivery_list.pop_front(); 
        if(!delivery_schedule.pkt_delivery_list.empty()) {
          delivery_schedule.next_timestamp=std::get<0>(delivery_schedule.pkt_delivery_list.front());
        }
        else {
          std::cout<<link_name<<" bad luck, no more credit left, kill simulation \n";
          exit(5);
        }
     }
     /* new token count in bytes */
     long double new_token_count=token_count+current_byte_credit; 
     last_token_update=current_ts;
     token_count=(new_token_count);
}

TraceLink::DeliverySchedule::DeliverySchedule(std::string t_file_name) :
     file_name(t_file_name) ,
     next_timestamp(-1),
     current_byte_credit(0),
     pkt_delivery_list(std::list<std::tuple<uint64_t,double>>()) {
     /* populate pkt_delivery_list using the file */
     std::ifstream pkt_stream (file_name.c_str());
     uint64_t time;
     uint32_t bytes;
     if(!pkt_stream.good()) {
         std::cout<<"Trace file "<<file_name<<" does not exist ... exiting \n";
         exit(-5);
     } 
     uint64_t prev_time=-1;
     while (true) {
       pkt_stream>>time>>bytes;
       if( pkt_stream.eof() ) break;
       if(time==prev_time) {
         std::tuple<uint64_t,double> front=pkt_delivery_list.back();
         uint32_t stored_credit=std::get<1>(front);
         uint32_t new_credit=stored_credit+bytes;
         pkt_delivery_list.pop_back(); /* remove old credit */
         pkt_delivery_list.push_back(std::tuple<uint64_t,double>(time*1e6,new_credit)); /* push agg. credit */ 
       }
       else {
        pkt_delivery_list.push_back(std::tuple<uint64_t,double>(time*1e6,bytes)); /* time in ns, trace comes in in ms */ 
       }
       prev_time=time;
     }
     /* before interpolation */
     FILE* fh=fopen((file_name+"pre-interp").c_str(),"w");
     dump_schedule(fh);
     interpolate();
     fh=fopen((file_name+"post-interp").c_str(),"w");
     dump_schedule(fh);
     
     assert(pkt_delivery_list.size()>=2);
     current_byte_credit=std::get<1>(pkt_delivery_list.front());
     pkt_delivery_list.pop_front();
     next_timestamp=std::get<0>(pkt_delivery_list.front());
}
void TraceLink::DeliverySchedule::dump_schedule(FILE* fh) {
    for (std::list<std::tuple<uint64_t,double>>::iterator it = pkt_delivery_list.begin(); it != pkt_delivery_list.end(); it++) {
        std::tuple<uint64_t,double> current=*it;
        double current_bytes=std::get<1>(current);
        uint64_t current_time=std::get<0>(current);
        fprintf(fh,"%ld \t %.5f \n",current_time,current_bytes);
    }
}
void TraceLink::DeliverySchedule::interpolate() {
    /* Spread out credits to make sure there are some credits every millisecond 
       This will reduce burstiness */
    std::list<std::tuple<uint64_t,double>>::iterator it = pkt_delivery_list.begin();
    while ( it != pkt_delivery_list.end() ) {
        std::tuple<uint64_t,double> current=*it;
        double current_bytes=std::get<1>(current);
        uint64_t current_time=std::get<0>(current);
        /* fetch the next packet delivery */
        it++;
        if(it == pkt_delivery_list.end()) {
          /* end of packet delivery trace */
          break;
        }
        uint64_t next_time=std::get<0>(*it);
        uint64_t delta_in_ms=(uint64_t)((next_time-current_time)/1e6);
        assert(delta_in_ms>=1); 
        
        if(delta_in_ms==1) { 
          continue;
        }
        else if(delta_in_ms > 100) {
          continue; /* too long must be an outage, no point interpolating */
        }
        else  { 
          /* Go back to the credit you want to spread out */
          it--;
          /* Erase it and move to the next credit */
          it=pkt_delivery_list.erase(it);
          /* Now, spread the bytes evenly */ 
          double bytes_per_ms = current_bytes/delta_in_ms;
          uint16_t i=0;
          for ( i=0 ; i<delta_in_ms ; i++ ) {
             std::tuple<uint64_t,double> interp_credit(current_time+i*1e6,bytes_per_ms);
             pkt_delivery_list.insert(it,interp_credit);
          }
        }
    }
}

uint64_t TraceLink::calc_next_time(uint32_t head_size) {
     long double required_tokens=head_size-token_count;
      /* Run through the pkt_delivery_list to check how long before you accumulate sufficient tokens */ 
     long double acc=0; 
     for (std::list<std::tuple<uint64_t,double>>::iterator it = delivery_schedule.pkt_delivery_list.begin(); it != delivery_schedule.pkt_delivery_list.end(); it++) {
        std::tuple<uint64_t,uint32_t> current=*it;
        acc=acc+std::get<1>(current);
        if(acc>=required_tokens) {
          return std::get<0>(current)+begin_time;
          /* because the trace timestamps are relative
             to the beginning of the link going on */
        }
     }
     std::cout<<"No more credit to transmit this packet \n";
     return -1; /* If there is no credit left */ 
}
