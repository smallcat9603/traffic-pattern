//
// circuit-switch-table.cc
//
// mesh/torus/fat-tree/fully-connected
// Usage: cat test.txt | ./circuit-switch-table.out -D 2 -a 4 -T 0  <-- 2-D 16-node mesh
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

using namespace std;

//
// Communication node pair
//
struct Pair {
   // pair id (from 0, 1, 2, ...)     
   int pair_id;
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
// output and error check (mesh/torus)
//
void show_paths (vector<Cross_Paths> Crossing_Paths, int ct, int switch_num, \
int max_id, vector<Pair> pairs, int hops, int Vch, int Host_Num, int max_cp, int max_cp_dst,
bool path_based, int degree)
{
   // for each channel
   cout << " === Port information for each switch === " << endl;
   for (int i=0; i < Vch*(degree+1+2*Host_Num)*switch_num; i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
      if (i%(Vch*(degree+1+2*Host_Num)) == 0) cout << " SW " << i/(Vch*(degree+1+2*Host_Num)) << " : " << endl;
// for each node pair passing through a channel
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);

         if (i%(degree+1+2*Host_Num) == degree+1+2*Host_Num -2 ){
                 cout << "      Port 0 (from localhost) --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
         }
         else if (i%(degree+1+2*Host_Num) == degree+1+2*Host_Num -1 ){
                 cout << "      Port 0 (to localhost) --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
         }
         else {
                 cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
         }
         
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
	 cout << " ERROR : ID collision is occured!!." << endl;      
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
            cout << " Pair ID " << current_pair.pair_id << " (local ID " << slot_num << "): ";
            for (int j=1; j < current_pair.channels.size(); j++){ //current_pair.channels[0] --> src, current_pair.channels[current_pair.channels.size()-1] --> dst
                    target_sw = -1;
                    input_port = 0;
                    output_port = 0;
                    if (j == 1){ // source switch
                            target_sw = current_pair.src;
                            output_port = current_pair.channels[j]%(degree+1+2*Host_Num);
                            cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
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
                            cout << "SW " << target_sw;
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
                            cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
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

            }
            cout << endl;
    }

   for (int i=0; i < switch_num; i++){
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
        outputfile << degree+1 << " " << slots << endl;  // number of output ports, number of slots
        outputfile << "00" << endl;  // output 00 --> localhost
        bool slot_occupied = false;
        for (int s=0; s < slots; s++){
                if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size() > 0){
                        for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table.size(); j=j+4){
                                if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+degree+2*Host_Num].routing_table[j] << " ";
                                        slot_occupied = true;
                                }  
                        }        
                }
                else if (Vch==2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size() > 0){ //torus
                        for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size(); j=j+4){
                                if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j] << " ";
                                        slot_occupied = true;
                                }
                        }        
                }
                if (slot_occupied == false){
                        outputfile << "void";
                }
                outputfile << endl;   
                slot_occupied = false;             
        }
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
                        outputfile << "0" << op << endl;
                }
                else{
                        outputfile << op << endl;
                }  
                for (int s=0; s < slots; s++){
                        if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table.size() > 0){
                                for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table.size(); j=j+4){
                                        if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                        }  
                                }        
                        }
                        else if (Vch==2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size() > 0){ //torus
                                for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size(); j=j+4){
                                        if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                        }
                                }        
                        }
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                }
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
bool path_based, int PORT)
{
        // for each channel
        cout << " === Port information for each switch === " << endl;
        for (int i=0; i <= PORT*(node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)); i++){
        vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
        if (i%PORT == 0) {
                if ( i == PORT * (node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)) && node_num/(int)pow(Host_Num,2) <= 1 ) break; // two-layer switches
                cout << " Node " << i/PORT << " : " << endl;                
        }
