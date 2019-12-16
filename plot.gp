set datafile separator ","
set terminal pngcairo size 1000, 3000
set logscale x 10
if (ARG2 eq "YLOG") set logscale y 10
set multiplot layout 18, 1
do for [index=0:17] {
    plot ARG1 i index with linespoints title columnheader(1)
}