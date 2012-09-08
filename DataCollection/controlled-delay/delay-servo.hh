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

  const Socket & _data_socket;
  const Socket & _feedback_socket;

  const Socket::Address & _target;

  double _current_rate;
  unsigned int _packets_sent;

  static const unsigned int PACKET_SIZE = 1400; /* bytes */
  static constexpr double QUEUE_DURATION_TARGET = 2.000; /* seconds */
  static constexpr double STEERING_TIME = 0.05; /* seconds */
  static constexpr double MINIMUM_TX_RATE = 50.0; /* packets per second- Median 3G tput */
  static constexpr double BACKOFF_RATE = 5.0;  /* packets per second */


  int _unique_id;

  uint64_t _next_transmission, _last_transmission;

public:

  DelayServoSender( const std::string & s_name, const Socket & s_data_socket, const Socket & s_feedback_socket, const Socket::Address & s_target, uint32_t local_id) ;

  void tick( void );
  void recv(Feedback *feedback_pkt);

  uint64_t wait_time_ns( void ) const;
  int fd( void ) const { return _feedback_socket.get_sock(); }
};

class DelayServoReceiver {
private:

  const std::string _name;

  const Socket & _data_socket;
  const Socket & _feedback_socket;

  const Socket::Address & _source;

  RateEstimate _rate_estimator;

  unsigned int _packets_received;

  static const unsigned int PACKET_SIZE = 1400; /* bytes */
  static constexpr double MINIMUM_RX_RATE = 5.0; /* packets per second */

  int _unique_id;

  uint64_t _next_transmission, _last_transmission,_last_stat;

  History _hist;

public:

  DelayServoReceiver( const std::string & s_name, const Socket & s_data_socket, const Socket & s_feedback_socket, const Socket::Address & s_source, uint32_t remote_id ) ;

  void tick();
  void recv( Payload* payload);

  uint64_t wait_time_ns( void ) const;
  int fd( void ) const { return _data_socket.get_sock(); }
};
#endif
