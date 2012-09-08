#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include "delay-servo.hh"

DelayServoSender::DelayServoSender( const std::string & s_name, const Socket & s_data_socket, const Socket & s_feedback_socket, const Socket::Address & s_target, uint32_t local_id )
  : _name( s_name ), 
    _data_socket( s_data_socket ),
    _feedback_socket( s_feedback_socket ),
    _target( s_target ),
    _current_rate( BACKOFF_RATE ),
    _packets_sent( 0 ),
    _unique_id( local_id ),
    _next_transmission( Socket::timestamp() ),
    _last_transmission( _next_transmission )
{
  std::cout<<"Servo PARAMETERS \n";
  std::cout<<"PACKET_SIZE = "<<PACKET_SIZE<<"\n";
}

DelayServoReceiver::DelayServoReceiver( const std::string & s_name, const Socket & s_data_socket, const Socket & s_feedback_socket, const Socket::Address & s_source, uint32_t remote_id )
  : _name( s_name ), 
    _data_socket( s_data_socket ),
    _feedback_socket( s_feedback_socket ),
    _source ( s_source) ,
    _rate_estimator( MINIMUM_RX_RATE, 1000 ),
    _packets_received( 0 ),
    _unique_id( remote_id ),
    _next_transmission( Socket::timestamp() ),
    _last_transmission( _next_transmission ),
    _last_stat(Socket::timestamp()),
    _hist()
{
}
uint64_t DelayServoSender::wait_time_ns( void ) const
{
  uint64_t cur_ts=Socket::timestamp();
  if(_next_transmission > (cur_ts + 1e6) ) { 
    /* if it's at least 1 ms in the future , else don't sleep */
    return _next_transmission - cur_ts;
  }
  else return 0;
}
uint64_t DelayServoReceiver::wait_time_ns( void ) const
{
  uint64_t cur_ts=Socket::timestamp();
  if(_next_transmission > (cur_ts + 1e6) ) {
    /* if it's at least 1 ms in the future , else don't sleep */
    return _next_transmission - cur_ts;
  }
  else return 0;
}
void DelayServoReceiver::recv( Payload* contents )
{
   /* This has to be a data packet  */
   /* Update controlled delay estimate, 
      Make sure to echo sender ID on ACK */
  _packets_received++;
  double current_rate=0; 
  _rate_estimator.add_packet( *contents );
  current_rate=_rate_estimator.get_rate();
  _hist.packet_received( *contents , current_rate );
  double loss_rate = (double) _hist.num_lost() / (double) _hist.max_rx_seq_no();  
    printf( "%s seq = %d delay = %f recvrate = %f avg-latency at rx = %f Mbps = %f lost = %.5f%% arrivemilli = %ld pkts_received = %d \n",
          _name.c_str(),
          contents->sequence_number,
          (double) ((int64_t)contents->recv_timestamp - (int64_t)contents->sent_timestamp) / 1.0e9,
          _rate_estimator.get_rate(),
          _rate_estimator.get_latency(),
          _rate_estimator.get_rate() * PACKET_SIZE * 8.0 / 1.0e6,
          loss_rate * 100,
          contents->recv_timestamp / 1000000,
          _packets_received );
    _last_stat=Socket::timestamp();
}

void DelayServoReceiver::tick( void ) 
{
  /* Make a feedback packet with num_outstanding & current_rate 
     Send once in 20 ms */
  if ( Socket::timestamp() > _last_stat + 2e9 ) { 
      /* 2 seconds since last recv, actively purge */
      std::cout<<"Actively purging here \n";
      _hist.active_purge(_hist.max_rx_seq_no() + 10);
      _last_stat=Socket::timestamp();
  } 
  if ( Socket::timestamp() > _next_transmission ) /* 20 ms*/ {
       Feedback feedback;
       feedback.current_rate=_rate_estimator.get_rate();
       feedback.current_latency=_rate_estimator.get_latency();
       feedback.sender_id = _unique_id;
       _feedback_socket.send(Socket::Packet(_source, feedback.str(sizeof(Feedback))));
       _next_transmission=_next_transmission + 1.e6*20  ;
  }
}

void DelayServoSender::tick( void )
{
    uint64_t interpacket_delay=1e9/_current_rate;
    uint64_t now = Socket::timestamp();
    if ( _next_transmission <= now ) {
      /* Send packet */
      Payload outgoing;
      outgoing.sequence_number = _packets_sent++;
      outgoing.sent_timestamp = Socket::timestamp();
      outgoing.sender_id = _unique_id;
      _data_socket.send( Socket::Packet( _target, outgoing.str( PACKET_SIZE ) ) );
      _last_transmission = outgoing.sent_timestamp;
      _next_transmission = _last_transmission + interpacket_delay;
      std::cout<<_name<<" Transmitting in tick @ "<<now<<" with rate "<<1.0e9/interpacket_delay<<" packets per second \n";
    }
}

void DelayServoSender::recv(Feedback* feedback) {
   if(_packets_sent <= 20 )  {
     /* send 20 pkts at the beginning to get the min. latency with clk offset */
     _current_rate=BACKOFF_RATE;
     return ;   
   }
   else   {               
   /* Now act on feedback */
   if(feedback->current_latency >= 10.0) {
    /* Don't allow the queues to grow over 15 seconds */
    std::cout<<_name<<" Backing off \n";
    _current_rate=BACKOFF_RATE; 
    return ;
   }
   /* Check the above two against 4G as well */ 
   else {
    _current_rate=2*feedback->current_rate; /* To make sure the buffer is non empty */
    _current_rate=(_current_rate<MINIMUM_TX_RATE)?MINIMUM_TX_RATE:_current_rate;
    return ;
   }
  }
}
