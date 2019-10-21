#!/bin/sh

bytes=$(awk '{SUM += $2} END {print SUM}')
echo "$bytes / 1024" | bc -l

