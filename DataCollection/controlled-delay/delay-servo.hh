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
  unsigned int _packets_sent;

  static const unsigned int PACKET_SIZE = 1400; /* bytes */
  static constexpr double QUEUE_DURATION_TARGET = 2.000; /* seconds */
  static constexpr double STEERING_TIME = 0.05; /* seconds */
  static constexpr double MINIMUM_RATE = 20.0; /* packets per second */


  int _unique_id;

  uint64_t _next_transmission, _last_transmission;

  uint32_t _num_outstanding,_num_lost,_num_acks;

  unsigned int CWND_MIN ;  /* packets */
  unsigned int CWND_MAX ; /* packets */
  uint16_t _cwnd;                               /* cong window in packets */ 
  uint64_t _ramp_up_ns;                          /* time to ramp up to CWND_MAX */
public:

  DelayServoSender( const std::string & s_name, const Socket & s_sender, const Socket::Address & s_target,uint32_t sender_id , uint64_t s_ramp_ns,  unsigned int cwnd_min, unsigned int cwnd_max );

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

  unsigned int _packets_received;

  static const unsigned int PACKET_SIZE = 1400; /* bytes */
  static constexpr double MINIMUM_RATE = 20.0; /* packets per second */

  int _unique_id;

  uint64_t _next_transmission, _last_transmission,_last_stat;

  History _hist;

public:

  DelayServoReceiver( const std::string & s_name, const Socket & s_receiver,const Socket::Address & s_source , uint32_t remoted_id );

  void tick();
  void recv( Payload* payload);

  uint64_t wait_time_ns( void ) const;
  int fd( void ) const { return _receiver.get_sock(); }
};
#endif
