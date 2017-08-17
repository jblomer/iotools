#!/bin/sh

die() {
  echo "$1"
  exit 1
}

usage() {
  echo "$0 -i <input file> -o <output file> -n <iterations> [-s]"
  echo "    [-t <timeout>] [-r <initial seed>] [-e <expected result>]"
  echo "    -- <command>"
}

INPUT_FILE=
OUTPUT_FILE=
N=
TIMEOUT=30
SHOW_OUTPUT=0
SEED_OFFSET=0
EXPECTED_RESULT=

while getopts "hvi:o:n:t:sr:e:" option; do
  case $option in
    h)
      usage
      exit 0
    ;;
    v)
      usage
      exit 0
    ;;
    i)
      INPUT_FILE=$OPTARG
    ;;
    o)
      OUTPUT_FILE=$OPTARG
    ;;
    n)
      N=$OPTARG
    ;;
    t)
      TIMEOUT=$OPTARG
    ;;
    s)
      SHOW_OUTPUT=1
    ;;
    r)
      SEED_OFFSET=$(($OPTARG - 1))
    ;;
    e)
      EXPECTED_RESULT="$OPTARG"
    ;;
    ?)
      usage
      exit 1
    ;;
  esac
done
shift $(($OPTIND - 1))

[ "x$INPUT_FILE" = "x" ] && die "input missing"
[ "x$OUTPUT_FILE" = "x" ] && die "output missing"
[ "x$N" = "x" ] && die "number of trials missing"

echo "# running command $@"
for i in $(seq 1 $N); do
  CMD_BITFLIP=$(mktemp)
  ./mkfaulty -i $INPUT_FILE -o $OUTPUT_FILE -n 1 -s $(($SEED_OFFSET + $i)) \
    >$CMD_BITFLIP || die "failed creating bit-flipped file"
  BIT_NUM=$(grep "flipping bit at position Bit" $CMD_BITFLIP | \
    sed 's/^.*position Bit \([0-9]*\).*$/\1/')
  rm -f $CMD_BITFLIP

  CMD_STDOUT=$(mktemp)
  CMD_STDERR=$(mktemp)
  $@ >$CMD_STDOUT 2>$CMD_STDERR &
  PID=$!
  START_TIME=$SECONDS
  RETVAL="?"
  while [ "x$RETVAL" = "x?" -a $((${SECONDS}-${START_TIME})) -lt ${TIMEOUT} ];
  do
    /bin/sleep 1
    kill -s 0 $PID >/dev/null 2>&1
    if [ $? -ne 0 ]; then
      wait $PID
      RETVAL=$?
    fi
  done
  [ "x$RETVAL" = "x?" ] && kill -s 9 $PID >/dev/null

  RESULT=$(tail -n 1 $CMD_STDOUT | grep '^finished' | \
    sed 's/^.*result: \([0-9.-]*\).*$/\1/')
  if [ "x$RESULT" = "x" ]; then
    RESULT="?"
  fi
  NEVENTS=$(tail -n 1 $CMD_STDOUT | grep 'events' | \
    sed 's/^.*(\([0-9]*\) events).*$/\1/')
  if [ "x$NEVENTS" = "x" ]; then
    NEVENTS="?"
  fi
  grep -qi '\(error\)\|\(terminate called after\)' $CMD_STDERR
  HAS_ERRORS=$((1 - $?))
  SHORT_READ=0
  [ $NEVENTS != "500000" ] && SHORT_READ=1
  if [ $SHOW_OUTPUT -eq 1 ]; then
    echo "##### STDOUT #####"
    cat $CMD_STDOUT
    echo "##### STDERR #####"
    cat $CMD_STDERR
    echo "##################"
  fi
  rm -f $CMD_STDOUT $CMD_STDERR
  echo -n "SEED: $(($SEED_OFFSET + $i)) ERRPOS: $BIT_NUM RETVAL: $RETVAL HASERR: $HAS_ERRORS SHORTREAD: $SHORT_READ RESULT: $RESULT"
  if [ "x$EXPECTED_RESULT" != "x" ]; then
    if [ "x$EXPECTED_RESULT" = "x$RESULT" ]; then
      echo " CORRECT: 1"
    else
      echo " CORRECT: 0"
    fi
  else
    echo
  fi
done
