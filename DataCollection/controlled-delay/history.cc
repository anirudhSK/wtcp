#include <assert.h>
#include <iostream>
#include "history.hh"

History::History()
  : _received(),
    _num_outstanding( 0 ),
    _num_lost( 0 ),
    _next_exp_seq_no( 0 ),
    _max_rx_seq_no( 0 ) {

}

void History::packet_received( const Payload & p , double current_rate  ) {
  
  _max_rx_seq_no=std::max(_max_rx_seq_no,p.sequence_number);

  if ( p.sequence_number == _next_exp_seq_no ) {
     _next_exp_seq_no=p.sequence_number+1;
  }

  else {
    _received.push_back(p.sequence_number); /* out of order */
  }
 
  /* time out lost packets by moving _next_exp_seq_no */ 
  const uint32_t purge_count = std::max(0,(int)(p.sequence_number - (current_rate*reorder_window*1e-9))) ;
  std::cout<<"max_rx_seq_num is"<<_max_rx_seq_no<<"\n";
  std::cout<<"purge count is"<<purge_count<<"\n";
  std::cout<<"next exp seq num is"<<_next_exp_seq_no<<"\n";
  std::cout<<"num lost is"<<_num_lost<<"\n";

  _received.remove_if ( [&] ( const uint32_t & x ) { return x < purge_count; } );

  /* Check if you need to add to num_lost at all */ 
  _num_lost=_num_lost+std::max((int)0,(int)(purge_count-_next_exp_seq_no));

  /* Update next expected sequence number if required */
  _next_exp_seq_no=std::max(_next_exp_seq_no,purge_count);

  _num_outstanding = std::max((int)0,(int)(_max_rx_seq_no-_next_exp_seq_no-_received.size()));
  /* Number outstanding :
      Max rx seq num so far : lower bnd on number Tx
   -   next_exp_seq_no : all seq nums rx in-order or assumed lost 
   -   _received.size() : num received out of order so far 
   =  num_outstanding
  */
}
