#!/bin/bash

# cat r2/n400-o0-l2.txt |grep "iteration 1 value" |sort -k2n

# cat r2/n400-o0-l2.txt |grep "iteration 1 value" |cut -d " " -f 6 |sort -k1g > r2/default.csv
cat r2/n350* |grep "iteration 1 value" |cut -d " " -f 6 |sort -k1g > r2/default.csv

# cat r2/log-n5-of0-l160.txt |grep "mc .* iteration .* value" |sort -k2n -k4g
# cat r2/log-n5-of0-l160.txt |grep "mc .* iteration .* value" |grep "iteration 1.9" |sort -k2n -k4g

cat r2/log-n5-of0-l160.txt |grep "mc .* iteration .* value" |grep "iteration 1.[9|5]" |cut -d " " -f 6 |sort -k1g > r2/adjust.csv

# cat r6/log-n10-o0-l200-t100-f2.txt |grep "mc .* iteration .* value" |grep "iteration 1.[9|5]" |cut -d " " -f 6 |sort -k1g > r2/adjust-t100.csv
cat r5/log-n20-of0-l200-t300.txt |grep "mc .* iteration .* value" |grep "iteration 1[5-9][9|5]" |cut -d " " -f 6 |sort -k1g > r2/adjust-t300.csv
cat r7/log-n5-of0-l200-t50-f2.txt |grep "mc .* iteration .* value" |grep "iteration 1[5-9][9|5]" |cut -d " " -f 6 |sort -k1g > r2/adjust-t50.csv
