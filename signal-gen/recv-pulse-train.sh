#! /bin/bash
## receive a train of pulses, placed at some distance from each other 

# command line arguments
if [ $# -lt 7 ] ; then
  echo "Usage : ./recv-pulse-train.sh wav_file freq_file sample_rate pulse_width batch_separation amplitude1 amplitude2"
  exit
fi; 
# channel 1 and 2 respectively 
python recv-pulse.py $1 $2 $3 $4 $5 $6 1 2> $1.Chan1logs > $1.Chan1results &
python recv-pulse.py $1 $2 $3 $4 $5 $7 2 2> $1.Chan2logs > $1.Chan2results  
wait $! # wait till the first channel finishes up 
python subtract.py $1.Chan2results $1.Chan1results
