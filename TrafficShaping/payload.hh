#ifndef PAYLOAD_HH
#define PAYLOAD_HH

#include <string>

class Payload
{
public:
  uint8_t * pkt_data; 
  uint16_t size; 

  const std::string str( const size_t len ) const;
  bool operator==( const Payload & other ) const;
  Payload(uint8_t * ether_frame,uint16_t length); 
};
#endif
