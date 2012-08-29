#include <assert.h>
#include "latency-estimate.hh"
#include <iostream>
LatencyEstimate::LatencyEstimate( const double s_empty_latency_estimate, const unsigned int s_averaging_extent_ms )
  : empty_latency_estimate( s_empty_latency_estimate),
    averaging_extent_ms( s_averaging_extent_ms ),
    history()
{
}

void LatencyEstimate::add_latency_sample( const Payload p )
{
  history.push_back( p );
  housekeeping(p.sent_ts);
}

double LatencyEstimate::get_latency(uint64_t now) {
  housekeeping(now);
  const int num_packets = history.size();
  if ( num_packets <= 2 ) {
    return empty_latency_estimate;
  }
  else {
   /* Iterate and find average latency */
   double average=0;
   for (std::list<Payload>::iterator it = history.begin(); it != history.end(); it++) {
     average=average+((*it).sent_ts-(*it).rx_ts)/1.e9; /* ts are in ns, conv to ms */
  }
  return average/history.size();  
  }
}

void LatencyEstimate::housekeeping( uint64_t now )
{
  while ( !history.empty() ) {
    Payload front = history.front();
    assert( now >= front.sent_ts );
    if ( now - front.sent_ts > 1000000 * averaging_extent_ms ) {
      history.pop_front();
    } else {
      return;
    }
  }
}
