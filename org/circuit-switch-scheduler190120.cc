//
// circuit-switch-table.cc
//
// mesh/torus/fat-tree/fully-connected/full-mesh-connected-circles(FCC)
// Usage: cat test.txt | ./cst.out -D 2 -a 4 -T 0  <-- 2-D 16-node mesh
//
// % less test.txt
// 3  4    <--- path from node 3 to node 4
// 2  4 
//
// Usage: ./traffic_pattern_generator.out -t 0 -n 4 | ./circuit-switch-table.out -D 2 -a 4 -T 0 
//
// Wed Sep 06 19:24:42 JST 2018 huyao@nii.ac.jp
//
// This file estimates # of slots and generates routing table for each switch
//

#include <unistd.h> // getopt 
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>
#include <math.h>

#include <fstream> //file output
#include <sstream> //string operation

#include <thread>
#include <unistd.h> //sleep for secs
#include <time.h>

using namespace std;

//
// Communication node pair
//
struct Pair {
   // pair id (unique, from 0, 1, 2, ...)  
   int pair_id;
   // flow id (not unique)
   int flow_id; 
   // channels
   vector<int> channels;
   // source and destination
   int src; int dst;
   int h_src; int h_dst;
   // assigned ID (different from pair_id)
   int ID;
   // ID is valid or not
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
// Channels
//
struct Cross_Paths {
   // Communication node pair index going through a channel
   vector<int> pair_index;
   // list of ID in Pair for a channel
   vector<int> assigned_list;
   // list of h_dst in Pair for a channel
   vector<int> assigned_dst_list;
   // If all IDs are assigned
   bool Valid;
   // initialize
   Cross_Paths():Valid(false){}
   // routing table
   vector<int> routing_table;  // input port, slot number, src node, dst node, ...

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
// Job
//
struct Job {   
   int time_submit; // submit time in workload
   int time_run; // runtime in workload
   int num_nodes; // number of nodes required
   //vector<int> src_dst_pair; // pairs of src and dst in workload
   //vector<int> src_dst_pair_m; // pairs of src and dst after dispatched
   vector<Pair> src_dst_pairs;
   vector<Pair> src_dst_pairs_m;
   int job_id; // job id (unique)
   time_t time_submit_r; // real submit time
   time_t time_dispatch_r; // real dispatch time
   vector<int> nodes; // occupied system nodes
};

bool LSF(Job a, Job b) { return (a.num_nodes > b.num_nodes); } //large size first
bool SSF(Job a, Job b) { return (a.num_nodes < b.num_nodes); } //small size first
bool LRF(Job a, Job b) { return (a.time_run > b.time_run); } //large runtime first
bool SRF(Job a, Job b) { return (a.time_run < b.time_run); } //small runtime first

//
// output and error check (mesh/torus)
//
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
int max_id, vector<Pair> pairs, int hops, int Vch, int Host_Num, int max_cp, int max_cp_dst,
bool path_based, int degree, int default_slot)
{
   // for each channel
   //cout << " === Port information for each switch === " << endl;
   for (int i=0; i < Vch*(degree+1+2*Host_Num)*switch_num; i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
      //if (i%(Vch*(degree+1+2*Host_Num)) == 0) cout << " SW " << i/(Vch*(degree+1+2*Host_Num)) << " : " << endl;
// for each node pair passing through a channel
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);

        //  if (i%(degree+1+2*Host_Num) == degree+1+2*Host_Num -2 ){
        //          cout << "      Port 0 (from localhost) --> Pair ID " << pairs[t].pair_id << " (Slot " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
        //  }
        //  else if (i%(degree+1+2*Host_Num) == degree+1+2*Host_Num -1 ){
        //          cout << "      Port 0 (to localhost) --> Pair ID " << pairs[t].pair_id << " (Slot " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
        //  }
        //  else {
        //          cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (Slot " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
        //  }
         
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
	 cout << " ERROR : Slot # collision is occured!!." << endl;      
	 exit (1);
      }
   }

   // output
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;
   //int slots = 0; 
   //int pointer = 0; 
   //int total_slots = 0; 
   if (Vch == 0)
	cout << " (east, west, south, north, front, back, from host0,... to host0,...)"; 
   else
   	//cout << " (east, west, south, north, front, back, from host0,... east(vch2), south(vch2), west(vch2), north(vch2), front, back, from host(vch2)0,.. to host(vch2)0,.."
	cout << " === Number of slots === " << endl;
        //cout << " East, West, South, North, (Back, Front, ...) Out, In " << endl;
        cout << " X1, X2, (X3, X4, X5, X6, X7, X8, ...), Out, In " << endl;

   	cout << " SW " << setw(2) << port/((degree+1+2*Host_Num)*Vch) << ":  ";    while ( elem != Crossing_Paths.end() ){
      if (port%(degree+1+2*Host_Num)!=0) cout << " " << (*elem).pair_index.size();
      //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();} 

      //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

      elem ++; port++;
      if ( port%((degree+1+2*Host_Num)*Vch) == 0){
         //pointer = 0; 
	 cout << endl;		
	 if ( port != Vch*(degree+1+2*Host_Num)*switch_num)
	    cout << " SW " << setw(2) << port/((degree+1+2*Host_Num)*Vch) << ":  ";	
      }
   }
	//cout << endl; 
	// setting for comparing (maximum) Cross_Path 
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
	while ( pt != Crossing_Paths.end() ){
    	(*pt).Valid = true;
        ++pt;
	}


        // test
        // for (int i=0; i<Crossing_Paths[16].assigned_list.size(); i++){
        //     cout << Crossing_Paths[16].assigned_list[i] << endl;    
        // }
        // for (int i=0; i<pairs[0].channels.size(); i++){
        //      cout << pairs[0].channels[i] << endl;    
        // }     


   //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
   cout << " === The number of paths on this application ===" << endl << ct << " (all-to-all cases: " << (switch_num*Host_Num)*(switch_num*Host_Num-1) << ")" << endl;
   cout << " === The average hops ===" << endl << setiosflags(ios::fixed | ios::showpoint) << (float)hops/ct+1 << endl;
   //cout << " ID size(without ID modification)" << max_id << endl;
   //cout << " (Maximum) number of slots: " << slots;

   // routing table file output for each sw   
   int target_sw; // switch file to be written, sw0, sw1, ...
   int input_port; // of target_sw
   int output_port; // of target_sw
   int slot_num; // assigned slot number for a node pair
   string output_port_s; // string
   string input_port_s; // string
   string slot_num_s; // string
   system("rm output/sw*");  // delete previous output results
   cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
    for (int i=0; i < pairs.size(); i++){
            Pair current_pair = pairs[i];
            slot_num = current_pair.ID;
            //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
            cout << " Pair ID " << current_pair.pair_id << ": " << endl;
            for (int j=1; j < current_pair.channels.size(); j++){ //current_pair.channels[0] --> src, current_pair.channels[current_pair.channels.size()-1] --> dst
                    target_sw = -1;
                    input_port = 0;
                    output_port = 0;
                    if (j == 1){ // source switch
                            target_sw = current_pair.src;
                            output_port = current_pair.channels[j]%(degree+1+2*Host_Num);
                            //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                            cout << "   SW " << target_sw << " (port " << input_port << "->" << output_port << ")" << " - [slot " << slot_num << "] -> ";
                    }
                    else if (j == current_pair.channels.size()-1){ // destination switch
                            target_sw = current_pair.dst;
                            input_port = current_pair.channels[j-1]%(degree+1+2*Host_Num);
                            if (input_port%2 == 0){
                                    input_port--; // 2-->1, 4-->3, ...
                            }
                            else{
                                    input_port++; // 1-->2, 3-->4, ...
                            }
                            //cout << "SW " << target_sw;
                            cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                    }
                    else{
                            target_sw = current_pair.channels[j]/((degree+1+2*Host_Num)*Vch);
                            output_port = current_pair.channels[j]%(degree+1+2*Host_Num);
                            input_port = current_pair.channels[j-1]%(degree+1+2*Host_Num);
                            if (input_port%2 == 0){
                                    input_port--; // 2-->1, 4-->3, ...
                            }
                            else{
                                    input_port++; // 1-->2, 3-->4, ...
                            }
                            //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                            cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")" << " - [slot " << slot_num << "] -> ";
                    }
                //     char filename[100]; 
                //     sprintf(filename, "output/sw%d", target_sw); // save to output/ 
                //     char* fn = filename;
                //     ofstream outputfile(fn, ios::app); // iostream append
                //     stringstream ss_op;  // output port
                //     stringstream ss_ip;  // input port
                //     stringstream ss_sn;  // slot number
                //     output_port_s = "";
                //     input_port_s = "";
                //     slot_num_s = "";
                //     if (output_port < 10 && output_port > -1){
                //             ss_op << "0" << output_port;
                //             output_port_s = ss_op.str();
                //     }
                //     else{
                //             ss_op << output_port;
                //             output_port_s = ss_op.str();
                //     }
                //     if (input_port < 10 && input_port > -1){
                //             ss_ip << "0" << input_port;
                //             input_port_s = ss_ip.str();
                //     }
                //     else{
                //             ss_ip << input_port;
                //             input_port_s = ss_ip.str();
                //     }
                //     if (slot_num < 10 && slot_num > -1){
                //             ss_sn << "0" << slot_num;
                //             slot_num_s = ss_sn.str();
                //     }
                //     else{
                //             ss_sn << slot_num;
                //             slot_num_s = ss_sn.str();
                //     }
                //     outputfile << output_port_s << slot_num_s << " " << input_port_s << slot_num_s <<"  // from node "<< current_pair.h_src << " to node " << current_pair.h_dst << endl;
                //     outputfile.close();

                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port); // routing table <-- input port
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);  // routing table <-- slot number
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id);  // routing table <-- pair id

            }
            cout << endl;
    }

   cout << " === Port information for each switch === " << endl;
   for (int i=0; i < switch_num; i++){
        cout << " SW " << i << " : " << endl;
        char filename[100]; 
        sprintf(filename, "output/sw%d", i); // save to output/ 
        char* fn = filename;
        ofstream outputfile(fn, ios::app); // iostream append   
        int slots;
        if (path_based == true){
                slots = max_cp;
        } 
        else{
                slots = max_cp_dst;
        }
        if (slots > default_slot){
                cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;      
                exit (1);
        }
        outputfile << degree+1 << " " << default_slot << endl;  // number of output ports, number of slots
        outputfile << "0000" << endl;  // output 00 --> localhost
        bool slot_occupied = false;
        for (int s=0; s < slots; s++){
                if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size() > 0){
                        for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size(); j=j+5){
                                if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j] << " ";
                                        slot_occupied = true;
                                        cout << "      Port " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j] << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] << ") --> Port 0 (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] << "), from node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+2] << " to node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                }  
                        }        
                }
                else if (Vch==2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size() > 0){ //torus
                        for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size(); j=j+5){
                                if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j] << " ";
                                        slot_occupied = true;
                                        cout << "      Port " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j] << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] << ") --> Port 0 (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] << "), from node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+2] << " to node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                }
                        }        
                }
                if (slot_occupied == false){
                        outputfile << "void";
                }
                outputfile << endl;   
                slot_occupied = false;             
        }
        for (int s=0; s<default_slot-slots; s++) outputfile << "void" << endl; 

        // if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size() > 0){
        //         for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size(); j++){
        //                 outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j] << " ";
        //                 if ((j+1)%4 == 0) outputfile << endl;
        //         }        
        // }
        // else if (Vch == 2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size() > 0){ //torus
        //         for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size(); j++){
        //                 outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j] << " ";
        //                 if ((j+1)%4 == 0) outputfile << endl;
        //         }        
        // }
        for (int op=1; op < degree+1; op++){
                if (op < 10 && op > -1){
                        outputfile << "0" << op << "00" << endl;
                }
                else{
                        outputfile << op << "00" << endl;
                }  
                for (int s=0; s < slots; s++){
                        if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table.size() > 0){
                                for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table.size(); j=j+5){
                                        if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j] << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j+1] << ") --> Port " << op << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j+1] << "), from node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j+2] << " to node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                        }  
                                }        
                        }
                        else if (Vch==2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size() > 0){ //torus
                                for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size(); j=j+5){
                                        if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j] << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] << ") --> Port " << op << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] << "), from node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+2] << " to node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                        }
                                }        
                        }
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                }
                for (int s=0; s<default_slot-slots; s++) outputfile << "void" << endl; 

                // if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table.size() > 0){
                //         for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table.size(); j++){
                //                 outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j] << " ";
                //                 if ((j+1)%4 == 0) outputfile << endl;
                //         }        
                // }
                // else if (Vch == 2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size() > 0){ //torus
                //         for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size(); j++){
                //                 outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j] << " ";
                //                 if ((j+1)%4 == 0) outputfile << endl;
                //         }        
                // }

        }
        outputfile.close();
   }
    cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
    cout << " ### OVER ###" << endl;
}

