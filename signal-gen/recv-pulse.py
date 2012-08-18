import sys
from math import *
import numpy
import matplotlib.pyplot as p
from matplotlib.pyplot import *
timeSeries=[]

def read_from_file(fh) : 
     ''' File handling routines 
     '''
     stream1=[]
#     stream2=[]
     global timeSeries
     for line in fh.readlines() : 
         values=line.split()
         if( values[0] == ";") : # comment line ditch this
           continue; 
         else : 
           timeSeries.append(float(values[0]))
           stream1.append(float(values[1])) 
 #          stream2.append(float(values[2]))
     return (numpy.array(stream1))     

def band_pass_filter(b_low,b_high,filter_length) : 
    ''' 
        Get Band pass filter kernel 
        for lower limit b_low
        and upper limit b_high
        b_low and b_high are in units of radians per sample 
        Truncate to filter_length
    '''
    if(b_low==b_high) : 
#      print "Can't filter with no band, creating artificial one Hz band " 
      b_high=b_low+10; # need to make sure there is some room around it. 
    assert(b_low<b_high) 
    nvalues=range(1,filter_length) # generate the argument array from 1 until sample length -1  
    val_array=numpy.array(nvalues) 
    bph=(2*b_high*numpy.sin(2*pi*b_high*val_array))/(2*pi*b_high*val_array)
    if(b_low!=0) :
     bpl=(2*b_low *numpy.sin(2*pi*b_low *val_array))/(2*pi*b_low*val_array)
     return bph-bpl 
    else : 
     return bph

def quad_demod(stream,centre_freq,bandwidth,sample_rate) :
      stream_length=len(stream); 
      nvalues=range(0,stream_length);

      # lpf common to I and Q
      lpf=band_pass_filter(0,bandwidth/2,2000); 

      # modulate with cosine and lpf it
      cosine=numpy.cos( ((2* numpy.pi * centre_freq)/sample_rate) * numpy.array(nvalues));
      i_stream=cosine*stream ;
      i_stream_lpf=numpy.convolve(i_stream,lpf);  

      # modulate with sine and lpf it
      sine=numpy.sin( ((2* numpy.pi * centre_freq)/sample_rate) * numpy.array(nvalues));
      q_stream=sine*stream ;
      q_stream_lpf=numpy.convolve(q_stream,lpf); 

      # get the absolute value  
      abs_stream=numpy.sqrt(i_stream_lpf*i_stream_lpf + q_stream_lpf*q_stream_lpf);
      p.figure(1)
      p.plot(abs_stream); 
      p.show()

# Now the main routine 
if(len(sys.argv) < 5) : 
    print "Usage : python recv-pulse.py filename centre_freq bandwidth sample_rate "
    exit(5) 
fh=open(sys.argv[1],'r');
stream=read_from_file(fh) ;
freq=float(sys.argv[2]);
bandwidth=float(sys.argv[3]);
sample_rate=float(sys.argv[4]); 
tuned_stream=quad_demod(stream,freq+bandwidth/2,bandwidth,sample_rate);
for i in range(0,len(timeSeries)): 
     print timeSeries[i],"\t",tuned_stream[i]
 
