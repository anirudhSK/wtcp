import sys
from math import *
import numpy


timeSeries=[]
## May come in handy later 
##def sinc(x): 
##    ''' Normalized sinc function 
##    '''
##    return float(sin(pi*x)/(pi*x))

def get_discrete_freq(real_freq,sample_rate) : 
    '''
       Convert from real frequency to Omega freq. 
    '''
    rad_per_sec=float(real_freq)*2*pi
    rad_per_sample=rad_per_sec/sample_rate
    return rad_per_sample

def band_pass_filter(b_low,b_high,filter_length,) : 
    ''' 
        Get Band pass filter kernel 
        for lower limit b_low
        and upper limit b_high
        b_low and b_high are in units of radians per sample 
        Truncate to filter_length
    '''
    assert(b_low<b_high) 
    nvalues=range(1,filter_length) # generate the argument array from 1 until sample length -1  
    val_array=numpy.array(nvalues) 
    bph=(2*b_high*numpy.sin(2*pi*b_high*val_array))/(2*pi*b_high*val_array)
    bpl=(2*b_low *numpy.sin(2*pi*b_low *val_array))/(2*pi*b_low*val_array)
    return bph-bpl 
 
def averageStream(stream) : 
     '''
        Run a low pass filter through the stream 
     ''' 
     average=[0]*len(stream) 
     window=748 ## 44000 khz * 17 ms ie the AC frequency. 
     for i in range(window,len(stream)-window-1): 
        avg=0
        for j in range(i,i+window) : 
          avg=avg+stream[j]
        average[i]=float(avg)/window #/ float(prevAcc)
     return average

def detectPeak(stream) : 
     '''
        Peak detector in Python
     '''
     energy=[1]*len(stream)
     window=int(sys.argv[2])
     for i in range(window,len(stream)-window-1): 
        acc=0
        for j in range(i,i+window) : 
          acc=acc+stream[j]*stream[j]
        prevAcc=0
        for j in range(i-window,i) : 
          prevAcc=prevAcc+stream[j]*stream[j]
        if(prevAcc != 0) :
         energy[i]=float(acc)/float(prevAcc)
        else : 
         energy[i]=1 
     return energy 

def read_from_file(fh) : 
     ''' File handling routines 
     '''
     stream1=[]
     stream2=[]
     global timeSeries
     for line in fh.readlines() : 
         values=line.split()
         if( values[0] == ";") : # comment line ditch this
           continue; 
         else : 
           timeSeries.append(float(values[0]))
           stream1.append(float(values[1])) 
           stream2.append(float(values[2]))
     return (numpy.array(stream1),numpy.array(stream2))     

def do_bpf(stream,bpf) : 
     ''' 
        The actual band pass filtering operation
     '''
     stream_after_bpf=numpy.convolve(stream,bpf)
     return stream_after_bpf 

# Now the main routine 
fh=open(sys.argv[1],'r');
(left,right)=read_from_file(fh) ;
bl=get_discrete_freq(800,44000); # lower limit 800 Hz, sampling frequency is 44000 Hz
bh=get_discrete_freq(1200,44000); # higher limit 1200 Hz, sampling frequency is 44000 Hz
bpf=band_pass_filter(bl,bh,200);
clean_left_signal=do_bpf(left,bpf)
clean_right_signal=do_bpf(right,bpf)
left_peaks=detectPeak(clean_left_signal)
right_peaks=detectPeak(clean_right_signal)

left_delay=timeSeries[numpy.argmax(left_peaks)]
right_delay=timeSeries[numpy.argmax(right_peaks)]
print "Left delay is ",left_delay,"Right delay is",right_delay, "Net delay is",left_delay-right_delay

for i in range(0,len(timeSeries)): 
    print >> sys.stderr, timeSeries[i],"\t",clean_left_signal[i],"\t",clean_right_signal[i],"\t" , left_peaks[i],"\t",right_peaks[i] 
