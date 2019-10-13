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

MOUNT_DIR=$(mktemp -d)
LOG_DIR=$(mktemp -d)

FF_PHYS_PATH=$(dirname $WATCH_FILENAME) FF_LOG_PATH=$LOG_DIR \
  ./fuse_forward $MOUNT_DIR
echo "Interposition FS mounted on $MOUNT_DIR"

CMD=$(echo "$@" | sed s,@MOUNT_DIR@,$MOUNT_DIR,g)
echo "Running $CMD"
$CMD

fusermount -u $MOUNT_DIR
rmdir $MOUNT_DIR
mv $LOG_DIR/$(echo $WATCH_FILENAME | sed s,/,-,g) $OUTPUT_FILE
rm -rf $LOG_DIR
