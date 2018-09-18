//
// mk_localIDnum-htree.cc
//
// MPI $B%W%m%0%i%`$N%H%l!<%9$+$i!"$=$NDL?.$KI,MW$J%m!<%+%k%"%I%l%9%5%$%:$r7W;;$9$k!#(B
// $B%m!<%+%k%"%I%l%9$N$o$j$"$F%"%k%4%j%:%`Kh$K7W;;2DG=$G$"$k!#(B
// ($B%D%j!<%H%]%m%8(B( H-Tree) )
//
// MPE profile library $B$rMQ$$$F@8@.$5$l$?(B alog $B7A<0$N%U%!%$%k(B hoge.alog $B$rF~NO$H$9$k!#(B
// Usage: cat hoge.alog | alog2ins_sim.sh | ./mk_localIDnum
//
// Sun Nov 27 15:26:42 JST 2003 koibuchi@am.ics.keio.ac.jp
// 

#include <unistd.h> // getopt 
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>
#include <math.h>

using namespace std;


#define HOST_NUM 4 // the number of hosts per switch //4->16,64(koi) two-layer, three-layer switches
//huyao 170108 16->4096 three-layer switches

#define PORT 5 // the number of ports per switch //5->16,64(koi) two-layer, three-layer switches
//huyao 170108 17->4096 three-layer switches


//
// $B7PO)$NEPO?(B
//
struct Pair {
   int pair_id; //huyao170325
   // $B7PM3%A%c%M%k72(B
   vector<int> channels;
   // $B=PH/CO$HL\E*CO(B
   int src; int dst;
   // $B7PO)$N(B ID
   int ID;
   // ID $B$,M-8z$+$I$&$+!)(B
   bool Valid;
   //huyao 170325
   int hops;
   // $B=i4|2=(B
   Pair(int s, int d): src(s),dst(d),ID(-1),Valid(false),hops(-1){}  //huyao 170325
};

//
// $BJ*M}%A%c%M%kKh$KDL2a$9$k7PO)$r3JG<(B
//
struct Cross_Paths {
   // $BEv%A%c%M%k$rDL2a$9$kA47PO)(B(Pair) $B$N(B index$B72(B
   vector<int> pair_index;
   // $BEv%A%c%M%k$K$9$G$K$o$j$"$F$i$l$F$$$k(B ID $B72(B
   list<int> assigned_list;
   // $BDL2a(B routing paths $B$9$Y$F$K(B ID $B$,$o$j$"$F$i$l$F$$$k$+$I$&$+!)(B
   bool Valid;
   // $B=i4|2=(B
   Cross_Paths():Valid(false){}

   bool operator == (Cross_Paths &a){
      return Valid == a.Valid && pair_index.size() == a.pair_index.size();
   }
   bool operator < (Cross_Paths &a){
	 if (Valid == a.Valid)
	    return pair_index.size() < a.pair_index.size();
	 else if (Valid == false)
	    return false; 
	 else
	    return true;
   }
};

//
// $B=PNO7O(B $B$*$h$S!"%(%i!<%A%'%C%/(B
//
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int node_num, \
int max_id, vector<Pair> pairs, int hops, int max_cp)
{
   cout << endl;
   // $B3F%A%c%M%kKh$KDL2a$9$k7PO)$N(B ID $B$rI=<(!"$*$h$S%(%i!<%A%'%C%/(B
   for (int i=0; i <= PORT*(node_num+node_num/HOST_NUM+node_num/(int)pow(HOST_NUM,2)); i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
      cout << "Channels (" << i << ")" << endl;
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst 
	      << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);
      }
      sort ( ID_array.begin(), ID_array.end() );
      unsigned int k = 0;
      bool error = false;
      while ( k+1 < ID_array.size() && !error){
	 if ( ID_array[k] == ID_array[k+1] && ID_array[k] != -1 )
	    error = true;
	 k++;
      }
      if (error){
	 cout << "ERROR : ID collision is occured!!." << endl;      
	 exit (1);
      }
   }
   cout << endl;
   // $B3F%A%c%M%k$N(B crossing paths $B$rI=<((B
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;
   cout << " UP0, DOWN0, DOWN1, DOWN2, DOWN3, ..." << endl;
   cout << " Node " << setw(2) << port/PORT << ":  "; 
   while ( elem != Crossing_Paths.end() ){
      cout << " " << (*elem).pair_index.size();

      elem ++; port++;
     if ( port%PORT == 0 && elem != Crossing_Paths.end() ){
        if ( node_num/(int)pow(HOST_NUM,2) <= 1 && port == PORT * (node_num+node_num/HOST_NUM+node_num/(int)pow(HOST_NUM,2)) ) break;
        cout << endl;		
        cout << " Node " << setw(2) << port/PORT << ":  ";	
     }
   }
   cout << endl;

   // $B=EMW$J=PNO(B
   cout << "(Maximum) Crossing Paths: " 
       << max_cp << endl;
   cout << "The number of paths on this application : " << ct << " (all-to-all cases: "
	<< node_num*(node_num-1) << ")" << endl;
   cout << "The average hops : " << setiosflags(ios::fixed | ios::showpoint) <<
      (float)hops/ct << endl;
   cout << "ID size(without ID modification)" << max_id << endl;

}

