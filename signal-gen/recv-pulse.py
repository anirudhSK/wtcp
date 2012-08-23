import sys
from math import *
import numpy
import matplotlib.pyplot as p
from matplotlib.pyplot import *
import scipy.cluster as cluster
from Filter import *
def read_from_file(fh,channel) : 
     ''' File handling routines 
     '''
     stream=[]
     time_series=[]
     for line in fh.readlines() : 
         values=line.split()
         if( values[0] == ";") : # comment line ditch this
           continue; 
         if(channel==1) :
           stream.append(float(values[1])) 
         else :
           stream.append(float(values[2]))
     return numpy.array(stream)

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
      return numpy.nonzero(slice_stream)[0][0]

# Now the main routine
def get_sender_schedule(freq_file_fh) : 
   schedule=[]
   for line in freq_file_fh.readlines() : 
       if(len(line.split()) == 4) :
         schedule.append(0);
       else :
         freq=float(line.split()[5]);
         schedule.append(freq);  
   return schedule

if(len(sys.argv) < 7) : 
    print "Usage : python recv-pulse.py filename freq_file sample_rate channel pulse_width batch_separation"
    exit(5)

def search_for_freq(stream,start_time,end_time,frequency,sample_rate) : 
    upper_limit=int(end_time*sample_rate);
    if(upper_limit>len(stream)):
       upper_limit=len(stream);
    lower_limit=int(start_time*sample_rate);
    restricted_stream=stream[lower_limit:upper_limit];
    start_index=quad_demod(restricted_stream,frequency,0,sample_rate);
    start_time=float(lower_limit+start_index+1)/float(sample_rate)
    print "Frequency ",frequency," at time ",start_time;
    return start_time

# main function  
fh=open(sys.argv[1],'r');
freq_file_fh=open(sys.argv[2],'r');
sample_rate=float(sys.argv[3]); 
channel=int(sys.argv[4]);
pulse_width=float(sys.argv[5]);
batch_separation=float(sys.argv[6]);
stream=read_from_file(fh,channel) ;
# retrieve the stream based on what you sent 
schedule=get_sender_schedule(freq_file_fh);
locations=[0]*len(schedule)
for i in range(0,len(schedule)) :
    if (i==0) : 
      # search 1.5 seconds for the first frequency pulse 
      start_time=0
      end_time=1.5     
    else :
      start_time=locations[i-1]+pulse_width/2; # half way to the end of this pulse 
      end_time=locations[i-1]+5*pulse_width/2; # half way after the pulse you are looking for 

    if(schedule[i]!=0) : # not silence
      locations[i]=search_for_freq(stream,start_time,end_time,schedule[i],sample_rate) 
    else :  # if it is silence 
      locations[i]=locations[i-1] + batch_separation ; # in effect, "skip" the silence
