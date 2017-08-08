#!/bin/sh

if [ $(id -u) -ne 0 ]; then
  echo "Needs root"
  exit 1
fi

DEST=centos7-chroot
DIRS="/sys /proc /dev /var/run /tmp"

for dir in $DIRS; do
  mount --bind $dir $DEST$dir
done
mount --rbind /eos $DEST/eos

chroot $DEST

umount /eos/*
for dir in $DIRS; do
  umount $DEST$dir
done

