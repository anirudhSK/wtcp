#! usr/bin/python
import sys
FILE=open(sys.argv[1],"r")
packetLatency=dict()
minLatency=1000000
for line in FILE.readlines() :
      packet=int(line.split()[0])
      latency=float(line.split()[1])
      packetLatency[packet]=latency
      if (latency<minLatency) :
           minLatency=latency

for packet in packetLatency :
      print (packetLatency[packet]-minLatency)*1000
