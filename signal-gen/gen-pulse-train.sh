#! /bin/bash
## generate a train of pulses, spaced by some constant time from each other 

# command line arguments 
if [  $# -lt 5 ] ; then
   echo "Usage: ./gen-pulse-train.sh numPulses  pulse_width pulse_separation start_freq noise_bw"
   exit
fi
num_pulses=$1
pulse_width=$2
separation=$3
start_freq=$4
noise_bw=$5
silence_time=`echo "scale=7;$separation - $pulse_width" | bc`
# pulse_separation is the distance between start of pulses. Find the distance from end of pulse to start of the next one
i=0
while [ $i -lt $num_pulses ] ; do 
    freq_delta=`expr $i '*' 100`
    freq=`expr $start_freq '+' $freq_delta`
    ./tone-gen -c $freq -b $noise_bw -d $pulse_width -a 1 # amplitude of 1 
    mv noiseShifted.dat $freq.dat
    # generate silence 
    ./tone-gen -c $freq -b $noise_bw -d $silence_time -a 0 # amplitude of 0 
    mv noiseShifted.dat $freq-silence.dat
    sox --combine concatenate $freq.dat $freq-silence.dat $freq-combined.dat # combine the two 
    echo "Finished generating sound and silence for $freq Hz"
    i=`expr $i '+' 1`
done
cp $start_freq-combined.dat combined-file.dat
i=1
while [ $i -lt $num_pulses ] ; do 
    freq_delta=`expr $i '*' 100`
    freq=`expr $start_freq '+' $freq_delta`
    sox --combine concatenate combined-file.dat $freq-combined.dat temp.dat
    mv temp.dat combined-file.dat
    echo "Appending  sound and silence for $freq Hz to main file"
    i=`expr $i '+' 1`
done
sox combined-file.dat combined-file.wav
rm *.dat # clean up all the other files. 
