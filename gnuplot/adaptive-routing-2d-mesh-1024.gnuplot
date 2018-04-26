set yrange [0:256]
set key top right font "GothicBBB-Medium-RKSJ-H, 20"
#set key c tm horizontal box font "GothicBBB-Medium-RKSJ-H, 13"
#set grid
set style data histograms
#set style fill solid 1.00 border -1
set style fill pattern 3 border -1
#set title "Average Queuing Time (16 nodes per cabinet)"
set xtics rotate by -20 font "GothicBBB-Medium-RKSJ-H, 20"
set xtics font "GothicBBB-Medium-RKSJ-H, 20"
set xlabel "Traffic Pattern" font "GothicBBB-Medium-RKSJ-H, 20"
set ylabel "Minimal Necessary \# of Slots" font ",20"

plot  '/Users/smallcat/gnuplot/adaptive-routing-2d-mesh-1024.txt' using ($2):xtic(1) title "dimension order routing", '' using ($3) title "dimension adaptive routing", '' using ($4) title "minimal oblivious routing"

#set terminal png         # gnuplot recommends setting terminal before output
#set output '/Users/smallcat/gnuplot/adaptive-routing-2d-mesh-1024.png'  
 
set terminal postscript eps 20
#set term post eps color     
set output '/Users/smallcat/gnuplot/adaptive-routing-2d-mesh-1024.eps'  
replot
