#ifndef FEEDBACK_HH
#define FEEDBACK_HH

#include <string>

class Feedback
{
public:
  int sender_id;
  uint32_t num_outstanding_rx;
  uint32_t max_rx_seq_no;
  double current_rate;
  uint32_t num_lost;

  const std::string str( const size_t len ) const;
  bool operator==( const Feedback & other ) const;
};

#endif
