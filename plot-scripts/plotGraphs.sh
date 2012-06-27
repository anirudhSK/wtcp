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
    mv latency.cdf $f.cdf
    padding=`echo $f | cut -d '-' -f 4 | cut -d '.' -f 1`
    echo $padding
    payload=`expr 4 '*' $padding '+' 16`
    cp plotGraph plotScript
    echo "set output \"$f-$payload.png\"" >> plotScript
    echo "set title \"$f $payload\"" >> plotScript
    echo "plot '$f.plot' title 'Payload $payload', '$f.plot' u 1:(\$2<0?50000:0.001) title 'Losses' " >> plotScript 
    gnuplot -p plotScript
done
rm plot.eps
