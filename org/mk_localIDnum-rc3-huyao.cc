//
// mk_localIDnum-rc1.cc
//
// MPI $B%W%m%0%i%`$N%H%l!<%9$+$i!"$=$NDL?.$KI,MW$J%m!<%+%k%"%I%l%9%5%$%:$r7W;;$9$k!#(B
// $B%m!<%+%k%"%I%l%9$N$o$j$"$F%"%k%4%j%:%`Kh$K7W;;2DG=$G$"$k!#(B
// (2$B<!85(B Mesh + e-cube $B%k!<%F%#%s%0$r;HMQ$9$k!#(B)
//
// MPE profile library $B$rMQ$$$F@8@.$5$l$?(B alog $B7A<0$N%U%!%$%k(B hoge.alog $B$rF~NO$H$9$k!#(B
// Usage: cat hoge.alog | alog2ins_sim.sh | ./mk_localIDnum
//
// Sun Nov 16 15:26:42 JST 2003 koibuchi@am.ics.keio.ac.jp
// 2004$BG/(B 5$B7n(B23$BF|(B $BF|MKF|(B 18$B;~(B21$BJ,(B08$BIC(B JST koibuchi@am.ics.keio.ac.jp
// Thu Dec 16 22:37:48 JST 2004 koibuchi@am.ics.keio.ac.jp
//

#include <unistd.h> // getopt $B$KI,MW(B
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>

using namespace std;

#define degree 4 // 2$B<!85%a%C%7%e(B/$B%H!<%i%9(B

//
// $B7PO)$NEPO?(B
//
struct Pair {
   // $B7PM3%A%c%M%k72(B
   vector<int> channels;
   // $B=PH/CO$HL\E*CO(B
   int src; int dst;
   int h_src; int h_dst;
   // $B7PO)$N(B ID
   int ID;
   // ID $B$,M-8z$+$I$&$+!)(B
   bool Valid;
   // the distance between src and dst
   int hops;
   // $B=i4|2=(B
   Pair(int s, int d, int h_s, int h_d): 
	src(s),dst(d),h_src(h_s),h_dst(h_d),ID(-1),Valid(false),hops(-1){}

   bool operator < (Pair &a){
	 if (Valid == a.Valid)
	    return hops < a.hops;
	 else if (Valid == false)
	    return false; 
	 else
	    return true;
   }
};

//
// $BJ*M}%A%c%M%kKh$KDL2a$9$k7PO)$r3JG<(B
//
struct Cross_Paths {
   // $BEv%A%c%M%k$rDL2a$9$kA47PO)(B(Pair) $B$N(B index$B72(B
   vector<int> pair_index;
   // $BEv%A%c%M%k$K$9$G$K$o$j$"$F$i$l$F$$$k(B ID $B72(B
   vector<int> assigned_list;
   // $B$=$N(B ID $B$NL\E*CO(B (destination-based ID label $B$K;HMQ(B)
   vector<int> assigned_dst_list;
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
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
int max_id, vector<Pair> pairs, int hops, int Vch, int Host_Num, int max_cp,
bool path_based)
{
   cout << endl;
   // $B3F%A%c%M%kKh$KDL2a$9$k7PO)$N(B ID $B$rI=<(!"$*$h$S%(%i!<%A%'%C%/(B
   for (int i=0; i < Vch*(degree+1+2*Host_Num)*switch_num; i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << "Channels (" << i << ")" << endl;
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);
      }
      sort ( ID_array.begin(), ID_array.end() );
      unsigned int k = 0;
      bool error = false;
      while ( k+1 < ID_array.size() && !error){
	 if ( ID_array[k] == ID_array[k+1] && ID_array[k] != -1 && path_based)
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
   //int slots = 0; //huyao161226
   //int pointer = 0; //huyao161226
   int total_slots = 0; //huyao170307
   if (Vch == 0)
		cout << " (east, west, south, north, from host0,... to host0,...)";
   else
   		cout << " (east, west, south, north, from host0,... east(vch2), south(vch2), west(vch2), north(vch2), from host(vch2)0,.. to host(vch2)0,.." 
		<< endl;

   	cout << " Node " << setw(2) << port/((degree+1+2*Host_Num)*Vch) << ":  ";    while ( elem != Crossing_Paths.end() ){
      if (port%(degree+1+2*Host_Num)!=0) cout << " " << (*elem).pair_index.size();
      //if (pointer<4 && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();} //huyao161226

      //huyao 170307
      if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

      elem ++; port++;
      if ( port%((degree+1+2*Host_Num)*Vch) == 0){
         //pointer = 0; //huyao161226
	 cout << endl;		
	 if ( port != Vch*(degree+1+2*Host_Num)*switch_num)
	    cout << " Node " << setw(2) << port/((degree+1+2*Host_Num)*Vch) << ":  ";	
      }
   }
	cout << endl; 
	// setting for comparing (maximum) Cross_Path 
    vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
	while ( pt != Crossing_Paths.end() ){
    	(*pt).Valid = true;
        ++pt;
	}

   // $B=EMW$J=PNO(B
   cout << "(Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
//   cout << "(Maximum) Crossing Paths: " << max_cp << endl;
   cout << "The number of paths on this application : " << ct << " (all-to-all cases: " 	\
	<< (switch_num*Host_Num)*(switch_num*Host_Num-1) << ")" << endl;
   cout << "The average hops : " << setiosflags(ios::fixed | ios::showpoint) <<
		   (float)hops/ct+1 << endl;
   cout << "ID size(without ID modification)" << max_id << endl;
   //cout << "(Maximum) number of slots: " << slots;
   
   //huyao 170307
   cout << "total_slots: " << total_slots << endl;
   cout << "avg_slots: " << (float)total_slots/(switch_num*degree) << endl;
   cout << "unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots << endl;
   cout << "unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots)/(switch_num*degree) << endl;

   cout << endl;
}

