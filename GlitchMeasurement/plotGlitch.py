#! usr/bin/python
import sys
from heapq import *
from bisect import *
import operator
def find_le(a, x):
    '''Find rightmost index less than or equal to x'''
    i = bisect_right(a, x)
    if i:
        return i-1
    raise ValueError
def find_gt(a, x):
    'Find leftmost index greater than x'
    i = bisect_right(a, x)
    if i != len(a):
        return i
    raise ValueError
' File I/O ' 
FILE=open(sys.argv[1],"r")
D=float(sys.argv[2])
packetRx=dict() # dict from packet ID to reception time 
packetTx=[] # sorted list of (timeStamp,packetNumber) pairs, sorted by timeStamp
minLatency=100000000   # huge number
rxTimeStamps=[]
def searchSentPackets(startTime,endTime) : 
    'Find all packets sent between startTime and endTime and received before endTime . Remember to add minLatency '
    sentTimes=[]
    try : 
      leftEdge=find_gt(packetTx,(startTime,-1)) # find all packets sent after startTime 
      rightEdge=find_le(packetTx,(endTime,-1))  # and before end time 
    except ValueError :
      return(False,-1)
    for index in range(leftEdge,rightEdge) :    # check if it's received before endTime
        packetID=packetTx[index][1]             # get the packet number
        if(packetRx[packetID]<=endTime) :       # check if it's received before endTime
           sentTimes.append(packetTx[index][0]) # append sendTime for all such packets
    if(sentTimes==[]) : 
        return (False,-1)
    else : 
        return (True,min(sentTimes))            # get max of the sent times because you want to know till when you have no glitch
for line in FILE.readlines() :
      if (len(line.split()) < 9) : # there are some weird truncated final lines  
          continue
      packetString=(line.split()[2])
      packetNumber=int(packetString.split(':')[1])
      Tx=float(float(line.split()[6])*1000)     # Tx timestamp in milliseconds
      Rx=float(line.split()[9])*1000            # Rx timestamp in milliseconds
      latency=Rx-Tx                             # latency with whatever offset 
      packetRx[packetNumber]=Rx
      packetTx.append((Tx,packetNumber));
      heappush(rxTimeStamps,Rx);
      if (latency<minLatency) :
           minLatency=latency
for index in range(0,len(packetTx)): # (normalize to minLatency) 
      packetTx[index]=(packetTx[index][0]+minLatency,packetTx[index][1])
packetTx.sort()
glitchTime=0
startTime=rxTimeStamps[0]
finishTime=max(rxTimeStamps)
# Receiver algm : At every instant of time, look at whether you have _any_ packet from the sender from at most D ms in the past until now 
while(rxTimeStamps != [] ): 
       currentTime=heappop(rxTimeStamps)
       # look for all packets sent between currentTime-D and currentTime, that were received before or on currentTime. 
       (search,sendTime)=searchSentPackets(currentTime-D,currentTime) ;      
       # Stuff before currentTime-D is not useful because it is too late to be played out.  
       if(search==True) :
#          print "No glitch at time %13.10f until %13.10f " % (currentTime,sendTime+D)
          # if the packet reception results in a no-glitch, you are good till the packet expires at which point you glitch or the next time reception whichever is earlier. 
          if ((sendTime+D)!=currentTime)  : # otherwise it gets stuck in an infinite loop. 
             heappush(rxTimeStamps,sendTime+D) # you are good until sendTime+D, so check at that point in time. 
       else : 
 #         print "Glitch at time %13.10f until %13.10f "% (currentTime,rxTimeStamps[0])
          if(rxTimeStamps!=[]) :
            glitchTime+=rxTimeStamps[0]-currentTime
          # if the packet reception results in glitch, you are in a glitch until the next packet comes along anyway, so never mind. 
print "%.6f\t%.2f" %(D,(glitchTime/(finishTime-startTime))*100)
