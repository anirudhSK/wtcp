import sys
rate1=int(sys.argv[1])
time1=int(sys.argv[2]);
rate2=int(sys.argv[3])
time2=int(sys.argv[4]);

for i in range(0,time1*1000) :  # 1 hour in milliseconds 
  print i,"\t",(rate1/(8*1000))

for i in range(time1*1000,(time1+time2)*1000) :  # 1 hour in milliseconds 
  print i,"\t",(rate2/(8*1000))

