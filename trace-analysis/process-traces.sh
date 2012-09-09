#! /bin/bash
# determine the number of sessions to extract the trace for .
ls --format=single-column client-*-* | cut -f 2,3 -d "-" > sessions.list
server_logs=`ls --format=single-column server-*`
echo "Server logs are "$server_logs
server_time=`echo $server_logs | cut -f 2 -d "-"`
echo "Server time is "$server_time
mkdir plots 
for session in `cat sessions.list` ;
do 
   session_ts=`echo $session | cut -f 1 -d "-"`
   session_id=`echo $session | cut -f 2 -d "-"`
   echo "Session ts is "$session_ts;
   echo "Session id is "$session_id;

   grep "senderid=$session_id" $server_logs > server-$server_time-$session_id

   # prepare the files for simulation
   nice -n 19 python prep-for-simulation.py client-$session_ts-$session_id $session_id ;
   nice -n 19 python prep-for-simulation.py server-$server_time-$session_id $session_id ;

   # prepare the files for plotting
   nice -n 19 python prep-for-plotting.py client-$session_ts-$session_id $session_id ;
   nice -n 19 python prep-for-plotting.py server-$server_time-$session_id $session_id ;

   # remove the tmp server file
   rm server-$server_time-$session_id ;
  
   # copy files for gnuplot  
   cp downlink-$session_id.rtt downlink.rtt
   cp downlink-$session_id.one_way downlink.one_way
   cp uplink-$session_id.rtt uplink.rtt
   cp uplink-$session_id.one_way uplink.one_way

   # copy plotting templates and modify title alone
   cat top.p > tmp.p
   echo "set multiplot layout 2,1 title \"session $session_id\"" >> tmp.p
   cat bottom.p >> tmp.p
  
   nice -n 19 gnuplot -p tmp.p
   mv plot.png plots/$session_id.png 
done
