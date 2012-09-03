#ifndef PAYLOAD_HH
#define PAYLOAD_HH

#include <string>

class Payload
{
public:
  int sender_id;
  uint32_t sequence_number;
  uint64_t sent_timestamp, recv_timestamp;

  const std::string str( const size_t len ) const;
  bool operator==( const Payload & other ) const;
};

#endif
