#!/bin/sh

die() {
  echo "$1"
  exit 1
}

usage() {
  echo "$0 -o <output> -- <command>"
}

OUTPUT_FILE=

while getopts "hvo:" option; do
  case $option in
    h)
      usage
      exit 0
    ;;
    v)
      usage
      exit 0
    ;;
    o)
      OUTPUT_FILE=$OPTARG
    ;;
    ?)
      usage
      exit 1
    ;;
  esac
done
shift $(($OPTIND - 1))

[ "x$OUTPUT_FILE" = "x" ] && die "output missing"

free && ./clear_page_cache && free

DEV=net0

RX_N_BEFORE=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
RX_BYTE_BEFORE=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')
TX_N_BEFORE=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
TX_BYTE_BEFORE=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')

$@

RX_N_AFTER=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
RX_BYTE_AFTER=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')
TX_N_AFTER=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
TX_BYTE_AFTER=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')

echo "# $@" > $OUTPUT_FILE
echo "RX(bytes) $(($RX_BYTE_AFTER - $RX_BYTE_BEFORE))" >> $OUTPUT_FILE
echo "RX(packets) $(($RX_N_AFTER - $RX_N_BEFORE))" >> $OUTPUT_FILE
echo "TX(packets) $(($TX_N_AFTER - $TX_N_BEFORE))" >> $OUTPUT_FILE
echo "TX(bytes) $(($TX_BYTE_AFTER - $TX_BYTE_BEFORE))" >> $OUTPUT_FILE