// 
// Detect crossing_paths of ecube routing on mesh.
//
// Usage: cat $Trace | ./mk_localIDnum-htree-rc1
//
// For H-Tree 16 nodes (5switches) and 64 nodes (21switches)
//
//  Notice that, 
// % less $Trace
// 0  2    <--- path from node 0 to node 2
// 0  4 
//
int main(int argc, char *argv[])
{
   // $B%N!<%I?t(B = array_size*array_size
   static int array_size = 8;
   // ID Allocation $B%]%j%7$NA*Br(B
   static int Allocation = 0;
   int c;
   
   while((c = getopt(argc, argv, "a:A:v:T:")) != -1) {
      switch (c) {
      case 'a':
	 array_size = atoi(optarg);
	 break;
	  case 'A':
	 Allocation = atoi(optarg);	
	 break;
      default:
	 //usage(argv[0]);
	 cout << " This option is not supported. " << endl;
	 return EXIT_FAILURE;
      }
   }

   cout << "ID Allocation Policy is 0(low port first) / 1(Crossing Paths based method): " 
	<< Allocation << endl;
   
   // $B7PO)$N=PH/CO$HL\E*CO(B	
   int src = -1, dst = -1, current = -1;	
   static int node_num = array_size*array_size;
   // Crossing Paths $B$N3JG<(B (Host <-> Switch$B4V$r4^$`(B)
   vector<Cross_Paths> Crossing_Paths( PORT * (node_num+node_num/HOST_NUM+node_num/(int)pow(HOST_NUM,2)+1));
   
   // port num * switch num
   // $B7PO)?t$r3JG<(B
   int ct = 0;
   // $BA47PO)$NAm%[%C%W?t(B
   int hops = 0; 

   int before_hops = 0;

   // $B3F7PO)(B
   vector<Pair> pairs;
   
   // ########################################## //
   // ##############   PHASE 1   ############### //
   // ##        $B3F7PO)$NEPO?!$%+%&%s%H(B        ## //
   // ########################################## //
   
   while ( cin >> src){	
      cin	>> dst;
      current = src;

      //#######################//
      // switch port:0 UP, 1 DOWN0(or localhost), 2 DOWN1, 3 DOWN2, 4 DOWN3 //
      //#######################//
      
      // $B%A%c%M%k$O!"%A%c%M%k$N=PH/CO(B switch ID $B$H(B $B=PH/CO(B port $B$G<1JL(B
      Pair tmp_pair(src,dst);  
      pairs.push_back(tmp_pair);
      Crossing_Paths[current*PORT].pair_index.push_back(ct);
      pairs[ct].channels.push_back(current*PORT);
      //hops++;

      pairs[ct].pair_id = ct; //huyao170325 
      
      current = node_num + current/HOST_NUM;
      while ( current != dst ){ 
	 int t;
	 // root switch
	 if ( current == node_num + node_num/HOST_NUM 
		+ node_num/pow(HOST_NUM,2)){
	    t = current * PORT + dst/(int)pow(HOST_NUM,2)+1;
	    current = current - HOST_NUM + dst/(int)pow(HOST_NUM,2);
	 // middle layer switch
	 } else if ( current >= node_num + node_num/HOST_NUM){
	    if ( current-node_num-node_num/HOST_NUM != dst/(int)pow(HOST_NUM,2)){
	       t = current * PORT + 0;
	       current = node_num + node_num/HOST_NUM + node_num/(int)pow(HOST_NUM,2);
	    } else {
	       t = current * PORT + (dst/HOST_NUM)%HOST_NUM + 1;
	       current = node_num + dst/HOST_NUM;
	    }
	 // low layer switch
	 } else if ( current >= node_num){
	    if ( current-node_num != dst/HOST_NUM){
	       t = current * PORT + 0;
	       current = node_num + current/HOST_NUM;
	    } else {
	       t = current * PORT + dst%HOST_NUM+1;
	       current = dst;
	    }
	 // host->switch
	 } else {
	    // $BITMW(B
	 }
	 Crossing_Paths[t].pair_index.push_back(ct);
	 pairs[ct].channels.push_back(t);
	 hops++;
      }

      //huyao 170325
      pairs[ct].hops = hops-before_hops;  
      before_hops = hops;

      if ( current != dst ){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }
      ct++;	
   }	

   // ########################################## //
   // ##############   PHASE 2   ############### //
   // ##  $B3F7PO)$X$N(B local ID $B$N3d$jEv$F(B      ## //
   // ########################################## //

   // ID$B99?77?$GI,MW$H$J$k%5%$%:!#$9$J$o$A!"(Bcrossing paths$B!#(B
   int max_id = max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   int max_cp = max_id;
   for (int j = 0; j <= PORT * (node_num+node_num/HOST_NUM+node_num/(int)pow(HOST_NUM,2)); 
				   j++ ){ 
      // $B7PO)$rA*$V$?$a$N%A%c%M%k$rA*Br!#(B
      vector<Cross_Paths>::iterator elem = Crossing_Paths.begin()+j;
	switch (Allocation){
	// low port first
   	case 0:   
	   elem = Crossing_Paths.begin()+j;
	   break;
	// Crossing paths based method
	case 1:
	   elem = max_element(Crossing_Paths.begin(),Crossing_Paths.end());
	   break;
	default:
	   cerr << "Please select a legal allocation optiohn(-A 0 or 1)" << endl;
	   exit (1);
	   break;
	}

	// $B%A%c%M%k>e$G$O(B($BEPO?(B)$B7PO)=g$K(B local ID$BHV9f$r7h$a$F$$$/!#(B
	unsigned int path_ct = 0; 
	while ( path_ct < elem->pair_index.size() ){
	   // ID $B$r$o$j$"$F$k7PO)(B(pair)$B$NHV9f(B
	   int t = elem->pair_index[path_ct];      
	   // ID $B$,$9$G$K$o$j$"$F$i$l$F$$$k>l9g$O%9%-%C%W$9$k!#(B
	   if ( pairs[t].Valid == true ) {path_ct++; continue;}
	   // 0$B$+$i=g$K$o$j$"$F$k!#(B
	   int id_tmp = 0;
	   bool NG_ID = false;
	   
      NEXT_ID:
	 // $B$9$G$K;HMQ$5$l$F$$$k(B ID $B$+$I$&$+!)(B
	 unsigned int s_ct = 0; // $B7PM3%A%c%M%k$r;XDj(B
	 while ( s_ct < pairs[t].channels.size() && !NG_ID ){
	    int i = pairs[t].channels[s_ct];
	    list<int>::iterator find_ptr;
	    find_ptr = find ( Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
	    if (find_ptr != Crossing_Paths[i].assigned_list.end()) NG_ID = true;
	    s_ct++;
	 }
	 
	 if (NG_ID){
	    id_tmp++; NG_ID = false; goto NEXT_ID;
	 }
	 
	 pairs[t].ID = id_tmp;
	 unsigned int a_ct = 0;
	 while ( a_ct < pairs[t].channels.size() ){
	    int j = pairs[t].channels[a_ct];
	 	Crossing_Paths[j].assigned_list.push_back(id_tmp);
		a_ct++; 
	 }

	 pairs[t].Valid = true;	    
	 if (max_id < id_tmp) max_id = id_tmp;
	 
	 path_ct++;
      }
      elem->Valid = true;
   }
   
   show_paths(Crossing_Paths, ct, node_num, max_id, pairs, hops, max_cp);   
   return 0;
}


