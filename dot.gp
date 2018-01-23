reset
set xlabel 'string length'
set ylabel 'time(cycles)'
set title 'execute time'
set term png enhanced font 'Verdana,10'
set output 'dot.png'
set logscale x
set xtics rotate by 45 right

plot [:65536][:50000]\
'na1k_orig.txt' using 1:2 with points title 'Orig not aligned 1K',\
'na1m_orig.txt' using 1:2 with points title 'Orig not aligned 1M',\
'a1k_orig.txt' using 1:2 with points title 'Orig aligned 1K',\
'a1m_orig.txt' using 1:2 with points title 'Orig aligned 1M',\
'na1k_opt.txt' using 1:2 with points title 'Opt not aligned 1K',\
'na1m_opt.txt' using 1:2 with points title 'Opt not aligned 1M',\
'a1k_opt.txt' using 1:2 with points title 'Opt aligned 1K',\
'a1m_opt.txt' using 1:2 with points title 'Opt aligned 1M'
