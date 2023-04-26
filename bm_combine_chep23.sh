#!/bin/sh

OUTPUT=bm_chep23.txt
RESULTS="chep23/*"

cat /dev/null > $OUTPUT
for f in $RESULTS; do
  tokens=${f%.txt}
  tokens=${tokens##*/}
  sample=$(echo $tokens | cut -d\- -f1)
  method=$(echo $tokens | cut -d\- -f2)
  container=$(echo $tokens | cut -d\- -f3)
  run=$(echo $tokens | cut -d\- -f4)
  realtime=$(cat $f | grep "s elapsed" | sed -e 's/s elapsed).//' | awk '{print $NF}')

  if [ $container = "tree" ]; then
    bw=$(cat $f | grep "Disk IO" | awk '{print $4}')
  else
    bw=$(cat $f | grep "RNTupleDS.RPageSourceFile.bwRead|" | cut -d\| -f4)
  fi

  echo "$sample $method $container $run $realtime $bw" >> $OUTPUT
done
