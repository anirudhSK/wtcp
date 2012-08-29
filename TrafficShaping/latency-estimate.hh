#ifndef RATE_ESTIMATE_HH
#define RATE_ESTIMATE_HH
#include <list>
#include "payload.hh"

class LatencyEstimate
{
private:
  const double empty_latency_estimate; 
  /* initial estimate */
  const unsigned int averaging_extent_ms;
  std::list<Payload> history;

  void housekeeping( uint64_t now ); 
  /* cull samples older than averaging_extent_ms */

public:
  LatencyEstimate( const double s_empty_latency_estimate, const unsigned int s_averaging_extent_ms );
  void add_latency_sample( const Payload p );
  double get_latency( uint64_t now );
  /* latency in packets per second */
};

#endif
