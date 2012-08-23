import sys
from math import *
import numpy
import matplotlib.pyplot as p
from matplotlib.pyplot import *
import scipy.cluster as cluster
from Filter import *
from scipy.signal import kaiserord, lfilter, firwin, freqz
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

def low_pass_filter(cutoff,width,sample_rate) : 
    ''' 
        Get low pass filter kernel  for cutoff freq in Hz width in Hz
        Borrowed from http://www.scipy.org/Cookbook/FIRFilter '''
    nyq_rate = sample_rate / 2.0
    width = width/nyq_rate # normalize to Nyq Rate 
    ripple_db = 60.0 # 60 dB attenuation in the stop band
    # Compute the order and Kaiser parameter for the FIR filter.
    N,beta = kaiserord(ripple_db, width)
    cutoff=cutoff/nyq_rate
    taps = firwin(N, cutoff, window=('kaiser', beta))
    return taps

def run_low_pass(stream,kernel) : 
    filtered_signal=lfilter(kernel,1.0,stream);  
    # phase delay
    delay_in_samples = int(0.5 * (len(kernel)-1)) ;
    # "pull back" the delayed signal
    # ie start returning the signal from the delay_in_samples^{th} signal onwards as the correct signal. Pad enough zeros at the end to make it the same size.
    return numpy.concatenate((filtered_signal[delay_in_samples:],numpy.array([0]*delay_in_samples)))

def quad_demod(stream,centre_freq,bandwidth,sample_rate,amplitude) :
      ''' 
          quadrature demodulation of received signal 
      '''
      stream_length=len(stream); 
      nvalues=range(0,stream_length);
      
      # lpf common to I and Q
      lpf=low_pass_filter(50.0,50.0,sample_rate);
      # modulate with cosine and lpf it
      cosine=numpy.cos( ((2* numpy.pi * centre_freq)/sample_rate) * numpy.array(nvalues));
      i_stream=cosine*stream ;
      i_stream_lpf=run_low_pass(i_stream,lpf);

      # modulate with sine and lpf it
      sine=numpy.sin( ((2* numpy.pi * centre_freq)/sample_rate) * numpy.array(nvalues));
      q_stream=sine*stream ;
      q_stream_lpf=run_low_pass(q_stream,lpf); 

      # get the absolute value  
      abs_stream=numpy.sqrt(i_stream_lpf*i_stream_lpf + q_stream_lpf*q_stream_lpf);

      # find slicing threshold for peak detector. 
      slice_threshold=(amplitude/4); # / 2 for the original 1/2 while squaring, Another /2 for detecting the crossing. 
#      print "Slice threshold is",slice_threshold
      # slice the signal 
      slice_stream=numpy.array([0]*len(abs_stream)); 
      for i in range(0,len(slice_stream)): 
          slice_stream[i]=1 if (abs_stream[i]>slice_threshold) else 0;

      # find start of pulse 
      # plot slice_stream 
      ##subplot(211)
      ##p.plot(slice_stream);
 
      ### plot abs_stream
      ##subplot(212)
      ## p.figure()
      ## p.plot(abs_stream);
      ## p.plot(numpy.array([slice_threshold]*len(abs_stream)));
      ## p.draw()
      # show it now 
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


def search_for_freq(stream,start_time,end_time,frequency,sample_rate,amplitude) : 
    upper_limit=int(end_time*sample_rate);
    if(upper_limit>len(stream)):
       upper_limit=len(stream);
    lower_limit=int(start_time*sample_rate);
    restricted_stream=stream[lower_limit:upper_limit];
    start_index=quad_demod(restricted_stream,frequency,0,sample_rate,amplitude);
    start_time=float(lower_limit+start_index+1)/float(sample_rate)
    return start_time

# main function  
if(len(sys.argv) < 8) : 
    print "Usage : python recv-pulse.py filename freq_file sample_rate pulse_width batch_separation amplitude1 amplitude2"
    exit(5)

fh=open(sys.argv[1],'r');
freq_file_fh=open(sys.argv[2],'r');
sample_rate=float(sys.argv[3]); 
pulse_width=float(sys.argv[4]);
batch_separation=float(sys.argv[5]);
amplitude=[0.0]*2 # one for each channel cause they have different attenuations. 
amplitude[0]=float(sys.argv[6]);
amplitude[1]=float(sys.argv[7]);
locationAtChannels=[[],[]] # set of locations one for each channel 
schedule=get_sender_schedule(freq_file_fh);
for channel in [1,2] : 
   fh.seek(0); # reset to beginning of file for the other channel 
   stream=read_from_file(fh,channel) ;
   # retrieve the stream based on what you sent 
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
         locations[i]=search_for_freq(stream,start_time,end_time,schedule[i],sample_rate,amplitude[channel-1]) 
       else :  # if it is silence 
         locations[i]=locations[i-1] + batch_separation ; # in effect, "skip" the silence
   locationAtChannels[channel-1]=locations;
# now get latencies 
latency=[0]*len(schedule)
for i in range(0,len(schedule)) :
       latency[i]=locationAtChannels[0][i]-locationAtChannels[1][i] 
       print "At frequency",schedule[i]," latency is ",latency[i]
