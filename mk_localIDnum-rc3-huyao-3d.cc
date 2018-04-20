//
// mk_localIDnum-rc1.cc
//
// MPI プログラムのトレースから、その通信に必要なローカルアドレスサイズを計算する。
// ローカルアドレスのわりあてアルゴリズム毎に計算可能である。
// (2次元 Mesh + e-cube ルーティングを使用する。)
//
// MPE profile library を用いて生成された alog 形式のファイル hoge.alog を入力とする。
// Usage: cat hoge.alog | alog2ins_sim.sh | ./mk_localIDnum
//
// Sun Nov 16 15:26:42 JST 2003 koibuchi@am.ics.keio.ac.jp
// 2004年 5月23日 日曜日 18時21分08秒 JST koibuchi@am.ics.keio.ac.jp
// Thu Dec 16 22:37:48 JST 2004 koibuchi@am.ics.keio.ac.jp
//

#include <unistd.h> // getopt に必要
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>

using namespace std;

#define degree 6 // 3d huyao 170101

//
// 経路の登録
//
struct Pair {
   int pair_id; //huyao170325
   // 経由チャネル群
   vector<int> channels;
   // 出発地と目的地
   int src; int dst;
   int h_src; int h_dst;
   // 経路の ID
   int ID;
   // ID が有効かどうか？
   bool Valid;
   // the distance between src and dst
   int hops;
   // 初期化
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

//huyao170325
bool HopLtoS(Pair a, Pair b) {return (a.hops > b.hops);}
bool HopStoL(Pair a, Pair b) {return (a.hops < b.hops);}

//
// 物理チャネル毎に通過する経路を格納
//
struct Cross_Paths {
   // 当チャネルを通過する全経路(Pair) の index群
   vector<int> pair_index;
   // 当チャネルにすでにわりあてられている ID 群
   vector<int> assigned_list;
   // その ID の目的地 (destination-based ID label に使用)
   vector<int> assigned_dst_list;
   // 通過 routing paths すべてに ID がわりあてられているかどうか？
   bool Valid;
   // 初期化
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
// 出力系 および、エラーチェック
//
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
int max_id, vector<Pair> pairs, int hops, int Vch, int Host_Num, int max_cp,
bool path_based)
{
   cout << endl;
   // 各チャネル毎に通過する経路の ID を表示、およびエラーチェック
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

   // 各チャネルの crossing paths を表示
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;
   //int slots = 0; //huyao161226
   //int pointer = 0; //huyao161226
   int total_slots = 0; //huyao170307
   if (Vch == 0)
		cout << " (east, west, south, north, front, back, from host0,... to host0,...)"; //huyao170101
   else
   		cout << " (east, west, south, north, front, back, from host0,... east(vch2), south(vch2), west(vch2), north(vch2), front, back, from host(vch2)0,.. to host(vch2)0,.." //huyao170101
		<< endl;

   	cout << " Node " << setw(2) << port/((degree+1+2*Host_Num)*Vch) << ":  ";    while ( elem != Crossing_Paths.end() ){
      if (port%(degree+1+2*Host_Num)!=0) cout << " " << (*elem).pair_index.size();
      //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();} //huyao161226

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

   // 重要な出力
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

   //huyao 170325 optimization src_greedy
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


   //huyao 170325 optimization src_polling
   cout << endl;
   //huyao 170325 erase duplicate
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


   //huyao 170325 optimization hcLtoS_greedy
   cout << endl;
   sort(pairs.begin(), pairs.end(), HopLtoS);
   //huyao 170325 erase duplicate
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


   //huyao 170325 optimization hcLtoS_polling
   cout << endl;
   //huyao 170325 erase duplicate
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
   

   //huyao 170325 optimization hcStoL_greedy
   cout << endl;
   sort(pairs.begin(), pairs.end(), HopStoL);
   //huyao 170325 erase duplicate
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


   //huyao 170325 optimization hcStoL_polling
   cout << endl;
   //huyao 170325 erase duplicate
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
   // メッシュ/トーラスの辺の長さ
   static int array_size = 4;
   // ID Allocation ポリシの選択
   static int Allocation = 1;
	// ルータあたりのホスト数
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
   
   // 経路の出発地と目的地	
   int src = -1, dst = -1, h_src = -1, h_dst = -1;
	// 総スイッチ数(総ホスト数ではない)
   static int switch_num = array_size*array_size*array_size; //huyao 170101
   // Crossing Paths の格納 
	// (Host <-> Switch間を含む,ルータあたりのホスト数の変更により+2)
   vector<Cross_Paths> Crossing_Paths((degree+1+2*Host_Num)*switch_num*Vch);
   // 経路数を格納
   int ct = 0;
   // 総経路の総ホップ数
   int hops = 0; 

   // 経路
   vector<Pair> pairs;
   
   // ########################################## //
   // ##############   PHASE 1   ############### //
   // ##        各経路の登録，カウント        ## //
   // ########################################## //

   while ( cin >> h_src){	
      cin >> h_dst;
	  src = h_src/Host_Num;
	  dst = h_dst/Host_Num;

      bool wrap_around_x = false;
      bool wrap_around_y = false;
      bool wrap_around_z = false; //huyao 170101

      //#######################//
      // (switch port:0 localhost->switch, 1 +x, 2 -x, 3 +y, 4 -y, 5 switch-> localhost)
      // switch port:1 +x方向, 2 -x, 3 +y, 4 -y, 
	  // 5〜(4+Host_Num) localhost->switch 
	  // (5+Host_Num)〜(4+Host_Num*2) switch-> localhost
      //#######################//

      // チャネルは、チャネルの出発地 switch ID と 出発地 port で識別
      Pair tmp_pair(src,dst,h_src,h_dst);  
      pairs.push_back(tmp_pair);
	  // 目的地の番号毎に、ホストチャネルにおける仮想チャネル番号を分散
	  // 分散しない場合 (consume channelの場合も同様に変更する。)
	  // int t = Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
	  int t = (dst%2==1 && Topology==1) ? 
		Vch*src*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+h_src%Host_Num
		  : Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
      Crossing_Paths[t].pair_index.push_back(ct);
      pairs[ct].channels.push_back(t);      
      pairs[ct].pair_id = ct; //huyao170325 
      int delta_x, delta_y, delta_z, current, src_xy, dst_xy; //huyao 170101
      src_xy = src%(array_size*array_size); //huyao 170101
      dst_xy = dst%(array_size*array_size); //huyao 170101
      switch (Topology){
      case 0: //mesh
	 delta_z = dst/(array_size*array_size) - src/(array_size*array_size); //huyao 170101
	 delta_x = dst_xy%array_size - src_xy%array_size; //huyao 170101
	 delta_y = dst_xy/array_size - src_xy/array_size; //huyao 170101
	 current = src; 
	 break;

      case 1: // torus
	 //delta_z huyao 170102
	 delta_z = dst/(array_size*array_size) - src/(array_size*array_size);
	 if ( delta_z < 0 && abs(delta_z) > array_size/2 ) {
	    //delta_z = -( delta_z + array_size/2);
            //huyao 180417 rev
            delta_z = delta_z + array_size;
		wrap_around_z = true;		
	 } else if ( delta_z > 0 && abs(delta_z) > array_size/2 ) {
	    //delta_z = -( delta_z - array_size/2);
            //huyao 180417 rev
            delta_z = delta_z - array_size;
		wrap_around_z = true;		
	 }
	 delta_x = dst_xy%array_size - src_xy%array_size; //huyao 170101
	 if ( delta_x < 0 && abs(delta_x) > array_size/2 ) {
	    //delta_x = -( delta_x + array_size/2);
            //huyao 180417 rev
            delta_x = delta_x + array_size;
		wrap_around_x = true;		
	 } else if ( delta_x > 0 && abs(delta_x) > array_size/2 ) {
	    //delta_x = -( delta_x - array_size/2);
            //huyao 180417 rev
            delta_x = delta_x - array_size;
		wrap_around_x = true;		
	 }
	 delta_y = dst_xy/array_size - src_xy/array_size; //huyao 170101
	 if ( delta_y < 0 && abs(delta_y) > array_size/2 ) {
	    //delta_y = -( delta_y + array_size/2);
            //huyao 180417 rev
            delta_y = delta_y + array_size;
		wrap_around_y = true;		
	 } else if ( delta_y > 0 && abs(delta_y) > array_size/2 ) {
	    //delta_y = -( delta_y - array_size/2);
            //huyao 180417 rev
            delta_y = delta_y - array_size;
		wrap_around_y = true;		
	 }
	 current = src; 
	 break;
      default:
	 cerr << "Please select -t0, or -t1 option" << endl;
	 exit(1);
	 break;
      }

	 pairs[ct].hops = abs(delta_x) + abs(delta_y) + abs(delta_z); //huyao 170102

      //delta_z huyao 170102
      if (delta_z > 0){
	 while ( delta_z != 0 ){ // +x方向
	    int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 5;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    //if ( current % (array_size*array_size) == array_size-1) { //huyao 170103
	    if ( current >= array_size*array_size*(array_size-1)) {
	       wrap_around_z = false;
	       current = current - (array_size -1)*array_size*array_size;
	    } else current += array_size*array_size; 
	    delta_z--;
	    hops++;
	 }
      } else if (delta_z < 0){
	 while ( delta_z != 0 ){ // -x方向
	    int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 6;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    //if ( current % (array_size*array_size) == 0 ) { //huyao 170103
	    if ( current < array_size*array_size) {
	       wrap_around_z = false;
	       current = current + (array_size -1)*array_size*array_size;
	    } else current -= array_size*array_size;
	    hops++;
	    delta_z++;
	 }
      }
      
      // delta_z huyao 170102
      if (delta_z != 0){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

      // X 方向のルーティング
      if (delta_x > 0){
	 while ( delta_x != 0 ){ // +x方向
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 1;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( (current % (array_size*array_size)) % array_size == array_size-1) { //huyao 170102
	       wrap_around_x = false;
	       current = current - (array_size -1);
	    } else current++; 
	    delta_x--;
	    hops++;
	 }
      } else if (delta_x < 0){
	 while ( delta_x != 0 ){ // -x方向
	    int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 2;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( (current % (array_size*array_size)) % array_size == 0 ) { //huyao 170102
	       wrap_around_x = false;
	       current = current + (array_size - 1);
	    } else current--;
	    hops++;
	    delta_x++;
	 }
      }
      
      // X 方向のルーティングが終了していることを確認
      if (delta_x != 0){
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

      // Y 方向のルーティング	
      if (delta_y > 0){
	 while ( delta_y != 0 ){ // +y方向
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 3;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( (current % (array_size*array_size)) >= array_size*(array_size-1) ){ //huyao 170102
	       wrap_around_y = false;
	       current = current - array_size*(array_size -1);
	    } else current += array_size;
	    hops++;
	    delta_y--;
	 }
      } else if (delta_y < 0){
	 while ( delta_y != 0 ){ // -y方向
	    int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
	       Vch * current * (degree+1+2*Host_Num) + 4;
	    Crossing_Paths[t].pair_index.push_back(ct); 
	    pairs[ct].channels.push_back(t);
	    if ( (current % (array_size*array_size)) < array_size ) { //huyao 170102
	       wrap_around_y = false;
	       current = current + array_size*(array_size -1);
	    } else current -= array_size;
	    hops++;
	    delta_y++;
	 }
      }
      
      // X,Y 方向のルーティングが終了していることを確認
      if ( delta_x != 0 || delta_y != 0 || delta_z != 0){ //huyao 170102
	 cerr << "Routing Error " << endl;
	 exit (1);
      }

		// switch->host のチャネルにおける仮想チャネル間の分散
		// 分散しない場合は
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
   // ##  各経路への local ID の割り当て      ## //
   // ########################################## //

   // ID更新型で必要となるサイズ。すなわち、crossing paths。
   int max_cp = max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   int max_id = 0;
	
	int max_cp_dst = 0; // 必要となる dst-based renable label 数	

	// dst-based renewable label を計算
	int max_cp_dst_t = 0;
   for (int j = 0; j < Vch * (degree+1+2*Host_Num) * switch_num; j++ ){ 
	    vector<Cross_Paths>::iterator elem = Crossing_Paths.begin()+j;
		unsigned int p_ct = 0;
		while ( p_ct < elem->pair_index.size() ){
			int u = elem->pair_index[p_ct]; 
			bool is_duplicate = false; //同じ目的地を持つ経路があるか？
			// 目的地が一緒の経路対がそのチャネルを通過するかどうか？
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
        //huyao 180417
        int slot_max = 0;
        for (int i = 0; i < Crossing_Paths.size(); i++){
                if (i%(degree+1+2*Host_Num) != degree+2*Host_Num || i%(degree+1+2*Host_Num) != degree+2*Host_Num-1)
                        if(Crossing_Paths[i].pair_index.size() > slot_max)
                                slot_max = Crossing_Paths[i].pair_index.size();
        }
        cout << " slot_max = " << slot_max << endl;	


   for (int j = 0; j < Vch * (degree+1+2*Host_Num) * switch_num; j++ ){ 
      // 経路を選ぶためのチャネルを選択。
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

	// チャネル上では(登録)経路順に local ID番号を決めていく。
	unsigned int path_ct = 0; 
	while ( path_ct < elem->pair_index.size() ){
	   // ID をわりあてる経路(pair)の番号
	   int t = elem->pair_index[path_ct];      
	   // ID がすでにわりあてられている場合はスキップする。
	   if ( pairs[t].Valid == true ) {path_ct++; continue;}
	   // 0から順にわりあてる。
	   int id_tmp = 0;
	   bool NG_ID = false;
	   
      NEXT_ID:
	 // すでに使用されている ID かどうか？
	 unsigned int s_ct = 0; // 経由チャネルを指定
	 while ( s_ct < pairs[t].channels.size() && !NG_ID ){
	    int i = pairs[t].channels[s_ct];
	    vector<int>::iterator find_ptr;
	    find_ptr = find ( Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
	    if ( path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) NG_ID = true;
	    if (!path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) {
	       int tmp = 0;
			// index 探し
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
	//IDは0から割り当てているので個数は Max ID +1 個必要ということになる．
	 if (max_id <= id_tmp) max_id = id_tmp + 1; 

	  path_ct++;
      }
      elem->Valid = true;
   }
   
   show_paths(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Vch, Host_Num, max_cp, path_based);   
   
   return 0;
}


