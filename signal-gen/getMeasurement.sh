iterations=25
i=1
while [ $i -lt $iterations ] ; do 
   arecord -t wav -f cd Aug18thSameEth/rx$i.wav &
   sleep 1
#   aplay combined-file.wav
   aplay 500Hz.wav
   killall -s9 arecord 
   cd Aug18thSameEth
   sox rx$i.wav rx$i.dat
#   python getDelay.py rx$i.dat 100 > rx$i.plot
#   echo "delay is"$delay
   echo "Now iteration number $i over at time"
   date
   cd ..
   i=`expr $i '+' 1`
done
