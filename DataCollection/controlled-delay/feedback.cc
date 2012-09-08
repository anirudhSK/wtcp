#include <assert.h>
#include<iostream>
#include "feedback.hh"

const std::string Feedback::str( const size_t len ) const
{
  assert( len >= sizeof( Feedback ) );
  std::string padding( len - sizeof( Feedback ), 0 );
  return std::string( (char*)this, sizeof( Feedback ) ) + padding;
}

bool Feedback::operator==( const Feedback & other ) const
{
  return ( current_rate == other.current_rate
	  && sender_id == other.sender_id
          && current_latency == other.current_latency
          && sent_timestamp == other.sent_timestamp
          && recv_timestamp == other.recv_timestamp
         );
}