// for each node pair passing through a channel
        for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
                int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
                ID_array.push_back(pairs[t].ID);

                if (i%PORT == 0){
                        cout << "      Port " << i%PORT << " (UP) --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
                }
                else{
                        cout << "      Port " << i%PORT << " (DOWN) --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;         
                }
                
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
                cout << " ERROR : ID collision is occured!!." << endl;      
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
                cout << " Pair ID " << current_pair.pair_id << " (local ID " << slot_num << "): ";
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
                        if (j == 1){
                                cout << "Node " << current_pair.h_src << " (port 0)" << " --> ";
                        }
                        cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                        if (j == current_pair.channels.size() - 1){
                                cout << "Node " << current_pair.h_dst << " (port 0)";
                        }                        

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port); // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);  // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node

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
        
        for (int i=0; i < switch_num; i++){
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
        outputfile << PORT << " " << slots << endl;  // number of output ports, number of slots

        bool slot_occupied = false;
        for (int op=0; op < PORT; op++){
                if (op < 10 && op > -1){
                        outputfile << "0" << op << endl;
                }
                else{
                        outputfile << op << endl;
                }  
                for (int s=0; s < slots; s++){
                        for (int j=0; j < Crossing_Paths[PORT*(i+node_num)+op].routing_table.size(); j=j+4){ //i+node_num --> sw #
                                if (Crossing_Paths[PORT*(i+node_num)+op].routing_table[j+1] == s){  // j+1 --> slot number
                                        outputfile << Crossing_Paths[PORT*(i+node_num)+op].routing_table[j] << " "; // j --> input port
                                        slot_occupied = true;
                                }  
                        }        
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                }
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
bool path_based, int degree)
{
   // for each channel
   cout << " === Port information for each switch === " << endl;
   for (int i=0; i < (degree+1+2*Host_Num)*switch_num; i++){
      vector<int> ID_array(Crossing_Paths[i].pair_index.size(),-1);
//      cout << " Channels (" << i << ")" << endl;
      if (i%(degree+1+2*Host_Num) == 0) cout << " SW " << i/(degree+1+2*Host_Num) << " : " << endl;
// for each node pair passing through a channel
      for (unsigned int j=0; j < Crossing_Paths[i].pair_index.size(); j++){
	 int t = Crossing_Paths[i].pair_index[j];
//	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
 	 ID_array.push_back(pairs[t].ID);

         if ( (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -2) && (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -1) ) { // -2 and -1 --> no meaning
                 cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl; 
         }
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
	 cout << " ERROR : ID collision is occured!!." << endl;      
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
            cout << " Pair ID " << current_pair.pair_id << " (local ID " << slot_num << "): ";
            for (int j=0; j < current_pair.channels.size(); j++){ // only current_pair.channels[0] --> from src to dst
                    target_sw = -1;
                    input_port = 0;
                    output_port = 0;
                    if (j == 0){ // source switch
                        target_sw = current_pair.src;
                        output_port = current_pair.channels[j]%(degree+1+2*Host_Num); // output_port = current_pair.dst
                        cout << "SW " << target_sw << " (port " << output_port << ")" << " --> SW " << current_pair.dst;
                    }

                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port); // routing table <-- input port
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);  // routing table <-- slot number
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                    Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node

                    target_sw = current_pair.dst;
                    input_port = current_pair.src;
                    output_port = current_pair.dst; // localhost
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(input_port); // routing table <-- input port
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(slot_num);  // routing table <-- slot number
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(current_pair.h_src);  // routing table <-- src node
                    Crossing_Paths[current_pair.dst*(degree+1+2*Host_Num)+current_pair.dst].routing_table.push_back(current_pair.h_dst);  // routing table <-- dst node
            }
            cout << endl;
    }

   for (int i=0; i < switch_num; i++){
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
        outputfile << degree+1 << " " << slots << endl;  // number of output ports, number of slots
        bool slot_occupied = false;
        for (int op=0; op < degree+1; op++){
                if (op < 10 && op > -1){
                        outputfile << "0" << op << endl;
                }
                else{
                        outputfile << op << endl;
                }  
                for (int s=0; s < slots; s++){
                        if (Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table.size() > 0){
                                for (int j=0; j < Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table.size(); j=j+4){
                                        if (Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j+1] == s){  // j+1 --> slot number
                                                outputfile << Crossing_Paths[(degree+1+2*Host_Num)*i+op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                        }  
                                }        
                        }
                        if (slot_occupied == false){
                                outputfile << "void";
                        }
                        outputfile << endl;   
                        slot_occupied = false;                         
                }                                                   
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
   static int Vch = 1; // Mesh:1, Torus:2
   static int Topology = 0; // Mesh:0, Torus:1, Fat-tree:2, fully-connected:3
   int c;
   static bool path_based = false; //false:destination_based (slot # not updated), true:path_based (slot # updated)
   static int degree = 4; // (mesh or torus) degree = 2 * dimension, (fully-connected) degree = switch_num -1 
   static int dimension = 2; //mesh or torus
   
   while((c = getopt(argc, argv, "a:A:n:T:uD:")) != -1) {
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

   if (Topology==1) Vch=2; //Torus
   else if (Topology==0) Vch=1;//Mesh
   /*else{ 
      cerr << " The combination of Topology and Vchs is wrong!!" << endl;
      exit (1);
   }*/

   //cout << " ID Allocation Policy is 0(low port first) / 1(Crossing Paths based method): " << Allocation << endl;
   //cout << " Address method is 0(destination_based) / 1(path_based): " 
   cout << " ### start ###" << endl;
   cout << " === Update of slot number ===" << endl << " 0 (no) / 1 (yes): " <<  path_based << " (Use -d to deactivate the update) " << endl;
   
   // source and destination		
   int src = -1, dst = -1, h_src = -1, h_dst = -1;
   // number of nodes
   static int switch_num = pow(array_size,dimension); //mesh or torus or fully-connected
   static int node_num = pow(array_size,dimension); //fat-tree
   static int PORT = Host_Num + 1; //fat-tree
   // Crossing Paths
   // including Host <-> Switch

   if (Topology == 3){ // fully-connected
        degree = switch_num - 1;
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
   vector<Cross_Paths> Crossing_Paths(ports);

   // total number of node pairs
   int ct = 0;
   // total number of hops 
   int hops = 0; 

   int before_hops = 0; //fat-tree

   // node pairs
   vector<Pair> pairs;

   cout << " === topology ===" << endl;
   if (Topology == 0) cout << dimension << "-D mesh (" << switch_num << " switches/nodes)" << endl;
   else if (Topology == 1) cout << dimension << "-D torus (" << switch_num << " switches/nodes)" << endl;
   else if (Topology == 2) 
        if (node_num/(int)pow(Host_Num,2) == 1) cout << "fat tree (" << node_num << " nodes + " << node_num/Host_Num+node_num/(int)pow(Host_Num,2) << " switches)" << endl;
        else cout << "fat tree (" << node_num << " nodes + " << node_num/Host_Num+node_num/(int)pow(Host_Num,2)+1 << " switches)" << endl;
   else if (Topology == 3) cout << "fully connected (" << switch_num << " switches/nodes)" << endl;
   else cout << "Error: please specify -T [0-3] (0 mesh, 1 torus, 2 fat tree, 3 fully connected)" << endl;

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
        // int t = Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
        int t = (dst%2==1 && Topology==1) ? 
                Vch*src*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+h_src%Host_Num
                : Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
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
                        int t = (wrap_around_a) ? Vch*current*(degree+1+2*Host_Num)+7+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 7;
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
                        int t = (wrap_around_a) ? Vch*current*(degree+1+2*Host_Num)+8+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 8;
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
                        int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 5;
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
                        int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 6;
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
                        int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 1;
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
                        int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 2;
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
                        int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 3;
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
                        int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 4;
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
                        int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 5;
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
                        int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 6;
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
                        int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 1;
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
                        int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 2;
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
                        int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 3;
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
                        int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                        Vch * current * (degree+1+2*Host_Num) + 4;
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
                        while ( delta_x != 0 ){ // -x
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
                        while ( delta_y != 0 ){ // -y
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
                        while ( delta_y != 0 ){ // +y
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
        }      

        // switch->host 
        // t = Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
        t = (src%2==1 && Topology==1) ? 
                        Vch*dst*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num
                        : Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
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
	

   // ########################################## //
   // ##############   PHASE 2   ############### //
   // ##  local ID       ## //
   // ########################################## //

   int max_cp = max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size();
   int max_id = 0;
	
   int max_cp_dst = 0; // number of dst-based renewable labels

   // calculate number of dst-based renewable label
   int max_cp_dst_t = 0;

   vector<int>::iterator find_ptr;

   if (Topology == 0 || Topology == 1){ //mesh or torus
        for (int j = 0; j < Vch * (degree+1+2*Host_Num) * switch_num; j++ ){ 
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

        for (int j = 0; j < Vch * (degree+1+2*Host_Num) * switch_num; j++ ){ 
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
                // 
                unsigned int s_ct = 0; 
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
        
        show_paths(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Vch, Host_Num, max_cp, max_cp_dst, path_based, degree);
   }

   if (Topology == 2){ //fat-tree
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

        for (int j = 0; j <= PORT * (node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)); j++ ){ 
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

                // local ID
                unsigned int path_ct = 0; 
                while ( path_ct < elem->pair_index.size() ){
                // pair ID
                int t = elem->pair_index[path_ct];      
                // ID is already assigned
                if ( pairs[t].Valid == true ) {path_ct++; continue;}
                // assigned from 0
                int id_tmp = 0;
                bool NG_ID = false;
                
        NEXT_ID_TREE:
                // ID is used or not
                unsigned int s_ct = 0; // channel
                while ( s_ct < pairs[t].channels.size() && !NG_ID ){
                int i = pairs[t].channels[s_ct];
                //list<int>::iterator find_ptr;
                find_ptr = find ( Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
                if (path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) NG_ID = true;
                if (!path_based && find_ptr != Crossing_Paths[i].assigned_list.end()) {
                int tmp = 0;
                while (*find_ptr != Crossing_Paths[i].assigned_list[tmp]) {tmp++;}
                if (pairs[t].h_dst != Crossing_Paths[i].assigned_dst_list[tmp])
                                        NG_ID = true; 
                }
                s_ct++;
                }
                
                if (NG_ID){
                id_tmp++; NG_ID = false; goto NEXT_ID_TREE;
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
                //if (max_id < id_tmp) max_id = id_tmp;
                if (max_id <= id_tmp) max_id = id_tmp + 1; 
                
                path_ct++;
        }
        elem->Valid = true;
        }      
        show_paths_tree(Crossing_Paths, ct, node_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, PORT);
   }
   
   if (Topology == 3){ //fully-connected
        for (int j = 0; j < (degree+1+2*Host_Num) * switch_num; j++ ){ 
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

        for (int j = 0; j < (degree+1+2*Host_Num) * switch_num; j++ ){ 
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

                // local IDs are assigned
                unsigned int path_ct = 0; 
                while ( path_ct < elem->pair_index.size() ){
                int t = elem->pair_index[path_ct];      
                // check if IDs are assigned
                if ( pairs[t].Valid == true ) {path_ct++; continue;}
                // ID is assigned from 0
                int id_tmp = 0;
                bool NG_ID = false;
                
        NEXT_ID_FULLY:
                // 
                unsigned int s_ct = 0; 
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
                id_tmp++; NG_ID = false; goto NEXT_ID_FULLY;
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
   
        show_paths_fullyconnected(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree);
   }   

   return 0;
}