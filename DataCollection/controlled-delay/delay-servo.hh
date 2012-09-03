#ifndef DELAY_SERVO_HH
#define DELAY_SERVO_HH

#include <string>

#include "socket.hh"
#include "rate-estimate.hh"
#include "history.hh"
#include "feedback.hh"
class DelayServoSender {
private:

  const std::string _name;

  const Socket & _sender;
  const Socket::Address & _target;

  double _current_rate;
  unsigned int _packets_sent, _packets_received;

  static const unsigned int PACKET_SIZE = 1400; /* bytes */
  static constexpr double QUEUE_DURATION_TARGET = 1.0; /* seconds */
  static constexpr double STEERING_TIME = 0.05; /* seconds */
  static constexpr double MINIMUM_RATE = 5.0; /* packets per second */

  int _unique_id;

  uint64_t _next_transmission, _last_transmission;

  double _num_outstanding;
 
public:

  DelayServoSender( const std::string & s_name, const Socket & s_sender, const Socket::Address & s_target,uint32_t sender_id );

  void tick( void );
  void recv(Feedback *feedback_pkt);

  uint64_t wait_time_ns( void ) const;
  int fd( void ) const { return _sender.get_sock(); }
};

class DelayServoReceiver {
private:

  const std::string _name;

  const Socket & _receiver;
  const Socket::Address & _source;

  RateEstimate _rate_estimator;

  unsigned int _packets_sent, _packets_received;

  static const unsigned int PACKET_SIZE = 1400; /* bytes */

  int _unique_id;

  uint64_t _next_transmission, _last_transmission;

  History _hist;

public:

  DelayServoReceiver( const std::string & s_name, const Socket & s_receiver,const Socket::Address & s_source , uint32_t remoted_id );

  void tick();
  void recv( Payload* payload, uint64_t rx_timestamp);

  uint64_t wait_time_ns( void ) const;
  int fd( void ) const { return _receiver.get_sock(); }
};
#endif
