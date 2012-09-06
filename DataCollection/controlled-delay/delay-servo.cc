#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include "delay-servo.hh"

DelayServoSender::DelayServoSender( const std::string & s_name, const Socket & s_sender,const Socket::Address & s_target, uint32_t local_id )
  : _name( s_name ), 
    _sender( s_sender ),
    _target( s_target ),
    _current_rate( MINIMUM_RATE ),
    _packets_sent( 0 ),
    _unique_id( local_id ),
    _next_transmission( Socket::timestamp() ),
    _last_transmission( _next_transmission ),
    _num_outstanding(0),
    _num_lost(0),
    _num_acks(0),
    _cwnd(CWND_MIN)
{
  std::cout<<"Servo PARAMETERS \n";
  std::cout<<"PACKET_SIZE = "<<PACKET_SIZE<<"\n";
  std::cout<<"QUEUE_DURATION_TARGET = "<<QUEUE_DURATION_TARGET<<"\n";
  std::cout<<"STEERING_TIME = "<<STEERING_TIME<<"\n";
  std::cout<<"MINIMUM_RATE = "<<MINIMUM_RATE<<"\n";
}

DelayServoReceiver::DelayServoReceiver( const std::string & s_name, const Socket & s_receiver, const Socket::Address & s_source, uint32_t remote_id )
  : _name( s_name ), 
    _receiver( s_receiver ),
    _source( s_source ),
    _rate_estimator( MINIMUM_RATE, 1000 ),
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
  if(_next_transmission > cur_ts) {
    return _next_transmission - cur_ts;
  }
  else return 0;
}
uint64_t DelayServoReceiver::wait_time_ns( void ) const
{
  uint64_t cur_ts=Socket::timestamp();
  if(_next_transmission > cur_ts) {
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
    printf( "%s seq = %d delay = %f recvrate = %f outstanding at rx = %d Mbps = %f lost = %.5f%% arrivemilli = %ld pkts_received = %d \n",
          _name.c_str(),
          contents->sequence_number,
          (double) (contents->recv_timestamp - contents->sent_timestamp) / 1.0e9,
          _rate_estimator.get_rate(),
          _hist.num_outstanding_rx(),
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
      _hist.active_purge(_hist.max_rx_seq_no() + 10);
      _last_stat=Socket::timestamp();
  } 
  if ( Socket::timestamp() > _next_transmission ) /* 20 ms*/ {
       Feedback feedback;
       feedback.num_outstanding_rx =_hist.num_outstanding_rx(); 
       feedback.max_rx_seq_no=_hist.max_rx_seq_no();
       feedback.current_rate=_rate_estimator.get_rate();
       feedback.num_lost=_hist.num_lost();
       feedback.sender_id = _unique_id;
       _receiver.send(Socket::Packet(_source, feedback.str(sizeof(Feedback))));
       _next_transmission=_next_transmission + 1.e6*20  ;
  }
}

void DelayServoSender::tick( void )
{
   uint64_t now = Socket::timestamp();
   if( (_num_outstanding < _cwnd ) ) { /* slow start */
    uint64_t interpacket_delay=10.0e9/_cwnd; 
    /* Whatever be the cwnd ramp up to it in 10 seconds to be somewhat reasonable */ 
    if ( _next_transmission <= now ) {
      /* Send packet */
      Payload outgoing;
      outgoing.sequence_number = _packets_sent++;
      outgoing.sent_timestamp = Socket::timestamp();
      outgoing.sender_id = _unique_id;
      _sender.send( Socket::Packet( _target, outgoing.str( PACKET_SIZE ) ) );
      _last_transmission = outgoing.sent_timestamp;
      _next_transmission = _last_transmission + interpacket_delay;
      _num_outstanding++;
      std::cout<<_name<<" Transmitting in tick \n";
    }
   }
   else {
      _next_transmission=now+100*1e6; 
      /*wait for ACK(s) , but wake up in 100ms to avoid deadlock */ 
   }
}

void DelayServoSender::recv(Feedback* feedback) {
  /* This has to be a feedback packet  */
  if( feedback->sender_id == _unique_id) {
   assert(_packets_sent >= feedback->max_rx_seq_no);
   /* calculate the total number of new packets that the receiver
      either acked or purged in this ack */
   uint64_t orig_outstanding=_num_outstanding;
   _num_outstanding=feedback->num_outstanding_rx +(_packets_sent-feedback->max_rx_seq_no) ;
   int num_new_total=0;
   if (orig_outstanding > _num_outstanding) num_new_total=orig_outstanding-_num_outstanding;

   /* Now compute the number of packets that the receiver declared lost in this
      feedback packet */
   int old_lost_count=_num_lost;
   _num_lost=feedback->num_lost;
   int num_new_lost=feedback->num_lost - old_lost_count;

   assert(num_new_total >= num_new_lost) ; /* else we have  a bug somewhere */

   /* Take the difference and this is the number of packets newly acked */
   int num_new_acks=num_new_total-num_new_lost;
   _num_acks=_num_acks+num_new_acks;
   /* Add this much to the cwnd, clip it to bounds */
   _cwnd=_cwnd+num_new_acks-num_new_lost; 
   _cwnd=(_cwnd>CWND_MAX)?CWND_MAX:_cwnd;
   _cwnd=(_cwnd<CWND_MIN)?CWND_MIN:_cwnd;

   std::cout<<_name<<"@ "<<Socket::timestamp()<<" rx feedback num_outstanding "<<_num_outstanding<<" num_lost "<<_num_lost<<" new cwnd "<<_cwnd<<" num acks "<<_num_acks<<"\n";
   while ( (num_new_total>= 1) && (_num_outstanding < _cwnd) ) { 
      /* self clock to refill both acked packets and lost ones to maintain packet conservation */
      /* Send packet */
      Payload outgoing;
      outgoing.sequence_number = _packets_sent++;
      outgoing.sent_timestamp = Socket::timestamp();
      outgoing.sender_id = _unique_id;
      _sender.send( Socket::Packet( _target, outgoing.str( PACKET_SIZE ) ) );
      _num_outstanding++;
      num_new_total--;
    }
  }
}
