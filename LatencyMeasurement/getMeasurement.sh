iterations=50
i=25
while [ $i -lt $iterations ] ; do 
   arecord -t wav -f cd Aug2EthSameBox/rx$i.wav &
   sleep 1
   aplay 1kHz_44100Hz_16bit_05sec.wav
   killall -s9 arecord 
   cd Aug2EthSameBox 
   sox rx$i.wav rx$i.dat
#   python getDelay.py rx$i.dat 100 > rx$i.plot
#   echo "delay is"$delay
   echo "Now iteration number $i over at time"
   date
   cd ..
   i=`expr $i '+' 1`
done
