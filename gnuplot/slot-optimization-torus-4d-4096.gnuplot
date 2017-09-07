set yrange [0.0:1.2]
set key top left font "GothicBBB-Medium-RKSJ-H, 15"
#set key c tm horizontal box font "GothicBBB-Medium-RKSJ-H, 13"
set grid
set style data histograms
#set style fill solid 1.00 border -1
set style fill pattern 3 border -1
#set title "Average Queuing Time (16 nodes per cabinet)"
set xtics rotate by -25 font "GothicBBB-Medium-RKSJ-H, 15"
set ytics font "GothicBBB-Medium-RKSJ-H, 15"
#set xtics font "GothicBBB-Medium-RKSJ-H, 20"
set xlabel "Traffic Pattern" font "GothicBBB-Medium-RKSJ-H, 18"
set ylabel "Slot Utilization Efficiency" font ",18"

plot  'C:\Users\smallcat\Google ドライブ\博后\NII\traffic-pattern\gnuplot\slot-optimization-torus-4d-4096.txt' using ($2):xtic(1) title "baseline", '' using ($3) title "src\\_greedy", '' using ($4) title "src\\_polling", '' using ($5) title "hcLtoS\\_greedy", '' using ($6) title "hcLtoS\\_polling", '' using ($7) title "hcStoL\\_greedy", '' using ($8) title "hcStoL\\_polling"

#set terminal png         # gnuplot recommends setting terminal before output
#set output 'C:\Users\smallcat\Google ドライブ\博后\NII\traffic-pattern\gnuplot\slot-optimization-torus-4d-4096.png'  

set term post eps color "GothicBBB-Medium-RKSJ-H, 20"      
set output 'C:\Users\smallcat\Google ドライブ\博后\NII\traffic-pattern\gnuplot\slot-optimization-torus-4d-4096.eps'  
replot
