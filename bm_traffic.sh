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

DEV=net0
BM_NITER=${BM_NITER:-6}

if [ -f $OUTPUT_FILE ]; then
  mv $OUTPUT_FILE $OUTPUT_FILE.save
fi
echo "# $@" > $OUTPUT_FILE

for i in $(seq 1 $BM_NITER); do
  free && ./clear_page_cache && free

  RX_N_BEFORE=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
  RX_BYTE_BEFORE=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')
  TX_N_BEFORE=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
  TX_BYTE_BEFORE=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')

  $@

  RX_N_AFTER=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
  RX_BYTE_AFTER=$(ifconfig $DEV | grep "RX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')
  TX_N_AFTER=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*packets \([0-9]*\).*$/\1/')
  TX_BYTE_AFTER=$(ifconfig $DEV | grep "TX" | grep -v "errors" | sed 's/^.*bytes \([0-9]*\).*$/\1/')

  this_result=$(mktemp)
  if [ $i -eq 1 ]; then
    echo "RX_bytes $(($RX_BYTE_AFTER - $RX_BYTE_BEFORE))" >> $this_result
    echo "RX_packets $(($RX_N_AFTER - $RX_N_BEFORE))" >> $this_result
    echo "TX_packets $(($TX_N_AFTER - $TX_N_BEFORE))" >> $this_result
    echo "TX_bytes $(($TX_BYTE_AFTER - $TX_BYTE_BEFORE))" >> $this_result
  else
    echo "$(($RX_BYTE_AFTER - $RX_BYTE_BEFORE))" >> $this_result
    echo "$(($RX_N_AFTER - $RX_N_BEFORE))" >> $this_result
    echo "$(($TX_N_AFTER - $TX_N_BEFORE))" >> $this_result
    echo "$(($TX_BYTE_AFTER - $TX_BYTE_BEFORE))" >> $this_result
  fi
  if [ $i -eq 1 ]; then
    mv $this_result ${OUTPUT_FILE}.working
  else
    paste ${OUTPUT_FILE}.working $this_result > ${OUTPUT_FILE}.paste
    mv ${OUTPUT_FILE}.paste ${OUTPUT_FILE}.working
  fi
  rm -f $this_result
done
mv ${OUTPUT_FILE}.working ${OUTPUT_FILE}
