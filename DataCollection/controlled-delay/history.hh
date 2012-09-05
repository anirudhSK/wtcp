#ifndef HISTORY_HH
#define HISTORY_HH

#include "payload.hh"

#include <list>

class History
{
private:
  static const uint64_t reorder_window = 20 * 1000 * 1000; 
  /* 20 ms */

  std::list< uint32_t > _received;
  uint32_t _num_outstanding_rx;
  uint32_t _num_lost;
  uint32_t _next_exp_seq_no;   // next expected sequence number in order
  uint32_t _max_rx_seq_no;    // max received so far. 

public:
  History();
  void packet_sent( const Payload & p );
  void packet_received( const Payload & p , double current_rate );
  unsigned int num_outstanding_rx( void ) const { return _num_outstanding_rx; }
  unsigned int num_lost( void ) const { return _num_lost; }
  unsigned int max_rx_seq_no(void) const { return _max_rx_seq_no; }

};

#endif
