#set terminal png         # gnuplot recommends setting terminal before output
#set output 'C:\Users\smallcat\Google �h���C�u\���@\NII\traffic-pattern\gnuplot\latency-uniform.png'  

set term post eps color #"GothicBBB-Medium-RKSJ-H, 20"      
set output 'C:\Users\smallcat\Google �h���C�u\���@\NII\traffic-pattern\gnuplot\latency-uniform.eps'  
#replot

set size 1,1
set origin 0,0
set multiplot

set size 0.5,1
set origin 0.5,0
set yrange [0:20000]
set xlabel "Network Size (2^n)"
set ylabel "Practical Latency (ns)"
set key top left font ", 12"
#unset key
#set title "Average Turnaround Time"
plot  'C:\Users\smallcat\Google �h���C�u\���@\NII\traffic-pattern\gnuplot\latency-uniform.txt' using 2:xtic(1) title "2-D Mesh" with linespoints, '' using 3 title "2-D Torus" with linespoints, '' using 4 title "3-D Mesh" with linespoints, '' using 5 title "3-D Torus" with linespoints , '' using 6 title "4-D Mesh" with linespoints, '' using 7 title "4-D Torus" with linespoints, '' using 8 title "Fat-tree" with linespoints

set size 0.5,1
set origin 0,0
set yrange [0:20000]
set xlabel "Network Size (2^n)"
set ylabel "Theoretical Latency (ns)"
set key top left font ", 12"
#unset key
#set title "Average Turnaround Time"

plot  'C:\Users\smallcat\Google �h���C�u\���@\NII\traffic-pattern\gnuplot\minlatency-uniform.txt' using 2:xtic(1) title "2-D Mesh" with linespoints, '' using 3 title "2-D Torus" with linespoints, '' using 4 title "3-D Mesh" with linespoints, '' using 5 title "3-D Torus" with linespoints , '' using 6 title "4-D Mesh" with linespoints, '' using 7 title "4-D Torus" with linespoints, '' using 8 title "Fat-tree" with linespoints

unset multiplot
reset