//
// mk_localIDnum-rc3-tablegenerator.cc
//
// port output for each switch (2d mesh/torus)
//
// Usage: cat test.txt | ./mk_localIDnum-rc3-tablegenerator.out -a 4 -T 0 //16-node mesh
// (test.txt)
// 0  2    <--- path from node 0 to node 2
// 0  4
//
// Fri Feb 17 JST 2017
//

#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>

using namespace std;

#define degree 4 // mesh/torus

//
// path registration
//
struct Pair {
   vector<int> channels;
   int src; int dst;
   int h_src; int h_dst;
   // path ID
   int ID;
   // ID is valid
   bool Valid;
   // the distance between src and dst
   int hops;
   // initialize
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
// passing flows for one channel
//
struct Cross_Paths {
   vector<int> pair_index;
   vector<int> assigned_list;
   vector<int> assigned_dst_list;
   bool Valid;
   // initialize
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
// output
//
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
vector<Pair> pairs, int hops, int Vch, int Host_Num, bool path_based) //huyao 170217
{
   cout << endl;
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

   // show crossing paths for each channel
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;

   cout << " (Node : port0(+x) port1(-x) port2(+y) port3(-y)) outputPort_dataflows" << endl;

   cout << " Node " << setw(2) << port/((degree+1+2*Host_Num)*Vch) << ":  ";    
   while ( elem != Crossing_Paths.end() ){
      if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1) cout << " " << port%(degree+1+2*Host_Num)-1 << "_" << (*elem).pair_index.size();
      elem ++; port++;
      if ( port%((degree+1+2*Host_Num)*Vch) == 0){
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

   // important
   //cout << "(Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
//   cout << "(Maximum) Crossing Paths: " << max_cp << endl;
   //cout << "The number of paths on this application : " << ct << " (all-to-all cases: " 	\
	<< (switch_num*Host_Num)*(switch_num*Host_Num-1) << ")" << endl;
   //cout << "The average hops : " << setiosflags(ios::fixed | ios::showpoint) <<
   //		   (float)hops/ct+1 << endl;

   cout << endl;
}


int main(int argc, char *argv[])
{
   // side length of mesh or torus
   static int array_size = 4;
   // ID Allocation policy: 0 low port first, 1 Crossing Paths based method 
   static int Allocation = 1;
	// hosts per router
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

   //cout << "ID Allocation Policy is 0(low port first) / 1(Crossing Paths based method): " << Allocation << endl;
   //cout << "Address method is 0(destination_based) / 1(path_based): " 
   //	<<  path_based << endl;
   	
   int src = -1, dst = -1, h_src = -1, h_dst = -1;
   static int switch_num = array_size*array_size;
   // Crossing Paths  
	// include (Host <-> Switch)
   vector<Cross_Paths> Crossing_Paths((degree+1+2*Host_Num)*switch_num*Vch);

   // count
   int ct = 0;
   int hops = 0; 

   vector<Pair> pairs;
   
   //#######################//
   // routing //
   //#######################//

   while ( cin >> h_src){	
      cin >> h_dst;
	  src = h_src/Host_Num;
	  dst = h_dst/Host_Num;

      bool wrap_around_x = false;
      bool wrap_around_y = false;

      //#######################//
      // switch port:0 +x, 1 -x, 2 +y, 3 -y, 
	  // 4 localhost->switch 
	  // 5 switch-> localhost
      //#######################//

      Pair tmp_pair(src,dst,h_src,h_dst);  
      pairs.push_back(tmp_pair);
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

      // X 
      if (delta_x > 0){
	 while ( delta_x != 0 ){ // +x
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
	 while ( delta_x != 0 ){ // -x
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
      
      if (delta_x != 0){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

      // Y 
      if (delta_y > 0){
	 while ( delta_y != 0 ){ // +y
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
	 while ( delta_y != 0 ){ // -y
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

      if ( delta_x != 0 || delta_y != 0 ){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

		// switch->host 
      t = (src%2==1 && Topology==1) ? 
		Vch*dst*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num
		: Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
      Crossing_Paths[t].pair_index.push_back(ct);
      pairs[ct].channels.push_back(t);      
      ct++;	
   }

   cout << endl;
   if (Topology==0) cout << "###" << array_size*array_size << "-node mesh" << "###";
   if (Topology==1) cout << "###" << array_size*array_size << "-node torus" << "###";	
   
   show_paths(Crossing_Paths, ct, switch_num, pairs, hops, Vch, Host_Num, path_based); //huyao 170217
   
   return 0;
}


