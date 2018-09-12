# circuit-switch-table
This repo contains the work on circuit-switched network, including estimating # of slots, generating routing table for each switch.
## Source Files
### circuit-switch-table-2d.cc
* This file estimates # of slots and generates routing table for each switch.
* It supports 2-D mesh/torus as the host interconnection network (-a).
* It defaultly supports the update of slot # during slot ID allocation (add -d to deactivate the update).
* The generated routing tables (output-port output-slot input-port input-slot) for corresponding switches are saved in output/, which is refreshed (NOT appended!) after each execution.
* Switch Number: as follows.

<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/2dmesh.png?token=ADxdf1eiO4DEO7oO5y0mAHlzoTkJzifxks5bmkqmwA%3D%3D" width=256 height=256 />  
</center>

* Switch Port Number: East 0, West 1, South 2, North 3.

<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/sw.png?token=ADxdf3SPO-P_p9ufh3P7CwIziQNqFATQks5bmgCTwA%3D%3D" width=256 height=256 />
</center>

* Compilation:
> g++ -c circuit-switch-table-2d.cc  
> g++ circuit-switch-table-2d.o -o circuit-switch-table-2d.out
* Usage: 
> // for test in a 16-switch mesh (see traffic pattern details in test.txt)  
> cat test.txt | ./circuit-switch-table-2d.out -a 4 -T 0 

<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/test-run-0.png?token=ADxdf1Rbs6ogE7ZW4qFXY4q5gIP_5O8_ks5bmhHewA%3D%3D" width=256 height=128 />
</center>

* Routing table generated in output/ (file name starting with sw):

<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/test-run-1.png?token=ADxdf8X-LTMfX5kY9AYisxAfUp78gDs6ks5bmhIJwA%3D%3D" width=256 height=128 />
</center>

> // 64-switch torus (see traffic pattern details in traffic-pattern-generator.cc below)  
> cat ./traffic-pattern-generator.out -t 0 | ./circuit-switch-table-2d.out -a 8 -T 1
### circuit-switch-table-3d.cc
* This file estimates # of slots and generates routing table for each switch.
* It supports 3-D mesh/torus as the host interconnection network (-a).
* The same as circuit-switch-table-2d.cc.
* Switch Number: as follows.

<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/3dmesh.png?token=ADxdf9tXgF-ghOZKPNd1aCjeYO0bp08bks5bmks-wA%3D%3D" width=256 height=256 />
</center>

* Switch Port Number: East 0, West 1, South 2, North 3, back 4, front 5.

<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/sw-3d.png?token=ADxdfy_Vk1FZ6x5fTIXnac_S17FiXe1xks5bmktrwA%3D%3D" width=256 height=256 />
</center>

* Compilation: the same as circuit-switch-table-2d.cc 
* Usage: the same as circuit-switch-table-2d.cc
### test.txt
* This test file shows a simple src-dst traffic for 16-node mesh.
### traffic-pattern-generator.cc
* This file helps to generate various traditional traffic patterns, including uniform, matrix, reversal, hotspot, neighbor, shuffle, butterfly, complement, tornado, all-to-all.
* Compilation:
> g++ -c traffic-pattern-generator.cc  
> g++ traffic-pattern-generator.o -o traffic-pattern-generator.out
* Usage:
> // 16-node uniform (-t 0 uniform, 1 matrix, 2 reversal, 3 hotspot, 4 neighbor, 5 shuffle, 6 butterfly, 7 complement, 8 tornado, 9 all-to-all)  
> ./traffic-pattern-generator.out -t 0 -n 4  
<center>
<img src="https://raw.githubusercontent.com/KoibuchiLab/circuit-switch-table/master/fig/test-run-2.png?token=ADxdf-zCr9D4ayVyEBRR-1eCxwrxOR5Dks5bmhIswA%3D%3D" width=256 height=64 />
</center>

### traffic-pattern-generator.h
* This file is the head file for traffic-pattern-generator.cc.