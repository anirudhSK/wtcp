#include <assert.h>
#include<string.h>
#include "payload.hh"

const std::string Payload::str( const size_t len ) const
{
  assert( len >= sizeof( Payload ) );
  std::string padding( len - sizeof( Payload ), 0 );
  return std::string( (char*)this, sizeof( Payload ) ) + padding;
}

bool Payload::operator==( const Payload & other ) const
{
  return ( strcmp((char*)other.pkt_data,(char*) pkt_data)==0
	  && size == other.size);
}

Payload::Payload(uint8_t* ether_frame,uint16_t length,uint64_t t_rx_ts) 
   : pkt_data(ether_frame),
     size(length),
     rx_ts(t_rx_ts) ,
     sent_ts(0) {

}
