set title "Downlink "
time_offset=`echo $offset`
plot "downlink.rtt" u ($1/1.0e3 - time_offset):2 title "rtt", "downlink.one_way" u ($1/1.0e3 - time_offset):2 title "1way", "downlink.one_way" u ($1/1.0e3 - time_offset) :3 title "Throughput", 0.25 title "250 ms"
set title "Uplink "
plot "uplink.rtt" u ($1/1.0e3 - time_offset):2 title "rtt", "uplink.one_way" u ($1/1.0e3 - time_offset):2 title "1way", "downlink.one_way" u ($1/1.0e3 - time_offset):3 title "Throughput", 0.25 title "250 ms"
