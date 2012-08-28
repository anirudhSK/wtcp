import sys
# get sender schedule from generation program
def get_sender_schedule(freq_file_fh) : 
   schedule=[]
   for line in freq_file_fh.readlines() : 
       if(len(line.split()) == 4) :
         schedule.append((0,float(line.split()[3])));
       else :
         freq=float(line.split()[5])
         time=float(line.split()[8])
         schedule.append((freq,time));  
   return schedule

# main program 

fh=open(sys.argv[1]);
schedule_file=open(sys.argv[2]);
schedule=get_sender_schedule(schedule_file);
date=int(fh.readline()); # already in UNIX timestamp form
latency=[]
for line in fh.readlines():
    latency.append(float(line.split()[1]));

assert(len(latency)==len(schedule));

# now get UNIX timestamped data 
for i in range(0,len(schedule)) :
   if(schedule[i][0]!=0) : # ie it's a freq and not silence
      print (date+schedule[i][1]),"\t",latency[i]
      # so we have a global ref. 
