set multiplot layout 3,1 title "bit-rate, queue latency and audio latency vs. time"
set title "bit-rate"
set xlabel "seconds"
set ylabel "bit-rate in bps"
set xrange [`echo $start_time` : `echo $fin_time` ]
plot "uplink.logs" u ($4/1e9) : 16 title "uplink" ,  "downlink.logs" u ($4/1e9) : 16 title "downlink"

set title "queue-latency"
set xlabel "seconds"
set ylabel "Latency in ms"
set xrange [`echo $start_time` : `echo $fin_time` ]
plot "uplink.logs" u ($4/1e9) : ($13*8*1000:$16) title "uplink"  , "downlink.logs" u ($4/1e9) : ($13*8*1000:$16)  title "downlink"

set title "audio latency"
set xlabel "seconds"
set ylabel "Audio Latency in ms"
set xrange [`echo $start_time` : `echo $fin_time` ]
plot "audio_unix.txt" u 1:($2*1000) w lines 
