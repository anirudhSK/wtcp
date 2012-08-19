#! /bin/bash
# setup packet forwarder
if [ $# -lt 2 ]; then
   echo "Usage : ./setup.sh ingress egress"
   exit
fi ;

ingress=$1
egress=$2

# put both interfaces in promisc mode. 
# Otherwise, the ping requests and responses won't come back correctly. 
sudo ifconfig $ingress up promisc
sudo ifconfig $egress up promisc

# TODO: Set client MAC address

# Disable segmentation offloading to NIC. 

sudo ethtool --offload  $ingress gso off  tso off gro off  # lro off ufo off   # apparently lro and ufo are not on the cards. 
sudo ethtool --offload  $ingress gso off  tso off gro off  # lro off ufo off

# now start the pkt forwarding 

sudo ./packet-forwader $ingress $egress
