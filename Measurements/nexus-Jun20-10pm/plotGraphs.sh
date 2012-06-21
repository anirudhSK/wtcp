#! /bin/bash
rm -rf *.png
for f in *.txt ; do
    grep "Sender Dgm" $f > $f.udp # This is correct 
    set -v 
    set -x
    wc -l $f.udp
    set +v 
    set +x
    python plotLatencies.py $f.udp 
    mv txRx.plot $f.txrx
    mv latency.plot $f.plot
    mv latency.hist $f.hist
    padding=`echo $f | cut -d '-' -f 4 | cut -d '.' -f 1`
    echo $padding
    payload=`expr 4 '*' $padding '+' 16`
    cp plotGraph plotScript
    echo "set output \"$payload.eps\"" >> plotScript
    echo "set title \"Nexus Jun 20 10 pm Payload $payload\"" >> plotScript
    echo "plot '$f.plot' title 'Payload $payload' " >> plotScript 
    gnuplot -p plotScript
done
rm plot.eps