//
// output and error check (fat-tree)
//
void show_paths_tree (vector<Cross_Paths> Crossing_Paths, int ct, int node_num, \
int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
bool path_based, int PORT, int default_slot)
{
        // for each channel
        //cout << " === Port information for each switch === " << endl;
        for (int i=0; i <= PORT*(node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)); i++){
        vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
        //if (i%PORT == 0) {
        //        if ( i == PORT * (node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)) && node_num/(int)pow(Host_Num,2) <= 1 ) break; // two-layer switches
        //        cout << " Node " << i/PORT << " : " << endl;                
        //}
// for each node pair passing through a channel
        for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
                int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
                ID_array.push_back(pairs[t].ID);

                // if (i%PORT == 0){
                //         cout << "      Port " << i%PORT << " (UP) --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
                // }
                // else{
                //         cout << "      Port " << i%PORT << " (DOWN) --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;         
                // }
                
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
                cout << " ERROR : Slot # collision is occured!!." << endl;      
                exit (1);
        }
        }

        // output
        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
        int outport = 0;
        cout << " === Number of slots === " << endl;
        cout << " UP0, DOWN1, DOWN2, DOWN3, DOWN4, ... " << endl;
        cout << " Node " << setw(2) << outport/PORT << ":  ";    
        while ( elem != Crossing_Paths.end() ){
                cout << " " << (*elem).pair_index.size();
                elem ++; outport++;
                if ( outport%PORT == 0 && elem != Crossing_Paths.end() ){
                        if ( outport == PORT * (node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)) && node_num/(int)pow(Host_Num,2) <= 1 ) break; // two-layer switches
                        cout << endl;		
                        cout << " Node " << setw(2) << outport/PORT << ":  ";	
                }
        }
        cout << endl; 

        // setting for comparing (maximum) Cross_Path 
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
        while ( pt != Crossing_Paths.end() ){
                (*pt).Valid = true;
        ++pt;
        }

        //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
        //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
        cout << " === The number of paths on this application ===" << endl << ct << " (all-to-all cases: " << node_num*(node_num-1) << ")" << endl;
        cout << " === The average hops ===" << endl << setiosflags(ios::fixed | ios::showpoint) << (float)hops/ct+1 << endl;
        //cout << " ID size(without ID modification)" << max_id << endl;
        //cout << " (Maximum) number of slots: " << slots;

        // routing table file output for each sw   
        int target_sw; // switch file to be written, sw0, sw1, ...
        int input_port; // of target_sw
        int output_port; // of target_sw
        int slot_num; // assigned slot number for a node pair
        string output_port_s; // string
        string input_port_s; // string
        string slot_num_s; // string
        system("rm output/sw*");  // delete previous output results
        cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
        for (int i=0; i < pairs.size(); i++){
                Pair current_pair = pairs[i];
                slot_num = current_pair.ID;
                //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
                cout << " Pair ID " << current_pair.pair_id << ": " << endl;
                for (int j=1; j < current_pair.channels.size(); j++){ //current_pair.channels[0] --> from src node, current_pair.channels[-1] --> to dst node
                        target_sw = -1;
                        input_port = -1;
                        output_port = -1;

                        target_sw = current_pair.channels[j]/PORT - node_num;
                        output_port = current_pair.channels[j]%PORT;
                        input_port = current_pair.channels[j-1]%PORT;
                        if (input_port != 0){ // down --> down
                                input_port = 0; 
                        }
                        else{
                                if (output_port != 0){ // up --> down 
                                        input_port = (current_pair.channels[j-1]/PORT)%Host_Num + 1;
                                }
                                else{ // up --> up
                                        input_port = (current_pair.channels[j-1]/PORT)%Host_Num + 1;
                                }
                        }
                        if (j == 1){ //src node
                                //cout << "Node " << current_pair.h_src << " (port 0)" << " --> ";
                                cout << "   Node " << current_pair.h_src << " - [slot " << slot_num << "] -> ";
                        }
                        //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                        cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")" << " - [slot " << slot_num << "] -> ";
                        if (j == current_pair.channels.size() - 1){ //dst node
                                //cout << "Node " << current_pair.h_dst << " (port 0)";
                                cout << "Node " << current_pair.h_dst;
                        }                        

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port); // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);  // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id);  // routing table <-- pair id

                }
                cout << endl;
        }

        int switch_num;
        if (node_num/(int)pow(Host_Num,2) <= 1){
                switch_num = node_num/Host_Num+node_num/(int)pow(Host_Num,2);
        }
        else{
                switch_num = node_num/Host_Num+node_num/(int)pow(Host_Num,2) + 1;
        }
        
        cout << " === Port information for each switch === " << endl;
        for (int i=0; i < switch_num; i++){
        cout << " SW " << i << " : " << endl;
        char filename[100]; 
        sprintf(filename, "output/sw%d", i); // save to output/ 
        char* fn = filename;
        ofstream outputfile(fn, ios::app); // iostream append   
        int slots;
        if (path_based == true){
                slots = max_cp;
        } 
        else{
                slots = max_cp_dst;
        }
        if (slots > default_slot){
                cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;      
                exit (1);
        }
        outputfile << PORT << " " << default_slot << endl;  // number of output ports, number of slots

        bool slot_occupied = false;
        for (int op=0; op < PORT; op++){
                if (op < 10 && op > -1){
                        outputfile << "0" << op << "00" << endl;
                }
                else{
                        outputfile << op << "00" << endl;
                }  
                for (int s=0; s < slots; s++){
                        for (int j=0; j < Crossing_Paths[PORT*(i+node_num)+op].routing_table.size(); j=j+5){ //i+node_num --> sw #
                                if (Crossing_Paths[PORT*(i+node_num)+op].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j] << " "; // j --> input port
                                        slot_occupied = true;
                                        cout << "      Port " << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j] << " (Slot " << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j+1] << ") --> Port " << op << " (Slot " << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j+1] << "), from node " << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j+2] << " to node " << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                }  
                        }        
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                }
                for (int s=0; s<default_slot-slots; s++) outputfile << "void" << endl; 
        }
        outputfile.close();
        }
        cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
        cout << " ### OVER ###" << endl;
}

