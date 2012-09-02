#ifndef FEEDBACK_HH
#define FEEDBACK_HH

#include <string>

class Feedback
{
public:
  uint32_t num_outstanding;
  double current_rate;
  int sender_id;

  const std::string str( const size_t len ) const;
  bool operator==( const Feedback & other ) const;
};

#endif
