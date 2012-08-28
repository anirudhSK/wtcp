#! /bin/bash

# Steps
# 1. Make sure the video and the audio are all setup according to our settings before running the script i.e. do all the sound checks.
# 2. Caller has an unrestrained connection to Internet. Callee goes through traffic shaper. 
# 2a - Add a 20 ms latency using netem on the ingress. 
# 3. Start the trace driven shaping. Establish the call as soon as the trace driven shaping starts. Set up web cams as you want to 
# 4. From the point of time that the shaping starts, wait 10 minutes for the caller and callee to settle down. This 10 minutes is for you to do any setup as well. 
# 5. Now send a 10 minute audio pattern.
# 6. Receive it and process it. Print audio latencies using universal time stamps using the date just before aplay starts playing.  
# 7. Graph queue occupancy (normalized to seconds), network throughput and audio latency all on the same graph using the Universal clock reference. 

# Clean up first and foremost. 
echo "Cleaning up any vestiges"
sudo killall -s9 traffic-shaping arecord aplay python setup.sh recv-pulse-train.sh
# remove the 20 ms delay
sudo tc qdisc del dev $ingress root netem delay 20ms
sudo tc qdisc del dev $egress root netem delay 20ms

# cmd line args 

ingress=$1                 # interface in front of callee
egress=$2                  # fwding interface to send callee's packets into the world
uplink_trace=$3            # from ingress to egress
downlink_trace=$4          # reverse
audio_file=$5              # 10 min audio pattern 
audio_schedule=$6          # frequency schedule of the pattern
callee_mac=$7              # callee's mac
expt=$8                    # expt name 
if [ $# -lt 8 ] ; 
then
   echo "Usage : ./run-exp.sh ingress egress uplink_trace downlink_trace audio_file audio_schedule callee_mac expt_name"
   exit ; 
fi
# Step 1:
echo "Make sure the video and the audio are all setup .
      The caller must have an unrestrained conn to the Internet.
      The callee must be connected through this machine to the ingress $ingress.
      The MAC address supplied $callee_mac should be the callee's MAC.
      The egress $egress must be connected to the Internet.
      Do all the reqd sound checks. 
      Press enter to continue once done ..."
read

# 2a- add 20 ms latencies on both uplink and downlink
sudo tc qdisc add dev $ingress root netem delay 20ms
sudo tc qdisc add dev $egress root netem delay 20ms

# Step 3 setup traffic shaping in the background 
set -v
set -x
./setup.sh $ingress $egress $callee_mac $uplink_trace $downlink_trace $expt   
set +v
set +x
 # Step 4: wait till the call has been initiated and webcams have been setup, then start recording 
echo "Traffic shaping is set up. 
      Expt will start in 10 minutes. 
      Please do whatever you need to in 10 minutes.
      This includes call initiation and web cam setup
      Press enter if done "

read -t 600  

arecord -t wav -f cd $expt.wav &
date +%s > /tmp/audio_stats.txt
export start_time=`head -n 1 /tmp/audio_stats.txt`
sleep 1                    # wait till you can play the file

# Step 5 : Send audio pattern  
aplay $audio_file          
killall -s9 arecord

# Step 6: Process it to get latencies 
/usr/bin/time ./recv-pulse-train.sh $expt.wav $audio_schedule 44100 0.1 1 0.3 0.25 >> /tmp/audio_stats.txt 
python convert-into-unix.py /tmp/audio_stats.txt $audio_schedule > ./audio_unix.txt  
grep "uplink at time" $expt.logs > ./uplink.logs
grep "downlink at time" $expt.logs > ./downlink.logs
export fin_time=`expr $start_time '+' 900` # give it 15 minutes time 


# Step 7 : Plot quantities one on top of the other using multiplot 
gnuplot -p side-by-side.p

# Other stuff : Save the files for archival 
mv uplink.logs $expt.uplink
mv downlink.logs $expt.downlink
mv audio_unix.txt $expt.audio

# remove the 20 ms delay
sudo tc qdisc del dev $ingress root netem delay 20ms
sudo tc qdisc del dev $egress root netem delay 20ms
