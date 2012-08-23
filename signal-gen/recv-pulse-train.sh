#! /bin/bash
## receive a train of pulses, placed at some distance from each other 

# command line arguments
if [ $# -lt 6 ] ; then
  echo "Usage : ./recv-pulse-train.sh wav_file freq_file sample_rate pulse_width batch_separation amplitude1 amplitude2"
  exit
fi; 
wav_file=$1 # user inputs wav file, along with everything else
sox $wav_file recv.dat
python recv-pulse.py recv.dat $2 $3 $4 $5 $6 $7
