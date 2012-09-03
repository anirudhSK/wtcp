#ifndef FEEDBACK_HH
#define FEEDBACK_HH

#include <string>

class Feedback
{
public:
  int sender_id;
  uint32_t num_outstanding;
  double current_rate;

  const std::string str( const size_t len ) const;
  bool operator==( const Feedback & other ) const;
};

#endif
