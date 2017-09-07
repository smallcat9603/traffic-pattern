//
// mk_localIDnum-htree.cc
//
// MPI プログラムのトレースから、その通信に必要なローカルアドレスサイズを計算する。
// ローカルアドレスのわりあてアルゴリズム毎に計算可能である。
// (ツリートポロジ( H-Tree) )
//
// MPE profile library を用いて生成された alog 形式のファイル hoge.alog を入力とする。
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


#define HOST_NUM 16 // the number of hosts per switch //4->16,64(koi) two-layer, three-layer switches
//huyao 170108 16->4096 three-layer switches

#define PORT 17 // the number of ports per switch //5->16,64(koi) two-layer, three-layer switches
//huyao 170108 17->4096 three-layer switches


//
// 経路の登録
//
struct Pair {
   int pair_id; //huyao170325
   // 経由チャネル群
   vector<int> channels;
   // 出発地と目的地
   int src; int dst;
   // 経路の ID
   int ID;
   // ID が有効かどうか？
   bool Valid;
   //huyao 170325
   int hops;
   // 初期化
   Pair(int s, int d): src(s),dst(d),ID(-1),Valid(false),hops(-1){}  //huyao 170325
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
   list<int> assigned_list;
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
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int node_num, \
int max_id, vector<Pair> pairs, int hops, int max_cp)
{
   cout << endl;
   // 各チャネル毎に通過する経路の ID を表示、およびエラーチェック
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
   // 各チャネルの crossing paths を表示
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;
   int total_slots = 0; //huyao170307
   int total_slots_sw = 0; //huyao170330
   int total_slots_root_sw = 0; //huyao170330
   int total_slots_mid_sw = 0; //huyao170330
   int total_slots_low_sw = 0; //huyao170330
   cout << " (UP0, DOWN0, DOWN1, DOWN2, DOWN3)" << endl;
   cout << " Node " << setw(2) << port/PORT << ":  "; 
   while ( elem != Crossing_Paths.end() ){
      cout << " " << (*elem).pair_index.size();

      //huyao 170307 170330
      total_slots += (*elem).pair_index.size();
      if (port/PORT >= node_num){
      	total_slots_sw += (*elem).pair_index.size();
	// root switch
	if ( port/PORT == node_num + node_num/HOST_NUM 
		+ node_num/pow(HOST_NUM,2)){
	    total_slots_root_sw += (*elem).pair_index.size();
	 // middle layer switch
	 } else if ( port/PORT >= node_num + node_num/HOST_NUM){
	    total_slots_mid_sw += (*elem).pair_index.size();
	 // low layer switch
	 } else {
	    total_slots_low_sw += (*elem).pair_index.size();
	    }
      }

      elem ++; port++;
      if ( port%PORT == 0){
	 cout << endl;		
	 cout << " Node " << setw(2) << port/PORT << ":  ";	}
   }
   cout << endl;

   // 重要な出力
   cout << "(Maximum) Crossing Paths: " 
       << max_cp << endl;
   cout << "The number of paths on this application : " << ct << " (all-to-all cases: "
	<< node_num*(node_num-1) << ")" << endl;
   cout << "The average hops : " << setiosflags(ios::fixed | ios::showpoint) <<
      (float)hops/ct << endl;
   cout << "ID size(without ID modification)" << max_id << endl;

   //huyao 170307 170330
   cout << "total_slots: " << total_slots << endl;
   cout << "total_slots_sw: " << total_slots_sw << endl;
   cout << "total_slots_root_sw: " << total_slots_root_sw << endl;
   cout << "total_slots_mid_sw: " << total_slots_mid_sw << endl;
   cout << "total_slots_low_sw: " << total_slots_low_sw << endl;
   //if (HOST_NUM == 4) // node = 64, switch_num = 1+4+16=21 (3-layer)
   //if (HOST_NUM == 16) // node = 4096, switch_num = 1+16+256=273 (3-layer)
   cout << "avg_slots_sw: " << (float)total_slots_sw/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)) << endl;
   cout << "efficiency_sw: " << (float)total_slots_sw/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp << endl;
   cout << "unused_total_slots_sw: " << ((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw << endl;
   cout << "unused_avg_slots_sw: " << (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)) << endl;
   cout << "inefficiency_sw: " << (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp << endl;

   //huyao 170330
   float baseline_efficiency_sw = 0;
   float baseline_inefficiency_sw = 0;
   float baseline_avg_slots_sw = 0;
   float baseline_unused_avg_slots_sw = 0;
   baseline_efficiency_sw = (float)total_slots_sw/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   baseline_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   baseline_avg_slots_sw = (float)total_slots_sw/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   baseline_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float baseline_efficiency_root_sw = 0;
   float baseline_inefficiency_root_sw = 0;
   float baseline_avg_slots_root_sw = 0;
   float baseline_unused_avg_slots_root_sw = 0;
   baseline_efficiency_root_sw = (float)total_slots_root_sw/(1*(PORT-1))/max_cp;
   baseline_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw)/(1*(PORT-1))/max_cp;
   baseline_avg_slots_root_sw = (float)total_slots_root_sw/(1*(PORT-1));
   baseline_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw)/(1*(PORT-1));
   float baseline_efficiency_mid_sw = 0;
   float baseline_inefficiency_mid_sw = 0;
   float baseline_avg_slots_mid_sw = 0;
   float baseline_unused_avg_slots_mid_sw = 0;
   baseline_efficiency_mid_sw = (float)total_slots_mid_sw/(HOST_NUM*PORT)/max_cp;
   baseline_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   baseline_avg_slots_mid_sw = (float)total_slots_mid_sw/(HOST_NUM*PORT);
   baseline_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw)/(HOST_NUM*PORT);
   float baseline_efficiency_low_sw = 0;
   float baseline_inefficiency_low_sw = 0;
   float baseline_avg_slots_low_sw = 0;
   float baseline_unused_avg_slots_low_sw = 0;
   baseline_efficiency_low_sw = (float)total_slots_low_sw/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   baseline_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   baseline_avg_slots_low_sw = (float)total_slots_low_sw/((HOST_NUM*HOST_NUM)*PORT);
   baseline_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);

   cout << endl;

   //huyao 170325 optimization src_greedy 170330
   //sort(pairs.begin(), pairs.end(), HopLtoS);
   //sort(pairs.begin(), pairs.end(), HopStoL);
   int total_increased_slots = 0;
   int total_increased_slots_sw = 0;
   int total_increased_slots_root_sw = 0;
   int total_increased_slots_mid_sw = 0;
   int total_increased_slots_low_sw = 0;

   //cout << pairs.size() << endl;
   for (int p = 0; p < pairs.size(); p++){
	//cout << "c: " << pairs[p].channels.size() << endl;
        //cout << "h: " << pairs[p].hops << endl;
        bool moreslots = true;
        int increased_slots = 10000;
	for (int c = 1; c < pairs[p].channels.size(); c++) //0 src
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
		for (int c = 1; c < pairs[p].channels.size(); c++) {
                	for (int i=0; i<increased_slots; i++) 
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
			//pairs[p].channels.push_back(pairs[p].channels[c]); 
                        total_increased_slots += increased_slots;  
			
			//huyao 170330
			if (pairs[p].channels[c]/PORT >= node_num){
				total_increased_slots_sw += increased_slots;
				if (pairs[p].channels[c]/PORT == node_num + node_num/HOST_NUM 
					+ node_num/pow(HOST_NUM,2)){
				    total_increased_slots_root_sw += increased_slots;
				 // middle layer switch
				 } else if ( pairs[p].channels[c]/PORT >= node_num + node_num/HOST_NUM){
				    total_increased_slots_mid_sw += increased_slots;
				 // low layer switch
				 } else {
				    total_increased_slots_low_sw += increased_slots;
				    }
			}

                }
        }
   }
   cout << "src_greedy total_increased_slots: " << total_increased_slots << endl;
   //huyao 170330
   cout << "src_greedy total_increased_slots_sw: " << total_increased_slots_sw << endl;
   cout << "src_greedy total_increased_slots_root_sw: " << total_increased_slots_root_sw << endl;
   cout << "src_greedy total_increased_slots_mid_sw: " << total_increased_slots_mid_sw << endl;
   cout << "src_greedy total_increased_slots_low_sw: " << total_increased_slots_low_sw << endl;

   cout << endl;

   cout << "src_greedy total_slots: " << total_slots + total_increased_slots << endl;
   //huyao 170330
   int src_greedy_total_slots_sw = total_slots_sw + total_increased_slots_sw;
   float src_greedy_efficiency_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   float src_greedy_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;  
   float src_greedy_avg_slots_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float src_greedy_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)); 
   cout << "src_greedy total_slots_sw: " << src_greedy_total_slots_sw << endl;
   cout << "src_greedy avg_slots_sw: " << src_greedy_avg_slots_sw << endl;
   //cout << "src_greedy increased_avg_slots_sw: " << (float)(total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)) << endl;
   cout << "src_greedy efficiency_sw: " << src_greedy_efficiency_sw << endl;
   //cout << "src_greedy increased_efficiency_sw: " << (float)(total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp << endl;
   //cout << "src_greedy unused_total_slots_sw: " << ((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw - total_increased_slots_sw << endl;
   cout << "src_greedy unused_avg_slots_sw: " << src_greedy_unused_avg_slots_sw << endl;
   //cout << "src_greedy decreased_unused_avg_slots_sw: " << (float)(total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)) << endl;
   cout << "src_greedy inefficiency_sw: " << src_greedy_inefficiency_sw << endl;
   //cout << "src_greedy decreased_inefficiency_sw: " << (float)(total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp << endl;

   cout << endl;

   int src_greedy_total_slots_root_sw = total_slots_root_sw + total_increased_slots_root_sw;
   float src_greedy_efficiency_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;
   float src_greedy_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;  
   float src_greedy_avg_slots_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1));
   float src_greedy_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1)); 
   cout << "src_greedy total_slots_root_sw: " << src_greedy_total_slots_root_sw << endl;
   cout << "src_greedy avg_slots_root_sw: " << src_greedy_avg_slots_root_sw << endl;
   cout << "src_greedy efficiency_root_sw: " << src_greedy_efficiency_root_sw << endl;
   cout << "src_greedy unused_avg_slots_root_sw: " << src_greedy_unused_avg_slots_root_sw << endl;
   cout << "src_greedy inefficiency_root_sw: " << src_greedy_inefficiency_root_sw << endl;

   cout << endl;

   int src_greedy_total_slots_mid_sw = total_slots_mid_sw + total_increased_slots_mid_sw;
   float src_greedy_efficiency_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   float src_greedy_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;  
   float src_greedy_avg_slots_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT);
   float src_greedy_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT); 
   cout << "src_greedy total_slots_mid_sw: " << src_greedy_total_slots_mid_sw << endl;
   cout << "src_greedy avg_slots_mid_sw: " << src_greedy_avg_slots_mid_sw << endl;
   cout << "src_greedy efficiency_mid_sw: " << src_greedy_efficiency_mid_sw << endl;
   cout << "src_greedy unused_avg_slots_mid_sw: " << src_greedy_unused_avg_slots_mid_sw << endl;
   cout << "src_greedy inefficiency_mid_sw: " << src_greedy_inefficiency_mid_sw << endl;

   cout << endl;

   int src_greedy_total_slots_low_sw = total_slots_low_sw + total_increased_slots_low_sw;
   float src_greedy_efficiency_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   float src_greedy_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;  
   float src_greedy_avg_slots_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);
   float src_greedy_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT); 
   cout << "src_greedy total_slots_low_sw: " << src_greedy_total_slots_low_sw << endl;
   cout << "src_greedy avg_slots_low_sw: " << src_greedy_avg_slots_low_sw << endl;
   cout << "src_greedy efficiency_low_sw: " << src_greedy_efficiency_low_sw << endl;
   cout << "src_greedy unused_avg_slots_low_sw: " << src_greedy_unused_avg_slots_low_sw << endl;
   cout << "src_greedy inefficiency_low_sw: " << src_greedy_inefficiency_low_sw << endl;


   //huyao 170325 optimization src_polling
   cout << endl;
   //huyao 170325 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size(); c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //huyao 170330
   total_increased_slots_sw = 0;
   total_increased_slots_root_sw = 0;
   total_increased_slots_mid_sw = 0;
   total_increased_slots_low_sw = 0;
   //cout << pairs.size() << endl;
   bool cont = true;
   while (cont){
           int full_pairs = 0;
	   for (int p = 0; p < pairs.size(); p++){
		//cout << "c: " << pairs[p].channels.size() << endl;
		//cout << "h: " << pairs[p].hops << endl;
		bool moreslots = true;
		for (int c = 1; c < pairs[p].channels.size(); c++) //0 src
		{	
			if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
				moreslots = false;
				full_pairs++;
				break;
			//cout << pairs[p].channels[c] << endl;
		        }
		}
		if (moreslots == true) {
			for (int c = 1; c < pairs[p].channels.size(); c++) {
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
				//pairs[p].channels.push_back(pairs[p].channels[c]); 
		                total_increased_slots++;   

				//huyao 170330
				if (pairs[p].channels[c]/PORT >= node_num){
					total_increased_slots_sw++;
					if (pairs[p].channels[c]/PORT == node_num + node_num/HOST_NUM 
						+ node_num/pow(HOST_NUM,2)){
					    total_increased_slots_root_sw++;
					 // middle layer switch
					 } else if ( pairs[p].channels[c]/PORT >= node_num + node_num/HOST_NUM){
					    total_increased_slots_mid_sw++;
					 // low layer switch
					 } else {
					    total_increased_slots_low_sw++;
					    }
				}

		        }
		}
	   }
           if (full_pairs == pairs.size()){
		cont = false;          
           }
   }
   cout << "src_polling total_increased_slots: " << total_increased_slots << endl;
   //huyao 170330
   cout << "src_polling total_increased_slots_sw: " << total_increased_slots_sw << endl;
   cout << "src_polling total_increased_slots_root_sw: " << total_increased_slots_root_sw << endl;
   cout << "src_polling total_increased_slots_mid_sw: " << total_increased_slots_mid_sw << endl;
   cout << "src_polling total_increased_slots_low_sw: " << total_increased_slots_low_sw << endl;

   cout << endl;

   cout << "src_polling total_slots: " << total_slots + total_increased_slots << endl;
   //huyao 170330
   int src_polling_total_slots_sw = total_slots_sw + total_increased_slots_sw;
   float src_polling_efficiency_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   float src_polling_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;  
   float src_polling_avg_slots_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float src_polling_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)); 
   cout << "src_polling total_slots_sw: " << src_polling_total_slots_sw << endl;
   cout << "src_polling avg_slots_sw: " << src_polling_avg_slots_sw << endl;
   cout << "src_polling efficiency_sw: " << src_polling_efficiency_sw << endl;
   cout << "src_polling unused_avg_slots_sw: " << src_polling_unused_avg_slots_sw << endl;
   cout << "src_polling inefficiency_sw: " << src_polling_inefficiency_sw << endl;

   cout << endl;

   int src_polling_total_slots_root_sw = total_slots_root_sw + total_increased_slots_root_sw;
   float src_polling_efficiency_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;
   float src_polling_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;  
   float src_polling_avg_slots_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1));
   float src_polling_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1)); 
   cout << "src_polling total_slots_root_sw: " << src_polling_total_slots_root_sw << endl;
   cout << "src_polling avg_slots_root_sw: " << src_polling_avg_slots_root_sw << endl;
   cout << "src_polling efficiency_root_sw: " << src_polling_efficiency_root_sw << endl;
   cout << "src_polling unused_avg_slots_root_sw: " << src_polling_unused_avg_slots_root_sw << endl;
   cout << "src_polling inefficiency_root_sw: " << src_polling_inefficiency_root_sw << endl;

   cout << endl;

   int src_polling_total_slots_mid_sw = total_slots_mid_sw + total_increased_slots_mid_sw;
   float src_polling_efficiency_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   float src_polling_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;  
   float src_polling_avg_slots_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT);
   float src_polling_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT); 
   cout << "src_polling total_slots_mid_sw: " << src_polling_total_slots_mid_sw << endl;
   cout << "src_polling avg_slots_mid_sw: " << src_polling_avg_slots_mid_sw << endl;
   cout << "src_polling efficiency_mid_sw: " << src_polling_efficiency_mid_sw << endl;
   cout << "src_polling unused_avg_slots_mid_sw: " << src_polling_unused_avg_slots_mid_sw << endl;
   cout << "src_polling inefficiency_mid_sw: " << src_polling_inefficiency_mid_sw << endl;

   cout << endl;

   int src_polling_total_slots_low_sw = total_slots_low_sw + total_increased_slots_low_sw;
   float src_polling_efficiency_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   float src_polling_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;  
   float src_polling_avg_slots_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);
   float src_polling_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT); 
   cout << "src_polling total_slots_low_sw: " << src_polling_total_slots_low_sw << endl;
   cout << "src_polling avg_slots_low_sw: " << src_polling_avg_slots_low_sw << endl;
   cout << "src_polling efficiency_low_sw: " << src_polling_efficiency_low_sw << endl;
   cout << "src_polling unused_avg_slots_low_sw: " << src_polling_unused_avg_slots_low_sw << endl;
   cout << "src_polling inefficiency_low_sw: " << src_polling_inefficiency_low_sw << endl;


   //huyao 170325 optimization hcLtoS_greedy
   cout << endl;
   sort(pairs.begin(), pairs.end(), HopLtoS);
   //huyao 170325 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size(); c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   total_increased_slots_sw = 0;
   total_increased_slots_root_sw = 0;
   total_increased_slots_mid_sw = 0;
   total_increased_slots_low_sw = 0;

   //cout << pairs.size() << endl;
   for (int p = 0; p < pairs.size(); p++){
	//cout << "c: " << pairs[p].channels.size() << endl;
        //cout << "h: " << pairs[p].hops << endl;
        bool moreslots = true;
        int increased_slots = 10000;
	for (int c = 1; c < pairs[p].channels.size(); c++) //0 src
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
		for (int c = 1; c < pairs[p].channels.size(); c++) {
                	for (int i=0; i<increased_slots; i++) 
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
			//pairs[p].channels.push_back(pairs[p].channels[c]); 
                        total_increased_slots += increased_slots;  
			
			//huyao 170330
			if (pairs[p].channels[c]/PORT >= node_num){
				total_increased_slots_sw += increased_slots;
				if (pairs[p].channels[c]/PORT == node_num + node_num/HOST_NUM 
					+ node_num/pow(HOST_NUM,2)){
				    total_increased_slots_root_sw += increased_slots;
				 // middle layer switch
				 } else if ( pairs[p].channels[c]/PORT >= node_num + node_num/HOST_NUM){
				    total_increased_slots_mid_sw += increased_slots;
				 // low layer switch
				 } else {
				    total_increased_slots_low_sw += increased_slots;
				    }
			}

                }
        }
   }
   cout << "hcLtoS_greedy total_increased_slots: " << total_increased_slots << endl;
   //huyao 170330
   cout << "hcLtoS_greedy total_increased_slots_sw: " << total_increased_slots_sw << endl;
   cout << "hcLtoS_greedy total_increased_slots_root_sw: " << total_increased_slots_root_sw << endl;
   cout << "hcLtoS_greedy total_increased_slots_mid_sw: " << total_increased_slots_mid_sw << endl;
   cout << "hcLtoS_greedy total_increased_slots_low_sw: " << total_increased_slots_low_sw << endl;

   cout << endl;

   cout << "hcLtoS_greedy total_slots: " << total_slots + total_increased_slots << endl;
   //huyao 170330
   int hcLtoS_greedy_total_slots_sw = total_slots_sw + total_increased_slots_sw;
   float hcLtoS_greedy_efficiency_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   float hcLtoS_greedy_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;  
   float hcLtoS_greedy_avg_slots_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float hcLtoS_greedy_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)); 
   cout << "hcLtoS_greedy total_slots_sw: " << hcLtoS_greedy_total_slots_sw << endl;
   cout << "hcLtoS_greedy avg_slots_sw: " << hcLtoS_greedy_avg_slots_sw << endl;
   cout << "hcLtoS_greedy efficiency_sw: " << hcLtoS_greedy_efficiency_sw << endl;
   cout << "hcLtoS_greedy unused_avg_slots_sw: " << hcLtoS_greedy_unused_avg_slots_sw << endl;
   cout << "hcLtoS_greedy inefficiency_sw: " << hcLtoS_greedy_inefficiency_sw << endl;

   cout << endl;

   int hcLtoS_greedy_total_slots_root_sw = total_slots_root_sw + total_increased_slots_root_sw;
   float hcLtoS_greedy_efficiency_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;
   float hcLtoS_greedy_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;  
   float hcLtoS_greedy_avg_slots_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1));
   float hcLtoS_greedy_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1)); 
   cout << "hcLtoS_greedy total_slots_root_sw: " << hcLtoS_greedy_total_slots_root_sw << endl;
   cout << "hcLtoS_greedy avg_slots_root_sw: " << hcLtoS_greedy_avg_slots_root_sw << endl;
   cout << "hcLtoS_greedy efficiency_root_sw: " << hcLtoS_greedy_efficiency_root_sw << endl;
   cout << "hcLtoS_greedy unused_avg_slots_root_sw: " << hcLtoS_greedy_unused_avg_slots_root_sw << endl;
   cout << "hcLtoS_greedy inefficiency_root_sw: " << hcLtoS_greedy_inefficiency_root_sw << endl;

   cout << endl;

   int hcLtoS_greedy_total_slots_mid_sw = total_slots_mid_sw + total_increased_slots_mid_sw;
   float hcLtoS_greedy_efficiency_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   float hcLtoS_greedy_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;  
   float hcLtoS_greedy_avg_slots_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT);
   float hcLtoS_greedy_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT); 
   cout << "hcLtoS_greedy total_slots_mid_sw: " << hcLtoS_greedy_total_slots_mid_sw << endl;
   cout << "hcLtoS_greedy avg_slots_mid_sw: " << hcLtoS_greedy_avg_slots_mid_sw << endl;
   cout << "hcLtoS_greedy efficiency_mid_sw: " << hcLtoS_greedy_efficiency_mid_sw << endl;
   cout << "hcLtoS_greedy unused_avg_slots_mid_sw: " << hcLtoS_greedy_unused_avg_slots_mid_sw << endl;
   cout << "hcLtoS_greedy inefficiency_mid_sw: " << hcLtoS_greedy_inefficiency_mid_sw << endl;

   cout << endl;

   int hcLtoS_greedy_total_slots_low_sw = total_slots_low_sw + total_increased_slots_low_sw;
   float hcLtoS_greedy_efficiency_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   float hcLtoS_greedy_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;  
   float hcLtoS_greedy_avg_slots_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);
   float hcLtoS_greedy_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT); 
   cout << "hcLtoS_greedy total_slots_low_sw: " << hcLtoS_greedy_total_slots_low_sw << endl;
   cout << "hcLtoS_greedy avg_slots_low_sw: " << hcLtoS_greedy_avg_slots_low_sw << endl;
   cout << "hcLtoS_greedy efficiency_low_sw: " << hcLtoS_greedy_efficiency_low_sw << endl;
   cout << "hcLtoS_greedy unused_avg_slots_low_sw: " << hcLtoS_greedy_unused_avg_slots_low_sw << endl;
   cout << "hcLtoS_greedy inefficiency_low_sw: " << hcLtoS_greedy_inefficiency_low_sw << endl;


   //huyao 170325 optimization hcLtoS_polling
   cout << endl;
   //huyao 170325 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size(); c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //huyao 170330
   total_increased_slots_sw = 0;
   total_increased_slots_root_sw = 0;
   total_increased_slots_mid_sw = 0;
   total_increased_slots_low_sw = 0;
   //cout << pairs.size() << endl;
   cont = true;
   while (cont){
           int full_pairs = 0;
	   for (int p = 0; p < pairs.size(); p++){
		//cout << "c: " << pairs[p].channels.size() << endl;
		//cout << "h: " << pairs[p].hops << endl;
		bool moreslots = true;
		for (int c = 1; c < pairs[p].channels.size(); c++) //0 src
		{	
			if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
				moreslots = false;
				full_pairs++;
				break;
			//cout << pairs[p].channels[c] << endl;
		        }
		}
		if (moreslots == true) {
			for (int c = 1; c < pairs[p].channels.size(); c++) {
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
				//pairs[p].channels.push_back(pairs[p].channels[c]); 
		                total_increased_slots++;   

				//huyao 170330
				if (pairs[p].channels[c]/PORT >= node_num){
					total_increased_slots_sw++;
					if (pairs[p].channels[c]/PORT == node_num + node_num/HOST_NUM 
						+ node_num/pow(HOST_NUM,2)){
					    total_increased_slots_root_sw++;
					 // middle layer switch
					 } else if ( pairs[p].channels[c]/PORT >= node_num + node_num/HOST_NUM){
					    total_increased_slots_mid_sw++;
					 // low layer switch
					 } else {
					    total_increased_slots_low_sw++;
					    }
				}

		        }
		}
	   }
           if (full_pairs == pairs.size()){
		cont = false;          
           }
   }
   cout << "hcLtoS_polling total_increased_slots: " << total_increased_slots << endl;
   //huyao 170330
   cout << "hcLtoS_polling total_increased_slots_sw: " << total_increased_slots_sw << endl;
   cout << "hcLtoS_polling total_increased_slots_root_sw: " << total_increased_slots_root_sw << endl;
   cout << "hcLtoS_polling total_increased_slots_mid_sw: " << total_increased_slots_mid_sw << endl;
   cout << "hcLtoS_polling total_increased_slots_low_sw: " << total_increased_slots_low_sw << endl;

   cout << endl;

   cout << "hcLtoS_polling total_slots: " << total_slots + total_increased_slots << endl;
   //huyao 170330
   int hcLtoS_polling_total_slots_sw = total_slots_sw + total_increased_slots_sw;
   float hcLtoS_polling_efficiency_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   float hcLtoS_polling_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;  
   float hcLtoS_polling_avg_slots_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float hcLtoS_polling_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)); 
   cout << "hcLtoS_polling total_slots_sw: " << hcLtoS_polling_total_slots_sw << endl;
   cout << "hcLtoS_polling avg_slots_sw: " << hcLtoS_polling_avg_slots_sw << endl;
   cout << "hcLtoS_polling efficiency_sw: " << hcLtoS_polling_efficiency_sw << endl;
   cout << "hcLtoS_polling unused_avg_slots_sw: " << hcLtoS_polling_unused_avg_slots_sw << endl;
   cout << "hcLtoS_polling inefficiency_sw: " << hcLtoS_polling_inefficiency_sw << endl;

   cout << endl;

   int hcLtoS_polling_total_slots_root_sw = total_slots_root_sw + total_increased_slots_root_sw;
   float hcLtoS_polling_efficiency_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;
   float hcLtoS_polling_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;  
   float hcLtoS_polling_avg_slots_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1));
   float hcLtoS_polling_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1)); 
   cout << "hcLtoS_polling total_slots_root_sw: " << hcLtoS_polling_total_slots_root_sw << endl;
   cout << "hcLtoS_polling avg_slots_root_sw: " << hcLtoS_polling_avg_slots_root_sw << endl;
   cout << "hcLtoS_polling efficiency_root_sw: " << hcLtoS_polling_efficiency_root_sw << endl;
   cout << "hcLtoS_polling unused_avg_slots_root_sw: " << hcLtoS_polling_unused_avg_slots_root_sw << endl;
   cout << "hcLtoS_polling inefficiency_root_sw: " << hcLtoS_polling_inefficiency_root_sw << endl;

   cout << endl;

   int hcLtoS_polling_total_slots_mid_sw = total_slots_mid_sw + total_increased_slots_mid_sw;
   float hcLtoS_polling_efficiency_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   float hcLtoS_polling_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;  
   float hcLtoS_polling_avg_slots_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT);
   float hcLtoS_polling_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT); 
   cout << "hcLtoS_polling total_slots_mid_sw: " << hcLtoS_polling_total_slots_mid_sw << endl;
   cout << "hcLtoS_polling avg_slots_mid_sw: " << hcLtoS_polling_avg_slots_mid_sw << endl;
   cout << "hcLtoS_polling efficiency_mid_sw: " << hcLtoS_polling_efficiency_mid_sw << endl;
   cout << "hcLtoS_polling unused_avg_slots_mid_sw: " << hcLtoS_polling_unused_avg_slots_mid_sw << endl;
   cout << "hcLtoS_polling inefficiency_mid_sw: " << hcLtoS_polling_inefficiency_mid_sw << endl;

   cout << endl;

   int hcLtoS_polling_total_slots_low_sw = total_slots_low_sw + total_increased_slots_low_sw;
   float hcLtoS_polling_efficiency_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   float hcLtoS_polling_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;  
   float hcLtoS_polling_avg_slots_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);
   float hcLtoS_polling_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT); 
   cout << "hcLtoS_polling total_slots_low_sw: " << hcLtoS_polling_total_slots_low_sw << endl;
   cout << "hcLtoS_polling avg_slots_low_sw: " << hcLtoS_polling_avg_slots_low_sw << endl;
   cout << "hcLtoS_polling efficiency_low_sw: " << hcLtoS_polling_efficiency_low_sw << endl;
   cout << "hcLtoS_polling unused_avg_slots_low_sw: " << hcLtoS_polling_unused_avg_slots_low_sw << endl;
   cout << "hcLtoS_polling inefficiency_low_sw: " << hcLtoS_polling_inefficiency_low_sw << endl;
   
   //huyao 170325 optimization hcStoL_greedy
   cout << endl;
   sort(pairs.begin(), pairs.end(), HopStoL);
   //huyao 170325 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size(); c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   total_increased_slots_sw = 0;
   total_increased_slots_root_sw = 0;
   total_increased_slots_mid_sw = 0;
   total_increased_slots_low_sw = 0;

   //cout << pairs.size() << endl;
   for (int p = 0; p < pairs.size(); p++){
	//cout << "c: " << pairs[p].channels.size() << endl;
        //cout << "h: " << pairs[p].hops << endl;
        bool moreslots = true;
        int increased_slots = 10000;
	for (int c = 1; c < pairs[p].channels.size(); c++) //0 src
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
		for (int c = 1; c < pairs[p].channels.size(); c++) {
                	for (int i=0; i<increased_slots; i++) 
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
			//pairs[p].channels.push_back(pairs[p].channels[c]); 
                        total_increased_slots += increased_slots;  
			
			//huyao 170330
			if (pairs[p].channels[c]/PORT >= node_num){
				total_increased_slots_sw += increased_slots;
				if (pairs[p].channels[c]/PORT == node_num + node_num/HOST_NUM 
					+ node_num/pow(HOST_NUM,2)){
				    total_increased_slots_root_sw += increased_slots;
				 // middle layer switch
				 } else if ( pairs[p].channels[c]/PORT >= node_num + node_num/HOST_NUM){
				    total_increased_slots_mid_sw += increased_slots;
				 // low layer switch
				 } else {
				    total_increased_slots_low_sw += increased_slots;
				    }
			}

                }
        }
   }
   cout << "hcStoL_greedy total_increased_slots: " << total_increased_slots << endl;
   //huyao 170330
   cout << "hcStoL_greedy total_increased_slots_sw: " << total_increased_slots_sw << endl;
   cout << "hcStoL_greedy total_increased_slots_root_sw: " << total_increased_slots_root_sw << endl;
   cout << "hcStoL_greedy total_increased_slots_mid_sw: " << total_increased_slots_mid_sw << endl;
   cout << "hcStoL_greedy total_increased_slots_low_sw: " << total_increased_slots_low_sw << endl;

   cout << endl;

   cout << "hcStoL_greedy total_slots: " << total_slots + total_increased_slots << endl;
   //huyao 170330
   int hcStoL_greedy_total_slots_sw = total_slots_sw + total_increased_slots_sw;
   float hcStoL_greedy_efficiency_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   float hcStoL_greedy_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;  
   float hcStoL_greedy_avg_slots_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float hcStoL_greedy_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)); 
   cout << "hcStoL_greedy total_slots_sw: " << hcStoL_greedy_total_slots_sw << endl;
   cout << "hcStoL_greedy avg_slots_sw: " << hcStoL_greedy_avg_slots_sw << endl;
   cout << "hcStoL_greedy efficiency_sw: " << hcStoL_greedy_efficiency_sw << endl;
   cout << "hcStoL_greedy unused_avg_slots_sw: " << hcStoL_greedy_unused_avg_slots_sw << endl;
   cout << "hcStoL_greedy inefficiency_sw: " << hcStoL_greedy_inefficiency_sw << endl;

   cout << endl;

   int hcStoL_greedy_total_slots_root_sw = total_slots_root_sw + total_increased_slots_root_sw;
   float hcStoL_greedy_efficiency_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;
   float hcStoL_greedy_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;  
   float hcStoL_greedy_avg_slots_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1));
   float hcStoL_greedy_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1)); 
   cout << "hcStoL_greedy total_slots_root_sw: " << hcStoL_greedy_total_slots_root_sw << endl;
   cout << "hcStoL_greedy avg_slots_root_sw: " << hcStoL_greedy_avg_slots_root_sw << endl;
   cout << "hcStoL_greedy efficiency_root_sw: " << hcStoL_greedy_efficiency_root_sw << endl;
   cout << "hcStoL_greedy unused_avg_slots_root_sw: " << hcStoL_greedy_unused_avg_slots_root_sw << endl;
   cout << "hcStoL_greedy inefficiency_root_sw: " << hcStoL_greedy_inefficiency_root_sw << endl;

   cout << endl;

   int hcStoL_greedy_total_slots_mid_sw = total_slots_mid_sw + total_increased_slots_mid_sw;
   float hcStoL_greedy_efficiency_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   float hcStoL_greedy_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;  
   float hcStoL_greedy_avg_slots_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT);
   float hcStoL_greedy_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT); 
   cout << "hcStoL_greedy total_slots_mid_sw: " << hcStoL_greedy_total_slots_mid_sw << endl;
   cout << "hcStoL_greedy avg_slots_mid_sw: " << hcStoL_greedy_avg_slots_mid_sw << endl;
   cout << "hcStoL_greedy efficiency_mid_sw: " << hcStoL_greedy_efficiency_mid_sw << endl;
   cout << "hcStoL_greedy unused_avg_slots_mid_sw: " << hcStoL_greedy_unused_avg_slots_mid_sw << endl;
   cout << "hcStoL_greedy inefficiency_mid_sw: " << hcStoL_greedy_inefficiency_mid_sw << endl;

   cout << endl;

   int hcStoL_greedy_total_slots_low_sw = total_slots_low_sw + total_increased_slots_low_sw;
   float hcStoL_greedy_efficiency_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   float hcStoL_greedy_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;  
   float hcStoL_greedy_avg_slots_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);
   float hcStoL_greedy_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT); 
   cout << "hcStoL_greedy total_slots_low_sw: " << hcStoL_greedy_total_slots_low_sw << endl;
   cout << "hcStoL_greedy avg_slots_low_sw: " << hcStoL_greedy_avg_slots_low_sw << endl;
   cout << "hcStoL_greedy efficiency_low_sw: " << hcStoL_greedy_efficiency_low_sw << endl;
   cout << "hcStoL_greedy unused_avg_slots_low_sw: " << hcStoL_greedy_unused_avg_slots_low_sw << endl;
   cout << "hcStoL_greedy inefficiency_low_sw: " << hcStoL_greedy_inefficiency_low_sw << endl;


   //huyao 170325 optimization hcStoL_polling
   cout << endl;
   //huyao 170325 erase duplicate
   for (int p = 0; p < pairs.size(); p++){
	for (int c = 1; c < pairs[p].channels.size(); c++){
   		sort(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(),Crossing_Paths[pairs[p].channels[c]].pair_index.end());
   		Crossing_Paths[pairs[p].channels[c]].pair_index.erase(unique(Crossing_Paths[pairs[p].channels[c]].pair_index.begin(), Crossing_Paths[pairs[p].channels[c]].pair_index.end()), Crossing_Paths[pairs[p].channels[c]].pair_index.end());
        }
   }

   total_increased_slots = 0;
   //huyao 170330
   total_increased_slots_sw = 0;
   total_increased_slots_root_sw = 0;
   total_increased_slots_mid_sw = 0;
   total_increased_slots_low_sw = 0;
   //cout << pairs.size() << endl;
   cont = true;
   while (cont){
           int full_pairs = 0;
	   for (int p = 0; p < pairs.size(); p++){
		//cout << "c: " << pairs[p].channels.size() << endl;
		//cout << "h: " << pairs[p].hops << endl;
		bool moreslots = true;
		for (int c = 1; c < pairs[p].channels.size(); c++) //0 src
		{	
			if (Crossing_Paths[pairs[p].channels[c]].pair_index.size() >= max_cp) {
				moreslots = false;
				full_pairs++;
				break;
			//cout << pairs[p].channels[c] << endl;
		        }
		}
		if (moreslots == true) {
			for (int c = 1; c < pairs[p].channels.size(); c++) {
				Crossing_Paths[pairs[p].channels[c]].pair_index.push_back(pairs[p].pair_id);
				//pairs[p].channels.push_back(pairs[p].channels[c]); 
		                total_increased_slots++;   

				//huyao 170330
				if (pairs[p].channels[c]/PORT >= node_num){
					total_increased_slots_sw++;
					if (pairs[p].channels[c]/PORT == node_num + node_num/HOST_NUM 
						+ node_num/pow(HOST_NUM,2)){
					    total_increased_slots_root_sw++;
					 // middle layer switch
					 } else if ( pairs[p].channels[c]/PORT >= node_num + node_num/HOST_NUM){
					    total_increased_slots_mid_sw++;
					 // low layer switch
					 } else {
					    total_increased_slots_low_sw++;
					    }
				}

		        }
		}
	   }
           if (full_pairs == pairs.size()){
		cont = false;          
           }
   }
   cout << "hcStoL_polling total_increased_slots: " << total_increased_slots << endl;
   //huyao 170330
   cout << "hcStoL_polling total_increased_slots_sw: " << total_increased_slots_sw << endl;
   cout << "hcStoL_polling total_increased_slots_root_sw: " << total_increased_slots_root_sw << endl;
   cout << "hcStoL_polling total_increased_slots_mid_sw: " << total_increased_slots_mid_sw << endl;
   cout << "hcStoL_polling total_increased_slots_low_sw: " << total_increased_slots_low_sw << endl;

   cout << endl;

   cout << "hcStoL_polling total_slots: " << total_slots + total_increased_slots << endl;
   //huyao 170330
   int hcStoL_polling_total_slots_sw = total_slots_sw + total_increased_slots_sw;
   float hcStoL_polling_efficiency_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;
   float hcStoL_polling_inefficiency_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))/max_cp;  
   float hcStoL_polling_avg_slots_sw = (float)(total_slots_sw + total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1));
   float hcStoL_polling_unused_avg_slots_sw = (float)(((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1))*max_cp-total_slots_sw-total_increased_slots_sw)/((HOST_NUM*HOST_NUM+HOST_NUM)*PORT+1*(PORT-1)); 
   cout << "hcStoL_polling total_slots_sw: " << hcStoL_polling_total_slots_sw << endl;
   cout << "hcStoL_polling avg_slots_sw: " << hcStoL_polling_avg_slots_sw << endl;
   cout << "hcStoL_polling efficiency_sw: " << hcStoL_polling_efficiency_sw << endl;
   cout << "hcStoL_polling unused_avg_slots_sw: " << hcStoL_polling_unused_avg_slots_sw << endl;
   cout << "hcStoL_polling inefficiency_sw: " << hcStoL_polling_inefficiency_sw << endl;

   cout << endl;

   int hcStoL_polling_total_slots_root_sw = total_slots_root_sw + total_increased_slots_root_sw;
   float hcStoL_polling_efficiency_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;
   float hcStoL_polling_inefficiency_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1))/max_cp;  
   float hcStoL_polling_avg_slots_root_sw = (float)(total_slots_root_sw + total_increased_slots_root_sw)/(1*(PORT-1));
   float hcStoL_polling_unused_avg_slots_root_sw = (float)(1*(PORT-1)*max_cp-total_slots_root_sw-total_increased_slots_root_sw)/(1*(PORT-1)); 
   cout << "hcStoL_polling total_slots_root_sw: " << hcStoL_polling_total_slots_root_sw << endl;
   cout << "hcStoL_polling avg_slots_root_sw: " << hcStoL_polling_avg_slots_root_sw << endl;
   cout << "hcStoL_polling efficiency_root_sw: " << hcStoL_polling_efficiency_root_sw << endl;
   cout << "hcStoL_polling unused_avg_slots_root_sw: " << hcStoL_polling_unused_avg_slots_root_sw << endl;
   cout << "hcStoL_polling inefficiency_root_sw: " << hcStoL_polling_inefficiency_root_sw << endl;

   cout << endl;

   int hcStoL_polling_total_slots_mid_sw = total_slots_mid_sw + total_increased_slots_mid_sw;
   float hcStoL_polling_efficiency_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;
   float hcStoL_polling_inefficiency_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT)/max_cp;  
   float hcStoL_polling_avg_slots_mid_sw = (float)(total_slots_mid_sw + total_increased_slots_mid_sw)/(HOST_NUM*PORT);
   float hcStoL_polling_unused_avg_slots_mid_sw = (float)(HOST_NUM*PORT*max_cp-total_slots_mid_sw-total_increased_slots_mid_sw)/(HOST_NUM*PORT); 
   cout << "hcStoL_polling total_slots_mid_sw: " << hcStoL_polling_total_slots_mid_sw << endl;
   cout << "hcStoL_polling avg_slots_mid_sw: " << hcStoL_polling_avg_slots_mid_sw << endl;
   cout << "hcStoL_polling efficiency_mid_sw: " << hcStoL_polling_efficiency_mid_sw << endl;
   cout << "hcStoL_polling unused_avg_slots_mid_sw: " << hcStoL_polling_unused_avg_slots_mid_sw << endl;
   cout << "hcStoL_polling inefficiency_mid_sw: " << hcStoL_polling_inefficiency_mid_sw << endl;

   cout << endl;

   int hcStoL_polling_total_slots_low_sw = total_slots_low_sw + total_increased_slots_low_sw;
   float hcStoL_polling_efficiency_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;
   float hcStoL_polling_inefficiency_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT)/max_cp;  
   float hcStoL_polling_avg_slots_low_sw = (float)(total_slots_low_sw + total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT);
   float hcStoL_polling_unused_avg_slots_low_sw = (float)((HOST_NUM*HOST_NUM)*PORT*max_cp-total_slots_low_sw-total_increased_slots_low_sw)/((HOST_NUM*HOST_NUM)*PORT); 
   cout << "hcStoL_polling total_slots_low_sw: " << hcStoL_polling_total_slots_low_sw << endl;
   cout << "hcStoL_polling avg_slots_low_sw: " << hcStoL_polling_avg_slots_low_sw << endl;
   cout << "hcStoL_polling efficiency_low_sw: " << hcStoL_polling_efficiency_low_sw << endl;
   cout << "hcStoL_polling unused_avg_slots_low_sw: " << hcStoL_polling_unused_avg_slots_low_sw << endl;
   cout << "hcStoL_polling inefficiency_low_sw: " << hcStoL_polling_inefficiency_low_sw << endl;


   cout << endl;
   cout << "############################################################" << endl;
   cout << endl;

   cout << "total_slots: " << total_slots << endl;

   cout << endl;

   cout << "total_slots_sw: " << total_slots_sw << endl;
   cout << "src_greedy_total_slots_sw: " << src_greedy_total_slots_sw << endl;
   cout << "src_polling_total_slots_sw: " << src_polling_total_slots_sw << endl;
   cout << "hcLtoS_greedy_total_slots_sw: " << hcLtoS_greedy_total_slots_sw << endl;
   cout << "hcLtoS_polling_total_slots_sw: " << hcLtoS_polling_total_slots_sw << endl;
   cout << "hcStoL_greedy_total_slots_sw: " << hcStoL_greedy_total_slots_sw << endl;
   cout << "hcStoL_polling_total_slots_sw: " << hcStoL_polling_total_slots_sw << endl;

   cout << endl;

   cout << "baseline_efficiency_sw: " << baseline_efficiency_sw << endl;
   cout << "src_greedy_efficiency_sw: " << src_greedy_efficiency_sw << endl;
   cout << "src_polling_efficiency_sw: " << src_polling_efficiency_sw << endl;
   cout << "hcLtoS_greedy_efficiency_sw: " << hcLtoS_greedy_efficiency_sw << endl;
   cout << "hcLtoS_polling_efficiency_sw: " << hcLtoS_polling_efficiency_sw << endl;
   cout << "hcStoL_greedy_efficiency_sw: " << hcStoL_greedy_efficiency_sw << endl;
   cout << "hcStoL_polling_efficiency_sw: " << hcStoL_polling_efficiency_sw << endl;

   cout << endl;

   cout << "baseline_inefficiency_sw: " << baseline_inefficiency_sw << endl;
   cout << "src_greedy_inefficiency_sw: " << src_greedy_inefficiency_sw << endl;
   cout << "src_polling_inefficiency_sw: " << src_polling_inefficiency_sw << endl;
   cout << "hcLtoS_greedy_inefficiency_sw: " << hcLtoS_greedy_inefficiency_sw << endl;
   cout << "hcLtoS_polling_inefficiency_sw: " << hcLtoS_polling_inefficiency_sw << endl;
   cout << "hcStoL_greedy_inefficiency_sw: " << hcStoL_greedy_inefficiency_sw << endl;
   cout << "hcStoL_polling_inefficiency_sw: " << hcStoL_polling_inefficiency_sw << endl;

   cout << endl;

   cout << "baseline_avg_slots_sw: " << baseline_avg_slots_sw << endl;
   cout << "src_greedy_avg_slots_sw: " << src_greedy_avg_slots_sw << endl;
   cout << "src_polling_avg_slots_sw: " << src_polling_avg_slots_sw << endl;
   cout << "hcLtoS_greedy_avg_slots_sw: " << hcLtoS_greedy_avg_slots_sw << endl;
   cout << "hcLtoS_polling_avg_slots_sw: " << hcLtoS_polling_avg_slots_sw << endl;
   cout << "hcStoL_greedy_avg_slots_sw: " << hcStoL_greedy_avg_slots_sw << endl;
   cout << "hcStoL_polling_avg_slots_sw: " << hcStoL_polling_avg_slots_sw << endl;

   cout << endl;

   cout << "baseline_unused_avg_slots_sw: " << baseline_unused_avg_slots_sw << endl;
   cout << "src_greedy_unused_avg_slots_sw: " << src_greedy_unused_avg_slots_sw << endl;
   cout << "src_polling_unused_avg_slots_sw: " << src_polling_unused_avg_slots_sw << endl;
   cout << "hcLtoS_greedy_unused_avg_slots_sw: " << hcLtoS_greedy_unused_avg_slots_sw << endl;
   cout << "hcLtoS_polling_unused_avg_slots_sw: " << hcLtoS_polling_unused_avg_slots_sw << endl;
   cout << "hcStoL_greedy_unused_avg_slots_sw: " << hcStoL_greedy_unused_avg_slots_sw << endl;
   cout << "hcStoL_polling_unused_avg_slots_sw: " << hcStoL_polling_unused_avg_slots_sw << endl;

   cout << endl;

   cout << "total_slots_root_sw: " << total_slots_root_sw << endl;
   cout << "src_greedy_total_slots_root_sw: " << src_greedy_total_slots_root_sw << endl;
   cout << "src_polling_total_slots_root_sw: " << src_polling_total_slots_root_sw << endl;
   cout << "hcLtoS_greedy_total_slots_root_sw: " << hcLtoS_greedy_total_slots_root_sw << endl;
   cout << "hcLtoS_polling_total_slots_root_sw: " << hcLtoS_polling_total_slots_root_sw << endl;
   cout << "hcStoL_greedy_total_slots_root_sw: " << hcStoL_greedy_total_slots_root_sw << endl;
   cout << "hcStoL_polling_total_slots_root_sw: " << hcStoL_polling_total_slots_root_sw << endl;

   cout << endl;

   cout << "baseline_efficiency_root_sw: " << baseline_efficiency_root_sw << endl;
   cout << "src_greedy_efficiency_root_sw: " << src_greedy_efficiency_root_sw << endl;
   cout << "src_polling_efficiency_root_sw: " << src_polling_efficiency_root_sw << endl;
   cout << "hcLtoS_greedy_efficiency_root_sw: " << hcLtoS_greedy_efficiency_root_sw << endl;
   cout << "hcLtoS_polling_efficiency_root_sw: " << hcLtoS_polling_efficiency_root_sw << endl;
   cout << "hcStoL_greedy_efficiency_root_sw: " << hcStoL_greedy_efficiency_root_sw << endl;
   cout << "hcStoL_polling_efficiency_root_sw: " << hcStoL_polling_efficiency_root_sw << endl;

   cout << endl;

   cout << "baseline_inefficiency_root_sw: " << baseline_inefficiency_root_sw << endl;
   cout << "src_greedy_inefficiency_root_sw: " << src_greedy_inefficiency_root_sw << endl;
   cout << "src_polling_inefficiency_root_sw: " << src_polling_inefficiency_root_sw << endl;
   cout << "hcLtoS_greedy_inefficiency_root_sw: " << hcLtoS_greedy_inefficiency_root_sw << endl;
   cout << "hcLtoS_polling_inefficiency_root_sw: " << hcLtoS_polling_inefficiency_root_sw << endl;
   cout << "hcStoL_greedy_inefficiency_root_sw: " << hcStoL_greedy_inefficiency_root_sw << endl;
   cout << "hcStoL_polling_inefficiency_root_sw: " << hcStoL_polling_inefficiency_root_sw << endl;

   cout << endl;

   cout << "baseline_avg_slots_root_sw: " << baseline_avg_slots_root_sw << endl;
   cout << "src_greedy_avg_slots_root_sw: " << src_greedy_avg_slots_root_sw << endl;
   cout << "src_polling_avg_slots_root_sw: " << src_polling_avg_slots_root_sw << endl;
   cout << "hcLtoS_greedy_avg_slots_root_sw: " << hcLtoS_greedy_avg_slots_root_sw << endl;
   cout << "hcLtoS_polling_avg_slots_root_sw: " << hcLtoS_polling_avg_slots_root_sw << endl;
   cout << "hcStoL_greedy_avg_slots_root_sw: " << hcStoL_greedy_avg_slots_root_sw << endl;
   cout << "hcStoL_polling_avg_slots_root_sw: " << hcStoL_polling_avg_slots_root_sw << endl;

   cout << endl;

   cout << "baseline_unused_avg_slots_root_sw: " << baseline_unused_avg_slots_root_sw << endl;
   cout << "src_greedy_unused_avg_slots_root_sw: " << src_greedy_unused_avg_slots_root_sw << endl;
   cout << "src_polling_unused_avg_slots_root_sw: " << src_polling_unused_avg_slots_root_sw << endl;
   cout << "hcLtoS_greedy_unused_avg_slots_root_sw: " << hcLtoS_greedy_unused_avg_slots_root_sw << endl;
   cout << "hcLtoS_polling_unused_avg_slots_root_sw: " << hcLtoS_polling_unused_avg_slots_root_sw << endl;
   cout << "hcStoL_greedy_unused_avg_slots_root_sw: " << hcStoL_greedy_unused_avg_slots_root_sw << endl;
   cout << "hcStoL_polling_unused_avg_slots_root_sw: " << hcStoL_polling_unused_avg_slots_root_sw << endl;

   cout << endl;

   cout << "total_slots_mid_sw: " << total_slots_mid_sw << endl;
   cout << "src_greedy_total_slots_mid_sw: " << src_greedy_total_slots_mid_sw << endl;
   cout << "src_polling_total_slots_mid_sw: " << src_polling_total_slots_mid_sw << endl;
   cout << "hcLtoS_greedy_total_slots_mid_sw: " << hcLtoS_greedy_total_slots_mid_sw << endl;
   cout << "hcLtoS_polling_total_slots_mid_sw: " << hcLtoS_polling_total_slots_mid_sw << endl;
   cout << "hcStoL_greedy_total_slots_mid_sw: " << hcStoL_greedy_total_slots_mid_sw << endl;
   cout << "hcStoL_polling_total_slots_mid_sw: " << hcStoL_polling_total_slots_mid_sw << endl;

   cout << endl;

   cout << "baseline_efficiency_mid_sw: " << baseline_efficiency_mid_sw << endl;
   cout << "src_greedy_efficiency_mid_sw: " << src_greedy_efficiency_mid_sw << endl;
   cout << "src_polling_efficiency_mid_sw: " << src_polling_efficiency_mid_sw << endl;
   cout << "hcLtoS_greedy_efficiency_mid_sw: " << hcLtoS_greedy_efficiency_mid_sw << endl;
   cout << "hcLtoS_polling_efficiency_mid_sw: " << hcLtoS_polling_efficiency_mid_sw << endl;
   cout << "hcStoL_greedy_efficiency_mid_sw: " << hcStoL_greedy_efficiency_mid_sw << endl;
   cout << "hcStoL_polling_efficiency_mid_sw: " << hcStoL_polling_efficiency_mid_sw << endl;

   cout << endl;

   cout << "baseline_inefficiency_mid_sw: " << baseline_inefficiency_mid_sw << endl;
   cout << "src_greedy_inefficiency_mid_sw: " << src_greedy_inefficiency_mid_sw << endl;
   cout << "src_polling_inefficiency_mid_sw: " << src_polling_inefficiency_mid_sw << endl;
   cout << "hcLtoS_greedy_inefficiency_mid_sw: " << hcLtoS_greedy_inefficiency_mid_sw << endl;
   cout << "hcLtoS_polling_inefficiency_mid_sw: " << hcLtoS_polling_inefficiency_mid_sw << endl;
   cout << "hcStoL_greedy_inefficiency_mid_sw: " << hcStoL_greedy_inefficiency_mid_sw << endl;
   cout << "hcStoL_polling_inefficiency_mid_sw: " << hcStoL_polling_inefficiency_mid_sw << endl;

   cout << endl;

   cout << "baseline_avg_slots_mid_sw: " << baseline_avg_slots_mid_sw << endl;
   cout << "src_greedy_avg_slots_mid_sw: " << src_greedy_avg_slots_mid_sw << endl;
   cout << "src_polling_avg_slots_mid_sw: " << src_polling_avg_slots_mid_sw << endl;
   cout << "hcLtoS_greedy_avg_slots_mid_sw: " << hcLtoS_greedy_avg_slots_mid_sw << endl;
   cout << "hcLtoS_polling_avg_slots_mid_sw: " << hcLtoS_polling_avg_slots_mid_sw << endl;
   cout << "hcStoL_greedy_avg_slots_mid_sw: " << hcStoL_greedy_avg_slots_mid_sw << endl;
   cout << "hcStoL_polling_avg_slots_mid_sw: " << hcStoL_polling_avg_slots_mid_sw << endl;

   cout << endl;

   cout << "baseline_unused_avg_slots_mid_sw: " << baseline_unused_avg_slots_mid_sw << endl;
   cout << "src_greedy_unused_avg_slots_mid_sw: " << src_greedy_unused_avg_slots_mid_sw << endl;
   cout << "src_polling_unused_avg_slots_mid_sw: " << src_polling_unused_avg_slots_mid_sw << endl;
   cout << "hcLtoS_greedy_unused_avg_slots_mid_sw: " << hcLtoS_greedy_unused_avg_slots_mid_sw << endl;
   cout << "hcLtoS_polling_unused_avg_slots_mid_sw: " << hcLtoS_polling_unused_avg_slots_mid_sw << endl;
   cout << "hcStoL_greedy_unused_avg_slots_mid_sw: " << hcStoL_greedy_unused_avg_slots_mid_sw << endl;
   cout << "hcStoL_polling_unused_avg_slots_mid_sw: " << hcStoL_polling_unused_avg_slots_mid_sw << endl;

   cout << endl;

   cout << "total_slots_low_sw: " << total_slots_low_sw << endl;
   cout << "src_greedy_total_slots_low_sw: " << src_greedy_total_slots_low_sw << endl;
   cout << "src_polling_total_slots_low_sw: " << src_polling_total_slots_low_sw << endl;
   cout << "hcLtoS_greedy_total_slots_low_sw: " << hcLtoS_greedy_total_slots_low_sw << endl;
   cout << "hcLtoS_polling_total_slots_low_sw: " << hcLtoS_polling_total_slots_low_sw << endl;
   cout << "hcStoL_greedy_total_slots_low_sw: " << hcStoL_greedy_total_slots_low_sw << endl;
   cout << "hcStoL_polling_total_slots_low_sw: " << hcStoL_polling_total_slots_low_sw << endl;

   cout << endl;

   cout << "baseline_efficiency_low_sw: " << baseline_efficiency_low_sw << endl;
   cout << "src_greedy_efficiency_low_sw: " << src_greedy_efficiency_low_sw << endl;
   cout << "src_polling_efficiency_low_sw: " << src_polling_efficiency_low_sw << endl;
   cout << "hcLtoS_greedy_efficiency_low_sw: " << hcLtoS_greedy_efficiency_low_sw << endl;
   cout << "hcLtoS_polling_efficiency_low_sw: " << hcLtoS_polling_efficiency_low_sw << endl;
   cout << "hcStoL_greedy_efficiency_low_sw: " << hcStoL_greedy_efficiency_low_sw << endl;
   cout << "hcStoL_polling_efficiency_low_sw: " << hcStoL_polling_efficiency_low_sw << endl;

   cout << endl;

   cout << "baseline_inefficiency_low_sw: " << baseline_inefficiency_low_sw << endl;
   cout << "src_greedy_inefficiency_low_sw: " << src_greedy_inefficiency_low_sw << endl;
   cout << "src_polling_inefficiency_low_sw: " << src_polling_inefficiency_low_sw << endl;
   cout << "hcLtoS_greedy_inefficiency_low_sw: " << hcLtoS_greedy_inefficiency_low_sw << endl;
   cout << "hcLtoS_polling_inefficiency_low_sw: " << hcLtoS_polling_inefficiency_low_sw << endl;
   cout << "hcStoL_greedy_inefficiency_low_sw: " << hcStoL_greedy_inefficiency_low_sw << endl;
   cout << "hcStoL_polling_inefficiency_low_sw: " << hcStoL_polling_inefficiency_low_sw << endl;

   cout << endl;

   cout << "baseline_avg_slots_low_sw: " << baseline_avg_slots_low_sw << endl;
   cout << "src_greedy_avg_slots_low_sw: " << src_greedy_avg_slots_low_sw << endl;
   cout << "src_polling_avg_slots_low_sw: " << src_polling_avg_slots_low_sw << endl;
   cout << "hcLtoS_greedy_avg_slots_low_sw: " << hcLtoS_greedy_avg_slots_low_sw << endl;
   cout << "hcLtoS_polling_avg_slots_low_sw: " << hcLtoS_polling_avg_slots_low_sw << endl;
   cout << "hcStoL_greedy_avg_slots_low_sw: " << hcStoL_greedy_avg_slots_low_sw << endl;
   cout << "hcStoL_polling_avg_slots_low_sw: " << hcStoL_polling_avg_slots_low_sw << endl;

   cout << endl;

   cout << "baseline_unused_avg_slots_low_sw: " << baseline_unused_avg_slots_low_sw << endl;
   cout << "src_greedy_unused_avg_slots_low_sw: " << src_greedy_unused_avg_slots_low_sw << endl;
   cout << "src_polling_unused_avg_slots_low_sw: " << src_polling_unused_avg_slots_low_sw << endl;
   cout << "hcLtoS_greedy_unused_avg_slots_low_sw: " << hcLtoS_greedy_unused_avg_slots_low_sw << endl;
   cout << "hcLtoS_polling_unused_avg_slots_low_sw: " << hcLtoS_polling_unused_avg_slots_low_sw << endl;
   cout << "hcStoL_greedy_unused_avg_slots_low_sw: " << hcStoL_greedy_unused_avg_slots_low_sw << endl;
   cout << "hcStoL_polling_unused_avg_slots_low_sw: " << hcStoL_polling_unused_avg_slots_low_sw << endl;

   cout << endl;

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
   // ノード数 = array_size*array_size
   static int array_size = 8;
   // ID Allocation ポリシの選択
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
   
   // 経路の出発地と目的地	
   int src = -1, dst = -1, current = -1;	
   static int node_num = array_size*array_size;
   // Crossing Paths の格納 (Host <-> Switch間を含む)
   vector<Cross_Paths> Crossing_Paths( PORT * (node_num+node_num/HOST_NUM+node_num/(int)pow(HOST_NUM,2)+1));
   // port num * switch num
   // 経路数を格納
   int ct = 0;
   // 全経路の総ホップ数
   int hops = 0; 

   int before_hops = 0;

   // 各経路
   vector<Pair> pairs;
   
   // ########################################## //
   // ##############   PHASE 1   ############### //
   // ##        各経路の登録，カウント        ## //
   // ########################################## //
   
   while ( cin >> src){	
      cin	>> dst;
      current = src;

      //#######################//
      // switch port:0 UP, 1 DOWN0(or localhost), 2 DOWN1, 3 DOWN2, 4 DOWN3 //
      //#######################//
      
      // チャネルは、チャネルの出発地 switch ID と 出発地 port で識別
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
	    // 不要
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
   // ##  各経路への local ID の割り当て      ## //
   // ########################################## //

   // ID更新型で必要となるサイズ。すなわち、crossing paths。
   int max_id = max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   int max_cp = max_id;
   for (int j = 0; j <= PORT * (node_num+node_num/HOST_NUM+node_num/(int)pow(HOST_NUM,2)); 
				   j++ ){ 
      // 経路を選ぶためのチャネルを選択。
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