//
// output and error check (fully-connected)
//
void show_paths_fullyconnected (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
bool path_based, int degree, int default_slot)
{
   // for each channel
   //cout << " === Port information for each switch === " << endl;
   for (int i=0; i < (degree+1+2*Host_Num)*switch_num; i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
      //if (i%(degree+1+2*Host_Num) == 0) cout << " SW " << i/(degree+1+2*Host_Num) << " : " << endl;
// for each node pair passing through a channel
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);

         //if ( (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -2) && (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -1) ) { // -2 and -1 --> no meaning
         //        cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
         //}
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
	 cout << " ERROR : Slot # collision is occured!!." << endl;      
	 exit (1);
      }
   }

   // output
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;
   //int slots = 0; 
   //int pointer = 0; 
   //int total_slots = 0; 

        cout << " === Number of slots === " << endl;
        //cout << " East, West, South, North, (Back, Front, ...) Out, In " << endl;
        cout << " X1, X2, X3, X4, X5, X6, X7, X8, ... " << endl;

        cout << " SW " << setw(2) << port/(degree+1+2*Host_Num) << ":  ";    while ( elem != Crossing_Paths.end() ){
      if (port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1) cout << " " << (*elem).pair_index.size();
      //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();} 

      //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

      elem ++; port++;
      if ( port%(degree+1+2*Host_Num) == 0){
         //pointer = 0; 
	 cout << endl;		
	 if ( port != (degree+1+2*Host_Num)*switch_num)
	    cout << " SW " << setw(2) << port/(degree+1+2*Host_Num) << ":  ";	
      }
   }
	//cout << endl; 
	// setting for comparing (maximum) Cross_Path 
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
	while ( pt != Crossing_Paths.end() ){
    	(*pt).Valid = true;
        ++pt;
	}

   //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
   cout << " === The number of paths on this application ===" << endl << ct << " (all-to-all cases: " << (switch_num*Host_Num)*(switch_num*Host_Num-1) << ")" << endl;
   cout << " === The average hops ===" << endl << setiosflags(ios::fixed | ios::showpoint) << (float)hops/ct+1 << endl;
   //cout << " ID size(without ID modification)" << max_id << endl;
   //cout << " (Maximum) number of slots: " << slots;

   // routing table file output for each sw   
   int target_sw; // switch file to be written, sw0, sw1, ...
   int input_port; // of target_sw
   int output_port; // of target_sw
   int slot_num; // assigned slot number for a node pair
   system("rm output/sw*");  // delete previous output results
   cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
    for (int i=0; i < pairs.size(); i++){
            Pair current_pair = pairs[i];
            slot_num = current_pair.ID;
            //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
            cout << " Pair ID " << current_pair.pair_id << ": " << endl;
            for (int j=0; j < current_pair.channels.size(); j++){ // only current_pair.channels[0] --> from src to dst
                    target_sw = -1;
                    input_port = 0;
                    output_port = 0;
                    if (j == 0){ // source switch
                        target_sw = current_pair.src;
                        output_port = current_pair.channels[j]%(degree+1+2*Host_Num); // output_port = current_pair.dst
                        //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> SW " << current_pair.dst;
                        input_port = current_pair.src;
                        cout << "       SW " << target_sw << " (port " << input_port << "->" << output_port << ")" << " - [slot " << slot_num << "] -> ";
                    }

                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port); // routing table <-- input port
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);  // routing table <-- slot number
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id);  // routing table <-- pair id

                    target_sw = current_pair.dst;
                    input_port = current_pair.src;
                    output_port = current_pair.dst; // localhost
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(input_port); // routing table <-- input port
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(slot_num);  // routing table <-- slot number
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(current_pair.pair_id);  // routing table <-- pair id
                    
                    cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                }
            cout << endl;
    }

   cout << " === Port information for each switch === " << endl;
   for (int i=0; i < switch_num; i++){
        cout << " SW " << i << " : " << endl;
        char filename[100]; 
        sprintf(filename, "output/sw%d", i); // save to output/ 
        char* fn = filename;
        ofstream outputfile(fn, ios::app); // iostream append   
        int slots;
        if (path_based == true){
                slots = max_cp;
        } 
        else{
                slots = max_cp_dst;
        }
        if (slots > default_slot){
                cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;      
                exit (1);
        }
        outputfile << degree+1 << " " << default_slot << endl;  // number of output ports, number of slots
        bool slot_occupied = false;
        for (int op=0; op < degree+1; op++){
                if (op < 10 && op > -1){
                        outputfile << "0" << op << "00" << endl;
                }
                else{
                        outputfile << op << "00" << endl;
                }  
                for (int s=0; s < slots; s++){
                        if (Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table.size() > 0){
                                for (int j=0; j < Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table.size(); j=j+5){
                                        if (Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j] << " (Slot " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] << ") --> Port " << op << " (Slot " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] << "), from node " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+2] << " to node " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                        }  
                                }        
                        }
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                } 
                for (int s=0; s<default_slot-slots; s++) outputfile << "void" << endl;                                                   
        }
        outputfile.close();
   }
    cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
    cout << " ### OVER ###" << endl;
}

//
// output and error check (fcc)
//  
void show_paths_fcc (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
bool path_based, int degree, int default_slot)
{
   // for each channel
   //cout << " === Port information for each switch === " << endl;
   for (int i=0; i < (degree+1+2*Host_Num)*switch_num; i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
      //if (i%(degree+1+2*Host_Num) == 0) cout << " SW " << i/(degree+1+2*Host_Num) << " : " << endl;
// for each node pair passing through a channel
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);

         //if ( (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -2) && (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -1) ) { // -2 and -1 --> no meaning
         //        cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
         //}
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
	 cout << " ERROR : Slot # collision is occured!!." << endl;      
	 exit (1);
      }
   }

   // output
   vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
   int port = 0;
   //int slots = 0; 
   //int pointer = 0; 
   //int total_slots = 0; 

        cout << " === Number of slots === " << endl;
        //cout << " East, West, South, North, (Back, Front, ...) Out, In " << endl;
        cout << " backward, forward, inter-group1, inter-group1, inter-group2, inter-group3, inter-group4, inter-group5, inter-group6, ... " << endl;

        cout << " SW " << setw(2) << port/(degree+1+2*Host_Num) << ":  ";    while ( elem != Crossing_Paths.end() ){
      if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1) cout << " " << (*elem).pair_index.size();
      //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();} 

      //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

      elem ++; port++;
      if ( port%(degree+1+2*Host_Num) == 0){
         //pointer = 0; 
	 cout << endl;		
	 if ( port != (degree+1+2*Host_Num)*switch_num)
	    cout << " SW " << setw(2) << port/(degree+1+2*Host_Num) << ":  ";	
      }
   }
	//cout << endl; 
	// setting for comparing (maximum) Cross_Path 
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
	while ( pt != Crossing_Paths.end() ){
    	(*pt).Valid = true;
        ++pt;
	}

   //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
   //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
   cout << " === The number of paths on this application ===" << endl << ct << " (all-to-all cases: " << (switch_num*Host_Num)*(switch_num*Host_Num-1) << ")" << endl;
   cout << " === The average hops ===" << endl << setiosflags(ios::fixed | ios::showpoint) << (float)hops/ct+1 << endl;
   //cout << " ID size(without ID modification)" << max_id << endl;
   //cout << " (Maximum) number of slots: " << slots;

   // routing table file output for each sw   
   int target_sw; // switch file to be written, sw0, sw1, ...
   int input_port; // of target_sw
   int output_port; // of target_sw
   int slot_num; // assigned slot number for a node pair
   system("rm output/sw*");  // delete previous output results
   cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
    for (int i=0; i < pairs.size(); i++){
            Pair current_pair = pairs[i];
            slot_num = current_pair.ID;
            //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
            cout << " Pair ID " << current_pair.pair_id << ": " << endl;
            for (int j=1; j < current_pair.channels.size(); j++){ //current_pair.channels[0] --> src, current_pair.channels[current_pair.channels.size()-1] --> dst
                    target_sw = -1;
                    input_port = 0;
                    output_port = 0;
                    if (j == 1){ // source switch
                            target_sw = current_pair.src;
                            output_port = current_pair.channels[j]%(degree+1+2*Host_Num);
                            //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                            cout << "   SW " << target_sw << " (port " << input_port << "->" << output_port << ")" << " - [slot " << slot_num << "] -> ";
                    }
                    else if (j == current_pair.channels.size()-1){ // destination switch
                            target_sw = current_pair.dst;
                            input_port = current_pair.channels[j-1]%(degree+1+2*Host_Num);
                            if (input_port==1) input_port=2;
                            else if (input_port==2) input_port=1;
                            else input_port = (3+degree)-input_port;
                            //cout << "SW " << target_sw;
                            cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                    }
                    else{
                            target_sw = current_pair.channels[j]/(degree+1+2*Host_Num);
                            output_port = current_pair.channels[j]%(degree+1+2*Host_Num);
                            input_port = current_pair.channels[j-1]%(degree+1+2*Host_Num);
                            if (input_port==1) input_port=2;
                            else if (input_port==2) input_port=1;
                            else input_port = (3+degree)-input_port;
                            //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                            cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")" << " - [slot " << slot_num << "] -> ";
                    }
                //     char filename[100]; 
                //     sprintf(filename, "output/sw%d", target_sw); // save to output/ 
                //     char* fn = filename;
                //     ofstream outputfile(fn, ios::app); // iostream append
                //     stringstream ss_op;  // output port
                //     stringstream ss_ip;  // input port
                //     stringstream ss_sn;  // slot number
                //     output_port_s = "";
                //     input_port_s = "";
                //     slot_num_s = "";
                //     if (output_port < 10 && output_port > -1){
                //             ss_op << "0" << output_port;
                //             output_port_s = ss_op.str();
                //     }
                //     else{
                //             ss_op << output_port;
                //             output_port_s = ss_op.str();
                //     }
                //     if (input_port < 10 && input_port > -1){
                //             ss_ip << "0" << input_port;
                //             input_port_s = ss_ip.str();
                //     }
                //     else{
                //             ss_ip << input_port;
                //             input_port_s = ss_ip.str();
                //     }
                //     if (slot_num < 10 && slot_num > -1){
                //             ss_sn << "0" << slot_num;
                //             slot_num_s = ss_sn.str();
                //     }
                //     else{
                //             ss_sn << slot_num;
                //             slot_num_s = ss_sn.str();
                //     }
                //     outputfile << output_port_s << slot_num_s << " " << input_port_s << slot_num_s <<"  // from node "<< current_pair.h_src << " to node " << current_pair.h_dst << endl;
                //     outputfile.close();

                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port); // routing table <-- input port
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);  // routing table <-- slot number
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id);  // routing table <-- pair id

            }
            cout << endl;
    }

   cout << " === Port information for each switch === " << endl;
   for (int i=0; i < switch_num; i++){
        cout << " SW " << i << " : " << endl;
        char filename[100]; 
        sprintf(filename, "output/sw%d", i); // save to output/ 
        char* fn = filename;
        ofstream outputfile(fn, ios::app); // iostream append   
        int slots;
        if (path_based == true){
                slots = max_cp;
        } 
        else{
                slots = max_cp_dst;
        }
        if (slots > default_slot){
                cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;      
                exit (1);
        }
        outputfile << degree+1 << " " << default_slot << endl;  // number of output ports, number of slots
        outputfile << "0000" << endl;  // output 00 --> localhost
        bool slot_occupied = false;
        for (int s=0; s < slots; s++){
                if (Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size() > 0){
                        for (int j=0; j < Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size(); j=j+5){
                                if (Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j] << " ";
                                        slot_occupied = true;
                                        cout << "      Port " << Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j] << " (Slot " << Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] << ") --> Port 0 (Slot " << Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] << "), from node " << Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+2] << " to node " << Crossing_Paths[(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                }  
                        }        
                }
                if (slot_occupied == false){
                        outputfile << "void";
                }
                outputfile << endl;   
                slot_occupied = false;             
        }
        for (int s=0; s<default_slot-slots; s++) outputfile << "void" << endl; 

        for (int op=1; op < degree+1; op++){
                if (op < 10 && op > -1){
                        outputfile << "0" << op << "00" << endl;
                }
                else{
                        outputfile << op << "00" << endl;
                }  
                for (int s=0; s < slots; s++){
                        if (Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table.size() > 0){
                                for (int j=0; j < Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table.size(); j=j+5){
                                        if (Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j] << " (Slot " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] << ") --> Port " << op << " (Slot " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] << "), from node " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+2] << " to node " << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                        }  
                                }        
                        }
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                }
                for (int s=0; s<default_slot-slots; s++) outputfile << "void" << endl; 

        }
        outputfile.close();
   }
    cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
    cout << " ### OVER ###" << endl;
}

