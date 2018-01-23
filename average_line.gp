reset
set xlabel 'string length'
set ylabel 'time(cycles)'
set title 'average execute time'
set term png enhanced font 'Verdana,10'
set output 'average_line.png'
set logscale x
set xtics rotate by 45 right

plot [:65536][:50000]\
'average_orig.txt' using 1:2 with linespoints title 'Orig not aligned 1K',\
'average_orig.txt' using 3:4 with linespoints title 'Orig not aligned 1M',\
'average_orig.txt' using 5:6 with linespoints title 'Orig aligned 1K',\
'average_orig.txt' using 7:8 with linespoints title 'Orig aligned 1M',\
'average_opt.txt' using 1:2 with linespoints title 'Opt not aligned 1K',\
'average_opt.txt' using 3:4 with linespoints title 'Opt not aligned 1M',\
'average_opt.txt' using 5:6 with linespoints title 'Opt aligned 1K',\
'average_opt.txt' using 7:8 with linespoints title 'Opt aligned 1M'
