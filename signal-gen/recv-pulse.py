import sys
from math import *
import numpy
import matplotlib.pyplot as p
from matplotlib.pyplot import *
import scipy.cluster as cluster
timeSeries=[]

def read_from_file(fh,channel) : 
     ''' File handling routines 
     '''
     stream=[]
     global timeSeries
     for line in fh.readlines() : 
         values=line.split()
         if( values[0] == ";") : # comment line ditch this
           continue; 
         else : 
           timeSeries.append(float(values[0]))
         if(channel==1) :
           stream.append(float(values[1])) 
         else :
           stream.append(float(values[2]))
     return (numpy.array(stream))     

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
      ''' 
          quadrature demodulation of received signal 
      '''
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

      # find slicing threshold for peak detector. 
      centroids=cluster.vq.kmeans(numpy.array(abs_stream),10)[0];
      slice_threshold=(min(centroids)+max(centroids))/2;
      
#      print "centroids are ",centroids
#      print "Slice threshold is",slice_threshold
      # slice the signal 
      slice_stream=numpy.array([0]*len(abs_stream)); 
      for i in range(0,len(slice_stream)): 
          slice_stream[i]=1 if (abs_stream[i]>slice_threshold) else 0;

      # find start of pulse 
      start_index=numpy.nonzero(slice_stream)[0][0]
#      print "Pulse starts at index ",start_index," and time ",timeSeries[start_index]
      print timeSeries[start_index]
      # plot slice_stream 
      p.figure(1)
      subplot(211)
      p.plot(slice_stream);
 
      # plot abs_stream
      subplot(212)
      p.plot(abs_stream);
      p.plot(numpy.array([slice_threshold]*len(abs_stream)));

      # show it now 
      p.show()

# Now the main routine 
if(len(sys.argv) < 6) : 
    print "Usage : python recv-pulse.py filename centre_freq bandwidth sample_rate channel"
    exit(5) 
fh=open(sys.argv[1],'r');
freq=float(sys.argv[2]);
bandwidth=float(sys.argv[3]);
sample_rate=float(sys.argv[4]); 
channel=int(sys.argv[5]);
stream=read_from_file(fh,channel) ;
tuned_stream=quad_demod(stream,freq+bandwidth/2,bandwidth,sample_rate);
#for i in range(0,len(timeSeries)): 
#     print timeSeries[i],"\t",tuned_stream[i]
# 
