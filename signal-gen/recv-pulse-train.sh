#! /bin/bash
## receive a train of pulses, placed at some distance from each other 

# command line arguments 
if [  $# -lt 5 ] ; then
   echo "Usage: ./recv-pulse-train.sh recv_file noise_bw sample_rate first_freq num_pulses"
   exit
fi
recv_file=$1
noise_bw=$2
sample_rate=$3
first_freq=$4
num_pulses=$5
i=0
sox $recv_file recv.dat
while [ $i -lt $num_pulses ] ; do 
    freq_delta=`expr $i '*' 100`
    start_freq=`expr $first_freq '+' $freq_delta`
    set -v 
    set -x
    python recv-pulse.py recv.dat $start_freq $noise_bw $sample_rate > $start_freq-recv.dat
    set +v
    set +x
    echo "Finished tuning into pulse spanning $noise_bw starting at $start_freq Hz"
    i=`expr $i '+' 1`
done

