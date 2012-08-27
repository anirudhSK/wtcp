import sys
import numpy
chan1=open(sys.argv[1],'r');
chan2=open(sys.argv[2],'r');
l1=numpy.array(eval(chan1.readline()));
l2=numpy.array(eval(chan2.readline()));
latency=l1-l2;
for i in range(0,len(latency)) : 
     print i,"\t",latency[i]
