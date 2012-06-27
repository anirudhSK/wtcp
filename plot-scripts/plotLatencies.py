#! usr/bin/python
import sys
import math
from math import *
FILE=open(sys.argv[1],"r")
packetLatency=dict()
packetRx=dict()
packetTx=dict()
minLatency=100000000   # huge number
minTx=1000000000000000 # huge number
minRx=1000000000000000
txRxFile=open("txRx.plot","w")
latencyFile=open("latency.plot","w")
histFile=open("latency.hist","w")
cdfFile=open("latency.cdf","w")
for line in FILE.readlines() :
      if (len(line.split()) < 9) : # there are some weird truncated final lines  
          continue
      packetString=(line.split()[2])
      packetNumber=int(packetString.split(':')[1])
      Tx=float(line.split()[6])    # Tx timestamp
      Rx=float(line.split()[9])    # Rx timestamp 
      latency=Rx-Tx                # latency with whatever offset 
      packetRx[packetNumber]=Rx
      packetTx[packetNumber]=Tx
      packetLatency[packetNumber]=latency
      if (latency<minLatency) :
           minLatency=latency
      if (Tx < minTx) :
           minTx =Tx
      if (Rx <minRx) :
           minRx=Rx
for packet in range(min(packetTx.keys()),max(packetRx.keys())) :
      if (packet not in packetTx) : # dropped packet 
          txRxFile.write(str(packet)+"\t -1 \t -1\n");
          latencyFile.write(str(packet)+"\t -1 \n");
      else :
          txRxFile.write(str(packet)+"\t"+str(packetTx[packet]-minTx)+"\t"+str(packetRx[packet]-minRx)+"\n");
          latencyFile.write(str(packet)+"\t"+str((packetLatency[packet]-minLatency)*1000)+"\n");

# histogram generation
counts=dict()
variableLatency=dict()
for packet in packetLatency :
     variableLatency[packet]=(packetLatency[packet]-minLatency)*1000

histogramMin=min(variableLatency.values())
histogramMax=max(variableLatency.values())
binSize=1 # 1 ms bin size
for bins in range (int(floor(histogramMin)),int(ceil(histogramMax))) :
     counts[bins]=0
countsTotal=0
for packet in variableLatency :
     value=variableLatency[packet]
     binValue=int(floor(value))
     counts[binValue]=counts[binValue]+1
     countsTotal+=1

countSoFar=0
for bins in counts :
     histFile.write(str(bins)+"\t"+str(counts[bins])+"\n");
     countSoFar=countSoFar+counts[bins]
     cdfFile.write(str(bins)+"\t"+"%.4f"%(float(countSoFar)/float(countsTotal))+"\n");
