#! /bin/bash
rm -rf *.png
for f in *.txt ; do
    D=5
    grep "Sender Dgm" $f > $f.udp # This is correct 
    set -v 
    set -x
    wc -l $f.udp
    set +v 
    set +x
    rm $f.glitch
    while [ $D -lt 100 ] ; do
      python plotGlitch.py $f.udp $D >> $f.glitch 
      D=`expr $D '+' 5`
    done 
    padding=`echo $f | cut -d '-' -f 4 | cut -d '.' -f 1`
    echo $padding
    payload=`expr 4 '*' $padding '+' 16`
    mv $f.glitch $payload.glitch
##    cp plotGraph plotScript
##    echo "set output \"$payload.png\"" >> plotScript
##    echo "set title \"Nexus Jun 22 Walking 10 am Payload $payload\"" >> plotScript
##    echo "plot '$f.plot' title 'Payload $payload', '$f.plot' u 1:(\$2<0?50000:0.001) title 'Losses' " >> plotScript 
##    gnuplot -p plotScript
done
rm plot.eps
