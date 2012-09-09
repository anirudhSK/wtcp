#! /usr/bin/python
import sys
if(len(sys.argv) < 3 ) : 
  print "Usage : prep-for-plotting.py <file-name> <session-number> "
  exit
file_name=sys.argv[1];
fh=open(file_name,"r");
session_number=int(sys.argv[2]);

#parse file_name to determine client vs server log_pos-timestamp-sessionID
log_details=file_name.split("-")
log_pos=log_details[0];
log_time=int(log_details[1]);

if(log_pos=="client") :
  log_session=int(log_details[2]);
  if(session_number != log_session) :
        print "Client log file session number doesn't match input session number ";
        exit
  else :
    rtt_handle=open("uplink-"+str(session_number)+".rtt","w");
    one_way_handle=open("downlink-"+str(session_number)+".one_way","w");
elif(log_pos=="server") :
  rtt_handle=open("downlink-"+str(session_number)+".rtt","w");
  one_way_handle=open("uplink-"+str(session_number)+".one_way","w");

packet_schedule=dict();

for line in fh.readlines() :
   fields=line.split();
   sender_id=int(fields[3].split("=")[1].strip(","))
   recv_time_ms=int(int(fields[6].split("=")[1].strip(","))/1.0e6)  
   if (recv_time_ms in packet_schedule) :
     packet_schedule[recv_time_ms]+=1400;
   else :
     packet_schedule[recv_time_ms]=1400;
   if(sender_id==session_number) :
      if(fields[0]=="OUTGOING") :
         rtt=float(field[7].split("=")[1].strip(","))        
         rtt_handle.write(str(recv_time_ms)+"\t"+str(rtt)+"\n");
      if (fields[0]=="INCOMING") :
         one_way=float(fields[7].split("=")[1].strip(","))        
         one_sec_rate_est=0
         for i in range(recv_time_ms-1000,recv_time_ms) : 
             if(i in packet_schedule) :
                one_sec_rate_est+=packet_schedule[i];
         one_way_handle.write(str(recv_time_ms)+"\t"+str(one_way)+"\t"+str((8*one_sec_rate_est)/1.0e6)+"\n");
 
