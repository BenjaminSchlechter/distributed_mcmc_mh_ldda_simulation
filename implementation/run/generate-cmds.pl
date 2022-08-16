#!/bin/perl

# seq 0 19 |parallel echo "sbatch -t 300 --partition=single run.sh r1/log-{}.txt -n 10 -of {} -l 800"

use strict;
use warnings;

#seq 0 19 |parallel echo "sbatch -t 300 --partition=single run.sh r1/log-{}.txt -n 10 -of {} -l 800"

my @T = (300,200,100,75,50,40,30,20,10,5);
my $n = 5;
my $l = 200;
my $num_to_gen = 20;

foreach my $t (@T) {
	for my $i (0..int(($num_to_gen+$n-1)/$n)-1) {
		print "sbatch -t 30 --partition=dev_single run.sh r7/log-n$n-of$i-l$l-t$t-f2.txt -n $n -of $i -l $l -t $t -f 2"."\n";
	}
}

