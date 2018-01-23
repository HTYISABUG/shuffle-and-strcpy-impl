reset
set xlabel 'array size'
set ylabel 'time(cycles)'
set title 'average execute time'
set term png enhanced font 'Verdana,10'
set output 'shuffle.png'
set logscale x
set xtics rotate by 45 right

plot [:][:100000000]\
'top_in.txt' using 1:2 with linespoints title 'Top-in Shuffle',\
'riffle.txt' using 1:2 with linespoints title 'Riffle Shuffle'
