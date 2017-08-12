#!/bin/sh

if [ $(id -u) -ne 0 ]; then
  echo "root required"
  exit 1
fi

MS=$1
DEV=net0

if [ "x$MS" = "x" ]; then
  tc qdisc del dev $DEV root netem
  tc qdisc del dev ifb0 root netem
else
  modprobe ifb
  ip link set dev ifb0 up
  tc qdisc add dev $DEV ingress
  tc filter add dev $DEV parent ffff: \
    protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0

  tc qdisc del dev $DEV root netem
  tc qdisc del dev ifb0 root netem

  tc qdisc add dev ifb0 root netem delay $(($MS / 2))ms
  tc qdisc add dev $DEV root netem delay $(($MS / 2))ms
fi

ping -c4 eospps.cern.ch
