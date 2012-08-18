#! /bin/bash
## generate a train of pulses, placed at some distance from each other 

# command line arguments 
if [  $# -lt 4 ] ; then
   echo "Usage: ./gen-pulse-train.sh numPulses  pulsewidth pulse separation start_freq "
   exit
fi
num_pulses=$1
pulse_width=$2
separation=$3
start_freq=$4
silence_time=`echo "scale=7;$separation - $pulse_width" | bc`
i=0
while [ $i -lt $num_pulses ] ; do 
    freq_delta=`expr $i '*' 100`
    freq=`expr $start_freq '+' $freq_delta`
    ./tone-gen -c $freq -b 20 -d $pulse_width -a 1
    mv noiseShifted.dat $freq.dat
    # generate silence 
    ./tone-gen -c $freq -b 20 -d $silence_time -a 0
    mv noiseShifted.dat $freq-silence.dat
    i=`expr $i '+' 1`
    echo "Finished generating sound and silence for $freq Hz"
done
