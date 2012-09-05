#include <assert.h>
#include <iostream>
#include "history.hh"

History::History()
  : _received(),
    _num_outstanding_rx( 0 ),
    _num_lost( 0 ),
    _next_exp_seq_no( 0 ),
    _max_rx_seq_no( 0 ) {

}

void History::packet_received( const Payload & p , double current_rate  ) {
 
  _max_rx_seq_no=std::max(_max_rx_seq_no,p.sequence_number);
  /* remove p.sequence_number from buffer just in case it's a duplicate   */
  _received.remove_if ( [&] ( const uint32_t & x ) { return x == p.sequence_number ; } );
  if ( p.sequence_number == _next_exp_seq_no ) {
     _next_exp_seq_no++;
  }

  else if (p.sequence_number > _next_exp_seq_no) {
    _received.push_back(p.sequence_number);
  }
  /* INVARIANT : At this point _received only has unique packets >= _next_exp_seq_no+1 and <= _max_rx_seq_no */
 
  /* time out lost packets by purging everything on or before purge_seq */ 
  const uint32_t purge_seq = std::max((int)0,(int)(p.sequence_number - (current_rate*reorder_window*1e-9))) ;

  uint32_t old_size=_received.size(); 
  _received.remove_if ( [&] ( const uint32_t & x ) { return x <= purge_seq; } );
  uint32_t new_size=_received.size();
  
  /* This is the number of packets rx between x >= _next_exp_seq_no + 1  and x <= purge_seq */
  uint32_t pkts_rx_before_purge_seq=old_size-new_size;

  /* This is the number that should have been received */
  uint32_t pkts_to_be_received=std::max(0,(int)(purge_seq-_next_exp_seq_no)); 
 
  /* Hence this is the number lost */ 
  _num_lost=_num_lost+std::max(0,(int)(pkts_to_be_received-pkts_rx_before_purge_seq));
  
  /* Update next expected sequence number to reflect purging */
  _next_exp_seq_no=std::max(_next_exp_seq_no,purge_seq);

  /* INVARIANT : At this point _received only has unique packets >= purge_seq+1 and <= _max_rx_seq_no */
  /* Hence, invariant is preserved */


  _num_outstanding_rx = std::max((int)0,(int)(_max_rx_seq_no-_next_exp_seq_no-_received.size()));
  /* Number outstanding :
      Max rx seq num so far : lower bnd on number Tx
   -   next_exp_seq_no : all seq nums rx in-order or assumed lost 
   -   _received.size() : num received out of order so far 
   =  num_outstanding
  */
}