// 
// Detect crossing_paths of ecube routing on fat-tree.
//
int main(int argc, char *argv[])
{
   // side length of mesh/torus
   static int array_size = 4;
   // ID Allocation policy
   static int Allocation = 1; // 0:low port first， 1:Crossing Paths based method
   // number of hosts for each router
   static int Host_Num = 1;
   // The number of VCHs
   static int Vch = 1; // Mesh:1, Torus:2-->1
   static int Topology = 0; // Mesh:0, Torus:1, Fat-tree:2, fully-connected:3, full-mesh-connected-circles(fcc):4
   int c;
   static bool path_based = false; //false:destination_based (slot # not updated), true:path_based (slot # updated)
   static int degree = 4; // (mesh or torus) degree = 2 * dimension, (fully-connected) degree = switch_num -1, (fcc) degree = inter-group + intra-group (2 for ring)
   static int dimension = 2; //mesh or torus

   static int group_switch_num = 4; //fcc
   static int inter_group = 6; //fcc
   static int intra_group = 2; //fcc

   static int queue_policy = 0; //fcfs:0, lsf:1, ssf:2, lrf:3, srf:4
   
   while((c = getopt(argc, argv, "a:A:n:T:uD:d:m:q:")) != -1) {
      switch (c) {
      case 'a':
	 array_size = atoi(optarg);
	 break;
        /*case 'A':
	 Allocation = atoi(optarg);	
	 break;*/
	/*  case 'v':
	 Vch = atoi(optarg);	
	 break; */
      case 'T':
	 Topology = atoi(optarg);	
	 break;
      case 'n':
	 Host_Num = atoi(optarg);	
	 break;
      case 'u':
	 path_based = true;
	 break;
      case 'D':
         dimension = atoi(optarg);
	 break;
      case 'd': //fcc switch ports (not include host)
         inter_group = atoi(optarg) - intra_group;
	 break;      
      case 'm': //fcc # of switches in one group
         group_switch_num = atoi(optarg);
	 break;    
      case 'q': //queuing policy
         queue_policy = atoi(optarg);
	 break;              
      default:
	 //usage(argv[0]);
	 cout << " This option is not supported. " << endl;
	 return EXIT_FAILURE;
      }
   }

   if (dimension > 4 || dimension < 1){
        cerr << " Please input -D $dimension (1 <= $dimension <= 4)" << endl;
        exit (1);   
   }

   if (Topology == 0 || Topology == 1){ // mesh or torus
        degree = 2 * dimension;
   }

   if (Topology==1) Vch=1; //Torus(2-->1)
   else if (Topology==0) Vch=1;//Mesh
   /*else{ 
      cerr << " The combination of Topology and Vchs is wrong!!" << endl;
      exit (1);
   }*/

   //cout << " ID Allocation Policy is 0(low port first) / 1(Crossing Paths based method): " << Allocation << endl;
   //cout << " Address method is 0(destination_based) / 1(path_based): " 
   cout << " ### start ###" << endl;
   cout << " === Update of slot number ===" << endl << " 0 (no) / 1 (yes): " <<  path_based << " (Use -u to activate the update) " << endl;
   
   // source and destination		
   int src = -1, dst = -1, h_src = -1, h_dst = -1;
   // number of nodes
   static int switch_num = pow(array_size,dimension); //mesh or torus or fully-connected or fcc
   static int node_num = pow(array_size,dimension); //fat-tree or fcc
   static int PORT = Host_Num + 1; //fat-tree

   static int groups = inter_group*group_switch_num+1; //fcc 25
 
   // Crossing Paths
   // including Host <-> Switch

   if (Topology == 3){ // fully-connected
        degree = switch_num - 1;
   }

   if (Topology == 4){ // fcc
        degree = inter_group + intra_group; //6 inter-group, 2 intra-group
        switch_num = groups*group_switch_num; //100
        node_num = switch_num*Host_Num; //100
   }

   static int ports;
   if (Topology == 0 || Topology == 1){ //mesh or torus
        ports = (degree+1+2*Host_Num)*switch_num*Vch;
   }
   if (Topology == 2){ //fat-tree
        ports = PORT * (node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)+1);
   }
   if (Topology == 3){ //fully-connected
        ports = (degree+1+2*Host_Num)*switch_num; // localhost port = switch ID
   }
   if (Topology == 4){ //fcc
        ports = (degree+1+2*Host_Num)*switch_num; // 11*100=1100 localhost port = 0
   }   
   vector<Cross_Paths> Crossing_Paths(ports);

   // switch connection initiation (fcc)
   // 0:not used, 1:left, 2:right, 3-8:inter-group
   vector<int> Switch_Topo(ports);

   // total number of node pairs
   int ct = 0;
   // total number of hops 
   int hops = 0; 

   int before_hops = 0; //fat-tree or fcc

   // node pairs
   vector<Pair> pairs;

   cout << " === topology ===" << endl;
   if (Topology == 0) cout << dimension << "-D mesh (" << switch_num << " switches/nodes)" << endl;
   else if (Topology == 1) cout << dimension << "-D torus (" << switch_num << " switches/nodes)" << endl;
   else if (Topology == 2) 
        if (node_num/(int)pow(Host_Num,2) == 1) cout << "fat tree (" << node_num << " nodes + " << node_num/Host_Num+node_num/(int)pow(Host_Num,2) << " switches)" << endl;
        else cout << "fat tree (" << node_num << " nodes + " << node_num/Host_Num+node_num/(int)pow(Host_Num,2)+1 << " switches)" << endl;
   else if (Topology == 3) cout << "fully connected (" << switch_num << " switches/nodes)" << endl;
   else if (Topology == 4) cout << "full mesh connected circles (" << switch_num << " switches / "<< node_num << " nodes, " << groups << " groups, " << group_switch_num << " switches/group, degree = " << degree << ")" << endl;
   else cout << "Error: please specify -T [0-3] (0 mesh, 1 torus, 2 fat tree, 3 fully connected, 4 full mesh connected circles)" << endl;

   // ########################################## //
   // ##############   PHASE 0   ############### //
   // ##        workload loading       ## //
   // ########################################## //

   //node is available (0) or unavailable (1)
   vector<int> sys(node_num);

   int temp_time_submit, temp_time_run;
   int temp_num_nodes, temp_pair_src, temp_pair_dst, temp_flow_id, temp_job_id;
   int pre_job_id = -1;

   vector<Job> all_jobs;
   vector<Job> queue;

   int pairID = 0;
   while (cin >> temp_time_submit >> temp_time_run >> temp_num_nodes >> temp_pair_src >> temp_pair_dst >> temp_flow_id >> temp_job_id){
        if (temp_job_id != pre_job_id){
                Job j;
                j.time_submit = temp_time_submit;
                j.time_run = temp_time_run;
                j.num_nodes = temp_num_nodes;

                // j.src_dst_pair.push_back(temp_pair_src);
                // j.src_dst_pair.push_back(temp_pair_dst);
                // j.src_dst_pair.push_back(temp_flow_id);
                // j.src_dst_pair.push_back(pairID);
                // pairID++;
                // j.src_dst_pair.push_back(-1); // slot #
                int h_src = temp_pair_src;
                int h_dst = temp_pair_dst;
                int src = h_src/Host_Num;
                int dst = h_dst/Host_Num;
                if(Topology == 2){ // fat-tree
                     src = h_src;
                     dst = h_dst;   
                }
                Pair pair(src,dst,h_src,h_dst);
                pair.pair_id = pairID;
                pairID++;
                pair.flow_id = temp_flow_id;
                j.src_dst_pairs.push_back(pair);

                j.job_id = temp_job_id;
                j.time_submit_r = -1;
                j.time_dispatch_r = -1;
                all_jobs.push_back(j);
                pre_job_id = temp_job_id;
        }
        else{
                //src0, dst0, flow id0, pairID0, slot #0, src1, dst1, flow id1, pairID1, slot #1, ...
                // all_jobs[all_jobs.size()-1].src_dst_pair.push_back(temp_pair_src);
                // all_jobs[all_jobs.size()-1].src_dst_pair.push_back(temp_pair_dst);
                // all_jobs[all_jobs.size()-1].src_dst_pair.push_back(temp_flow_id);
                // all_jobs[all_jobs.size()-1].src_dst_pair.push_back(pairID);
                // pairID++;
                // all_jobs[all_jobs.size()-1].src_dst_pair.push_back(-1); // slot #
                int h_src = temp_pair_src;
                int h_dst = temp_pair_dst;
                int src = h_src/Host_Num;
                int dst = h_dst/Host_Num;
                if(Topology == 2){ // fat-tree
                     src = h_src;
                     dst = h_dst;   
                }
                Pair pair(src,dst,h_src,h_dst);
                pair.pair_id = pairID;
                pairID++;
                pair.flow_id = temp_flow_id;
                all_jobs[all_jobs.size()-1].src_dst_pairs.push_back(pair);                
        }

   }

   //job submit thread
   bool all_submitted = false;
   thread sj{submit_jobs, all_jobs, queue, queue_policy, all_submitted};
   sj.join();

   //dispatch jobs
   while(true){
        if(queue.size() > 0){
                if(queue[0].num_nodes<1 || queue[0].num_nodes>switch_num || queue[0].time_run<0){
                        cout << "Job " << queue[0].job_id << "can not be dispatched due to errorous request" << endl;
                        queue.erase(queue.begin());
                        //continue;
                }
                else{
                        dispatch_random(sys, queue, all_jobs, Topology, Host_Num);
                }
        }
        else if(all_submitted == true){
                cout << "All jobs have been dispatched" << endl;
                break;  
        }
   }

   // ########################################## //
   // ##############   PHASE 1   ############### //
   // ##        routing       ## //
   // ########################################## //

   if (Topology == 0 || Topology == 1) //mesh or torus
   {
        while ( cin >> h_src){	
        cin >> h_dst;
                src = h_src/Host_Num;
                dst = h_dst/Host_Num;

        bool wrap_around_x = false;
        bool wrap_around_y = false;
        bool wrap_around_z = false; //3D
        bool wrap_around_a = false; //4D

        //#######################//
        // e.g. 3D
        // (switch port: 0 (not used), 1 +x, 2 -x, 3 -y, 4 +y, 5 -z, 6 +z, 7 localhost->switch, 8 switch-> localhost)
        // switch port:1 +x, 2 -x, 3 +y, 4 -y, 5 -z, 6 +z,
                // from 7 to (6+Host_Num) localhost->switch,
                // from (7+Host_Num) to (6+Host_Num*2) switch-> localhost
        //#######################//

        // channel <-- node pair ID, node pair <-- channel ID 
        Pair tmp_pair(src,dst,h_src,h_dst);  
        pairs.push_back(tmp_pair);
        int t = Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
        // int t = (dst%2==1 && Topology==1) ? 
        //         Vch*src*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+h_src%Host_Num
        //         : Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        pairs[ct].channels.push_back(t);  // node pair <-- channel ID     
        pairs[ct].pair_id = ct; 
        int delta_x, delta_y, delta_z, delta_a, current, src_xy, dst_xy, src_xyz, dst_xyz; // 2D, 3D, 4D
        if (dimension == 2 || dimension == 1){ //2D, 1D
                src_xy = src; 
                dst_xy = dst; 
        }
        if (dimension == 3){ //3D
                src_xy = src%int(pow(array_size,2)); 
                dst_xy = dst%int(pow(array_size,2)); 
        }
        if (dimension == 4){ //4D
                src_xyz = src%int(pow(array_size,3)); 
                dst_xyz = dst%int(pow(array_size,3)); 
                src_xy = src_xyz%int(pow(array_size,2)); 
                dst_xy = dst_xyz%int(pow(array_size,2));   
        }
        switch (Topology){
        case 0: //mesh
                //  if (dimension == 2){ //2D
                //         delta_x = dst%array_size - src%array_size;
                //         delta_y = dst/array_size - src/array_size;
                //  }
                if (dimension == 3){ //3D
                        delta_z = dst/int(pow(array_size,2)) - src/int(pow(array_size,2)); 
                        // delta_x = dst_xy%array_size - src_xy%array_size; 
                        // delta_y = dst_xy/array_size - src_xy/array_size; 
                }
                if (dimension == 4){ //4D
                delta_a = dst/int(pow(array_size,3)) - src/int(pow(array_size,3));
                delta_z = dst_xyz/int(pow(array_size,2)) - src_xyz/int(pow(array_size,2)); 
                //        delta_x = dst_xy%array_size - src_xy%array_size; 
                //        delta_y = dst_xy/array_size - src_xy/array_size; 
                }
                //4D, 3D, 2D, 1D(delta_y=0)
                delta_x = dst_xy%array_size - src_xy%array_size; 
                delta_y = dst_xy/array_size - src_xy/array_size; 
                current = src; 
                break;

        case 1: // torus
                if (dimension == 4){ //4D
                        delta_a = dst/int(pow(array_size,3)) - src/int(pow(array_size,3));
                        if ( delta_a < 0 && abs(delta_a) > array_size/2 ) {
                        //delta_a = -( delta_a + array_size/2);
                        delta_a = delta_a + array_size;
                                wrap_around_a = true;		
                        } else if ( delta_a > 0 && abs(delta_a) > array_size/2 ) {
                        //delta_a = -( delta_a - array_size/2);
                        delta_a = delta_a - array_size;
                                wrap_around_a = true;		
                        }
                        delta_z = dst_xyz/int(pow(array_size,2)) - src_xyz/int(pow(array_size,2));
                        if ( delta_z < 0 && abs(delta_z) > array_size/2 ) {
                        //delta_z = -( delta_z + array_size/2);
                        delta_z = delta_z + array_size;
                                wrap_around_z = true;		
                        } else if ( delta_z > 0 && abs(delta_z) > array_size/2 ) {
                        //delta_z = -( delta_z - array_size/2);
                        delta_z = delta_z - array_size;
                                wrap_around_z = true;		
                        }
                }
                if (dimension == 3){ //3D
                        delta_z = dst/int(pow(array_size,2)) - src/int(pow(array_size,2));
                        if ( delta_z < 0 && abs(delta_z) > array_size/2 ) {
                        //delta_z = -( delta_z + array_size/2);
                        delta_z = delta_z + array_size;
                                wrap_around_z = true;		
                        } else if ( delta_z > 0 && abs(delta_z) > array_size/2 ) {
                        //delta_z = -( delta_z - array_size/2);
                        delta_z = delta_z - array_size;
                                wrap_around_z = true;		
                        }
                }
                //4D, 3D, 2D, 1D(delta_y=0)
                delta_x = dst_xy%array_size - src_xy%array_size;
                if ( delta_x < 0 && abs(delta_x) > array_size/2 ) {
                //delta_x = -( delta_x + array_size/2);
                delta_x = delta_x + array_size;
                        wrap_around_x = true;		
                } else if ( delta_x > 0 && abs(delta_x) > array_size/2 ) {
                //delta_x = -( delta_x - array_size/2);
                delta_x = delta_x - array_size;
                        wrap_around_x = true;		
                }
                delta_y = dst_xy/array_size - src_xy/array_size;
                if ( delta_y < 0 && abs(delta_y) > array_size/2 ) {
                //delta_y = -( delta_y + array_size/2);
                delta_y = delta_y + array_size;
                        wrap_around_y = true;		
                } else if ( delta_y > 0 && abs(delta_y) > array_size/2 ) {
                //delta_y = -( delta_y - array_size/2);
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

        if (dimension == 4) //4D
        {
                pairs[ct].hops = abs(delta_x) + abs(delta_y)+ abs(delta_z) + abs(delta_a);
        }
        if (dimension == 3) //3D
        {
                pairs[ct].hops = abs(delta_x) + abs(delta_y)+ abs(delta_z);
        }
        if (dimension == 2 || dimension == 1) //2D, 1D(delta_y=0)
        {
                pairs[ct].hops = abs(delta_x) + abs(delta_y);
        }

        if (dimension == 4){ //4D routing
                if (delta_a > 0){
                        while ( delta_a != 0 ){  //-a
                        int t = Vch * current * (degree+1+2*Host_Num) + 7;
                        // int t = (wrap_around_a) ? Vch*current*(degree+1+2*Host_Num)+7+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 7;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        //if ( current % (array_size*array_size) == array_size-1) { 
                        if ( current >= array_size*array_size*array_size*(array_size-1)) {
                        wrap_around_a = false;
                        current = current - (array_size -1)*array_size*array_size*array_size;
                        } else current += array_size*array_size*array_size; 
                        delta_a--;
                        hops++;
                        }
                } else if (delta_a < 0){
                        while ( delta_a != 0 ){  //+a
                        int t = Vch * current * (degree+1+2*Host_Num) + 8;
                        // int t = (wrap_around_a) ? Vch*current*(degree+1+2*Host_Num)+8+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 8;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        //if ( current % (array_size*array_size) == 0 ) { 
                        if ( current < array_size*array_size*array_size) {
                        wrap_around_a = false;
                        current = current + (array_size -1)*array_size*array_size*array_size;
                        } else current -= array_size*array_size*array_size;
                        hops++;
                        delta_a++;
                        }
                }
                
                if (delta_a != 0){
                        cerr << "Routing Error " << endl;
                        exit (1);
                } 

                if (delta_z > 0){
                        while ( delta_z != 0 ){ // -z
                        int t = Vch * current * (degree+1+2*Host_Num) + 5;
                        // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 5;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        //if ( current % (array_size*array_size) == array_size-1) { 
                        if ( (current%(array_size*array_size*array_size)) >= array_size*array_size*(array_size-1)) { 
                        wrap_around_z = false;
                        current = current - (array_size -1)*array_size*array_size;
                        } else current += array_size*array_size; 
                        delta_z--;
                        hops++;
                        }
                } else if (delta_z < 0){
                        while ( delta_z != 0 ){ // +z
                        int t = Vch * current * (degree+1+2*Host_Num) + 6;
                        // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 6;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        //if ( current % (array_size*array_size) == 0 ) { 
                        if ( (current%(array_size*array_size*array_size)) < array_size*array_size) {
                        wrap_around_z = false;
                        current = current + (array_size -1)*array_size*array_size;
                        } else current -= array_size*array_size;
                        hops++;
                        delta_z++;
                        }
                }
                
                if (delta_z != 0){
                        cerr << "Routing Error " << endl;
                        exit (1);
                }

                // X 
                if (delta_x > 0){
                        while ( delta_x != 0 ){ // +x
                        int t = Vch * current * (degree+1+2*Host_Num) + 1;
                        // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 1;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        if ( ((current%(array_size*array_size*array_size)) % (array_size*array_size)) % array_size == array_size-1) { 
                        wrap_around_x = false;
                        current = current - (array_size -1);
                        } else current++; 
                        delta_x--;
                        hops++;
                        }
                } else if (delta_x < 0){
                        while ( delta_x != 0 ){ // -x
                        int t = Vch * current * (degree+1+2*Host_Num) + 2;
                        // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 2;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        if ( ((current%(array_size*array_size*array_size)) % (array_size*array_size)) % array_size == 0 ) { 
                        wrap_around_x = false;
                        current = current + (array_size - 1);
                        } else current--;
                        hops++;
                        delta_x++;
                        }
                }
                
                if (delta_x != 0){
                        cerr << "Routing Error " << endl;
                        exit (1);
                }

                // Y 
                if (delta_y > 0){
                        while ( delta_y != 0 ){ // -y
                        int t = Vch * current * (degree+1+2*Host_Num) + 3;
                        // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 3;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        if ( ((current%(array_size*array_size*array_size)) % (array_size*array_size)) >= array_size*(array_size-1) ){ 
                        wrap_around_y = false;
                        current = current - array_size*(array_size -1);
                        } else current += array_size;
                        hops++;
                        delta_y--;
                        }
                } else if (delta_y < 0){
                        while ( delta_y != 0 ){ // +y
                        int t = Vch * current * (degree+1+2*Host_Num) + 4;
                        // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 4;
                        Crossing_Paths[t].pair_index.push_back(ct); 
                        pairs[ct].channels.push_back(t);
                        if ( ((current%(array_size*array_size*array_size)) % (array_size*array_size)) < array_size ) {
                        wrap_around_y = false;
                        current = current + array_size*(array_size -1);
                        } else current -= array_size;
                        hops++;
                        delta_y++;
                        }
                }
                
                if ( delta_x != 0 || delta_y != 0 || delta_z != 0 || delta_a != 0){ 
                        cerr << "Routing Error " << endl;
                        exit (1);
                }         
        }
        
        if (dimension == 3){ //3D routing
                if (delta_z > 0){
                        while ( delta_z != 0 ){ // -z
                        int t = Vch * current * (degree+1+2*Host_Num) + 5;
                        // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 5;
                        Crossing_Paths[t].pair_index.push_back(ct);  // channel <-- node pair ID
                        pairs[ct].channels.push_back(t); // node pair <-- channel ID
                        //if ( current % (array_size*array_size) == array_size-1) { 
                        if ( current >= array_size*array_size*(array_size-1)) {
                        wrap_around_z = false;
                        current = current - (array_size -1)*array_size*array_size;
                        } else current += array_size*array_size; 
                        delta_z--;
                        hops++;
                        }
                } else if (delta_z < 0){
                        while ( delta_z != 0 ){ // +z
                        int t = Vch * current * (degree+1+2*Host_Num) + 6;
                        // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 6;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t); // node pair <-- channel ID
                        //if ( current % (array_size*array_size) == 0 ) { 
                        if ( current < array_size*array_size) {
                        wrap_around_z = false;
                        current = current + (array_size -1)*array_size*array_size;
                        } else current -= array_size*array_size;
                        hops++;
                        delta_z++;
                        }
                }
                
                if (delta_z != 0){
                        cerr << "Routing Error " << endl;
                        exit (1);
                }

                // X
                if (delta_x > 0){
                        while ( delta_x != 0 ){ // +x
                        int t = Vch * current * (degree+1+2*Host_Num) + 1;
                        // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 1;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t); // node pair <-- channel ID
                        if ( (current % (array_size*array_size)) % array_size == array_size-1) { 
                        wrap_around_x = false;
                        current = current - (array_size -1);
                        } else current++; 
                        delta_x--;
                        hops++;
                        }
                } else if (delta_x < 0){
                        while ( delta_x != 0 ){ // -x
                        int t = Vch * current * (degree+1+2*Host_Num) + 2;
                        // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 2;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t); // node pair <-- channel ID
                        if ( (current % (array_size*array_size)) % array_size == 0 ) { 
                        wrap_around_x = false;
                        current = current + (array_size - 1);
                        } else current--;
                        hops++;
                        delta_x++;
                        }
                }
                
                // check X routing is finished 
                if (delta_x != 0){
                        cerr << "Routing Error " << endl;
                        exit (1);
                }

                // Y 
                if (delta_y > 0){
                        while ( delta_y != 0 ){ // -y
                        int t = Vch * current * (degree+1+2*Host_Num) + 3;
                        // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 3;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t); // node pair <-- channel ID
                        if ( (current % (array_size*array_size)) >= array_size*(array_size-1) ){ 
                        wrap_around_y = false;
                        current = current - array_size*(array_size -1);
                        } else current += array_size;
                        hops++;
                        delta_y--;
                        }
                } else if (delta_y < 0){
                        while ( delta_y != 0 ){ // +y
                        int t = Vch * current * (degree+1+2*Host_Num) + 4;
                        // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 4;
                        Crossing_Paths[t].pair_index.push_back(ct);  // channel <-- node pair ID
                        pairs[ct].channels.push_back(t); // node pair <-- channel ID
                        if ( (current % (array_size*array_size)) < array_size ) { 
                        wrap_around_y = false;
                        current = current + array_size*(array_size -1);
                        } else current -= array_size;
                        hops++;
                        delta_y++;
                        }
                }
                
                // check if X,Y,Z routing are finished 
                if ( delta_x != 0 || delta_y != 0 || delta_z != 0){ 
                        cerr << "Routing Error " << endl;
                        exit (1);
                }        
        }

        if (dimension == 2 || dimension == 1){ //2D routing, 1D routing (delta_y=0)
                // X 
                if (delta_x > 0){
                        while ( delta_x != 0 ){ // +x
                        int t = Vch * current * (degree+1+2*Host_Num) + 1;
                        // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 1;
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
                        while ( delta_x != 0 ){ // -x
                        int t = Vch * current * (degree+1+2*Host_Num) + 2;
                        // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 2;
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
                        while ( delta_y != 0 ){ // -y
                        int t = Vch * current * (degree+1+2*Host_Num) + 3;
                        // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 3;
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
                        while ( delta_y != 0 ){ // +y
                        int t = Vch * current * (degree+1+2*Host_Num) + 4;
                        // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                        // Vch * current * (degree+1+2*Host_Num) + 4;
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
        }      

        // switch->host 
        t = Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
        // t = (src%2==1 && Topology==1) ? 
        //                 Vch*dst*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num
        //                 : Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        pairs[ct].channels.push_back(t); // node pair <-- channel ID     
        ct++;	
        }
   }
   
   if (Topology == 2){ // fat-tree

        while ( cin >> h_src){	
        cin	>> h_dst;
        int current = h_src;

        src = h_src;
        dst = h_dst;

        //#######################//
        // switch port:0 UP, 1 DOWN1(or localhost), 2 DOWN2, 3 DOWN3, 4 DOWN4 //
        //#######################//
        
        // channel --> switch ID + output port
        Pair tmp_pair(src,dst,h_src,h_dst);  
        pairs.push_back(tmp_pair);
        Crossing_Paths[current*PORT].pair_index.push_back(ct);
        pairs[ct].channels.push_back(current*PORT);
        //hops++;

        pairs[ct].pair_id = ct; 
        
        current = node_num + current/Host_Num;

        while ( current != dst ){ 
                int t;
                // root switch
                if ( current == node_num + node_num/Host_Num + node_num/pow(Host_Num,2)){
                t = current * PORT + dst/(int)pow(Host_Num,2)+1;
                current = current - Host_Num + dst/(int)pow(Host_Num,2);
                // middle layer switch
                } else if ( current >= node_num + node_num/Host_Num){
                if ( current-node_num-node_num/Host_Num != dst/(int)pow(Host_Num,2)){
                t = current * PORT + 0;
                current = node_num + node_num/Host_Num + node_num/(int)pow(Host_Num,2);
                } else {
                t = current * PORT + (dst/Host_Num)%Host_Num + 1;
                current = node_num + dst/Host_Num;
                }
                // low layer switch
                } else if ( current >= node_num){
                if ( current-node_num != dst/Host_Num){
                t = current * PORT + 0;
                current = node_num + current/Host_Num;
                } else {
                t = current * PORT + dst%Host_Num+1;
                current = dst;
                }
                }
                else { //host->switch
	        }      
                Crossing_Paths[t].pair_index.push_back(ct);
                pairs[ct].channels.push_back(t);
                hops++;
        }

        pairs[ct].hops = hops-before_hops;  
        before_hops = hops;

        if ( current != dst ){
                cerr << "Routing Error " << endl;
                exit (1);
        }
        ct++;	
        }
   }
   
   
   if (Topology == 3) //fully-connected
   {
        while ( cin >> h_src){	
        cin >> h_dst;
                src = h_src/Host_Num;
                dst = h_dst/Host_Num;

        //#######################//
        // switch port <-- destination switch ID
        // localhost port = switch ID
        //#######################//

        // channel <-- node pair ID, node pair <-- channel ID 
        Pair tmp_pair(src,dst,h_src,h_dst);  
        pairs.push_back(tmp_pair);

        // localhost(h_src) --> src
        //int t = src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
        //Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        //pairs[ct].channels.push_back(t);  // node pair <-- channel ID     
        pairs[ct].pair_id = ct; 
        pairs[ct].hops = 1;

        // src --> dst
        int t = src * (degree+1+2*Host_Num) + dst; // output port = destination switch ID
        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        pairs[ct].channels.push_back(t);  // node pair <-- channel ID 
        hops++;      

        // dst --> localhost(h_dst)
        //t = dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
        //Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        //Crossing_Paths[dst*(degree+1+2*Host_Num)+dst].pair_index.push_back(ct); 
        //pairs[ct].channels.push_back(t); // node pair <-- channel ID  
        //t = dst*(degree+1+2*Host_Num)+dst;   
        //pairs[ct].channels.push_back(t); // output port <-- destination switch ID

        ct++;	
        }
   }

      
   if (Topology == 4){ // full mesh connected circles (FCC)
        
        // switch topo initiation
        // switch n <--> switch 3-n, port p <--> port 11-p
        // 0:not used, 1:left, 2:right, 3-8:inter-group
        for(int g=0; g<groups; g++){
                for(int s=0; s<group_switch_num; s++){
                        int sw = g*group_switch_num+s;
                        if(s==0) Switch_Topo[sw*(degree+1+2*Host_Num)+1] = sw+(group_switch_num-1);
                        else Switch_Topo[sw*(degree+1+2*Host_Num)+1] = sw-1;
                        if(s==group_switch_num-1) Switch_Topo[sw*(degree+1+2*Host_Num)+2] = sw-(group_switch_num-1);
                        else Switch_Topo[sw*(degree+1+2*Host_Num)+2] = sw+1;
                        for(int j=3; j<=degree; j++){
                                Switch_Topo[sw*(degree+1+2*Host_Num)+j] = ((g+s*inter_group+j-3+1)%groups)*group_switch_num+(group_switch_num-1-s);
                        }
                }
        }

        while ( cin >> h_src){	
        cin >> h_dst;
                src = h_src/Host_Num;
                dst = h_dst/Host_Num;

        int current = src;        

        //#######################//
        // switch port: 0 localhost, 1 left, 2 right, 3-8 inter-group
        // localhost port = 0
        //#######################//

        // channel <-- node pair ID, node pair <-- channel ID 
        Pair tmp_pair(src,dst,h_src,h_dst);  
        pairs.push_back(tmp_pair);

        // localhost(h_src) --> src
        int t = src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        pairs[ct].channels.push_back(t);  // node pair <-- channel ID     
        pairs[ct].pair_id = ct; 

        // src --> dst   
        // group #
        int src_group = src/group_switch_num;
        int dst_group = dst/group_switch_num;
        // switch offset in a group
        int src_offset = src%group_switch_num;
        int dst_offset = dst%group_switch_num;

        int diff_group = dst_group-src_group;
        // gateway for inter-group
        int src_gw_offset = -1;
        int dst_gw_offset = -1;
        int src_gw = -1;
        int dst_gw = -1;
        int src_gw_port = -1;
        int dst_gw_port = -1;
        if (diff_group==0){ //intra-group routing
                if(dst_offset-src_offset==0) continue;
                else if(dst_offset-src_offset>group_switch_num/2 || (src_offset-dst_offset>0 && src_offset-dst_offset<group_switch_num/2)){
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+1; //backward
                                current = current - 1;
                                if(current<src_group*group_switch_num) current = current+group_switch_num;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                hops++;
                        }
                }
                else{
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+2; //forward
                                current = current + 1;
                                if(current>=(src_group+1)*group_switch_num) current = current-group_switch_num;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                hops++;
                        }
                }
        }
        else { //intra-src-group routing + inter-group routing + intra-dst-group routing
                if(diff_group<0) diff_group+=groups; 
                src_gw_offset = (diff_group-1)/inter_group;
                dst_gw_offset = (group_switch_num-1)-src_gw_offset;
                src_gw = src_group*group_switch_num+src_gw_offset;
                dst_gw = dst_group*group_switch_num+dst_gw_offset;
                src_gw_port = (diff_group-1)%inter_group+3;
                dst_gw_port = (3+degree)-src_gw_port;

                //intra-src-group routing
                if(src_gw_offset-src_offset==0) ;
                else if(src_gw_offset-src_offset>group_switch_num/2 || (src_offset-src_gw_offset>0 && src_offset-src_gw_offset<group_switch_num/2)){
                        while ( current != src_gw ){
                                t = current*(degree+1+2*Host_Num)+1; //backward
                                current = current - 1;
                                if(current<src_group*group_switch_num) current = current+group_switch_num;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                hops++;
                        }
                }
                else{
                        while ( current != src_gw ){
                                t = current*(degree+1+2*Host_Num)+2; //forward
                                current = current + 1;
                                if(current>=(src_group+1)*group_switch_num) current = current-group_switch_num;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                hops++;
                        }
                }

                //inter-src-dst-group routing
                t = current*(degree+1+2*Host_Num)+src_gw_port;
                current = dst_gw;
                Crossing_Paths[t].pair_index.push_back(ct);
                pairs[ct].channels.push_back(t);
                hops++;

                //intra-dst-group routing
                if(dst_offset-dst_gw_offset==0) ;
                else if(dst_offset-dst_gw_offset>group_switch_num/2 || (dst_gw_offset-dst_offset>0 && dst_gw_offset-dst_offset<group_switch_num/2)){
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+1; //backward
                                current = current - 1;
                                if(current<dst_group*group_switch_num) current = current+group_switch_num;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                hops++;
                        }
                }
                else{
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+2; //forward
                                current = current + 1;
                                if(current>=(dst_group+1)*group_switch_num) current = current-group_switch_num;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                hops++;
                        }
                }                
        }

        // dst --> localhost(h_dst)
        t = dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
        pairs[ct].channels.push_back(t); // node pair <-- channel ID  

        pairs[ct].hops = hops-before_hops;  
        before_hops = hops;

        if ( current != dst ){
                cerr << "Routing Error " << endl;
                exit (1);
        }
        ct++;

        }
   }
	

   // ########################################## //
   // ##############   PHASE 2   ############### //
   // ##  Slot #       ## //
   // ########################################## //

   int max_cp = max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   int max_id = 0;
	
   int max_cp_dst = 0; // number of dst-based renewable labels

   // calculate number of dst-based renewable label
   int max_cp_dst_t = 0;

   int default_slot = 8;

   vector<int>::iterator find_ptr;

   for (int j = 0; j < ports; j++ ){ 
        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin()+j;
                unsigned int p_ct = 0;
                while ( p_ct < elem->pair_index.size() ){
                        int u = elem->pair_index[p_ct]; 
                        bool is_duplicate = false; //check if there is a same destination
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
   cout << " === Max. number of slots (w/o update) ===" << endl << max_cp_dst << endl;	
   cout << " === Max. number of slots (w/ update) ===" << endl << max_cp << endl;

   /*int slot_max = 0;
   for (int i = 0; i < Crossing_Paths.size(); i++){
        if (i%(degree+1+2*Host_Num) != degree+2*Host_Num && i%(degree+1+2*Host_Num) != degree+2*Host_Num-1)
        if(Crossing_Paths[i].pair_index.size() > slot_max)
        slot_max = Crossing_Paths[i].pair_index.size();
   }
   cout << " slot_max = " << slot_max << endl;	*/

   for (int j = 0; j < ports; j++ ){ 
        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin()+j;
        elem = max_element(Crossing_Paths.begin(),Crossing_Paths.end());

        // local IDs are assigned
        unsigned int path_ct = 0; 
        while ( path_ct < elem->pair_index.size() ){
        int t = elem->pair_index[path_ct];      
        // check if IDs are assigned
        if ( pairs[t].Valid == true ) {path_ct++; continue;}
        // ID is assigned from 0
        int id_tmp = 0;
        bool NG_ID = false;
                
        NEXT_ID:
                // ID is used or not
                unsigned int s_ct = 0; // channel
                while ( s_ct < pairs[t].channels.size() && !NG_ID ){
                int i = pairs[t].channels[s_ct];
                //vector<int>::iterator find_ptr;
                find_ptr = find ( Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
                if ( path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) NG_ID = true;
                if (!path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) {
                int tmp = 0;
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
                if (max_id <= id_tmp) max_id = id_tmp + 1; 

                path_ct++;
        }
        elem->Valid = true;
   }

   if (Topology == 0 || Topology == 1) // mesh or torus
   show_paths(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Vch, Host_Num, max_cp, max_cp_dst, path_based, degree, default_slot);     
        
   if (Topology == 2) // fat-tree
   show_paths_tree(Crossing_Paths, ct, node_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, PORT, default_slot);
   
   if (Topology == 3) // fully-connected
   show_paths_fullyconnected(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree, default_slot); 

   if (Topology == 4) // full mesh connected circles (FCC)
   show_paths_fcc(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree, default_slot);  

   return 0;
}

//job submit thread
void submit_jobs(vector<Job> all_jobs, vector<Job> queue, int queue_policy, bool all_submitted){
        queue.push_back(all_jobs[0]);
        all_jobs[0].time_submit_r = time(NULL);
        int current_submit = all_jobs[0].time_submit;
        cout << "Job " << all_jobs[0].job_id << " is submitted"
        for (int i=1; i<all_jobs.size(); i++){
                int submit_interval = all_jobs[i].time_submit - current_submit;
                if (submit_interval > 0){
                        sleep(submit_interval);
                        queue.push_back(all_jobs[i]);
                        current_submit = all_jobs[i].time_submit;
                        if (queue.size()>2){
                                if (queue_policy == 1){ //bf
                                        sort(queue.begin()+1, queue.end(), LSF);
                                }
                                else if (queue_policy == 2){ //sf
                                        sort(queue.begin()+1, queue.end(), SSF);
                                } 
                                else if (queue_policy == 3){ //rlf
                                        sort(queue.begin()+1, queue.end(), LRF);
                                } 
                                else if (queue_policy == 4){ //rsf
                                        sort(queue.begin()+1, queue.end(), SRF);
                                } 
                        }  
                        all_jobs[i].time_submit_r = time(NULL);
                        cout << "Job " << all_jobs[i].job_id << " is submitted" << endl;                                                                                         
                }
                else{
                        cout << "Job " << all_jobs[i].job_id << " is not submitted due to errorous submit time" << endl;
                }
                if (i == all_jobs.size()-1){
                        all_submitted = true;
                        cout << "All jobs have been submitted" << endl;
                }
        }
}

//dispatch jobs (random)
void dispatch_random(vector<int> sys, vector<Job> queue, vector<Job> all_jobs, int topo, int hosts){
        vector<int> ava_nodes;
        for (int i=0; i<sys.size(); i++){
                if(sys[i] == 0){ //available
                       ava_nodes.push_back(i); 
                       if (queue[0].num_nodes == ava_nodes.size()){
                               cout << "Job " << queue[0].job_id << " is dispatched" << endl;
                               //for (int j=0; j<ava_nodes.size(); j++) cout << ava_nodes[i] << " " << endl;
                               for (int j=0; j<all_jobs.size(); j++){
                                       if(all_jobs[j].job_id == queue[0].job_id){
                                                all_jobs[j].time_dispatch_r = time(NULL);
                                                for (int n=0; n<ava_nodes.size(); n++){
                                                        sys[ava_nodes[n]] = 1; //occupied
                                                        all_jobs[j].nodes.push_back(ava_nodes[n]);
                                                } 
                                                all_jobs[j].src_dst_pairs_m = src_dst_pairs_map(all_jobs[j].src_dst_pairs, all_jobs[j].nodes, topo, hosts);
                                                //todo
                                                thread rn{release_nodes, all_jobs[j]};
                                                rn.join();  
                                                queue.erase(queue.begin()); 
                                                return;          
                                       }
                               }       
                       }
                }
        }
}

//map src_dst_pairs to src_dst_pairs_m
vector<int> src_dst_pairs_map(vector<int> src_dst_pairs, vector<int> nodes, int topo, int hosts){
        vector<int> src_dst_pairs_m;
        vector<int> temp;
        for(int i=0; i<src_dst_pairs.size(); i++){
                //if (i%5 < 2) temp.push_back(src_dst_pair[i]); // i%5 == 0 --> src, i%5 == 1 --> dst, i%5 == 2 --> flow id, i%5 == 3 --> pair id, i%5 == 4 --> slot #
                temp.push_back(src_dst_pairs[i].h_src);
                temp.push_back(src_dst_pairs[i].h_dst);
        }
        sort(temp.begin(), temp.end());
        temp.erase(unique(temp.begin(), temp.end()), temp.end());
        for(int i=0; i<src_dst_pairs.size(); i++){
                // if (i%5 < 2){
                //         for(int j=0; j<temp.size(); j++){
                //                 if(src_dst_pair[i] == temp[j]){
                //                         src_dst_pair_m.push_back(nodes[j]);
                //                 }
                //         }
                // }
                // else{
                //         src_dst_pair_m.push_back(src_dst_pair[i]);
                // }
                int h_src_m = -1;
                int h_dst_m = -1;
                for(int j=0; j<temp.size(); j++){
                        if(src_dst_pairs[i].h_src == temp[j]){
                                h_src_m = nodes[j];
                        }
                        if(src_dst_pairs[i].h_dst == temp[j]){
                                h_dst_m = nodes[j];
                        }      
                        if (h_src_m != -1 and h_dst_m != -1){
                                int src_m = h_src_m/hosts;
                                int dst_m = h_dst_m/hosts;
                                if(topo == 2){ // fat-tree
                                        src_m = h_src_m;
                                        dst_m = h_dst_m;   
                                }
                                Pair pair(src_m,dst_m,h_src_m,h_dst_m);
                                pair.pair_id = src_dst_pairs[i].pair_id;
                                pair.flow_id = src_dst_pairs[i].flow_id;
                                src_dst_pairs_m.push_back(pair);  
                                break;                               
                        }                  
                }
        }
        return src_dst_pairs_m;
}

//release nodes after execution
void release_nodes(Job job)
{
       sleep(job.time_run);

       for (int n=0; n<job.nodes.size(); n++){
               sys[job.nodes[n]] = 0; //released
       }       

       for(int i=0; i<job.src_dst_pairs_m.size(); i++){
               update_after_release(src_dst_pairs_m[i])
       }

       cout << "Job " << job.job_id << " is finished" << endl;  
}

//update routing tables after nodes released 
//void update_after_release(Pair pair, int topo, int degree, int Host_Num, int PORT, vector<int> sys, vector<Pair> queue)
void update_after_release(Pair pair)
{
//        vector<int> nodelist; // list of nodes occupied by pair
//        if (topo == 0 || topo == 1) // mesh or torus
//        {
//                for (int i=1; i<pair.channels.size(); i++){
//                        int node;
//                        node = pair.channels[i]/(degree+1+2*Host_Num);
//                        nodelist.push_back(node);
//                }
//        }
//        if (topo == 2) // fat-tree
//        {
//                for (int i=1; i<pair.channels.size(); i++){
//                        int node;
//                        node = pair.channels[i]/PORT;
//                        nodelist.push_back(node);
//                }
//        } 
//        if (topo == 3) // fully-connected
//        {
//                nodelist.push_back(pair.src);
//                nodelist.push_back(pair.dst);
//        }    
//        if (topo == 4) // full mesh connected circles (FCC)
//        {
//                for (int i=1; i<pair.channels.size(); i++){
//                        int node;
//                        node = pair.channels[i]/(degree+1+2*Host_Num);
//                        nodelist.push_back(node);
//                }
//        }
//        //release nodes
//        for (int i=0; i<nodelist.size(); i++){
//                sys[i] = 0;
//        } 

       //update Crossing_Paths
       for (int i=0; i<pair.channels.size(); i++){
               vector<int> v = Crossing_paths[pair.channels[i]].pair_index;
               v.erase(remove(v.begin(), v.end(), pair.pair_id), v.end());
               for (int j=4; j<Crossing_paths[pair.channels[i]].routing_table.size(); j=j+5){
                       if (Crossing_paths[pair.channels[i]].routing_table[j] == pair.pair_id){
                               Crossing_paths[pair.channels[i]].routing_table[j-4] = -1;
                               Crossing_paths[pair.channels[i]].routing_table[j-3] = -1;
                               Crossing_paths[pair.channels[i]].routing_table[j-2] = -1;
                               Crossing_paths[pair.channels[i]].routing_table[j-1] = -1;
                               Crossing_paths[pair.channels[i]].routing_table[j] = -1;
                       }
               }
       }
}