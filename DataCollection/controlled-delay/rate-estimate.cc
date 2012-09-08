#include <assert.h>
#include<iostream>
#include "rate-estimate.hh"

RateEstimate::RateEstimate( const double s_empty_rate_estimate, const unsigned int s_averaging_extent_ms )
  : empty_rate_estimate( s_empty_rate_estimate),
    averaging_extent_ms( s_averaging_extent_ms ),
    history()
{
}

void RateEstimate::add_packet( const Payload & p )
{
  history.push_back( p );
  housekeeping();
}

double RateEstimate::get_rate( void )
{
  housekeeping();
  const int num_packets = history.size();
  if ( num_packets <= 2 ) {
    return empty_rate_estimate;
  }

  return 1000.0 * (double) num_packets / (double) averaging_extent_ms;
}

void RateEstimate::housekeeping( void )
{
  const uint64_t now = Socket::timestamp();
  while ( !history.empty() ) {
    Payload & front = history.front();
    assert( now >= front.recv_timestamp );
    if ( now - front.recv_timestamp > 1000000 * averaging_extent_ms ) {
      history.pop_front();
    } else {
      return;
    }
  }
}

double RateEstimate::get_latency(void) 
{
  /* Estimate the latency */
  housekeeping();
  const int num_packets = history.size();
  if ( num_packets <= 2 ) {
    return empty_latency_estimate;
  }
  else {
   /* Iterate and find average latency */
   double average=0;
   int discard=0;
   for (std::list<Payload>::iterator it = history.begin(); it != history.end(); it++) {
     double sample= ((int64_t)(*it).recv_timestamp-(int64_t)(*it).sent_timestamp)/1.0e9; /* ts are in ns, conv to ms */
     if(sample<0) {
        std::cout<<"Discarding sample because latency can't be negative \n";
        discard++;
     }
     else {
        average=average+sample;
     }
   }
   if (history.size()-discard >= 2) {
    return average/(history.size()-discard);  
   }
   else  {
    return empty_latency_estimate;
   }
  }
}
