#!/bin/sh

die() {
  echo "$1"
  exit 1
}

usage() {
  echo "$0 -o <output> -w <watch filename> -- <command>"
}

OUTPUT_FILE=
WATCH_FILENAME=

while getopts "hvo:w:" option; do
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
    w)
      WATCH_FILENAME=$OPTARG
    ;;
    ?)
      usage
      exit 1
    ;;
  esac
done
shift $(($OPTIND - 1))

[ "x$OUTPUT_FILE" = "x" ] && die "output missing"
[ "x$WATCH_FILENAME" = "x" ] && die "watch file name missing"

SOCKET_FILE=$(mktemp)

./iotrace_capture -o $OUTPUT_FILE -s $SOCKET_FILE &
PID_CAPTURE=$!
echo "Capturing on $SOCKET_FILE from PID $PID_CAPTURE"
while [ ! -f $SOCKET_FILE ]; do
  sleep 1
done
sleep 2

echo "Capturing in PID $PID_CAPTURE"
echo "Running command $@"
LD_PRELOAD=./libiotrace.so IOTRACE_FILENAME=$WATCH_FILENAME \
  IOTRACE_FANOUT=$SOCKET_FILE $@
kill -QUIT $PID_CAPTURE
wait $PID_CAPTURE

rm -f $SOCKET_FILE $SOCKET_FILE.ready