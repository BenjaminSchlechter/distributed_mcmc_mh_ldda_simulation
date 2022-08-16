#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=40
#SBATCH --mem=8000
#SBATCH --job-name=mcmcprobsat


echo "$ run.sh $@"

date

# S C BATCH --time=7
# S C BATCH --nodes=1
# S C BATCH --cpus-per-task=1
# S C BATCH --ntasks-per-node=80

# to run use $ sbatch --partition=dev_multiple run.sh output_filename [main parameters]
# scontrol show job

# i.e. for a single node with 40 tasks:
# sbatch --partition=dev_single run.sh log.txt -n 5 -o 0 -l 20

# sbatch -t 7 --partition=dev_single run.sh log.txt -n 10 -o 0 -l 20

# local test: ./run.sh out.txt -n 1 -o 0 -l 2

([ -f "main.cpp" ]) || (echo "wrong directory" && exit)

echo "preparing environment & load modules"

module list
[ $(module list compiler/gnu/11.1 2>&1 |grep -ci "None found") == 1 ] && echo "loading gnu compiler 11.1" && module load compiler/gnu/11.1

[ $(module list mpi/openmpi/default 2>&1 |grep -ci "None found") != 1 ] && echo "unload default openmpi" && module unload mpi/openmpi/default
[ $(module list mpi/openmpi/4.1 2>&1 |grep -ci "None found") != 1 ] && echo "unload openmpi 4.1" && module unload mpi/openmpi/4.1

[ $(module list compiler/gnu/11.1 2>&1 |grep -ci "None found") == 1 ] && echo "loading gnu compiler 11.1" && module load compiler/gnu/11.1
[ $(module list mpi/openmpi/4.1 2>&1 |grep -ci "None found") == 1 ] && echo "loading openmpi 4.1" && module load mpi/openmpi/4.1

[ $(module list devel/cmake/3.18 2>&1 |grep -ci "None found") == 1 ] && echo "loading cmake 3.18" && module load devel/cmake/3.18

module list

g++ --version |head -1

echo "building"

#rm main &> /dev/null
([ -f "main" ]) || (mpic++ -I "bitsery/include/" -O2 -std=c++20 -Wall -Wextra -o main main.cpp)

([ -f "main" ]) || (echo "fatal: build error! exit now" && exit)


outfile="$1"
([ -f $outfile ]) && (echo "output file already exists!") && exit

shift 1

echo "running"

# socket node --use-hwthread-cpus 
#-mca coll ^hcoll
#--mca osc_ucx_priority 0
#--mca mpi_warn_on_fork 0

date

(mpirun --mca pml ^ucx --mca mpi_warn_on_fork 0 --bind-to core --map-by core main $@ &> $outfile) || echo ""

echo "done."