// 
// Detect crossing_paths of ecube routing on mesh.
//
// Usage: cat $Trace | ./mk_localIDnum
//
//
//  Notice that, 
// % less $Trace
// 0  2    <--- path from node 0 to node 2
// 0  4 
//
int main(int argc, char *argv[])
{
   // $B%a%C%7%e(B/$B%H!<%i%9$NJU$ND9$5(B
   static int array_size = 4;
   // ID Allocation $B%]%j%7$NA*Br(B
   static int Allocation = 1;
	// $B%k!<%?$"$?$j$N%[%9%H?t(B
	static int Host_Num = 1;
   // The number of VCHs
   static int Vch = 1;
   static int Topology = 0; // Mesh:0, Torus:1
   int c;
   static bool path_based = true;
   
   while((c = getopt(argc, argv, "a:A:Z:T:d")) != -1) {
      switch (c) {
      case 'a':
	 array_size = atoi(optarg);
	 break;
	  case 'A':
	 Allocation = atoi(optarg);	
	 break;
	/*  case 'v':
	 Vch = atoi(optarg);	
	 break; */
	  case 'T':
	 Topology = atoi(optarg);	
	 break;
	case 'Z':
	Host_Num = atoi(optarg);	
	break;
      case 'd':
	 path_based = false;
	 break;
      default:
	 //usage(argv[0]);
	 cout << " This option is not supported. " << endl;
	 return EXIT_FAILURE;
      }
   }

   if (Topology==1) Vch=2; //Torus
   else if (Topology==0) Vch=1;//Mesh
   else{ 
      cerr << " The combination of Topology and Vchs is wrong!!" << endl;
      exit (1);
   }

   cout << "ID Allocation Policy is 0(low port first) / 1(Crossing Paths based method): " << Allocation << endl;
   cout << "Address method is 0(destination_based) / 1(path_based): " 
	<<  path_based << endl;
   
   // $B7PO)$N=PH/CO$HL\E*CO(B	
   int src = -1, dst = -1, h_src = -1, h_dst = -1;
	// $BAm%9%$%C%A?t(B($BAm%[%9%H?t$G$O$J$$(B)
   static int switch_num = array_size*array_size;
   // Crossing Paths $B$N3JG<(B 
	// (Host <-> Switch$B4V$r4^$`(B,$B%k!<%?$"$?$j$N%[%9%H?t$NJQ99$K$h$j(B+2)
   vector<Cross_Paths> Crossing_Paths((degree+1+2*Host_Num)*switch_num*Vch);
   // $B7PO)?t$r3JG<(B
   int ct = 0;
   // $BAm7PO)$NAm%[%C%W?t(B
   int hops = 0; 

   // $B7PO)(B
   vector<Pair> pairs;
   
   // ########################################## //
   // ##############   PHASE 1   ############### //
   // ##        $B3F7PO)$NEPO?!$%+%&%s%H(B        ## //
   // ########################################## //

   while ( cin >> h_src){	
      cin >> h_dst;
	  src = h_src/Host_Num;
	  dst = h_dst/Host_Num;

      bool wrap_around_x = false;
      bool wrap_around_y = false;

      //#######################//
      // (switch port:0 localhost->switch, 1 +x, 2 -x, 3 +y, 4 -y, 5 switch-> localhost)
      // switch port:1 +x$BJ}8~(B, 2 -x, 3 +y, 4 -y, 
	  // 5$B!A(B(4+Host_Num) localhost->switch 
	  // (5+Host_Num)$B!A(B(4+Host_Num*2) switch-> localhost
      //#######################//

      // $B%A%c%M%k$O!"%A%c%M%k$N=PH/CO(B switch ID $B$H(B $B=PH/CO(B port $B$G<1JL(B
      Pair tmp_pair(src,dst,h_src,h_dst);  
      pairs.push_back(tmp_pair);
	  // $BL\E*CO$NHV9fKh$K!"%[%9%H%A%c%M%k$K$*$1$k2>A[%A%c%M%kHV9f$rJ,;6(B
	  // $BJ,;6$7$J$$>l9g(B (consume channel$B$N>l9g$bF1MM$KJQ99$9$k!#(B)
	  // int t = Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
	  int t = (dst%2==1 && Topology==1) ? 
		Vch*src*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+h_src%Host_Num
		  : Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
      Crossing_Paths[t].pair_index.push_back(ct);
      pairs[ct].channels.push_back(t);      
      int delta_x, delta_y, current;
      switch (Topology){
      case 0: //mesh
	 delta_x = dst%array_size - src%array_size;
	 delta_y = dst/array_size - src/array_size;
	 current = src; 
	 break;

      case 1: // torus
	 delta_x = dst%array_size - src%array_size;
	 if ( delta_x < 0 && abs(delta_x) > array_size/2 ) {
	    delta_x = -( delta_x + array_size/2);
		wrap_around_x = true;		
	 } else if ( delta_x > 0 && abs(delta_x) > array_size/2 ) {
	    delta_x = -( delta_x - array_size/2);
		wrap_around_x = true;		
	 }
	 delta_y = dst/array_size - src/array_size;
	 if ( delta_y < 0 && abs(delta_y) > array_size/2 ) {
	    delta_y = -( delta_y + array_size/2);
		wrap_around_y = true;		
	 } else if ( delta_y > 0 && abs(delta_y) > array_size/2 ) {
	    delta_y = -( delta_y - array_size/2);
		wrap_around_y = true;		
	 }
	 current = src; 
	 break;
      default:
	 cerr << "Please select -t0, or -t1 option" << endl;
	 exit(1);
	 break;
      }

	 pairs[ct].hops = abs(delta_x) + abs(delta_y); 

      // X $BJ}8~$N%k!<%F%#%s%0(B
      if (delta_x > 0){
	 while ( delta_x != 0 ){ // +x$BJ}8~(B
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 1;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( current % array_size == array_size-1) {
	       wrap_around_x = false;
	       current = current - (array_size -1);
	    } else current++; 
	    delta_x--;
	    hops++;
	 }
      } else if (delta_x < 0){
	 while ( delta_x != 0 ){ // -x$BJ}8~(B
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 2;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( current % array_size == 0 ) {
	       wrap_around_x = false;
	       current = current + (array_size - 1 );
	    } else current--;
	    hops++;
	    delta_x++;
	 }
      }
      
      // X $BJ}8~$N%k!<%F%#%s%0$,=*N;$7$F$$$k$3$H$r3NG'(B
      if (delta_x != 0){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

      // Y $BJ}8~$N%k!<%F%#%s%0(B	
      if (delta_y > 0){
	 while ( delta_y != 0 ){ // +y$BJ}8~(B
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 3;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( current >= array_size*(array_size-1) ){
	       wrap_around_y = false;
	       current = current - array_size*(array_size -1);
	    } else current += array_size;
	    hops++;
	    delta_y--;
	 }
      } else if (delta_y < 0){
	 while ( delta_y != 0 ){ // -y$BJ}8~(B
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 4;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( current < array_size ) {
	       wrap_around_y = false;
	       current = current + array_size*(array_size -1);
	    } else current -= array_size;
	    hops++;
	    delta_y++;
	 }
      }
      
      // X,Y $BJ}8~$N%k!<%F%#%s%0$,=*N;$7$F$$$k$3$H$r3NG'(B
      if ( delta_x != 0 || delta_y != 0 ){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

		// switch->host $B$N%A%c%M%k$K$*$1$k2>A[%A%c%M%k4V$NJ,;6(B
		// $BJ,;6$7$J$$>l9g$O(B
	    // t = Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
      t = (src%2==1 && Topology==1) ? 
		Vch*dst*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num
		: Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
      Crossing_Paths[t].pair_index.push_back(ct);
      pairs[ct].channels.push_back(t);      
      ct++;	
   }	


   // ########################################## //
   // ##############   PHASE 2   ############### //
   // ##  $B3F7PO)$X$N(B local ID $B$N3d$jEv$F(B      ## //
   // ########################################## //

   // ID$B99?77?$GI,MW$H$J$k%5%$%:!#$9$J$o$A!"(Bcrossing paths$B!#(B
   int max_cp = max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   int max_id = 0;
	
	int max_cp_dst = 0; // $BI,MW$H$J$k(B dst-based renable label $B?t(B	

	// dst-based renewable label $B$r7W;;(B
	int max_cp_dst_t = 0;
   for (int j = 0; j < Vch * (degree+1+2*Host_Num) * switch_num; j++ ){ 
	    vector<Cross_Paths>::iterator elem = Crossing_Paths.begin()+j;
		unsigned int p_ct = 0;
		while ( p_ct < elem->pair_index.size() ){
			int u = elem->pair_index[p_ct]; 
			bool is_duplicate = false; //$BF1$8L\E*CO$r;}$D7PO)$,$"$k$+!)(B
			// $BL\E*CO$,0l=o$N7PO)BP$,$=$N%A%c%M%k$rDL2a$9$k$+$I$&$+!)(B
			unsigned int p_ct_t = 0;
			while ( p_ct_t < p_ct ){
				int v = elem->pair_index[p_ct_t]; 
				if (pairs[u].h_dst == pairs[v].h_dst){
					is_duplicate = true;
				}
				p_ct_t++;
			}
			if (!is_duplicate) max_cp_dst_t++;
			p_ct++;
		}
		if (max_cp_dst_t > max_cp_dst) max_cp_dst = max_cp_dst_t;
		max_cp_dst_t = 0;
   }
	cout << " dst_based renewable label = " << max_cp_dst << endl;	
	cout << " path_based renewable label = " << max_cp << endl;	


   for (int j = 0; j < Vch * (degree+1+2*Host_Num) * switch_num; j++ ){ 
      // $B7PO)$rA*$V$?$a$N%A%c%M%k$rA*Br!#(B
      vector<Cross_Paths>::iterator elem = Crossing_Paths.begin()+j;
	switch (Allocation){
	// low port first
   	case 0:   
	   if ( j%(degree+1+2*Host_Num)== 0)	 
	      elem = Crossing_Paths.begin()+j;
	   break;
	   // Crossing paths based method
	case 1:
	   elem = max_element(Crossing_Paths.begin(),Crossing_Paths.end());
	   break;
	default:
	   cerr << "Please select a legal allocation option (-A 0 or 1)" << endl;
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
	    vector<int>::iterator find_ptr;
	    find_ptr = find ( Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
	    if ( path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) NG_ID = true;
	    if (!path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) {
	       int tmp = 0;
			// index $BC5$7(B
	       while (*find_ptr != Crossing_Paths[i].assigned_list[tmp]) {tmp++;}
	       if (pairs[t].h_dst != Crossing_Paths[i].assigned_dst_list[tmp])
				NG_ID = true; 
	    }
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
		int t = elem->pair_index[path_ct];      	
		Crossing_Paths[j].assigned_dst_list.push_back(pairs[t].h_dst);
		a_ct++; 
	 }

	 pairs[t].Valid = true;	    
	//ID$B$O(B0$B$+$i3d$jEv$F$F$$$k$N$G8D?t$O(B Max ID +1 $B8DI,MW$H$$$&$3$H$K$J$k!%(B
	 if (max_id <= id_tmp) max_id = id_tmp + 1; 

	  path_ct++;
      }
      elem->Valid = true;
   }
   
   show_paths(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Vch, Host_Num, max_cp, path_based);   
   
   return 0;
}


