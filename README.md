# circuit-switch-table
This repo contains the work on circuit-switched network, including estimating # of slots, generating routing table for each switch.
## Source Files
### circuit-switch-table.cc
* This file estimates # of slots and generates routing table for each switch.
* It supports 
    
     mesh (-T 0 -D [1-4])
     
     torus (-T 1 -D [1-4])
     
     fat-tree (-T 2)
     
     fully-connected (-T 3) 

   as the host interconnection network (-a).
* It defaultly does NOT support the update of slot # during slot ID allocation (add -u to activate the update).
* The generated routing tables for corresponding switches are saved in output/, which is refreshed (NOT appended!) after each execution.
* Switch Number (2-D): as follows.

<div align=center>
<img src="fig/2dmesh.png" width=512 height=512 />  
</div>

* Switch Port Number (2-D): East 0, West 1, South 2, North 3.

<div align=center>
<img src="fig/sw-port-2d-rev.png" width=512 height=512 />
</div>

* Compilation:
> g++ -c circuit-switch-table.cc  
> g++ circuit-switch-table.o -o circuit-switch-table.out
* Usage: 
> // for test in a 16-switch 2-D mesh (see traffic pattern details in test.txt)  
> cat test.txt | ./circuit-switch-table.out -a 4 -T 0 -D 2

<div align=center>
<img src="fig/test-run-0.png" width=512 height=256 />
</div>

* Routing table generated in output/ (file name starting with sw):

<div align=center>
<img src="fig/test-run-1.png" width=512 height=256 />
</div>

> // 64-switch 2-D torus (see traffic pattern details in traffic-pattern-generator.cc below)  
> cat ./traffic-pattern-generator.out -t 0 | ./circuit-switch-table.out -a 8 -T 1 -D 2

* Switch Number (3-D): as follows.

<div align=center>
<img src="fig/3dmesh.png" width=512 height=512 />
</div>

* Switch Port Number (3-D): East 0, West 1, South 2, North 3, back 4, front 5.

<div align=center>
<img src="fig/sw-port-3d-rev.png" width=512 height=512 />
</div>

* Switch Port Number (fat-tree): 

<div align=center>
<img src="fig/sw-port-tree.png" width=512 height=256 />
</div>

* Switch Port Number (fully-connected): 

<div align=center>
<img src="fig/sw-port-fully.png" width=512 height=512 />
</div>

### test.txt
* This test file shows a simple src-dst traffic for 16-node 2-D mesh.
### traffic-pattern-generator.cc
* This file helps to generate various traditional traffic patterns, including (-t) 0 uniform, 1 matrix, 2 reversal, 3 hotspot, 4 neighbor, 5 shuffle, 6 butterfly, 7 complement, 8 tornado, 9 all-to-all.
* Compilation:
> g++ -c traffic-pattern-generator.cc  
> g++ traffic-pattern-generator.o -o traffic-pattern-generator.out
* Usage:
> // 16-node uniform (-t 0 uniform, 1 matrix, 2 reversal, 3 hotspot, 4 neighbor, 5 shuffle, 6 butterfly, 7 complement, 8 tornado, 9 all-to-all)  
> ./traffic-pattern-generator.out -t 0 -n 4  
<div align=center>
<img src="fig/test-run-2.png" width=512 height=128 />
</div>

### traffic-pattern-generator.h
* This file is the head file for traffic-pattern-generator.cc.
