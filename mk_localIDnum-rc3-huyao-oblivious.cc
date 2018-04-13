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
   int pair_id; //huyao170315
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

//huyao170315
bool HopLtoS(Pair a, Pair b) {return (a.hops > b.hops);}
bool HopStoL(Pair a, Pair b) {return (a.hops < b.hops);}

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
      cout << "Channels (" << i << ")" << endl;
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
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
   cout << "efficiency: " << (float)total_slots/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots << endl;
   cout << "unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots)/(switch_num*degree) << endl;
   cout << "inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   float baseline_efficiency = (float)total_slots/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float baseline_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float baseline_avg_slots = (float)total_slots/(switch_num*degree);
   float baseline_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots)/(switch_num*degree);

   cout << endl;

   //huyao 170313 optimization src_greedy
   //sort(pairs.begin(), pairs.end(), HopLtoS);
   //sort(pairs.begin(), pairs.end(), HopStoL);
   int total_increased_slots = 0;
   //cout << pairs.size() << endl;
   for (int p = 0; p < pairs.size(); p++){
	//cout << "c: " << pairs[p].channels.size() << endl;
        //cout << "h: " << pairs[p].hops << endl;
        bool moreslots = true;
        int increased_slots = 10000;
	for (int c = 1; c < pairs[p].channels.size()-1; c++) //0 src, pairs[p].channels.size()-1 des
	{	
		if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
			moreslots = false;
			increased_slots = 0;
			break;
		//cout << pairs[p].channels[c] << endl;
                }
                if (max_cp-Crossing_Paths[pairs[p].channels[c]].pair_index.size() < increased_slots) {
			increased_slots = max_cp-Crossing_Paths[pairs[p].channels[c]].pair_index.size();
		}
	}
	if (moreslots == true) {
		for (int c = 1; c < pairs[p].channels.size()-1; c++) {
                	for (int i=0; i<increased_slots; i++) 
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
			//pairs[p].channels.push_back(pairs[p].channels[c]); 
                        total_increased_slots += increased_slots;   
                }
        }
   }
   cout << "src_greedy total_increased_slots: " << total_increased_slots << endl;

   cout << endl;

   cout << "src_greedy src_greedy total_slots: " << total_slots + total_increased_slots << endl;
   cout << "src_greedy avg_slots: " << (float)(total_slots + total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_greedy increased_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_greedy efficiency: " << (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "src_greedy increased_efficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "src_greedy unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots - total_increased_slots << endl;
   cout << "src_greedy unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_greedy decreased_unused_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_greedy inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "src_greedy decreased_inefficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   int src_greedy_total_slots = total_slots + total_increased_slots;
   float src_greedy_efficiency = (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float src_greedy_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();  
   float src_greedy_avg_slots = (float)(total_slots + total_increased_slots)/(switch_num*degree);
   float src_greedy_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree); 


   //huyao 170319 optimization src_polling
   cout << endl;
   //huyao 170319 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size()-1; c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //cout << pairs.size() << endl;
   bool cont = true;
   while (cont){
           int full_pairs = 0;
	   for (int p = 0; p < pairs.size(); p++){
		//cout << "c: " << pairs[p].channels.size() << endl;
		//cout << "h: " << pairs[p].hops << endl;
		bool moreslots = true;
		for (int c = 1; c < pairs[p].channels.size()-1; c++) //0 src, pairs[p].channels.size()-1 des
		{	
			if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
				moreslots = false;
				full_pairs++;
				break;
			//cout << pairs[p].channels[c] << endl;
		        }
		}
		if (moreslots == true) {
			for (int c = 1; c < pairs[p].channels.size()-1; c++) {
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
				//pairs[p].channels.push_back(pairs[p].channels[c]); 
		                total_increased_slots++;   
		        }
		}
	   }
           if (full_pairs == pairs.size()){
		cont = false;          
           }
   }
   cout << "src_polling total_increased_slots: " << total_increased_slots << endl;

   cout << endl;

   cout << "src_polling total_slots: " << total_slots + total_increased_slots << endl;
   cout << "src_polling avg_slots: " << (float)(total_slots + total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_polling increased_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_polling efficiency: " << (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "src_polling increased_efficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "src_polling unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots - total_increased_slots << endl;
   cout << "src_polling unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_polling decreased_unused_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "src_polling inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "src_polling decreased_inefficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   int src_polling_total_slots = total_slots + total_increased_slots;
   float src_polling_efficiency = (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float src_polling_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size(); 
   float src_polling_avg_slots = (float)(total_slots + total_increased_slots)/(switch_num*degree);
   float src_polling_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree); 


   //huyao 170319 optimization hcLtoS_greedy
   cout << endl;
   sort(pairs.begin(), pairs.end(), HopLtoS);
   //huyao 170319 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size()-1; c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //cout << pairs.size() << endl;
   for (int p = 0; p < pairs.size(); p++){
	//cout << "c: " << pairs[p].channels.size() << endl;
        //cout << "h: " << pairs[p].hops << endl;
        bool moreslots = true;
        int increased_slots = 10000;
	for (int c = 1; c < pairs[p].channels.size()-1; c++) //0 src, pairs[p].channels.size()-1 des
	{	
		if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
			moreslots = false;
			increased_slots = 0;
			break;
		//cout << pairs[p].channels[c] << endl;
                }
                if (max_cp-Crossing_Paths[pairs[p].channels[c]].pair_index.size() < increased_slots) {
			increased_slots = max_cp-Crossing_Paths[pairs[p].channels[c]].pair_index.size();
		}
	}
	if (moreslots == true) {
		for (int c = 1; c < pairs[p].channels.size()-1; c++) {
                	for (int i=0; i<increased_slots; i++) 
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
			//pairs[p].channels.push_back(pairs[p].channels[c]); 
                        total_increased_slots += increased_slots;   
                }
        }
   }
   cout << "hcLtoS_greedy total_increased_slots: " << total_increased_slots << endl;

   cout << endl;

   cout << "hcLtoS_greedy total_slots: " << total_slots + total_increased_slots << endl;
   cout << "hcLtoS_greedy avg_slots: " << (float)(total_slots + total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_greedy increased_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_greedy efficiency: " << (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcLtoS_greedy increased_efficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcLtoS_greedy unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots - total_increased_slots << endl;
   cout << "hcLtoS_greedy unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_greedy decreased_unused_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_greedy inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcLtoS_greedy decreased_inefficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   int hcLtoS_greedy_total_slots = total_slots + total_increased_slots;
   float hcLtoS_greedy_efficiency = (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float hcLtoS_greedy_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size(); 
   float hcLtoS_greedy_avg_slots = (float)(total_slots + total_increased_slots)/(switch_num*degree);
   float hcLtoS_greedy_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree); 


   //huyao 170319 optimization hcLtoS_polling
   cout << endl;
   //huyao 170319 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size()-1; c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //cout << pairs.size() << endl;
   cont = true;
   while (cont){
           int full_pairs = 0;
	   for (int p = 0; p < pairs.size(); p++){
		//cout << "c: " << pairs[p].channels.size() << endl;
		//cout << "h: " << pairs[p].hops << endl;
		bool moreslots = true;
		for (int c = 1; c < pairs[p].channels.size()-1; c++) //0 src, pairs[p].channels.size()-1 des
		{	
			if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
				moreslots = false;
				full_pairs++;
				break;
			//cout << pairs[p].channels[c] << endl;
		        }
		}
		if (moreslots == true) {
			for (int c = 1; c < pairs[p].channels.size()-1; c++) {
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
				//pairs[p].channels.push_back(pairs[p].channels[c]); 
		                total_increased_slots++;   
		        }
		}
	   }
           if (full_pairs == pairs.size()){
		cont = false;          
           }
   }
   cout << "hcLtoS_polling total_increased_slots: " << total_increased_slots << endl;

   cout << endl;

   cout << "hcLtoS_polling total_slots: " << total_slots + total_increased_slots << endl;
   cout << "hcLtoS_polling avg_slots: " << (float)(total_slots + total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_polling increased_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_polling efficiency: " << (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcLtoS_polling increased_efficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcLtoS_polling unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots - total_increased_slots << endl;
   cout << "hcLtoS_polling unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_polling decreased_unused_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcLtoS_polling inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcLtoS_polling decreased_inefficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   int hcLtoS_polling_total_slots = total_slots + total_increased_slots;
   float hcLtoS_polling_efficiency = (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float hcLtoS_polling_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size(); 
   float hcLtoS_polling_avg_slots = (float)(total_slots + total_increased_slots)/(switch_num*degree);
   float hcLtoS_polling_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree); 
   

   //huyao 170319 optimization hcStoL_greedy
   cout << endl;
   sort(pairs.begin(), pairs.end(), HopStoL);
   //huyao 170319 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size()-1; c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //cout << pairs.size() << endl;
   for (int p = 0; p < pairs.size(); p++){
	//cout << "c: " << pairs[p].channels.size() << endl;
        //cout << "h: " << pairs[p].hops << endl;
        bool moreslots = true;
        int increased_slots = 10000;
	for (int c = 1; c < pairs[p].channels.size()-1; c++) //0 src, pairs[p].channels.size()-1 des
	{	
		if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
			moreslots = false;
			increased_slots = 0;
			break;
		//cout << pairs[p].channels[c] << endl;
                }
                if (max_cp-Crossing_Paths[pairs[p].channels[c]].pair_index.size() < increased_slots) {
			increased_slots = max_cp-Crossing_Paths[pairs[p].channels[c]].pair_index.size();
		}
	}
	if (moreslots == true) {
		for (int c = 1; c < pairs[p].channels.size()-1; c++) {
                	for (int i=0; i<increased_slots; i++) 
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
			//pairs[p].channels.push_back(pairs[p].channels[c]); 
                        total_increased_slots += increased_slots;   
                }
        }
   }
   cout << "hcStoL_greedy total_increased_slots: " << total_increased_slots << endl;

   cout << endl;

   cout << "hcStoL_greedy total_slots: " << total_slots + total_increased_slots << endl;
   cout << "hcStoL_greedy avg_slots: " << (float)(total_slots + total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_greedy increased_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_greedy efficiency: " << (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcStoL_greedy increased_efficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcStoL_greedy unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots - total_increased_slots << endl;
   cout << "hcStoL_greedy unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_greedy decreased_unused_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_greedy inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcStoL_greedy decreased_inefficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   int hcStoL_greedy_total_slots = total_slots + total_increased_slots;
   float hcStoL_greedy_efficiency = (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float hcStoL_greedy_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float hcStoL_greedy_avg_slots = (float)(total_slots + total_increased_slots)/(switch_num*degree);
   float hcStoL_greedy_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree); 


   //huyao 170319 optimization hcStoL_polling
   cout << endl;
   //huyao 170319 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size()-1; c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //cout << pairs.size() << endl;
   cont = true;
   while (cont){
           int full_pairs = 0;
	   for (int p = 0; p < pairs.size(); p++){
		//cout << "c: " << pairs[p].channels.size() << endl;
		//cout << "h: " << pairs[p].hops << endl;
		bool moreslots = true;
		for (int c = 1; c < pairs[p].channels.size()-1; c++) //0 src, pairs[p].channels.size()-1 des
		{	
			if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
				moreslots = false;
				full_pairs++;
				break;
			//cout << pairs[p].channels[c] << endl;
		        }
		}
		if (moreslots == true) {
			for (int c = 1; c < pairs[p].channels.size()-1; c++) {
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
				//pairs[p].channels.push_back(pairs[p].channels[c]); 
		                total_increased_slots++;   
		        }
		}
	   }
           if (full_pairs == pairs.size()){
		cont = false;          
           }
   }
   cout << "hcStoL_polling total_increased_slots: " << total_increased_slots << endl;

   cout << endl;

   cout << "hcStoL_polling total_slots: " << total_slots + total_increased_slots << endl;
   cout << "hcStoL_polling avg_slots: " << (float)(total_slots + total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_polling increased_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_polling efficiency: " << (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcStoL_polling increased_efficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcStoL_polling unused_total_slots: " << switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots - total_increased_slots << endl;
   cout << "hcStoL_polling unused_avg_slots: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_polling decreased_unused_avg_slots: " << (float)(total_increased_slots)/(switch_num*degree) << endl;
   cout << "hcStoL_polling inefficiency: " << (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   cout << "hcStoL_polling decreased_inefficiency: " << (float)(total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;

   int hcStoL_polling_total_slots = total_slots + total_increased_slots;
   float hcStoL_polling_efficiency = (float)(total_slots + total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float hcStoL_polling_inefficiency = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree)/max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   float hcStoL_polling_avg_slots = (float)(total_slots + total_increased_slots)/(switch_num*degree);
   float hcStoL_polling_unused_avg_slots = (float)(switch_num*degree*(max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size())-total_slots-total_increased_slots)/(switch_num*degree); 

   cout << endl;

   cout << "total_slots: " << total_slots << endl;
   cout << "src_greedy_total_slots: " << src_greedy_total_slots << endl;
   cout << "src_polling_total_slots: " << src_polling_total_slots << endl;
   cout << "hcLtoS_greedy_total_slots: " << hcLtoS_greedy_total_slots << endl;
   cout << "hcLtoS_polling_total_slots: " << hcLtoS_polling_total_slots << endl;
   cout << "hcStoL_greedy_total_slots: " << hcStoL_greedy_total_slots << endl;
   cout << "hcStoL_polling_total_slots: " << hcStoL_polling_total_slots << endl;

   cout << endl;

   cout << "baseline_efficiency: " << baseline_efficiency << endl;
   cout << "src_greedy_efficiency: " << src_greedy_efficiency << endl;
   cout << "src_polling_efficiency: " << src_polling_efficiency << endl;
   cout << "hcLtoS_greedy_efficiency: " << hcLtoS_greedy_efficiency << endl;
   cout << "hcLtoS_polling_efficiency: " << hcLtoS_polling_efficiency << endl;
   cout << "hcStoL_greedy_efficiency: " << hcStoL_greedy_efficiency << endl;
   cout << "hcStoL_polling_efficiency: " << hcStoL_polling_efficiency << endl;

   cout << endl;

   cout << "baseline_inefficiency: " << baseline_inefficiency << endl;
   cout << "src_greedy_inefficiency: " << src_greedy_inefficiency << endl;
   cout << "src_polling_inefficiency: " << src_polling_inefficiency << endl;
   cout << "hcLtoS_greedy_inefficiency: " << hcLtoS_greedy_inefficiency << endl;
   cout << "hcLtoS_polling_inefficiency: " << hcLtoS_polling_inefficiency << endl;
   cout << "hcStoL_greedy_inefficiency: " << hcStoL_greedy_inefficiency << endl;
   cout << "hcStoL_polling_inefficiency: " << hcStoL_polling_inefficiency << endl;

   cout << endl;

   cout << "baseline_avg_slots: " << baseline_avg_slots << endl;
   cout << "src_greedy_avg_slots: " << src_greedy_avg_slots << endl;
   cout << "src_polling_avg_slots: " << src_polling_avg_slots << endl;
   cout << "hcLtoS_greedy_avg_slots: " << hcLtoS_greedy_avg_slots << endl;
   cout << "hcLtoS_polling_avg_slots: " << hcLtoS_polling_avg_slots << endl;
   cout << "hcStoL_greedy_avg_slots: " << hcStoL_greedy_avg_slots << endl;
   cout << "hcStoL_polling_avg_slots: " << hcStoL_polling_avg_slots << endl;

   cout << endl;

   cout << "baseline_unused_avg_slots: " << baseline_unused_avg_slots << endl;
   cout << "src_greedy_unused_avg_slots: " << src_greedy_unused_avg_slots << endl;
   cout << "src_polling_unused_avg_slots: " << src_polling_unused_avg_slots << endl;
   cout << "hcLtoS_greedy_unused_avg_slots: " << hcLtoS_greedy_unused_avg_slots << endl;
   cout << "hcLtoS_polling_unused_avg_slots: " << hcLtoS_polling_unused_avg_slots << endl;
   cout << "hcStoL_greedy_unused_avg_slots: " << hcStoL_greedy_unused_avg_slots << endl;
   cout << "hcStoL_polling_unused_avg_slots: " << hcStoL_polling_unused_avg_slots << endl;


   //vector<int> abc;
   //abc.push_back(1);
   //cout << abc.size() << endl;
   //abc.push_back(2);
   //cout << abc.size() << endl;
   //abc.push_back(3);
   //cout << abc.size() << endl;
   //abc.push_back(1);
   //cout << abc.size() << endl;
   //abc.push_back(1);
   //sort(abc.begin(),abc.end());
   //abc.erase(unique(abc.begin(), abc.end()), abc.end());
   //cout << abc.size() << endl;

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

   //huyao180408 routing option
   vector<Cross_Paths> Crossing_Paths_xy(Crossing_Paths.size());
   vector<Cross_Paths> Crossing_Paths_yx(Crossing_Paths.size());
   vector<Pair> pairs_xy;
   vector<Pair> pairs_yx;
   
   // ########################################## //
   // ##############   PHASE 1   ############### //
   // ##        $B3F7PO)$NEPO?!$%+%&%s%H(B        ## //
   // ########################################## //

   while ( cin >> h_src){	
      cin >> h_dst;
	  src = h_src/Host_Num;
	  dst = h_dst/Host_Num;

      //huyao180410 mini-oblivious routing
      int xx, yy, xx_sd, yy_sd;
      int src_sd, dst_sd;
      src_sd = h_src/Host_Num;
      dst_sd = h_dst/Host_Num;
      switch (Topology){
      case 0: //mesh
	 xx = dst%array_size - src%array_size;
	 yy = dst/array_size - src/array_size;
         xx_sd = dst_sd%array_size - src_sd%array_size;
	 yy_sd = dst_sd/array_size - src_sd/array_size;
	 break;
      case 1: // torus
	 xx = dst%array_size - src%array_size;
         xx_sd = dst_sd%array_size - src_sd%array_size;
	 if ( xx < 0 && abs(xx) > array_size/2 ) {
	    xx = -( xx + array_size/2);
            xx_sd = -( xx_sd + array_size/2);
	 } else if ( xx > 0 && abs(xx) > array_size/2 ) {
	    xx = -( xx - array_size/2);	
            xx_sd = -( xx_sd - array_size/2);	
	 }
	 yy = dst/array_size - src/array_size;
         yy_sd = dst_sd/array_size - src_sd/array_size;
	 if ( yy < 0 && abs(yy) > array_size/2 ) {
	    yy = -( yy + array_size/2);	
            yy_sd = -( yy_sd + array_size/2);
	 } else if ( yy > 0 && abs(yy) > array_size/2 ) {
	    yy = -( yy - array_size/2);	
            yy_sd = -( yy_sd - array_size/2);	
	 }
	 break;
      default:
	 cerr << "Please select -t0, or -t1 option" << endl;
	 exit(1);
	 break;
      }
      int obli = 2; 
      while (obli > 0){  
      //if ((xx_sd == 0 && abs(yy_sd) < 2) || (yy_sd == 0 && abs(xx_sd) < 2)){       
      if (abs(xx_sd) < 2 || abs(yy_sd) < 2){        
         obli -= 2;
      } else{
         obli -= 1;
         if (obli == 1){
            h_dst = h_dst - (xx/2)*Host_Num;
            h_dst = h_dst - (yy/2)*array_size*Host_Num;      
            dst = dst - xx/2;
            dst = dst - (yy/2)*array_size;
        //     cout << "hehe1" << endl; 
        //     cout << h_src << endl;    
        //     cout << src << endl;               
        //     cout << h_dst << endl;    
        //     cout << dst << endl; 
        //     cout << "hehe1" << endl;  
         }
         if (obli == 0){
            h_src = h_dst;
            src = dst;
            h_dst = dst_sd*Host_Num;
            dst = dst_sd;
        //     cout << "hehe2" << endl; 
        //     cout << h_src << endl;    
        //     cout << src << endl;               
        //     cout << h_dst << endl;    
        //     cout << dst << endl; 
        //     cout << "hehe2" << endl;  
         }
      }        

      bool wrap_around_x = false;
      bool wrap_around_y = false;
      bool wrap_around_x_sd = false;  //huyao180410
      bool wrap_around_y_sd = false;  //huyao180410

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
      pairs[ct].pair_id = ct; //huyao170315     
      int delta_x, delta_y, current;
      int delta_x_sd, delta_y_sd; //huyao180410
      switch (Topology){
      case 0: //mesh
	 delta_x = dst%array_size - src%array_size;
	 delta_y = dst/array_size - src/array_size;
         delta_x_sd = dst%array_size - src%array_size; //huyao180410
         delta_y_sd = dst/array_size - src/array_size; //huyao180410
	 current = src; 
	 break;

      case 1: // torus
	 delta_x = dst%array_size - src%array_size;
         delta_x_sd = dst%array_size - src%array_size; //huyao180410
	 if ( delta_x < 0 && abs(delta_x) > array_size/2 ) {
	    delta_x = -( delta_x + array_size/2);
            delta_x_sd = -( delta_x_sd + array_size/2); //huyao180410
		wrap_around_x = true;	
                wrap_around_x_sd = true;  //huyao180410	
	 } else if ( delta_x > 0 && abs(delta_x) > array_size/2 ) {
	    delta_x = -( delta_x - array_size/2);
            delta_x_sd = -( delta_x_sd - array_size/2); //huyao180410
		wrap_around_x = true;	
                wrap_around_x_sd = true;  //huyao180410		
	 }
	 delta_y = dst/array_size - src/array_size;
         delta_y_sd = dst/array_size - src/array_size; //huyao180410
	 if ( delta_y < 0 && abs(delta_y) > array_size/2 ) {
	    delta_y = -( delta_y + array_size/2);
            delta_y_sd = -( delta_y_sd + array_size/2); //huyao180410
		wrap_around_y = true;	
                wrap_around_y_sd = true;  //huyao180410		
	 } else if ( delta_y > 0 && abs(delta_y) > array_size/2 ) {
	    delta_y = -( delta_y - array_size/2);
            delta_y_sd = -( delta_y_sd - array_size/2); //huyao180410
		wrap_around_y = true;	
                wrap_around_y_sd = true;  //huyao180410		
	 }
	 current = src; 
	 break;
      default:
	 cerr << "Please select -t0, or -t1 option" << endl;
	 exit(1);
	 break;
      }

	 pairs[ct].hops = abs(delta_x) + abs(delta_y); 
    

      //huyao180408 mini-adaptive routing 
      hops += abs(delta_x) + abs(delta_y);
      //Crossing_Paths_xy.reserve(Crossing_Paths.size());//先㝫メモリ領域を確保㝙る。
      //copy(Crossing_Paths.begin(), Crossing_Paths.end(), back_inserter(Crossing_Paths_xy));
      //Crossing_Paths_xy = NULL;
      copy(Crossing_Paths.begin(), Crossing_Paths.end(), Crossing_Paths_xy.begin());
      //cout << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
      //cout << max_element(Crossing_Paths_xy.begin(),Crossing_Paths_xy.end())->pair_index.size() << endl;
      //Crossing_Paths_yx.reserve(Crossing_Paths.size());//先㝫メモリ領域を確保㝙る。
      //copy(Crossing_Paths.begin(), Crossing_Paths.end(), back_inserter(Crossing_Paths_yx));
      //Crossing_Paths_yx = NULL;
      copy(Crossing_Paths.begin(), Crossing_Paths.end(), Crossing_Paths_yx.begin());
      //pairs_xy = NULL;
      //pairs_xy.reserve(pairs.size());//先㝫メモリ領域を確保㝙る。
      //copy(pairs.begin(), pairs.end(), back_inserter(pairs_xy));
      //copy(pairs.begin(), pairs.end(), pairs_xy.begin());
      pairs_xy = pairs;
      //pairs_yx = NULL;
      //pairs_yx.reserve(pairs.size());//先㝫メモリ領域を確保㝙る。
      //copy(pairs.begin(), pairs.end(), back_inserter(pairs_yx));  
      //copy(pairs.begin(), pairs.end(), pairs_yx.begin());  
      pairs_yx = pairs; 

      // X $BJ}8~$N%k!<%F%#%s%0(B
      if (delta_x > 0){
	 while ( delta_x != 0 ){ // +x$BJ}8~(B
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 1;
	    Crossing_Paths_xy[t].pair_index.push_back(ct);  //huyao180410
	    pairs_xy[ct].channels.push_back(t); //huyao180410
	    if ( current % array_size == array_size-1) {
	       wrap_around_x = false;
	       current = current - (array_size -1);
	    } else current++; 
	    delta_x--;
	    //hops++; 
	 }
      } else if (delta_x < 0){
	 while ( delta_x != 0 ){ // -x$BJ}8~(B
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 2;
	    Crossing_Paths_xy[t].pair_index.push_back(ct);  //huyao180410
	    pairs_xy[ct].channels.push_back(t);  //huyao180410
	    if ( current % array_size == 0 ) {
	       wrap_around_x = false;
	       current = current + (array_size - 1 );
	    } else current--;
	    //hops++;
	    delta_x++;
	 }
      }
      
      // X $BJ}8~$N%k!<%F%#%s%0$,=*N;$7$F$$$k$3$H$r3NG'(B
//       if (delta_x != 0){
// 	 cerr << "Routing Error " << endl;
// 	 exit (1);
//       }

      // Y $BJ}8~$N%k!<%F%#%s%0(B	
      if (delta_y > 0){
	 while ( delta_y != 0 ){ // +y$BJ}8~(B
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 3;
	    Crossing_Paths_xy[t].pair_index.push_back(ct);  //huyao180410
	    pairs_xy[ct].channels.push_back(t);  //huyao180410
	    if ( current >= array_size*(array_size-1) ){
	       wrap_around_y = false;
	       current = current - array_size*(array_size -1);
	    } else current += array_size;
	    //hops++;
	    delta_y--;
	 }
      } else if (delta_y < 0){
	 while ( delta_y != 0 ){ // -y$BJ}8~(B
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 4;
	    Crossing_Paths_xy[t].pair_index.push_back(ct);  //huyao180410
	    pairs_xy[ct].channels.push_back(t); //huyao180410
	    if ( current < array_size ) {
	       wrap_around_y = false;
	       current = current + array_size*(array_size -1);
	    } else current -= array_size;
	    //hops++;
	    delta_y++;
	 }
      }
      
      // X,Y $BJ}8~$N%k!<%F%#%s%0$,=*N;$7$F$$$k$3$H$r3NG'(B
      if ( delta_x != 0 || delta_y != 0 ){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

      //huyao180410 y-x routing
      delta_x = delta_x_sd;
      delta_y = delta_y_sd;
      current = src;
      wrap_around_x = wrap_around_x_sd;
      wrap_around_y = wrap_around_y_sd;
      //cout << delta_x << endl;
      //cout << delta_y << endl;
      //cout << current << endl;
      //cout << wrap_around_x << endl;
      //cout << wrap_around_y << endl;
      if (delta_y > 0){
	 while ( delta_y != 0 ){ // +y$BJ}8~(B
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 3;
	    Crossing_Paths_yx[t].pair_index.push_back(ct);  //huyao180410
	    pairs_yx[ct].channels.push_back(t);  //huyao180410 
	    if ( current >= array_size*(array_size-1) ){
	       wrap_around_y = false;
	       current = current - array_size*(array_size -1);
	    } else current += array_size;
	    //hops++;
	    delta_y--;
	 }
      } else if (delta_y < 0){
	 while ( delta_y != 0 ){ // -y$BJ}8~(B
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 4;
	    Crossing_Paths_yx[t].pair_index.push_back(ct);  //huyao180410
	    pairs_yx[ct].channels.push_back(t); //huyao180410
	    if ( current < array_size ) {
	       wrap_around_y = false;
	       current = current + array_size*(array_size -1);
	    } else current -= array_size;
	    //hops++;
	    delta_y++;
	 }
      }
      //cout << current << endl;
      if (delta_x > 0){
	 while ( delta_x != 0 ){ // +x$BJ}8~(B
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 1;
	    Crossing_Paths_yx[t].pair_index.push_back(ct);  //huyao180410
	    pairs_yx[ct].channels.push_back(t); //huyao180410
	    if ( current % array_size == array_size-1) {
	       wrap_around_x = false;
	       current = current - (array_size -1);
	    } else current++; 
	    delta_x--;
	    //hops++; 
	 }
      } else if (delta_x < 0){
	 while ( delta_x != 0 ){ // -x$BJ}8~(B
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 2;
	    Crossing_Paths_yx[t].pair_index.push_back(ct);  //huyao180410
	    pairs_yx[ct].channels.push_back(t);  //huyao180410
	    if ( current % array_size == 0 ) {
	       wrap_around_x = false;
	       current = current + (array_size - 1 );
	    } else current--;
	    //hops++;
	    delta_x++;
	 }
      }
      if ( delta_x != 0 || delta_y != 0 ){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }
      //test
//       cout << "----Crossing_Paths----" << endl;
//       cout << Crossing_Paths.size() << endl;
//       vector<Cross_Paths>::iterator ppt = Crossing_Paths.begin();
//       int aaa = 0;
//       cout << " Node " << setw(2) << aaa/((degree+1+2*Host_Num)*Vch) << ":  ";    
//       while ( ppt != Crossing_Paths.end() ){
//       if (aaa%(degree+1+2*Host_Num)!=0) cout << " " << (*ppt).pair_index.size();
//       aaa ++; ppt++;
//       if ( aaa%((degree+1+2*Host_Num)*Vch) == 0){
// 	 cout << endl;		
// 	 if ( aaa != Vch*(degree+1+2*Host_Num)*switch_num)
// 	    cout << " Node " << setw(2) << aaa/((degree+1+2*Host_Num)*Vch) << ":  ";	
//       }
//       }
//       cout << "----Crossing_Paths----" << endl;
//       cout << "----Crossing_Paths_xy----" << endl;
//       cout << Crossing_Paths_xy.size() << endl;
//       ppt = Crossing_Paths_xy.begin();
//       aaa = 0;
//       cout << " Node " << setw(2) << aaa/((degree+1+2*Host_Num)*Vch) << ":  ";    
//       while ( ppt != Crossing_Paths_xy.end() ){
//       if (aaa%(degree+1+2*Host_Num)!=0) cout << " " << (*ppt).pair_index.size();
//       aaa ++; ppt++;
//       if ( aaa%((degree+1+2*Host_Num)*Vch) == 0){
// 	 cout << endl;		
// 	 if ( aaa != Vch*(degree+1+2*Host_Num)*switch_num)
// 	    cout << " Node " << setw(2) << aaa/((degree+1+2*Host_Num)*Vch) << ":  ";	
//       }
//       }
//       cout << "----Crossing_Paths_xy----" << endl;
//       cout << "----Crossing_Paths_yx----" << endl;
//       cout << Crossing_Paths_yx.size() << endl;
//       ppt = Crossing_Paths_yx.begin();
//       aaa = 0;
//       cout << " Node " << setw(2) << aaa/((degree+1+2*Host_Num)*Vch) << ":  ";    
//       while ( ppt != Crossing_Paths_yx.end() ){
//       if (aaa%(degree+1+2*Host_Num)!=0) cout << " " << (*ppt).pair_index.size();
//       aaa ++; ppt++;
//       if ( aaa%((degree+1+2*Host_Num)*Vch) == 0){
// 	 cout << endl;		
// 	 if ( aaa != Vch*(degree+1+2*Host_Num)*switch_num)
// 	    cout << " Node " << setw(2) << aaa/((degree+1+2*Host_Num)*Vch) << ":  ";	
//       }
//       }
//       cout << "----Crossing_Paths_yx----" << endl;
//       vector<Pair>::iterator pppt = pairs.begin();
//       cout << "----pairs----" << endl;
//       cout << pairs.size() << endl;
//       //ppt = pairs.begin();
//       aaa = 0;  
//       while ( pppt != pairs.end() ){
//       cout << "Pair:" << aaa << endl;        
//       for (int i = 0; i < (*pppt).channels.size(); i++){
//           cout << (*pppt).channels[i] << ", " << endl;
//       }
//       aaa ++; pppt++;
//       }
//       cout << "----pairs----" << endl;
//       cout << "----pairs_xy----" << endl;
//       cout << pairs_xy.size() << endl;
//       pppt = pairs_xy.begin();
//       aaa = 0;  
//       while ( pppt != pairs_xy.end() ){
//       cout << "Pair:" << aaa << endl;        
//       for (int i = 0; i < (*pppt).channels.size(); i++){
//           cout << (*pppt).channels[i] << ", " << endl;
//       }
//       aaa ++; pppt++;
//       }
//       cout << "----pairs_xy----" << endl;
//       cout << "----pairs_yx----" << endl;
//       cout << pairs_yx.size() << endl;
//       pppt = pairs_yx.begin();
//       aaa = 0;  
//       while ( pppt != pairs_yx.end() ){
//       cout << "Pair:" << aaa << endl;        
//       for (int i = 0; i < (*pppt).channels.size(); i++){
//           cout << (*pppt).channels[i] << ", " << endl;
//       }
//       aaa ++; pppt++;
//       }
//       cout << "----pairs_yx----" << endl;      
      //cout << max_element(Crossing_Paths_xy.begin(),Crossing_Paths_xy.end())->pair_index.size() << endl;
      //cout << max_element(Crossing_Paths_yx.begin(),Crossing_Paths_yx.end())->pair_index.size() << endl;
      //cout << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
      if (max_element(Crossing_Paths_yx.begin(),Crossing_Paths_yx.end())->pair_index.size() < max_element(Crossing_Paths_xy.begin(),Crossing_Paths_xy.end())->pair_index.size()){
         //Crossing_Paths.reserve(Crossing_Paths_yx.size());
         //copy(Crossing_Paths_yx.begin(), Crossing_Paths_yx.end(), back_inserter(Crossing_Paths));  
         //Crossing_Paths = NULL;
         copy(Crossing_Paths_yx.begin(), Crossing_Paths_yx.end(), Crossing_Paths.begin()); 
         pairs = pairs_yx;
      }
      else{
         //Crossing_Paths.reserve(Crossing_Paths_xy.size());
         //cout << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl; 
         //cout << max_element(Crossing_Paths_xy.begin(),Crossing_Paths_xy.end())->pair_index.size() << endl;         
         //copy(Crossing_Paths_xy.begin(), Crossing_Paths_xy.end(), back_inserter(Crossing_Paths)); 
         //Crossing_Paths = NULL;
         copy(Crossing_Paths_xy.begin(), Crossing_Paths_xy.end(), Crossing_Paths.begin());  
         //cout << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl; 
         //cout << max_element(Crossing_Paths_xy.begin(),Crossing_Paths_xy.end())->pair_index.size() << endl; 
         pairs = pairs_xy;
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
   }//huyao180410 mini-oblivious routing	


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


