//
// circuit-switch-table.cc
//
// mesh/torus/fat-tree/fully-connected/full-mesh-connected-circles(FCC)/topology-file
// Usage: cat test.txt | ./cst.out -D 2 -a 4 -T 0  <-- 2-D 16-node mesh
//
// % less test.txt
// 3  4    <--- path from node 3 to node 4
// 2  4
//
// % less test_flow_id.txt
// 3  4  0  <--- path from node 3 to node 4 (flow 0)
// 2  4  1 
//
// Usage: ./tpg.out -t 0 -n 4 | ./cst.out -D 2 -a 4 -T 0
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

static void usage(char *myname);
void usage(char *myname)
{
        cerr << "usage: " << myname << " [-anTuDdmt]" << endl;
        cerr << "\t-T <n>\t: Topology 0: Mesh, 1: Torus, 2: Fat Tree, 3: Fully Connected, 4: Dragonfly (FCC), 5: Topology File" << endl;        
        cerr << "\t-a <n>\t: Set array size for mesh or torus (-T [0|1])" << endl;
        cerr << "\t-n <n>\t: Set the number of hosts to one switch for fat-tree (-T 2)" << endl;
        cerr << "\t-u   \t: Allow to update a slot number (at intermediate switches on a path)" << endl;
        cerr << "\t-D <n>\t: Set switch degree for mesh or torus (-T [0|1])" << endl;
        cerr << "\t-d <n>\t: Set switch ports (not include host) for dragonfly (-T 4)" << endl;
        cerr << "\t-m <n>\t: Set the number of switches in a group for dragonfly (-T 4)" << endl;
        cerr << "\t-t <filename>\t: Topology file name (-T 5)" << endl;
}

//
// Flow
//
struct Flow
{
        vector<int> pairs_id;
        // specified flow id
        int id;
        // channels
        vector<int> channels;
        // assigned time slot #
        int ID;
};

//
// Communication node pair
//
struct Pair
{
        // pair id (unique, from 0, 1, 2, ...)
        int pair_id;
        // flow id (not unique)
        int flow_id;
        // channels
        vector<int> channels;
        // source and destination
        int src;
        int dst;
        int h_src;
        int h_dst;
        // assigned ID (different from pair_id)
        int ID;
        // ID is valid or not
        bool Valid;
        // the distance between src and dst
        int hops;
        // initialize
        Pair(int s, int d, int h_s, int h_d) : src(s), dst(d), h_src(h_s), h_dst(h_d), flow_id(-1), ID(-1), Valid(false), hops(-1) {}

        bool operator<(Pair &a)
        {
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
struct Cross_Paths
{
        // Communication node pair index going through a channel
        vector<int> pair_index;
        // Communication flow index going through a channel
        vector<int> flow_index;
        // list of ID in Pair for a channel
        vector<int> assigned_list;
        // list of h_dst in Pair for a channel
        vector<int> assigned_dst_list;
        // If all IDs are assigned
        bool Valid;
        // initialize
        Cross_Paths() : Valid(false) {}
        // routing table
        vector<int> routing_table; // input port, slot number, src node, dst node, ...

        bool operator==(const Cross_Paths &a) const
        {
                return Valid == a.Valid && pair_index.size() == a.pair_index.size();
        }
        bool operator<(const Cross_Paths &a) const
        {
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
void show_paths(vector<Cross_Paths> Crossing_Paths, int ct, int switch_num,
                int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
                bool path_based, int degree, int default_slot, vector<Flow> flows)
{
        // for each channel
        //cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < (degree + 1 + 2 * Host_Num) * switch_num; i++)
        {
                vector<int> ID_array(Crossing_Paths[i].pair_index.size(), -1);
                //      cout << " Channels (" << i << ")" << endl;
                //if (i%(Vch*(degree+1+2*Host_Num)) == 0) cout << " SW " << i/(Vch*(degree+1+2*Host_Num)) << " : " << endl;
                // for each node pair passing through a channel
                for (unsigned int j = 0; j < Crossing_Paths[i].pair_index.size(); j++)
                {
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
                sort(ID_array.begin(), ID_array.end());
                unsigned int k = 0;
                bool error = false;
                while (k + 1 < ID_array.size() && !error)
                {
                        if (ID_array[k] == ID_array[k + 1] && ID_array[k] != -1 && path_based)
                                error = true;
                        k++;
                }
                if (error)
                {
                        cout << " ERROR : Slot # collision is occured!!." << endl;
                        exit(1);
                }
        }

        // output
        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
        int port = 0;
        //int slots = 0;
        //int pointer = 0;
        //int total_slots = 0;

        //cout << " (east, west, south, north, front, back, from host0,... east(vch2), south(vch2), west(vch2), north(vch2), front, back, from host(vch2)0,.. to host(vch2)0,.."
        cout << " === Number of slots === " << endl;
        //cout << " East, West, South, North, (Back, Front, ...) Out, In " << endl;
        cout << " X1, X2, (X3, X4, X5, X6, X7, X8, ...), Out, In " << endl;

        cout << " SW " << setw(2) << port / (degree + 1 + 2 * Host_Num) << ":  ";
        while (elem != Crossing_Paths.end())
        {
                if (port % (degree + 1 + 2 * Host_Num) != 0)
                {
                        if (flows.size() == 0)
                                cout << " " << (*elem).pair_index.size();
                        else
                                cout << " " << (*elem).flow_index.size();
                }

                //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();}

                //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

                elem++;
                port++;
                if (port % (degree + 1 + 2 * Host_Num) == 0)
                {
                        //pointer = 0;
                        cout << endl;
                        if (port != (degree + 1 + 2 * Host_Num) * switch_num)
                                cout << " SW " << setw(2) << port / (degree + 1 + 2 * Host_Num) << ":  ";
                }
        }
        //cout << endl;
        // setting for comparing (maximum) Cross_Path
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
        while (pt != Crossing_Paths.end())
        {
                (*pt).Valid = true;
                ++pt;
        }

        //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
        //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
        cout << " === The number of paths on this application ===" << endl
             << ct << " (all-to-all cases: " << (switch_num * Host_Num) * (switch_num * Host_Num - 1) << ")" << endl;
        cout << " === The average hops ===" << endl
             << setiosflags(ios::fixed | ios::showpoint) << (float)hops / ct + 1 << endl;
        //cout << " ID size(without ID modification)" << max_id << endl;
        //cout << " (Maximum) number of slots: " << slots;

        // routing table file output for each sw
        int target_sw;           // switch file to be written, sw0, sw1, ...
        int input_port;          // of target_sw
        int output_port;         // of target_sw
        int slot_num;            // assigned slot number for a node pair
        string output_port_s;    // string
        string input_port_s;     // string
        string slot_num_s;       // string
        system("rm output/sw*"); // delete previous output results
        if (flows.size() > 0)
                system("rm output/flow*");                          // delete previous output results
        cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
        for (int i = 0; i < pairs.size(); i++)
        {
                Pair current_pair = pairs[i];
                slot_num = current_pair.ID;
                if (flows.size() > 0)
                        slot_num = flows[current_pair.flow_id].ID;
                //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
                cout << " Pair ID " << current_pair.pair_id << " (Flow ID " << current_pair.flow_id << "): " << endl;

                if (flows.size() > 0)
                {
                        char filename[100];
                        sprintf(filename, "output/flow%d", current_pair.flow_id); // save to output/
                        char *fn = filename;
                        ofstream outputfile(fn, ios::app); // iostream append
                        outputfile << current_pair.src << " " << current_pair.src << " " << slot_num << endl;
                        outputfile << current_pair.dst << " " << current_pair.dst << " " << slot_num << endl;
                        outputfile.close();
                }

                for (int j = 1; j < current_pair.channels.size(); j++)
                { //current_pair.channels[0] --> src, current_pair.channels[current_pair.channels.size()-1] --> dst
                        target_sw = -1;
                        input_port = 0;
                        output_port = 0;
                        if (j == 1)
                        { // source switch
                                target_sw = current_pair.src;
                                output_port = current_pair.channels[j] % (degree + 1 + 2 * Host_Num);
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                                cout << "   SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
                        }
                        else if (j == current_pair.channels.size() - 1)
                        { // destination switch
                                target_sw = current_pair.dst;
                                input_port = current_pair.channels[j - 1] % (degree + 1 + 2 * Host_Num);
                                if (input_port % 2 == 0)
                                {
                                        input_port--; // 2-->1, 4-->3, ...
                                }
                                else
                                {
                                        input_port++; // 1-->2, 3-->4, ...
                                }
                                //cout << "SW " << target_sw;
                                cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                        }
                        else
                        {
                                target_sw = current_pair.channels[j] / (degree + 1 + 2 * Host_Num);
                                output_port = current_pair.channels[j] % (degree + 1 + 2 * Host_Num);
                                input_port = current_pair.channels[j - 1] % (degree + 1 + 2 * Host_Num);
                                if (input_port % 2 == 0)
                                {
                                        input_port--; // 2-->1, 4-->3, ...
                                }
                                else
                                {
                                        input_port++; // 1-->2, 3-->4, ...
                                }
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                                cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
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

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port);           // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);             // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);   // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);   // routing table <-- dst node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id); // routing table <-- pair id
                }
                cout << endl;
        }

        cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < switch_num; i++)
        {
                cout << " SW " << i << " : " << endl;
                char filename[100];
                sprintf(filename, "output/sw%d", i); // save to output/
                char *fn = filename;
                ofstream outputfile(fn, ios::app); // iostream append
                int slots;
                if (path_based == true)
                {
                        slots = max_cp;
                }
                else
                {
                        slots = max_cp_dst;
                }
                if (slots > default_slot)
                {
                        cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;
                        exit(1);
                }
                outputfile << degree + 1 << " " << default_slot << endl; // number of output ports, number of slots
                outputfile << "0000" << endl;                            // output 00 --> localhost
                bool slot_occupied = false;
                for (int s = 0; s < default_slot; s++)
                {
                        if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table.size() > 0)
                        {
                                for (int j = 0; j < Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table.size(); j = j + 5)
                                {
                                        if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 1] == s)
                                        { // j+1 --> slot number
                                                outputfile << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j] << " ";
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j]
                                                     << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 1]
                                                     << ") --> Port 0 (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 1]
                                                     << "), from node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 2]
                                                     << " to node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 3]
                                                     << " (Pair ID " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 4]
                                                     << ", Flow ID " << pairs[Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 4]].flow_id
                                                     << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                        }
                                }
                        }
                        // else if (Vch==2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size() > 0){ //torus
                        //         for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table.size(); j=j+5){
                        //                 if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] == s){  // j+1 --> slot number
                        //                         outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j] << " ";
                        //                         slot_occupied = true;
                        //                         cout << "      Port " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j] << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] << ") --> Port 0 (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+1] << "), from node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+2] << " to node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+degree+2*Host_Num].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                        //                 }
                        //         }
                        // }

                        if (slot_occupied == false)
                        {
                                outputfile << "void";
                        }
                        outputfile << endl;
                        slot_occupied = false;
                }
                // for (int s = 0; s < default_slot - slots; s++)
                //         outputfile << "void" << endl;

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

                for (int op = 1; op < degree + 1; op++)
                {
                        if (op < 10 && op > -1)
                        {
                                outputfile << "0" << op << "00" << endl;
                        }
                        else
                        {
                                outputfile << op << "00" << endl;
                        }
                        for (int s = 0; s < default_slot; s++)
                        {
                                if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table.size() > 0)
                                {
                                        for (int j = 0; j < Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table.size(); j = j + 5)
                                        {
                                                if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1] == s)
                                                {                                                                                                   // j+1 --> slot number
                                                        outputfile << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j] << " "; // j --> input port
                                                        slot_occupied = true;
                                                        cout << "      Port " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j]
                                                             << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1]
                                                             << ") --> Port " << op << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1]
                                                             << "), from node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 2]
                                                             << " to node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 3]
                                                             << " (Pair ID " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]
                                                             << ", Flow ID " << pairs[Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]].flow_id
                                                             << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                                }
                                        }
                                }
                                // else if (Vch==2 && Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size() > 0){ //torus
                                //         for (int j=0; j < Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table.size(); j=j+5){
                                //                 if (Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] == s){  // j+1 --> slot number
                                //                         outputfile << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j] << " "; // j --> input port
                                //                         slot_occupied = true;
                                //                         cout << "      Port " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j] << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] << ") --> Port " << op << " (Slot " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+1] << "), from node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+2] << " to node " << Crossing_Paths[Vch*(degree+1+2*Host_Num)*i+(degree+1+2*Host_Num)+op].routing_table[j+3] << endl; // j+2 --> src node, j+3 --> dst node
                                //                 }
                                //         }
                                // }

                                if (slot_occupied == false)
                                {
                                        outputfile << "void";
                                }
                                outputfile << endl;
                                slot_occupied = false;
                        }
                        // for (int s = 0; s < default_slot - slots; s++)
                        //         outputfile << "void" << endl;

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
void show_paths_tree(vector<Cross_Paths> Crossing_Paths, int ct, int node_num,
                     int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
                     bool path_based, int PORT, int default_slot, vector<Flow> flows)
{
        // for each channel
        //cout << " === Port information for each switch === " << endl;
        for (int i = 0; i <= PORT * (node_num + node_num / Host_Num + node_num / (int)pow(Host_Num, 2)); i++)
        {
                vector<int> ID_array(Crossing_Paths[i].pair_index.size(), -1);
                //      cout << " Channels (" << i << ")" << endl;
                //if (i%PORT == 0) {
                //        if ( i == PORT * (node_num+node_num/Host_Num+node_num/(int)pow(Host_Num,2)) && node_num/(int)pow(Host_Num,2) <= 1 ) break; // two-layer switches
                //        cout << " Node " << i/PORT << " : " << endl;
                //}
                // for each node pair passing through a channel
                for (unsigned int j = 0; j < Crossing_Paths[i].pair_index.size(); j++)
                {
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
                sort(ID_array.begin(), ID_array.end());
                unsigned int k = 0;
                bool error = false;
                while (k + 1 < ID_array.size() && !error)
                {
                        if (ID_array[k] == ID_array[k + 1] && ID_array[k] != -1 && path_based)
                                error = true;
                        k++;
                }
                if (error)
                {
                        cout << " ERROR : Slot # collision is occured!!." << endl;
                        exit(1);
                }
        }

        // output
        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin();
        int outport = 0;
        cout << " === Number of slots === " << endl;
        cout << " UP0, DOWN1, DOWN2, DOWN3, DOWN4, ... " << endl;
        cout << " Node " << setw(2) << outport / PORT << ":  ";
        while (elem != Crossing_Paths.end())
        {
                if (flows.size() == 0)
                        cout << " " << (*elem).pair_index.size();
                else
                        cout << " " << (*elem).flow_index.size();
                elem++;
                outport++;
                if (outport % PORT == 0 && elem != Crossing_Paths.end())
                {
                        if (outport == PORT * (node_num + node_num / Host_Num + node_num / (int)pow(Host_Num, 2)) && node_num / (int)pow(Host_Num, 2) <= 1)
                                break; // two-layer switches
                        cout << endl;
                        cout << " Node " << setw(2) << outport / PORT << ":  ";
                }
        }
        cout << endl;

        // setting for comparing (maximum) Cross_Path
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
        while (pt != Crossing_Paths.end())
        {
                (*pt).Valid = true;
                ++pt;
        }

        //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
        //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
        cout << " === The number of paths on this application ===" << endl
             << ct << " (all-to-all cases: " << node_num * (node_num - 1) << ")" << endl;
        cout << " === The average hops ===" << endl
             << setiosflags(ios::fixed | ios::showpoint) << (float)hops / ct + 1 << endl;
        //cout << " ID size(without ID modification)" << max_id << endl;
        //cout << " (Maximum) number of slots: " << slots;

        // routing table file output for each sw
        int target_sw;           // switch file to be written, sw0, sw1, ...
        int input_port;          // of target_sw
        int output_port;         // of target_sw
        int slot_num;            // assigned slot number for a node pair
        string output_port_s;    // string
        string input_port_s;     // string
        string slot_num_s;       // string
        system("rm output/sw*"); // delete previous output results
        if (flows.size() > 0)
                system("rm output/flow*");                          // delete previous output results
        cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
        for (int i = 0; i < pairs.size(); i++)
        {
                Pair current_pair = pairs[i];
                slot_num = current_pair.ID;
                if (flows.size() > 0)
                        slot_num = flows[current_pair.flow_id].ID;
                //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
                cout << " Pair ID " << current_pair.pair_id << " (Flow ID " << current_pair.flow_id << "): " << endl;

                if (flows.size() > 0)
                {
                        char filename[100];
                        sprintf(filename, "output/flow%d", current_pair.flow_id); // save to output/
                        char *fn = filename;
                        ofstream outputfile(fn, ios::app); // iostream append
                        outputfile << current_pair.src << " " << current_pair.src << " " << slot_num << endl;
                        outputfile << current_pair.dst << " " << current_pair.dst << " " << slot_num << endl;
                        outputfile.close();
                }

                for (int j = 1; j < current_pair.channels.size(); j++)
                { //current_pair.channels[0] --> from src node, current_pair.channels[-1] --> to dst node
                        target_sw = -1;
                        input_port = -1;
                        output_port = -1;

                        target_sw = current_pair.channels[j] / PORT - node_num;
                        output_port = current_pair.channels[j] % PORT;
                        input_port = current_pair.channels[j - 1] % PORT;
                        if (input_port != 0)
                        { // down --> down
                                input_port = 0;
                        }
                        else
                        {
                                if (output_port != 0)
                                { // up --> down
                                        input_port = (current_pair.channels[j - 1] / PORT) % Host_Num + 1;
                                }
                                else
                                { // up --> up
                                        input_port = (current_pair.channels[j - 1] / PORT) % Host_Num + 1;
                                }
                        }
                        if (j == 1)
                        { //src node
                                //cout << "Node " << current_pair.h_src << " (port 0)" << " --> ";
                                cout << "   Node " << current_pair.h_src << " - [slot " << slot_num << "] -> ";
                        }
                        //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                        cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                             << " - [slot " << slot_num << "] -> ";
                        if (j == current_pair.channels.size() - 1)
                        { //dst node
                                //cout << "Node " << current_pair.h_dst << " (port 0)";
                                cout << "Node " << current_pair.h_dst;
                        }

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port);           // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);             // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);   // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);   // routing table <-- dst node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id); // routing table <-- pair id
                }
                cout << endl;
        }

        int switch_num;
        if (node_num / (int)pow(Host_Num, 2) <= 1)
        {
                switch_num = node_num / Host_Num + node_num / (int)pow(Host_Num, 2);
        }
        else
        {
                switch_num = node_num / Host_Num + node_num / (int)pow(Host_Num, 2) + 1;
        }

        cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < switch_num; i++)
        {
                cout << " SW " << i << " : " << endl;
                char filename[100];
                sprintf(filename, "output/sw%d", i); // save to output/
                char *fn = filename;
                ofstream outputfile(fn, ios::app); // iostream append
                int slots;
                if (path_based == true)
                {
                        slots = max_cp;
                }
                else
                {
                        slots = max_cp_dst;
                }
                if (slots > default_slot)
                {
                        cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;
                        exit(1);
                }
                outputfile << PORT << " " << default_slot << endl; // number of output ports, number of slots

                bool slot_occupied = false;
                for (int op = 0; op < PORT; op++)
                {
                        if (op < 10 && op > -1)
                        {
                                outputfile << "0" << op << "00" << endl;
                        }
                        else
                        {
                                outputfile << op << "00" << endl;
                        }
                        for (int s = 0; s < default_slot; s++)
                        {
                                for (int j = 0; j < Crossing_Paths[PORT * (i + node_num) + op].routing_table.size(); j = j + 5)
                                { //i+node_num --> sw #
                                        if (Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 1] == s)
                                        {                                                                                         // j+1 --> slot number
                                                outputfile << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j] << " "; // j --> input port
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j]
                                                     << " (Slot " << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 1] << ") --> Port " << op
                                                     << " (Slot " << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 1]
                                                     << "), from node " << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 2]
                                                     << " to node " << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 3]
                                                     << " (Pair ID " << Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 4]
                                                     << ", Flow ID " << pairs[Crossing_Paths[PORT * (i + node_num) + op].routing_table[j + 4]].flow_id
                                                     << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                        }
                                }
                                if (slot_occupied == false)
                                {
                                        outputfile << "void";
                                }
                                outputfile << endl;
                                slot_occupied = false;
                        }
                        // for (int s = 0; s < default_slot - slots; s++)
                        //         outputfile << "void" << endl;
                }
                outputfile.close();
        }
        cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
        cout << " ### OVER ###" << endl;
}

//
// output and error check (fully-connected)
//
void show_paths_fullyconnected(vector<Cross_Paths> Crossing_Paths, int ct, int switch_num,
                               int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
                               bool path_based, int degree, int default_slot, vector<Flow> flows)
{
        // for each channel
        //cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < (degree + 1 + 2 * Host_Num) * switch_num; i++)
        {
                vector<int> ID_array(Crossing_Paths[i].pair_index.size(), -1);
                //      cout << " Channels (" << i << ")" << endl;
                //if (i%(degree+1+2*Host_Num) == 0) cout << " SW " << i/(degree+1+2*Host_Num) << " : " << endl;
                // for each node pair passing through a channel
                for (unsigned int j = 0; j < Crossing_Paths[i].pair_index.size(); j++)
                {
                        int t = Crossing_Paths[i].pair_index[j];
                        //	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
                        ID_array.push_back(pairs[t].ID);

                        //if ( (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -2) && (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -1) ) { // -2 and -1 --> no meaning
                        //        cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
                        //}
                }
                sort(ID_array.begin(), ID_array.end());
                unsigned int k = 0;
                bool error = false;
                while (k + 1 < ID_array.size() && !error)
                {
                        if (ID_array[k] == ID_array[k + 1] && ID_array[k] != -1 && path_based)
                                error = true;
                        k++;
                }
                if (error)
                {
                        cout << " ERROR : Slot # collision is occured!!." << endl;
                        exit(1);
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

        cout << " SW " << setw(2) << port / (degree + 1 + 2 * Host_Num) << ":  ";
        while (elem != Crossing_Paths.end())
        {
                if (port % (degree + 1 + 2 * Host_Num) != degree + 2 * Host_Num && port % (degree + 1 + 2 * Host_Num) != degree + 2 * Host_Num - 1)
                {
                        if (flows.size() == 0)
                                cout << " " << (*elem).pair_index.size();
                        else
                                cout << " " << (*elem).flow_index.size();
                }
                //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();}

                //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

                elem++;
                port++;
                if (port % (degree + 1 + 2 * Host_Num) == 0)
                {
                        //pointer = 0;
                        cout << endl;
                        if (port != (degree + 1 + 2 * Host_Num) * switch_num)
                                cout << " SW " << setw(2) << port / (degree + 1 + 2 * Host_Num) << ":  ";
                }
        }
        //cout << endl;
        // setting for comparing (maximum) Cross_Path
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
        while (pt != Crossing_Paths.end())
        {
                (*pt).Valid = true;
                ++pt;
        }

        //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
        //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
        cout << " === The number of paths on this application ===" << endl
             << ct << " (all-to-all cases: " << (switch_num * Host_Num) * (switch_num * Host_Num - 1) << ")" << endl;
        cout << " === The average hops ===" << endl
             << setiosflags(ios::fixed | ios::showpoint) << (float)hops / ct + 1 << endl;
        //cout << " ID size(without ID modification)" << max_id << endl;
        //cout << " (Maximum) number of slots: " << slots;

        // routing table file output for each sw
        int target_sw;           // switch file to be written, sw0, sw1, ...
        int input_port;          // of target_sw
        int output_port;         // of target_sw
        int slot_num;            // assigned slot number for a node pair
        system("rm output/sw*"); // delete previous output results
        if (flows.size() > 0)
                system("rm output/flow*");                          // delete previous output results
        cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
        for (int i = 0; i < pairs.size(); i++)
        {
                Pair current_pair = pairs[i];
                slot_num = current_pair.ID;
                if (flows.size() > 0)
                        slot_num = flows[current_pair.flow_id].ID;
                //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
                cout << " Pair ID " << current_pair.pair_id << " (Flow ID " << current_pair.flow_id << "): " << endl;

                if (flows.size() > 0)
                {
                        char filename[100];
                        sprintf(filename, "output/flow%d", current_pair.flow_id); // save to output/
                        char *fn = filename;
                        ofstream outputfile(fn, ios::app); // iostream append
                        outputfile << current_pair.src << " " << current_pair.src << " " << slot_num << endl;
                        outputfile << current_pair.dst << " " << current_pair.dst << " " << slot_num << endl;
                        outputfile.close();
                }

                for (int j = 0; j < current_pair.channels.size(); j++)
                { // only current_pair.channels[0] --> from src to dst
                        target_sw = -1;
                        input_port = 0;
                        output_port = 0;
                        if (j == 0)
                        { // source switch
                                target_sw = current_pair.src;
                                output_port = current_pair.channels[j] % (degree + 1 + 2 * Host_Num); // output_port = current_pair.dst
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> SW " << current_pair.dst;
                                input_port = current_pair.src;
                                cout << "       SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
                        }

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port);           // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);             // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);   // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);   // routing table <-- dst node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id); // routing table <-- pair id

                        target_sw = current_pair.dst;
                        input_port = current_pair.src;
                        output_port = current_pair.dst;                                                                                                  // localhost
                        Crossing_Paths[current_pair.dst * (degree + 1 + 2 * Host_Num) + current_pair.dst].routing_table.push_back(input_port);           // routing table <-- input port
                        Crossing_Paths[current_pair.dst * (degree + 1 + 2 * Host_Num) + current_pair.dst].routing_table.push_back(slot_num);             // routing table <-- slot number
                        Crossing_Paths[current_pair.dst * (degree + 1 + 2 * Host_Num) + current_pair.dst].routing_table.push_back(current_pair.h_src);   // routing table <-- src node
                        Crossing_Paths[current_pair.dst * (degree + 1 + 2 * Host_Num) + current_pair.dst].routing_table.push_back(current_pair.h_dst);   // routing table <-- dst node
                        Crossing_Paths[current_pair.dst * (degree + 1 + 2 * Host_Num) + current_pair.dst].routing_table.push_back(current_pair.pair_id); // routing table <-- pair id

                        cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                }
                cout << endl;
        }

        cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < switch_num; i++)
        {
                cout << " SW " << i << " : " << endl;
                char filename[100];
                sprintf(filename, "output/sw%d", i); // save to output/
                char *fn = filename;
                ofstream outputfile(fn, ios::app); // iostream append
                int slots;
                if (path_based == true)
                {
                        slots = max_cp;
                }
                else
                {
                        slots = max_cp_dst;
                }
                if (slots > default_slot)
                {
                        cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;
                        exit(1);
                }
                outputfile << degree + 1 << " " << default_slot << endl; // number of output ports, number of slots
                bool slot_occupied = false;
                for (int op = 0; op < degree + 1; op++)
                {
                        if (op < 10 && op > -1)
                        {
                                outputfile << "0" << op << "00" << endl;
                        }
                        else
                        {
                                outputfile << op << "00" << endl;
                        }
                        for (int s = 0; s < default_slot; s++)
                        {
                                if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table.size() > 0)
                                {
                                        for (int j = 0; j < Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table.size(); j = j + 5)
                                        {
                                                if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1] == s)
                                                {                                                                                                   // j+1 --> slot number
                                                        outputfile << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j] << " "; // j --> input port
                                                        slot_occupied = true;
                                                        cout << "      Port " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j]
                                                             << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1]
                                                             << ") --> Port " << op << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1]
                                                             << "), from node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 2]
                                                             << " to node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 3]
                                                             << " (Pair ID " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]
                                                             << ", Flow ID " << pairs[Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]].flow_id
                                                             << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                                }
                                        }
                                }
                                if (slot_occupied == false)
                                {
                                        outputfile << "void";
                                }
                                outputfile << endl;
                                slot_occupied = false;
                        }
                        // for (int s = 0; s < default_slot - slots; s++)
                        //         outputfile << "void" << endl;
                }
                outputfile.close();
        }
        cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
        cout << " ### OVER ###" << endl;
}

//
// output and error check (fcc)
//
void show_paths_fcc(vector<Cross_Paths> Crossing_Paths, int ct, int switch_num,
                    int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
                    bool path_based, int degree, int default_slot, vector<Flow> flows)
{
        // for each channel
        //cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < (degree + 1 + 2 * Host_Num) * switch_num; i++)
        {
                vector<int> ID_array(Crossing_Paths[i].pair_index.size(), -1);
                //      cout << " Channels (" << i << ")" << endl;
                //if (i%(degree+1+2*Host_Num) == 0) cout << " SW " << i/(degree+1+2*Host_Num) << " : " << endl;
                // for each node pair passing through a channel
                for (unsigned int j = 0; j < Crossing_Paths[i].pair_index.size(); j++)
                {
                        int t = Crossing_Paths[i].pair_index[j];
                        //	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
                        ID_array.push_back(pairs[t].ID);

                        //if ( (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -2) && (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -1) ) { // -2 and -1 --> no meaning
                        //        cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
                        //}
                }
                sort(ID_array.begin(), ID_array.end());
                unsigned int k = 0;
                bool error = false;
                while (k + 1 < ID_array.size() && !error)
                {
                        if (ID_array[k] == ID_array[k + 1] && ID_array[k] != -1 && path_based)
                                error = true;
                        k++;
                }
                if (error)
                {
                        cout << " ERROR : Slot # collision is occured!!." << endl;
                        exit(1);
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

        cout << " SW " << setw(2) << port / (degree + 1 + 2 * Host_Num) << ":  ";
        while (elem != Crossing_Paths.end())
        {
                if (port % (degree + 1 + 2 * Host_Num) != 0 && port % (degree + 1 + 2 * Host_Num) != degree + 2 * Host_Num && port % (degree + 1 + 2 * Host_Num) != degree + 2 * Host_Num - 1)
                {
                        if (flows.size() == 0)
                                cout << " " << (*elem).pair_index.size();
                        else
                                cout << " " << (*elem).flow_index.size();
                }
                //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();}

                //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

                elem++;
                port++;
                if (port % (degree + 1 + 2 * Host_Num) == 0)
                {
                        //pointer = 0;
                        cout << endl;
                        if (port != (degree + 1 + 2 * Host_Num) * switch_num)
                                cout << " SW " << setw(2) << port / (degree + 1 + 2 * Host_Num) << ":  ";
                }
        }
        //cout << endl;
        // setting for comparing (maximum) Cross_Path
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
        while (pt != Crossing_Paths.end())
        {
                (*pt).Valid = true;
                ++pt;
        }

        //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
        //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
        cout << " === The number of paths on this application ===" << endl
             << ct << " (all-to-all cases: " << (switch_num * Host_Num) * (switch_num * Host_Num - 1) << ")" << endl;
        cout << " === The average hops ===" << endl
             << setiosflags(ios::fixed | ios::showpoint) << (float)hops / ct + 1 << endl;
        //cout << " ID size(without ID modification)" << max_id << endl;
        //cout << " (Maximum) number of slots: " << slots;

        // routing table file output for each sw
        int target_sw;           // switch file to be written, sw0, sw1, ...
        int input_port;          // of target_sw
        int output_port;         // of target_sw
        int slot_num;            // assigned slot number for a node pair
        system("rm output/sw*"); // delete previous output results
        if (flows.size() > 0)
                system("rm output/flow*");                          // delete previous output results
        cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
        for (int i = 0; i < pairs.size(); i++)
        {
                Pair current_pair = pairs[i];
                slot_num = current_pair.ID;
                if (flows.size() > 0)
                        slot_num = flows[current_pair.flow_id].ID;
                //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
                cout << " Pair ID " << current_pair.pair_id << " (Flow ID " << current_pair.flow_id << "): " << endl;

                if (flows.size() > 0)
                {
                        char filename[100];
                        sprintf(filename, "output/flow%d", current_pair.flow_id); // save to output/
                        char *fn = filename;
                        ofstream outputfile(fn, ios::app); // iostream append
                        outputfile << current_pair.src << " " << current_pair.src << " " << slot_num << endl;
                        outputfile << current_pair.dst << " " << current_pair.dst << " " << slot_num << endl;
                        outputfile.close();
                }

                for (int j = 1; j < current_pair.channels.size(); j++)
                { //current_pair.channels[0] --> src, current_pair.channels[current_pair.channels.size()-1] --> dst
                        target_sw = -1;
                        input_port = 0;
                        output_port = 0;
                        if (j == 1)
                        { // source switch
                                target_sw = current_pair.src;
                                output_port = current_pair.channels[j] % (degree + 1 + 2 * Host_Num);
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                                cout << "   SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
                        }
                        else if (j == current_pair.channels.size() - 1)
                        { // destination switch
                                target_sw = current_pair.dst;
                                input_port = current_pair.channels[j - 1] % (degree + 1 + 2 * Host_Num);
                                if (input_port == 1)
                                        input_port = 2;
                                else if (input_port == 2)
                                        input_port = 1;
                                else
                                        input_port = (3 + degree) - input_port;
                                //cout << "SW " << target_sw;
                                cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                        }
                        else
                        {
                                target_sw = current_pair.channels[j] / (degree + 1 + 2 * Host_Num);
                                output_port = current_pair.channels[j] % (degree + 1 + 2 * Host_Num);
                                input_port = current_pair.channels[j - 1] % (degree + 1 + 2 * Host_Num);
                                if (input_port == 1)
                                        input_port = 2;
                                else if (input_port == 2)
                                        input_port = 1;
                                else
                                        input_port = (3 + degree) - input_port;
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                                cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
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

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port);           // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);             // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);   // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);   // routing table <-- dst node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id); // routing table <-- pair id
                }
                cout << endl;
        }

        cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < switch_num; i++)
        {
                cout << " SW " << i << " : " << endl;
                char filename[100];
                sprintf(filename, "output/sw%d", i); // save to output/
                char *fn = filename;
                ofstream outputfile(fn, ios::app); // iostream append
                int slots;
                if (path_based == true)
                {
                        slots = max_cp;
                }
                else
                {
                        slots = max_cp_dst;
                }
                if (slots > default_slot)
                {
                        cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;
                        exit(1);
                }
                outputfile << degree + 1 << " " << default_slot << endl; // number of output ports, number of slots
                outputfile << "0000" << endl;                            // output 00 --> localhost
                bool slot_occupied = false;
                for (int s = 0; s < default_slot; s++)
                {
                        if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table.size() > 0)
                        {
                                for (int j = 0; j < Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table.size(); j = j + 5)
                                {
                                        if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 1] == s)
                                        { // j+1 --> slot number
                                                outputfile << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j] << " ";
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j]
                                                     << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 1]
                                                     << ") --> Port 0 (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 1]
                                                     << "), from node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 2]
                                                     << " to node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 3]
                                                     << " (Pair ID " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 4]
                                                     << ", Flow ID " << pairs[Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + degree + 2 * Host_Num].routing_table[j + 4]].flow_id
                                                     << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                        }
                                }
                        }
                        if (slot_occupied == false)
                        {
                                outputfile << "void";
                        }
                        outputfile << endl;
                        slot_occupied = false;
                }
                // for (int s = 0; s < default_slot - slots; s++)
                //         outputfile << "void" << endl;

                for (int op = 1; op < degree + 1; op++)
                {
                        if (op < 10 && op > -1)
                        {
                                outputfile << "0" << op << "00" << endl;
                        }
                        else
                        {
                                outputfile << op << "00" << endl;
                        }
                        for (int s = 0; s < default_slot; s++)
                        {
                                if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table.size() > 0)
                                {
                                        for (int j = 0; j < Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table.size(); j = j + 5)
                                        {
                                                if (Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1] == s)
                                                {                                                                                                   // j+1 --> slot number
                                                        outputfile << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j] << " "; // j --> input port
                                                        slot_occupied = true;
                                                        cout << "      Port " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j]
                                                             << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1] << ") --> Port " << op
                                                             << " (Slot " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 1] << "), from node "
                                                             << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 2]
                                                             << " to node " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 3]
                                                             << " (Pair ID " << Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]
                                                             << ", Flow ID " << pairs[Crossing_Paths[(degree + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]].flow_id
                                                             << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                                }
                                        }
                                }
                                if (slot_occupied == false)
                                {
                                        outputfile << "void";
                                }
                                outputfile << endl;
                                slot_occupied = false;
                        }
                        // for (int s = 0; s < default_slot - slots; s++)
                        //         outputfile << "void" << endl;
                }
                outputfile.close();
        }
        cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
        cout << " ### OVER ###" << endl;
}

//
// output and error check (topology file)
//
void show_paths_tf(vector<Cross_Paths> Crossing_Paths, int ct, int switch_num,
                   int max_id, vector<Pair> pairs, int hops, int Host_Num, int max_cp, int max_cp_dst,
                   bool path_based, int degree, vector<int> Switch_Topo, vector<int> topo_sws_uni, int default_slot, vector<Flow> flows)
{
        // for each channel
        //cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < ((switch_num - 1) + 1 + 2 * Host_Num) * switch_num; i++)
        {
                vector<int> ID_array(Crossing_Paths[i].pair_index.size(), -1);
                //      cout << " Channels (" << i << ")" << endl;
                //if (i%(degree+1+2*Host_Num) == 0) cout << " SW " << i/(degree+1+2*Host_Num) << " : " << endl;
                // for each node pair passing through a channel
                for (unsigned int j = 0; j < Crossing_Paths[i].pair_index.size(); j++)
                {
                        int t = Crossing_Paths[i].pair_index[j];
                        //	 cout << " channel from " << pairs[t].src << " to " << pairs[t].dst << " is assigned to the ID " << pairs[t].ID << endl;
                        ID_array.push_back(pairs[t].ID);

                        //if ( (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -2) && (i%(degree+1+2*Host_Num) != degree+1+2*Host_Num -1) ) { // -2 and -1 --> no meaning
                        //        cout << "      Port " << i%(degree+1+2*Host_Num) << " --> Pair ID " << pairs[t].pair_id << " (local ID " << pairs[t].ID << "), from node " << pairs[t].h_src << " to node " << pairs[t].h_dst << endl;
                        //}
                }
                sort(ID_array.begin(), ID_array.end());
                unsigned int k = 0;
                bool error = false;
                while (k + 1 < ID_array.size() && !error)
                {
                        if (ID_array[k] == ID_array[k + 1] && ID_array[k] != -1 && path_based)
                                error = true;
                        k++;
                }
                if (error)
                {
                        cout << " ERROR : Slot # collision is occured!!." << endl;
                        exit(1);
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
        cout << " SW0, SW1, SW2, SW3, SW4, ..., out, in " << endl;

        cout << " SW " << setw(2) << port / ((switch_num - 1) + 1 + 2 * Host_Num) << ":  ";
        while (elem != Crossing_Paths.end())
        {
                // if (port % ((switch_num - 1) + 1 + 2 * Host_Num) != (switch_num - 1) + 2 * Host_Num && port % ((switch_num - 1) + 1 + 2 * Host_Num) != (switch_num - 1) + 2 * Host_Num - 1)
                // {
                        if (flows.size() == 0)
                                cout << " " << (*elem).pair_index.size();
                        else
                                cout << " " << (*elem).flow_index.size();
                // }
                //if (pointer<degree && slots<(*elem).pair_index.size())  {pointer++; slots = (*elem).pair_index.size();}

                //if (port%(degree+1+2*Host_Num)!=0 && port%(degree+1+2*Host_Num)!=degree+2*Host_Num && port%(degree+1+2*Host_Num)!=degree+2*Host_Num-1)  total_slots += (*elem).pair_index.size();

                elem++;
                port++;
                if (port % ((switch_num - 1) + 1 + 2 * Host_Num) == 0)
                {
                        //pointer = 0;
                        cout << endl;
                        if (port != ((switch_num - 1) + 1 + 2 * Host_Num) * switch_num)
                                cout << " SW " << setw(2) << port / ((switch_num - 1) + 1 + 2 * Host_Num) << ":  ";
                }
        }
        //cout << endl;
        // setting for comparing (maximum) Cross_Path
        vector<Cross_Paths>::iterator pt = Crossing_Paths.begin();
        while (pt != Crossing_Paths.end())
        {
                (*pt).Valid = true;
                ++pt;
        }

        //cout << " (Maximum) Crossing Paths: " << max_element(Crossing_Paths.begin(),Crossing_Paths.end())->pair_index.size() << endl;
        //cout << " (Maximum) Crossing Paths: " << max_cp << endl;
        cout << " === The number of paths on this application ===" << endl
             << ct << " (all-to-all cases: " << (switch_num * Host_Num) * (switch_num * Host_Num - 1) << ")" << endl;
        cout << " === The average hops ===" << endl
             << setiosflags(ios::fixed | ios::showpoint) << (float)hops / ct << endl;
        //cout << " ID size(without ID modification)" << max_id << endl;
        //cout << " (Maximum) number of slots: " << slots;

        // routing table file output for each sw
        int target_sw;           // switch file to be written, sw0, sw1, ...
        int input_port;          // of target_sw
        int output_port;         // of target_sw
        int slot_num;            // assigned slot number for a node pair
        system("rm output/sw*"); // delete previous output results
        if (flows.size() > 0)
                system("rm output/flow*");                          // delete previous output results
        cout << " === Routing path for each node pair ===" << endl; //routing information of each node pair
        for (int i = 0; i < pairs.size(); i++)
        {
                Pair current_pair = pairs[i];
                slot_num = current_pair.ID;
                if (flows.size() > 0)
                        slot_num = flows[current_pair.flow_id].ID;
                //cout << " Pair ID " << current_pair.pair_id << " (Slot " << slot_num << "): ";
                cout << " Pair ID " << current_pair.pair_id << " (Flow ID " << current_pair.flow_id << "): " << endl;

                if (flows.size() > 0)
                {
                        char filename[100];
                        sprintf(filename, "output/flow%d", current_pair.flow_id); // save to output/
                        char *fn = filename;
                        ofstream outputfile(fn, ios::app); // iostream append
                        outputfile << current_pair.src << " " << current_pair.src << " " << slot_num << endl;
                        outputfile << current_pair.dst << " " << current_pair.dst << " " << slot_num << endl;
                        outputfile.close();
                }

                int src_index = -1;
                int dst_index = -1;
                for (int i = 0; i < switch_num; i++)
                {
                        if (topo_sws_uni[i] == current_pair.src)
                        {
                                src_index = i;
                        }
                        if (topo_sws_uni[i] == current_pair.dst)
                        {
                                dst_index = i;
                        }
                        if (src_index != -1 && dst_index != -1)
                                break;
                }
                if (src_index == -1 || dst_index == -1)
                        cout << "Error: src/dst number is wrong" << endl;

                for (int j = 1; j < current_pair.channels.size(); j++)
                { //current_pair.channels[0] --> src, current_pair.channels[current_pair.channels.size()-1] --> dst
                        target_sw = -1;
                        input_port = 0;
                        output_port = 0;
                        if (j == 1)
                        { // source switch
                                target_sw = current_pair.src;
                                //output_port = Switch_Topo[src_index*((switch_num-1)+1+2*Host_Num)+current_pair.channels[j]%((switch_num-1)+1+2*Host_Num)];
                                output_port = Switch_Topo[current_pair.channels[j]];
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                                cout << "   SW " << target_sw << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
                        }
                        else if (j == current_pair.channels.size() - 1)
                        { // destination switch
                                target_sw = current_pair.dst;
                                input_port = Switch_Topo[dst_index * ((switch_num - 1) + 1 + 2 * Host_Num) + current_pair.channels[j - 1] / ((switch_num - 1) + 1 + 2 * Host_Num)];
                                //cout << "SW " << target_sw;
                                cout << "SW " << target_sw << " (port " << input_port << "->" << output_port << ")";
                        }
                        else
                        {
                                target_sw = current_pair.channels[j] / ((switch_num - 1) + 1 + 2 * Host_Num);
                                output_port = Switch_Topo[current_pair.channels[j]];
                                input_port = Switch_Topo[target_sw * ((switch_num - 1) + 1 + 2 * Host_Num) + current_pair.channels[j - 1] / ((switch_num - 1) + 1 + 2 * Host_Num)];
                                //cout << "SW " << target_sw << " (port " << output_port << ")" << " --> ";
                                cout << "SW " << topo_sws_uni[target_sw] << " (port " << input_port << "->" << output_port << ")"
                                     << " - [slot " << slot_num << "] -> ";
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

                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(input_port);           // routing table <-- input port
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(slot_num);             // routing table <-- slot number
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_src);   // routing table <-- src node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.h_dst);   // routing table <-- dst node
                        Crossing_Paths[current_pair.channels[j]].routing_table.push_back(current_pair.pair_id); // routing table <-- pair id
                }
                cout << endl;
        }

        cout << " === Port information for each switch === " << endl;
        for (int i = 0; i < switch_num; i++)
        {
                cout << " SW " << topo_sws_uni[i] << " : " << endl;
                char filename[100];
                sprintf(filename, "output/sw%d", topo_sws_uni[i]); // save to output/
                char *fn = filename;
                ofstream outputfile(fn, ios::app); // iostream append
                int slots;
                if (path_based == true)
                {
                        slots = max_cp;
                }
                else
                {
                        slots = max_cp_dst;
                }
                if (slots > default_slot)
                {
                        cout << " ERROR : Slot # (" << slots << ") is larger than the default slot " << default_slot << endl;
                        exit(1);
                }
                outputfile << degree + 1 << " " << default_slot << endl; // number of output ports, number of slots
                outputfile << "0000" << endl;                            // output 00 --> localhost
                bool slot_occupied = false;
                for (int s = 0; s < default_slot; s++)
                {
                        if (Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table.size() > 0)
                        {
                                for (int j = 0; j < Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table.size(); j = j + 5)
                                {
                                        if (Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 1] == s)
                                        { // j+1 --> slot number
                                                outputfile << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j] << " ";
                                                slot_occupied = true;
                                                cout << "      Port " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j]
                                                     << " (Slot " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 1]
                                                     << ") --> Port 0 (Slot " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 1]
                                                     << "), from node " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 2]
                                                     << " to node " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 3]
                                                     << " (Pair ID " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 4]
                                                     << ", Flow ID " << pairs[Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + (switch_num - 1) + 2 * Host_Num].routing_table[j + 4]].flow_id
                                                     << ")" << endl; // j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                        }
                                }
                        }
                        if (slot_occupied == false)
                        {
                                outputfile << "void";
                        }
                        outputfile << endl;
                        slot_occupied = false;
                }
                // for (int s = 0; s < default_slot - slots; s++)
                //         outputfile << "void" << endl;

                for (int n = 1; n < degree+1; n++){                     
                        for (int op = 0; op < (switch_num - 1) + 1; op++)
                        {
                                int out_port = Switch_Topo[((switch_num - 1) + 1 + 2 * Host_Num) * i + op];
                                //if (out_port != -1)
                                if (out_port == n)
                                {
                                        if (out_port < 10 && out_port > -1)
                                        {
                                                outputfile << "0" << out_port << "00" << endl;
                                        }
                                        else
                                        {
                                                outputfile << out_port << "00" << endl;
                                        }
                                        for (int s = 0; s < default_slot; s++)
                                        {
                                                vector<int> temp_ip;
                                                vector<int>::iterator it;   
                                                if (Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table.size() > 0)
                                                {
                                                        for (int j = 0; j < Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table.size(); j = j + 5)
                                                        {
                                                                if (Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 1] == s)
                                                                {                                                                                                             
                                                                        int input_port = Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j];
                                                                        it = find(temp_ip.begin(), temp_ip.end(), input_port);
                                                                        if (it == temp_ip.end()){
                                                                                outputfile << input_port << " ";
                                                                                temp_ip.push_back(input_port);
                                                                        }
                                                                        
                                                                        slot_occupied = true;
                                                                        cout << "      Port " << input_port
                                                                        << " (Slot " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 1]
                                                                        << ") --> Port " << out_port << " (Slot " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 1]
                                                                        << "), from node " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 2]
                                                                        << " to node " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 3]
                                                                        << " (Pair ID " << Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]
                                                                        << ", Flow ID " << pairs[Crossing_Paths[((switch_num - 1) + 1 + 2 * Host_Num) * i + op].routing_table[j + 4]].flow_id
                                                                        << ")" << endl; // j --> input port, j+1 --> slot number, j+2 --> src node, j+3 --> dst node, j+4 --> pair id
                                                                }
                                                        }
                                                }
                                                if (slot_occupied == false)
                                                {
                                                        outputfile << "void";
                                                }
                                                outputfile << endl;
                                                slot_occupied = false;
                                        }
                                        // for (int s = 0; s < default_slot - slots; s++)
                                        //         outputfile << "void" << endl;
                                }
                        }
                }
                outputfile.close();
        }
        cout << " !!! Routing tables for each sw are saved to output/ !!!" << endl;
        cout << " ### OVER ###" << endl;
}

// ########################################## //
// ##  C program for Dijkstra's single source shortest path algorithm.
// ##  The program is for adjacency matrix representation of the graph. ## //
// ##  Modified based on https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/  ## //
// ########################################## //

// A utility function to find the vertex with minimum distance value, from the set of vertices not yet included in shortest path tree
int minDistance(int dist[], bool sptSet[], int V)
{
        // Initialize min value
        int min = 10000, min_index;

        for (int v = 0; v < V; v++)
                if (sptSet[v] == false && dist[v] <= min)
                {
                        min = dist[v];
                        min_index = v;
                }

        return min_index;
}

// Function to print shortest path from source to j using parent array
void printPath(int parent[], int j, int src, int dst, vector<vector<int>> &pair_path, int V)
{
        // Base Case : If j is source
        if (parent[j] == -1)
                return;

        printPath(parent, parent[j], src, dst, pair_path, V);

        //printf("%d ", j);
        pair_path[src * V + dst].push_back(j);
}

// A utility function to print the constructed distance array
void printSolution(int dist[], int V, int parent[], int src, vector<vector<int>> &pair_path)
{
        //int src = 0;
        //printf("Vertex\t Distance\tPath");
        for (int i = 0; i < V; i++)
        {
                if (i != src)
                {
                        //printf("\n%d -> %d \t\t %d\t\t%d ", src, i, dist[i], src);
                        int dst = i;
                        printPath(parent, i, src, dst, pair_path, V);
                }
        }
}

// Funtion that implements Dijkstra's single source shortest path algorithm for a graph represented using adjacency matrix representation
void dijkstra(int V, vector<int> graph, int src, vector<vector<int>> &pair_path)
{

        // The output array. dist[i] will hold the shortest distance from src to i
        int dist[V];

        // sptSet[i] will true if vertex i is included / in shortest path tree or shortest distance from src to i is finalized
        bool sptSet[V];

        // Parent array to store shortest path tree
        int parent[V];

        // Initialize all distances as INFINITE and stpSet[] as false
        for (int i = 0; i < V; i++)
        {
                parent[i] = -1;
                dist[i] = 10000;
                sptSet[i] = false;
        }

        // Distance of source vertex from itself is always 0
        dist[src] = 0;

        // Find shortest path for all vertices
        for (int count = 0; count < V - 1; count++)
        {
                // Pick the minimum distance vertex from the set of vertices not yet processed. u is always equal to src in first iteration.
                int u = minDistance(dist, sptSet, V);

                // Mark the picked vertex as processed
                sptSet[u] = true;

                // Update dist value of the adjacent vertices of the picked vertex.
                for (int v = 0; v < V; v++)

                        // Update dist[v] only if is not in sptSet, there is an edge from u to v, and total weight of path from src to v through u is smaller than current value of dist[v]
                        if (!sptSet[v] && graph[u * V + v] && dist[u] + graph[u * V + v] < dist[v])
                        {
                                parent[v] = u;
                                dist[v] = dist[u] + graph[u * V + v];
                        }
        }

        // print the constructed distance array
        printSolution(dist, V, parent, src, pair_path);
}

//
// Detect crossing_paths of ecube routing on fat-tree.
//
int main(int argc, char *argv[])
{
        // side length of mesh/torus
        static int array_size = 4;
        // number of hosts for each router
        static int Host_Num = 1;
        // Mesh:0, Torus:1, Fat-tree:2, fully-connected:3, full-mesh-connected-circles(fcc):4, topology-file:5
        static int Topology = 0;
        int c;
        //false:destination_based (slot # not updated), true:path_based (slot # updated)
        static bool path_based = true;
        // (mesh or torus) degree = 2 * dimension, (fully-connected) degree = switch_num -1, (fcc) degree = inter-group + intra-group (2 for ring)
        static int degree = 4;
        //mesh or torus
        static int dimension = 2;

        //fcc
        static int group_switch_num = 4;
        static int inter_group = 6;
        static int intra_group = 2;

        // topology file
        static char *topology_file;

        // default number of slots in one switch
        static int default_slot = 8;

        while ((c = getopt(argc, argv, "a:A:n:T:uD:d:m:t:s:h")) != -1)
        {
                switch (c)
                {
                case 'a':
                        array_size = atoi(optarg);
                        break;
                case 'T':
                        Topology = atoi(optarg);
                        break;
                case 'n':
                        Host_Num = atoi(optarg);
                        break;
                case 'u':
                        path_based = false;
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
                case 't': //topology file (Topology = 5)
                        topology_file = optarg;
                        break;
                case 's': //number of slots in one switch
                        default_slot = atoi(optarg);
                        break;                        
                case 'h':
                case '?':
                default:
                        usage(argv[0]);
                        return EXIT_FAILURE;
                }
        }

        if (dimension > 4 || dimension < 1)
        {
                cerr << " Please input -D $dimension (1 <= $dimension <= 4)" << endl;
                exit(1);
        }

        if (Topology == 0 || Topology == 1)
        { // mesh or torus
                degree = 2 * dimension;
        }

        cout << " ### start ###" << endl;
        // cout << " === Update of slot number ===" << endl
        //      << " 0 (no) / 1 (yes): " << path_based << " (Use -u to activate the update) " << endl;

        // source and destination
        int src = -1, dst = -1, h_src = -1, h_dst = -1;

        // flow id
        int flowid = -1;

        // number of nodes
        static int switch_num = pow(array_size, dimension); //mesh or torus or fully-connected or fcc
        static int node_num = pow(array_size, dimension);   //fat-tree or fcc
        static int PORT = Host_Num + 1;                     //fat-tree

        static int groups = inter_group * group_switch_num + 1; //fcc 25

        // Crossing Paths
        // including Host <-> Switch

        if (Topology == 3)
        { // fully-connected
                degree = switch_num - 1;
        }

        if (Topology == 4)
        {                                               // fcc
                degree = inter_group + intra_group;     //6 inter-group, 2 intra-group
                switch_num = groups * group_switch_num; //100
                node_num = switch_num * Host_Num;       //100
        }

        //topology loaded and analyzed when Topology == 5 (topology file)
        vector<int> topo_file;
        vector<int> topo_sws_dup; // duplicate
        vector<int> topo_sws_uni; // unique
        if (Topology == 5)
        { // topology file
                ifstream infile_feat(topology_file);
                if (!topology_file)
                {
                        cerr << " Please input -t [topo-file-name] to specify the topology file name." << endl;
                        exit(1);
                }
                string line_data;
                int data;
                while (!infile_feat.eof())
                {
                        getline(infile_feat, line_data);
                        stringstream stringin(line_data);
                        int column = 0;
                        while (stringin >> data)
                        { // src_sw, src_port, dst_sw, dst_port
                                if (column == 0)
                                { // src_sw
                                        topo_file.push_back(data);
                                        topo_sws_dup.push_back(data);
                                        topo_sws_uni.push_back(data);
                                        column++;
                                }
                                else if (column == 1)
                                { // src_port
                                        topo_file.push_back(data);
                                        column++;
                                }
                                else if (column == 2)
                                { // dst_sw
                                        topo_file.push_back(data);
                                        topo_sws_dup.push_back(data);
                                        topo_sws_uni.push_back(data);
                                        column++;
                                }
                                else if (column == 3)
                                { // dst_port
                                        topo_file.push_back(data);
                                        column++;
                                }
                        }
                }
                infile_feat.close();

                sort(topo_sws_uni.begin(), topo_sws_uni.end());
                topo_sws_uni.erase(unique(topo_sws_uni.begin(), topo_sws_uni.end()), topo_sws_uni.end());
                switch_num = topo_sws_uni.size();
                degree = 0;
                int cnt;
                for (int i = 0; i < switch_num; i++)
                {
                        cnt = count(topo_sws_dup.begin(), topo_sws_dup.end(), topo_sws_uni[i]);
                        if (cnt > degree)
                                degree = cnt;
                }
        }

        static int ports;
        if (Topology == 0 || Topology == 1)
        { //mesh or torus
                ports = (degree + 1 + 2 * Host_Num) * switch_num;
        }
        if (Topology == 2)
        { //fat-tree
                ports = PORT * (node_num + node_num / Host_Num + node_num / (int)pow(Host_Num, 2) + 1);
        }
        if (Topology == 3)
        {                                                         //fully-connected
                ports = (degree + 1 + 2 * Host_Num) * switch_num; // localhost port = switch ID
        }
        if (Topology == 4)
        {                                                         //fcc
                ports = (degree + 1 + 2 * Host_Num) * switch_num; // 11*100=1100 localhost port = 0
        }
        if (Topology == 5)
        { //topology file
                ports = ((switch_num - 1) + 1 + 2 * Host_Num) * switch_num;
        }
        vector<Cross_Paths> Crossing_Paths(ports);

        // switch connection initiation (fcc) 0:not used, 1:left, 2:right, 3-8:inter-group
        // sw-port (topology file), store ports described in topology files
        vector<int> Switch_Topo(ports);

        // total number of node pairs
        int ct = 0;
        // total number of hops
        int hops = 0;
        //fat-tree or fcc
        int before_hops = 0;

        // node pairs
        vector<Pair> pairs;

        // flows
        vector<Flow> flows; // flowid --> flows[flowid]
        int pre_flow_id = -1;

        cout << " === topology ===" << endl;
        if (Topology == 0)
                cout << dimension << "-D mesh (" << switch_num << " switches/nodes)" << endl;
        else if (Topology == 1)
                cout << dimension << "-D torus (" << switch_num << " switches/nodes)" << endl;
        else if (Topology == 2)
                if (node_num / (int)pow(Host_Num, 2) == 1)
                        cout << "fat tree (" << node_num << " nodes + " << node_num / Host_Num + node_num / (int)pow(Host_Num, 2) << " switches)" << endl;
                else
                        cout << "fat tree (" << node_num << " nodes + " << node_num / Host_Num + node_num / (int)pow(Host_Num, 2) + 1 << " switches)" << endl;
        else if (Topology == 3)
                cout << "fully connected (" << switch_num << " switches/nodes)" << endl;
        else if (Topology == 4)
                cout << "full mesh connected circles (" << switch_num << " switches / " << node_num << " nodes, " << groups << " groups, " << group_switch_num << " switches/group, degree = " << degree << ")" << endl;
        else if (Topology == 5)
                cout << "topology file (" << switch_num << " switches)" << endl;
        else
                cout << "Error: please specify -T [0-5] (0 mesh, 1 torus, 2 fat tree, 3 fully connected, 4 full mesh connected circles, 5 topology file)" << endl;

        // ########################################## //
        // ##############   PHASE 1   ############### //
        // ##        routing       ## //
        // ########################################## //

        if (Topology == 0 || Topology == 1) //mesh or torus
        {
                while (cin >> h_src)
                {
                        cin >> h_dst;
                        src = h_src / Host_Num;
                        dst = h_dst / Host_Num;

                        //read flow id
                        char array[20];
                        cin.getline(array, 20);
                        stringstream stringin(array);
                        while (stringin >> flowid)
                        {
                        }

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
                        Pair tmp_pair(src, dst, h_src, h_dst);
                        pairs.push_back(tmp_pair);
                        int t = src * (degree + 1 + 2 * Host_Num) + degree + 1 + h_src % Host_Num;
                        // int t = (dst%2==1 && Topology==1) ?
                        //         Vch*src*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+h_src%Host_Num
                        //         : Vch*src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);            // node pair <-- channel ID

                        pairs[ct].pair_id = ct;
                        pairs[ct].flow_id = flowid;
                        if (flowid != pre_flow_id)
                        {
                                Flow f;
                                f.id = flowid;
                                f.ID = -1;
                                f.pairs_id.push_back(ct);
                                flows.push_back(f);
                                pre_flow_id = flowid;
                        }
                        else if (flowid != -1)
                        {
                                flows[flows.size() - 1].pairs_id.push_back(ct);
                        }
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }

                        int delta_x, delta_y, delta_z, delta_a, current, src_xy, dst_xy, src_xyz, dst_xyz; // 2D, 3D, 4D
                        if (dimension == 2 || dimension == 1)
                        { //2D, 1D
                                src_xy = src;
                                dst_xy = dst;
                        }
                        if (dimension == 3)
                        { //3D
                                src_xy = src % int(pow(array_size, 2));
                                dst_xy = dst % int(pow(array_size, 2));
                        }
                        if (dimension == 4)
                        { //4D
                                src_xyz = src % int(pow(array_size, 3));
                                dst_xyz = dst % int(pow(array_size, 3));
                                src_xy = src_xyz % int(pow(array_size, 2));
                                dst_xy = dst_xyz % int(pow(array_size, 2));
                        }
                        switch (Topology)
                        {
                        case 0: //mesh
                                //  if (dimension == 2){ //2D
                                //         delta_x = dst%array_size - src%array_size;
                                //         delta_y = dst/array_size - src/array_size;
                                //  }
                                if (dimension == 3)
                                { //3D
                                        delta_z = dst / int(pow(array_size, 2)) - src / int(pow(array_size, 2));
                                        // delta_x = dst_xy%array_size - src_xy%array_size;
                                        // delta_y = dst_xy/array_size - src_xy/array_size;
                                }
                                if (dimension == 4)
                                { //4D
                                        delta_a = dst / int(pow(array_size, 3)) - src / int(pow(array_size, 3));
                                        delta_z = dst_xyz / int(pow(array_size, 2)) - src_xyz / int(pow(array_size, 2));
                                        //        delta_x = dst_xy%array_size - src_xy%array_size;
                                        //        delta_y = dst_xy/array_size - src_xy/array_size;
                                }
                                //4D, 3D, 2D, 1D(delta_y=0)
                                delta_x = dst_xy % array_size - src_xy % array_size;
                                delta_y = dst_xy / array_size - src_xy / array_size;
                                current = src;
                                break;

                        case 1: // torus
                                if (dimension == 4)
                                { //4D
                                        delta_a = dst / int(pow(array_size, 3)) - src / int(pow(array_size, 3));
                                        if (delta_a < 0 && abs(delta_a) > array_size / 2)
                                        {
                                                //delta_a = -( delta_a + array_size/2);
                                                delta_a = delta_a + array_size;
                                                wrap_around_a = true;
                                        }
                                        else if (delta_a > 0 && abs(delta_a) > array_size / 2)
                                        {
                                                //delta_a = -( delta_a - array_size/2);
                                                delta_a = delta_a - array_size;
                                                wrap_around_a = true;
                                        }
                                        delta_z = dst_xyz / int(pow(array_size, 2)) - src_xyz / int(pow(array_size, 2));
                                        if (delta_z < 0 && abs(delta_z) > array_size / 2)
                                        {
                                                //delta_z = -( delta_z + array_size/2);
                                                delta_z = delta_z + array_size;
                                                wrap_around_z = true;
                                        }
                                        else if (delta_z > 0 && abs(delta_z) > array_size / 2)
                                        {
                                                //delta_z = -( delta_z - array_size/2);
                                                delta_z = delta_z - array_size;
                                                wrap_around_z = true;
                                        }
                                }
                                if (dimension == 3)
                                { //3D
                                        delta_z = dst / int(pow(array_size, 2)) - src / int(pow(array_size, 2));
                                        if (delta_z < 0 && abs(delta_z) > array_size / 2)
                                        {
                                                //delta_z = -( delta_z + array_size/2);
                                                delta_z = delta_z + array_size;
                                                wrap_around_z = true;
                                        }
                                        else if (delta_z > 0 && abs(delta_z) > array_size / 2)
                                        {
                                                //delta_z = -( delta_z - array_size/2);
                                                delta_z = delta_z - array_size;
                                                wrap_around_z = true;
                                        }
                                }
                                //4D, 3D, 2D, 1D(delta_y=0)
                                delta_x = dst_xy % array_size - src_xy % array_size;
                                if (delta_x < 0 && abs(delta_x) > array_size / 2)
                                {
                                        //delta_x = -( delta_x + array_size/2);
                                        delta_x = delta_x + array_size;
                                        wrap_around_x = true;
                                }
                                else if (delta_x > 0 && abs(delta_x) > array_size / 2)
                                {
                                        //delta_x = -( delta_x - array_size/2);
                                        delta_x = delta_x - array_size;
                                        wrap_around_x = true;
                                }
                                delta_y = dst_xy / array_size - src_xy / array_size;
                                if (delta_y < 0 && abs(delta_y) > array_size / 2)
                                {
                                        //delta_y = -( delta_y + array_size/2);
                                        delta_y = delta_y + array_size;
                                        wrap_around_y = true;
                                }
                                else if (delta_y > 0 && abs(delta_y) > array_size / 2)
                                {
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
                                pairs[ct].hops = abs(delta_x) + abs(delta_y) + abs(delta_z) + abs(delta_a);
                        }
                        if (dimension == 3) //3D
                        {
                                pairs[ct].hops = abs(delta_x) + abs(delta_y) + abs(delta_z);
                        }
                        if (dimension == 2 || dimension == 1) //2D, 1D(delta_y=0)
                        {
                                pairs[ct].hops = abs(delta_x) + abs(delta_y);
                        }

                        if (dimension == 4)
                        { //4D routing
                                if (delta_a > 0)
                                {
                                        while (delta_a != 0)
                                        { //-a
                                                int t = current * (degree + 1 + 2 * Host_Num) + 7;
                                                // int t = (wrap_around_a) ? Vch*current*(degree+1+2*Host_Num)+7+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 7;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                //if ( current % (array_size*array_size) == array_size-1) {
                                                if (current >= array_size * array_size * array_size * (array_size - 1))
                                                {
                                                        wrap_around_a = false;
                                                        current = current - (array_size - 1) * array_size * array_size * array_size;
                                                }
                                                else
                                                        current += array_size * array_size * array_size;
                                                delta_a--;
                                                hops++;
                                        }
                                }
                                else if (delta_a < 0)
                                {
                                        while (delta_a != 0)
                                        { //+a
                                                int t = current * (degree + 1 + 2 * Host_Num) + 8;
                                                // int t = (wrap_around_a) ? Vch*current*(degree+1+2*Host_Num)+8+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 8;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                //if ( current % (array_size*array_size) == 0 ) {
                                                if (current < array_size * array_size * array_size)
                                                {
                                                        wrap_around_a = false;
                                                        current = current + (array_size - 1) * array_size * array_size * array_size;
                                                }
                                                else
                                                        current -= array_size * array_size * array_size;
                                                hops++;
                                                delta_a++;
                                        }
                                }

                                if (delta_a != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }

                                if (delta_z > 0)
                                {
                                        while (delta_z != 0)
                                        { // -z
                                                int t = current * (degree + 1 + 2 * Host_Num) + 5;
                                                // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 5;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                //if ( current % (array_size*array_size) == array_size-1) {
                                                if ((current % (array_size * array_size * array_size)) >= array_size * array_size * (array_size - 1))
                                                {
                                                        wrap_around_z = false;
                                                        current = current - (array_size - 1) * array_size * array_size;
                                                }
                                                else
                                                        current += array_size * array_size;
                                                delta_z--;
                                                hops++;
                                        }
                                }
                                else if (delta_z < 0)
                                {
                                        while (delta_z != 0)
                                        { // +z
                                                int t = current * (degree + 1 + 2 * Host_Num) + 6;
                                                // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 6;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                //if ( current % (array_size*array_size) == 0 ) {
                                                if ((current % (array_size * array_size * array_size)) < array_size * array_size)
                                                {
                                                        wrap_around_z = false;
                                                        current = current + (array_size - 1) * array_size * array_size;
                                                }
                                                else
                                                        current -= array_size * array_size;
                                                hops++;
                                                delta_z++;
                                        }
                                }

                                if (delta_z != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }

                                // X
                                if (delta_x > 0)
                                {
                                        while (delta_x != 0)
                                        { // +x
                                                int t = current * (degree + 1 + 2 * Host_Num) + 1;
                                                // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 1;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (((current % (array_size * array_size * array_size)) % (array_size * array_size)) % array_size == array_size - 1)
                                                {
                                                        wrap_around_x = false;
                                                        current = current - (array_size - 1);
                                                }
                                                else
                                                        current++;
                                                delta_x--;
                                                hops++;
                                        }
                                }
                                else if (delta_x < 0)
                                {
                                        while (delta_x != 0)
                                        { // -x
                                                int t = current * (degree + 1 + 2 * Host_Num) + 2;
                                                // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 2;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (((current % (array_size * array_size * array_size)) % (array_size * array_size)) % array_size == 0)
                                                {
                                                        wrap_around_x = false;
                                                        current = current + (array_size - 1);
                                                }
                                                else
                                                        current--;
                                                hops++;
                                                delta_x++;
                                        }
                                }

                                if (delta_x != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }

                                // Y 
                                if (delta_y > 0)
                                {
                                        while (delta_y != 0)
                                        { // -y
                                                int t = current * (degree + 1 + 2 * Host_Num) + 3;
                                                // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 3;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (((current % (array_size * array_size * array_size)) % (array_size * array_size)) >= array_size * (array_size - 1))
                                                {
                                                        wrap_around_y = false;
                                                        current = current - array_size * (array_size - 1);
                                                }
                                                else
                                                        current += array_size;
                                                hops++;
                                                delta_y--;
                                        }
                                }
                                else if (delta_y < 0)
                                {
                                        while (delta_y != 0)
                                        { // +y
                                                int t = current * (degree + 1 + 2 * Host_Num) + 4;
                                                // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 4;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (((current % (array_size * array_size * array_size)) % (array_size * array_size)) < array_size)
                                                {
                                                        wrap_around_y = false;
                                                        current = current + array_size * (array_size - 1);
                                                }
                                                else
                                                        current -= array_size;
                                                hops++;
                                                delta_y++;
                                        }
                                }

                                if (delta_x != 0 || delta_y != 0 || delta_z != 0 || delta_a != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }
                        }

                        if (dimension == 3)
                        { //3D routing
                                if (delta_z > 0)
                                {
                                        while (delta_z != 0)
                                        { // -z
                                                int t = current * (degree + 1 + 2 * Host_Num) + 5;
                                                // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+5+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 5;
                                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                //if ( current % (array_size*array_size) == array_size-1) {
                                                if (current >= array_size * array_size * (array_size - 1))
                                                {
                                                        wrap_around_z = false;
                                                        current = current - (array_size - 1) * array_size * array_size;
                                                }
                                                else
                                                        current += array_size * array_size;
                                                delta_z--;
                                                hops++;
                                        }
                                }
                                else if (delta_z < 0)
                                {
                                        while (delta_z != 0)
                                        { // +z
                                                int t = current * (degree + 1 + 2 * Host_Num) + 6;
                                                // int t = (wrap_around_z) ? Vch*current*(degree+1+2*Host_Num)+6+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 6;
                                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                //if ( current % (array_size*array_size) == 0 ) {
                                                if (current < array_size * array_size)
                                                {
                                                        wrap_around_z = false;
                                                        current = current + (array_size - 1) * array_size * array_size;
                                                }
                                                else
                                                        current -= array_size * array_size;
                                                hops++;
                                                delta_z++;
                                        }
                                }

                                if (delta_z != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }

                                // X
                                if (delta_x > 0)
                                {
                                        while (delta_x != 0)
                                        { // +x
                                                int t = current * (degree + 1 + 2 * Host_Num) + 1;
                                                // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 1;
                                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if ((current % (array_size * array_size)) % array_size == array_size - 1)
                                                {
                                                        wrap_around_x = false;
                                                        current = current - (array_size - 1);
                                                }
                                                else
                                                        current++;
                                                delta_x--;
                                                hops++;
                                        }
                                }
                                else if (delta_x < 0)
                                {
                                        while (delta_x != 0)
                                        { // -x
                                                int t = current * (degree + 1 + 2 * Host_Num) + 2;
                                                // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 2;
                                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if ((current % (array_size * array_size)) % array_size == 0)
                                                {
                                                        wrap_around_x = false;
                                                        current = current + (array_size - 1);
                                                }
                                                else
                                                        current--;
                                                hops++;
                                                delta_x++;
                                        }
                                }

                                // check X routing is finished
                                if (delta_x != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }

                                // Y
                                if (delta_y > 0)
                                {
                                        while (delta_y != 0)
                                        { // -y
                                                int t = current * (degree + 1 + 2 * Host_Num) + 3;
                                                // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 3;
                                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if ((current % (array_size * array_size)) >= array_size * (array_size - 1))
                                                {
                                                        wrap_around_y = false;
                                                        current = current - array_size * (array_size - 1);
                                                }
                                                else
                                                        current += array_size;
                                                hops++;
                                                delta_y--;
                                        }
                                }
                                else if (delta_y < 0)
                                {
                                        while (delta_y != 0)
                                        { // +y
                                                int t = current * (degree + 1 + 2 * Host_Num) + 4;
                                                // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 4;
                                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if ((current % (array_size * array_size)) < array_size)
                                                {
                                                        wrap_around_y = false;
                                                        current = current + array_size * (array_size - 1);
                                                }
                                                else
                                                        current -= array_size;
                                                hops++;
                                                delta_y++;
                                        }
                                }

                                // check if X,Y,Z routing are finished
                                if (delta_x != 0 || delta_y != 0 || delta_z != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }
                        }

                        if (dimension == 2 || dimension == 1)
                        { //2D routing, 1D routing (delta_y=0)
                                // X 
                                if (delta_x > 0)
                                {
                                        while (delta_x != 0)
                                        { // +x
                                                int t = current * (degree + 1 + 2 * Host_Num) + 1;
                                                // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+1+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 1;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (current % array_size == array_size - 1)
                                                {
                                                        wrap_around_x = false;
                                                        current = current - (array_size - 1);
                                                }
                                                else
                                                        current++;
                                                delta_x--;
                                                hops++;
                                        }
                                }
                                else if (delta_x < 0)
                                {
                                        while (delta_x != 0)
                                        { // -x
                                                int t = current * (degree + 1 + 2 * Host_Num) + 2;
                                                // int t = (wrap_around_x) ? Vch*current*(degree+1+2*Host_Num)+2+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 2;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (current % array_size == 0)
                                                {
                                                        wrap_around_x = false;
                                                        current = current + (array_size - 1);
                                                }
                                                else
                                                        current--;
                                                hops++;
                                                delta_x++;
                                        }
                                }

                                if (delta_x != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }

                                // Y
                                if (delta_y > 0)
                                {
                                        while (delta_y != 0)
                                        { // -y
                                                int t = current * (degree + 1 + 2 * Host_Num) + 3;
                                                // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+3+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 3;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (current >= array_size * (array_size - 1))
                                                {
                                                        wrap_around_y = false;
                                                        current = current - array_size * (array_size - 1);
                                                }
                                                else
                                                        current += array_size;
                                                hops++;
                                                delta_y--;
                                        }
                                }
                                else if (delta_y < 0)
                                {
                                        while (delta_y != 0)
                                        { // +y
                                                int t = current * (degree + 1 + 2 * Host_Num) + 4;
                                                // int t = (wrap_around_y) ? Vch*current*(degree+1+2*Host_Num)+4+(degree+1+2*Host_Num) :
                                                // Vch * current * (degree+1+2*Host_Num) + 4;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                if (current < array_size)
                                                {
                                                        wrap_around_y = false;
                                                        current = current + array_size * (array_size - 1);
                                                }
                                                else
                                                        current -= array_size;
                                                hops++;
                                                delta_y++;
                                        }
                                }

                                if (delta_x != 0 || delta_y != 0)
                                {
                                        cerr << "Routing Error " << endl;
                                        exit(1);
                                }
                        }

                        // switch->host
                        t = dst * (degree + 1 + 2 * Host_Num) + degree + 1 + Host_Num + h_dst % Host_Num;
                        // t = (src%2==1 && Topology==1) ?
                        //                 Vch*dst*(degree+1+2*Host_Num)+(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num
                        //                 : Vch*dst*(degree+1+2*Host_Num)+degree+1+Host_Num+h_dst%Host_Num;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }
                        ct++;
                }
        }

        if (Topology == 2)
        { // fat-tree

                while (cin >> h_src)
                {
                        cin >> h_dst;
                        int current = h_src;

                        src = h_src;
                        dst = h_dst;

                        //read flow id
                        char array[20];
                        cin.getline(array, 20);
                        stringstream stringin(array);
                        while (stringin >> flowid)
                        {
                        }

                        //#######################//
                        // switch port:0 UP, 1 DOWN1(or localhost), 2 DOWN2, 3 DOWN3, 4 DOWN4 //
                        //#######################//

                        // channel --> switch ID + output port
                        Pair tmp_pair(src, dst, h_src, h_dst);
                        pairs.push_back(tmp_pair);
                        Crossing_Paths[current * PORT].pair_index.push_back(ct);
                        pairs[ct].channels.push_back(current * PORT);
                        //hops++;

                        pairs[ct].pair_id = ct;
                        pairs[ct].flow_id = flowid;
                        if (flowid != pre_flow_id)
                        {
                                Flow f;
                                f.id = flowid;
                                f.ID = -1;
                                f.pairs_id.push_back(ct);
                                flows.push_back(f);
                                pre_flow_id = flowid;
                        }
                        else if (flowid != -1)
                        {
                                flows[flows.size() - 1].pairs_id.push_back(ct);
                        }
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(current * PORT);
                                Crossing_Paths[current * PORT].flow_index.push_back(flowid);
                        }

                        current = node_num + current / Host_Num;

                        while (current != dst)
                        {
                                int t;
                                // root switch
                                if (current == node_num + node_num / Host_Num + node_num / pow(Host_Num, 2))
                                {
                                        t = current * PORT + dst / (int)pow(Host_Num, 2) + 1;
                                        current = current - Host_Num + dst / (int)pow(Host_Num, 2);
                                        // middle layer switch
                                }
                                else if (current >= node_num + node_num / Host_Num)
                                {
                                        if (current - node_num - node_num / Host_Num != dst / (int)pow(Host_Num, 2))
                                        {
                                                t = current * PORT + 0;
                                                current = node_num + node_num / Host_Num + node_num / (int)pow(Host_Num, 2);
                                        }
                                        else
                                        {
                                                t = current * PORT + (dst / Host_Num) % Host_Num + 1;
                                                current = node_num + dst / Host_Num;
                                        }
                                        // low layer switch
                                }
                                else if (current >= node_num)
                                {
                                        if (current - node_num != dst / Host_Num)
                                        {
                                                t = current * PORT + 0;
                                                current = node_num + current / Host_Num;
                                        }
                                        else
                                        {
                                                t = current * PORT + dst % Host_Num + 1;
                                                current = dst;
                                        }
                                }
                                else
                                { //host->switch
                                }
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                if (flows.size() > 0)
                                {
                                        flows[flows.size() - 1].channels.push_back(t);
                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                }
                                hops++;
                        }

                        pairs[ct].hops = hops - before_hops;
                        before_hops = hops;

                        if (current != dst)
                        {
                                cerr << "Routing Error " << endl;
                                exit(1);
                        }
                        ct++;
                }
        }

        if (Topology == 3) //fully-connected
        {
                while (cin >> h_src)
                {
                        cin >> h_dst;
                        src = h_src / Host_Num;
                        dst = h_dst / Host_Num;

                        //read flow id
                        char array[20];
                        cin.getline(array, 20);
                        stringstream stringin(array);
                        while (stringin >> flowid)
                        {
                        }

                        //#######################//
                        // switch port <-- destination switch ID
                        // localhost port = switch ID
                        //#######################//

                        // channel <-- node pair ID, node pair <-- channel ID
                        Pair tmp_pair(src, dst, h_src, h_dst);
                        pairs.push_back(tmp_pair);

                        // localhost(h_src) --> src
                        //int t = src*(degree+1+2*Host_Num)+degree+1+h_src%Host_Num;
                        //Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        //pairs[ct].channels.push_back(t);  // node pair <-- channel ID
                        pairs[ct].pair_id = ct;
                        pairs[ct].hops = 1;
                        pairs[ct].flow_id = flowid;
                        if (flowid != pre_flow_id)
                        {
                                Flow f;
                                f.id = flowid;
                                f.ID = -1;
                                f.pairs_id.push_back(ct);
                                flows.push_back(f);
                                pre_flow_id = flowid;
                        }
                        else if (flowid != -1)
                        {
                                flows[flows.size() - 1].pairs_id.push_back(ct);
                        }

                        // src --> dst
                        int t = src * (degree + 1 + 2 * Host_Num) + dst; // output port = destination switch ID
                        Crossing_Paths[t].pair_index.push_back(ct);      // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);                 // node pair <-- channel ID
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }
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

        if (Topology == 4)
        { // full mesh connected circles (FCC)

                // switch topo initiation
                // switch n <--> switch 3-n, port p <--> port 11-p
                // 0:not used, 1:left, 2:right, 3-8:inter-group
                for (int g = 0; g < groups; g++)
                {
                        for (int s = 0; s < group_switch_num; s++)
                        {
                                int sw = g * group_switch_num + s;
                                if (s == 0)
                                        Switch_Topo[sw * (degree + 1 + 2 * Host_Num) + 1] = sw + (group_switch_num - 1);
                                else
                                        Switch_Topo[sw * (degree + 1 + 2 * Host_Num) + 1] = sw - 1;
                                if (s == group_switch_num - 1)
                                        Switch_Topo[sw * (degree + 1 + 2 * Host_Num) + 2] = sw - (group_switch_num - 1);
                                else
                                        Switch_Topo[sw * (degree + 1 + 2 * Host_Num) + 2] = sw + 1;
                                for (int j = 3; j <= degree; j++)
                                {
                                        Switch_Topo[sw * (degree + 1 + 2 * Host_Num) + j] = ((g + s * inter_group + j - 3 + 1) % groups) * group_switch_num + (group_switch_num - 1 - s);
                                }
                        }
                }

                while (cin >> h_src)
                {
                        cin >> h_dst;
                        src = h_src / Host_Num;
                        dst = h_dst / Host_Num;

                        int current = src;

                        //read flow id
                        char array[20];
                        cin.getline(array, 20);
                        stringstream stringin(array);
                        while (stringin >> flowid)
                        {
                        }

                        //#######################//
                        // switch port: 0 localhost, 1 left, 2 right, 3-8 inter-group
                        // localhost port = 0
                        //#######################//

                        // channel <-- node pair ID, node pair <-- channel ID
                        Pair tmp_pair(src, dst, h_src, h_dst);
                        pairs.push_back(tmp_pair);

                        // localhost(h_src) --> src
                        int t = src * (degree + 1 + 2 * Host_Num) + degree + 1 + h_src % Host_Num;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                        pairs[ct].pair_id = ct;
                        pairs[ct].flow_id = flowid;
                        if (flowid != pre_flow_id)
                        {
                                Flow f;
                                f.id = flowid;
                                f.ID = -1;
                                f.pairs_id.push_back(ct);
                                flows.push_back(f);
                                pre_flow_id = flowid;
                        }
                        else if (flowid != -1)
                        {
                                flows[flows.size() - 1].pairs_id.push_back(ct);
                        }
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }

                        // src --> dst
                        // group #
                        int src_group = src / group_switch_num;
                        int dst_group = dst / group_switch_num;
                        // switch offset in a group
                        int src_offset = src % group_switch_num;
                        int dst_offset = dst % group_switch_num;

                        int diff_group = dst_group - src_group;
                        // gateway for inter-group
                        int src_gw_offset = -1;
                        int dst_gw_offset = -1;
                        int src_gw = -1;
                        int dst_gw = -1;
                        int src_gw_port = -1;
                        int dst_gw_port = -1;
                        if (diff_group == 0)
                        { //intra-group routing
                                if (dst_offset - src_offset == 0)
                                        continue;
                                else if (dst_offset - src_offset > group_switch_num / 2 || (src_offset - dst_offset > 0 && src_offset - dst_offset < group_switch_num / 2))
                                {
                                        while (current != dst)
                                        {
                                                t = current * (degree + 1 + 2 * Host_Num) + 1; //backward
                                                current = current - 1;
                                                if (current < src_group * group_switch_num)
                                                        current = current + group_switch_num;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                hops++;
                                        }
                                }
                                else
                                {
                                        while (current != dst)
                                        {
                                                t = current * (degree + 1 + 2 * Host_Num) + 2; //forward
                                                current = current + 1;
                                                if (current >= (src_group + 1) * group_switch_num)
                                                        current = current - group_switch_num;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                hops++;
                                        }
                                }
                        }
                        else
                        { //intra-src-group routing + inter-group routing + intra-dst-group routing
                                if (diff_group < 0)
                                        diff_group += groups;
                                src_gw_offset = (diff_group - 1) / inter_group;
                                dst_gw_offset = (group_switch_num - 1) - src_gw_offset;
                                src_gw = src_group * group_switch_num + src_gw_offset;
                                dst_gw = dst_group * group_switch_num + dst_gw_offset;
                                src_gw_port = (diff_group - 1) % inter_group + 3;
                                dst_gw_port = (3 + degree) - src_gw_port;

                                //intra-src-group routing
                                if (src_gw_offset - src_offset == 0)
                                        ;
                                else if (src_gw_offset - src_offset > group_switch_num / 2 || (src_offset - src_gw_offset > 0 && src_offset - src_gw_offset < group_switch_num / 2))
                                {
                                        while (current != src_gw)
                                        {
                                                t = current * (degree + 1 + 2 * Host_Num) + 1; //backward
                                                current = current - 1;
                                                if (current < src_group * group_switch_num)
                                                        current = current + group_switch_num;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                hops++;
                                        }
                                }
                                else
                                {
                                        while (current != src_gw)
                                        {
                                                t = current * (degree + 1 + 2 * Host_Num) + 2; //forward
                                                current = current + 1;
                                                if (current >= (src_group + 1) * group_switch_num)
                                                        current = current - group_switch_num;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                hops++;
                                        }
                                }

                                //inter-src-dst-group routing
                                t = current * (degree + 1 + 2 * Host_Num) + src_gw_port;
                                current = dst_gw;
                                Crossing_Paths[t].pair_index.push_back(ct);
                                pairs[ct].channels.push_back(t);
                                if (flows.size() > 0)
                                {
                                        flows[flows.size() - 1].channels.push_back(t);
                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                }
                                hops++;

                                //intra-dst-group routing
                                if (dst_offset - dst_gw_offset == 0)
                                        ;
                                else if (dst_offset - dst_gw_offset > group_switch_num / 2 || (dst_gw_offset - dst_offset > 0 && dst_gw_offset - dst_offset < group_switch_num / 2))
                                {
                                        while (current != dst)
                                        {
                                                t = current * (degree + 1 + 2 * Host_Num) + 1; //backward
                                                current = current - 1;
                                                if (current < dst_group * group_switch_num)
                                                        current = current + group_switch_num;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                hops++;
                                        }
                                }
                                else
                                {
                                        while (current != dst)
                                        {
                                                t = current * (degree + 1 + 2 * Host_Num) + 2; //forward
                                                current = current + 1;
                                                if (current >= (dst_group + 1) * group_switch_num)
                                                        current = current - group_switch_num;
                                                Crossing_Paths[t].pair_index.push_back(ct);
                                                pairs[ct].channels.push_back(t);
                                                if (flows.size() > 0)
                                                {
                                                        flows[flows.size() - 1].channels.push_back(t);
                                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                                }
                                                hops++;
                                        }
                                }
                        }

                        // dst --> localhost(h_dst)
                        t = dst * (degree + 1 + 2 * Host_Num) + degree + 1 + Host_Num + h_dst % Host_Num;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }

                        pairs[ct].hops = hops - before_hops;
                        before_hops = hops;

                        if (current != dst)
                        {
                                cerr << "Routing Error " << endl;
                                exit(1);
                        }
                        ct++;
                }
        }

        if (Topology == 5)
        { // topology file

                // switch topo initiation (sw-port), store ports described in topology files
                // 0 - (switch_num-1) --> port (except itself); switch_num, (switch_num+1) --> not used
                for (int i = 0; i < Switch_Topo.size(); i++)
                {
                        Switch_Topo[i] = -1;
                }
                for (int i = 0; i < switch_num; i++)
                {
                        for (int j = 0; j < topo_file.size(); j = j + 2)
                        {
                                if (topo_sws_uni[i] == topo_file[j])
                                {
                                        int connect_sw = -1;
                                        int connect_port = -1;
                                        if (j % 4 == 0)
                                        {
                                                connect_sw = topo_file[j + 2];
                                                connect_port = topo_file[j + 1];
                                        }
                                        else if (j % 4 == 2)
                                        {
                                                connect_sw = topo_file[j - 2];
                                                connect_port = topo_file[j + 1];
                                        }
                                        for (int k = 0; k < switch_num; k++)
                                        {
                                                if (topo_sws_uni[k] == connect_sw)
                                                {
                                                        Switch_Topo[i * ((switch_num - 1) + 1 + 2 * Host_Num) + k] = connect_port;
                                                }
                                        }
                                }
                        }
                }

                // Number of vertices in the graph
                int V = switch_num;

                // graph initialization
                vector<int> graph(V * V);
                for (int i = 0; i < graph.size(); i++)
                {
                        if (Switch_Topo[(i / V) * ((switch_num - 1) + 1 + 2 * Host_Num) + i % V] == -1)
                        {
                                graph[i] = 0;
                        }
                        else
                        {
                                graph[i] = 1;
                        }
                }

                // dijkstra for each pair, store intermediate and dst sws (not including src sw)
                vector<vector<int>> pair_path(V * V);
                for (int i = 0; i < V; i++)
                {
                        dijkstra(V, graph, i, pair_path);
                }

                while (cin >> h_src)
                {
                        cin >> h_dst;
                        src = h_src / Host_Num;
                        dst = h_dst / Host_Num;

                        //read flow id
                        char array[20];
                        cin.getline(array, 20);
                        stringstream stringin(array);
                        while (stringin >> flowid)
                        {
                        }

                        // channel <-- node pair ID, node pair <-- channel ID
                        Pair tmp_pair(src, dst, h_src, h_dst);
                        pairs.push_back(tmp_pair);

                        //pair path
                        int src_index = -1;
                        int dst_index = -1;
                        for (int i = 0; i < switch_num; i++)
                        {
                                if (topo_sws_uni[i] == src)
                                {
                                        src_index = i;
                                }
                                if (topo_sws_uni[i] == dst)
                                {
                                        dst_index = i;
                                }
                                if (src_index != -1 && dst_index != -1)
                                        break;
                        }
                        if (src_index == -1 || dst_index == -1)
                                cout << "Error: src/dst number is wrong" << endl;

                        // localhost(h_src) --> src
                        int t = src_index * ((switch_num - 1) + 1 + 2 * Host_Num) + (switch_num - 1) + 1 + h_src % Host_Num;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                        pairs[ct].pair_id = ct;
                        pairs[ct].flow_id = flowid;
                        if (flowid != pre_flow_id)
                        {
                                Flow f;
                                f.id = flowid;
                                f.ID = -1;
                                f.pairs_id.push_back(ct);
                                flows.push_back(f);
                                pre_flow_id = flowid;
                        }
                        else if (flowid != -1)
                        {
                                flows[flows.size() - 1].pairs_id.push_back(ct);
                        }
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }

                        pairs[ct].hops = pair_path[src_index * V + dst_index].size() + 1;
                        hops += pairs[ct].hops;

                        // src --> dst
                        for (int i = 0; i < pair_path[src_index * V + dst_index].size(); i++)
                        {
                                if (i == 0)
                                {
                                        t = src_index * ((switch_num - 1) + 1 + 2 * Host_Num) + pair_path[src_index * V + dst_index][i];
                                }
                                else
                                {
                                        t = pair_path[src_index * V + dst_index][i - 1] * ((switch_num - 1) + 1 + 2 * Host_Num) + pair_path[src_index * V + dst_index][i];
                                }
                                Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                                pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                                if (flows.size() > 0)
                                {
                                        flows[flows.size() - 1].channels.push_back(t);
                                        Crossing_Paths[t].flow_index.push_back(flowid);
                                }
                        }

                        // dst --> localhost(h_dst)
                        t = dst_index * ((switch_num - 1) + 1 + 2 * Host_Num) + (switch_num - 1) + 1 + Host_Num + h_dst % Host_Num;
                        Crossing_Paths[t].pair_index.push_back(ct); // channel <-- node pair ID
                        pairs[ct].channels.push_back(t);            // node pair <-- channel ID
                        if (flows.size() > 0)
                        {
                                flows[flows.size() - 1].channels.push_back(t);
                                Crossing_Paths[t].flow_index.push_back(flowid);
                        }

                        ct++;
                }
        }

        // ########################################## //
        // ##############   PHASE 2   ############### //
        // ##  Slot #       ## //
        // ########################################## //

        if (flows.size() > 0)
        {
                // delete duplicate flow id in channel
                for (int i = 0; i < ports; i++)
                {
                        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin() + i;
                        sort(elem->flow_index.begin(), elem->flow_index.end());
                        elem->flow_index.erase(unique(elem->flow_index.begin(), elem->flow_index.end()), elem->flow_index.end());
                }

                // delete duplicate channel id in flow
                for (int i = 0; i < flows.size(); i++)
                {
                        vector<Flow>::iterator elem = flows.begin() + i;
                        sort(elem->channels.begin(), elem->channels.end());
                        elem->channels.erase(unique(elem->channels.begin(), elem->channels.end()), elem->channels.end());
                }
        }

        int max_cp = max_element(Crossing_Paths.begin(), Crossing_Paths.end())->pair_index.size();

        int max_id = 0;

        int max_cp_dst = 0; // number of dst-based renewable labels

        // calculate number of dst-based renewable label
        int max_cp_dst_t = 0;

        vector<int>::iterator find_ptr;

        int ID_max = 0;

        if (flows.size() > 0)
        {
                max_cp = 0;
                for (int j = 0; j < ports; j++)
                {
                        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin() + j;
                        if (elem->flow_index.size() > max_cp)
                                max_cp = elem->flow_index.size();
                        unsigned int p_ct = 0;
                        while (p_ct < elem->flow_index.size())
                        {
                                int u = elem->flow_index[p_ct];
                                bool is_duplicate = false; //check if there is a same destination
                                unsigned int p_ct_t = 0;
                                while (p_ct_t < p_ct)
                                {
                                        int v = elem->flow_index[p_ct_t];
                                        for (int m = 0; m < flows[u].pairs_id.size(); m++)
                                        {
                                                for (int n = 0; n < flows[v].pairs_id.size(); n++)
                                                {
                                                        if (pairs[flows[u].pairs_id[m]].h_dst == pairs[flows[v].pairs_id[n]].h_dst)
                                                        {
                                                                is_duplicate = true;
                                                                break;
                                                        }
                                                }
                                                if (is_duplicate == true)
                                                        break;
                                        }
                                        if (is_duplicate == true)
                                                break;
                                        p_ct_t++;
                                }
                                if (!is_duplicate)
                                        max_cp_dst_t++;
                                p_ct++;
                        }
                        if (max_cp_dst_t > max_cp_dst)
                                max_cp_dst = max_cp_dst_t;
                        max_cp_dst_t = 0;
                }
                // cout << " === Max. number of slots (w/o update) [FLOW] ===" << endl
                //      << max_cp_dst << endl;
                // cout << " === Max. number of slots (w/ update) [FLOW] ===" << endl
                //      << max_cp << endl;

                max_cp = 0;
                for (int j = 0; j < ports; j++)
                {
                        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin() + j;
                        for (int i = 0; i < ports; i++)
                        {
                                if (Crossing_Paths[i].flow_index.size() > max_cp)
                                {
                                        max_cp = Crossing_Paths[i].flow_index.size();
                                        elem = Crossing_Paths.begin() + i;
                                }
                        }

                        // local IDs are assigned
                        unsigned int path_ct = 0;
                        while (path_ct < elem->flow_index.size())
                        {
                                int t = elem->flow_index[path_ct];
                                // check if IDs are assigned
                                bool valid = true;
                                for (int i = 0; i < flows[t].pairs_id.size(); i++)
                                {
                                        if (pairs[flows[t].pairs_id[i]].Valid == false)
                                        {
                                                valid = false;
                                                break;
                                        }
                                }
                                if (valid == true)
                                {
                                        path_ct++;
                                        continue;
                                }
                                // ID is assigned from 0
                                int id_tmp = 0;
                                bool NG_ID = false;

                        NEXT_ID_FLOW:
                                // ID is used or not
                                unsigned int s_ct = 0; // channel
                                while (s_ct < flows[t].channels.size() && !NG_ID)
                                {
                                        int i = flows[t].channels[s_ct];
                                        //vector<int>::iterator find_ptr;
                                        find_ptr = find(Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
                                        if (path_based && find_ptr != Crossing_Paths[i].assigned_list.end())
                                                NG_ID = true;
                                        if (!path_based && find_ptr != Crossing_Paths[i].assigned_list.end())
                                        {
                                                int tmp = 0;
                                                while (*find_ptr != Crossing_Paths[i].assigned_list[tmp])
                                                {
                                                        tmp++;
                                                }
                                                NG_ID = true;
                                                for (int n = 0; n < flows[t].pairs_id.size(); n++)
                                                {
                                                        if (pairs[flows[t].pairs_id[n]].h_dst == Crossing_Paths[i].assigned_dst_list[tmp])
                                                        {
                                                                NG_ID = false;
                                                                break;
                                                        }
                                                }
                                        }
                                        s_ct++;
                                }
                                if (NG_ID)
                                {
                                        id_tmp++;
                                        NG_ID = false;
                                        goto NEXT_ID_FLOW;
                                }
                                flows[t].ID = id_tmp;

                                if (id_tmp > ID_max) ID_max = id_tmp;

                                unsigned int a_ct = 0;
                                while (a_ct < flows[t].channels.size())
                                {
                                        int j = flows[t].channels[a_ct];
                                        Crossing_Paths[j].assigned_list.push_back(id_tmp);
                                        int t = elem->flow_index[path_ct];
                                        for (int n = 0; n < flows[t].pairs_id.size(); n++)
                                        {
                                                Crossing_Paths[j].assigned_dst_list.push_back(pairs[flows[t].pairs_id[n]].h_dst);
                                        }
                                        a_ct++;
                                }

                                for (int n = 0; n < flows[t].pairs_id.size(); n++)
                                {
                                        pairs[flows[t].pairs_id[n]].Valid = true;
                                }
                                if (max_id <= id_tmp)
                                        max_id = id_tmp + 1;

                                path_ct++;
                        }
                        elem->Valid = true;
                }

                cout << " === Total number of occupied slots ===" << endl
                     << ID_max + 1 << endl;

        }
        else
        {
                for (int j = 0; j < ports; j++)
                {
                        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin() + j;
                        unsigned int p_ct = 0;
                        while (p_ct < elem->pair_index.size())
                        {
                                int u = elem->pair_index[p_ct];
                                bool is_duplicate = false; //check if there is a same destination
                                unsigned int p_ct_t = 0;
                                while (p_ct_t < p_ct)
                                {
                                        int v = elem->pair_index[p_ct_t];
                                        if (pairs[u].h_dst == pairs[v].h_dst)
                                        {
                                                is_duplicate = true;
                                        }
                                        p_ct_t++;
                                }
                                if (!is_duplicate)
                                        max_cp_dst_t++;
                                p_ct++;
                        }
                        if (max_cp_dst_t > max_cp_dst)
                                max_cp_dst = max_cp_dst_t;
                        max_cp_dst_t = 0;
                }
                // cout << " === Max. number of slots (w/o update) ===" << endl
                //      << max_cp_dst << endl;
                // cout << " === Max. number of slots (w/ update) ===" << endl
                //      << max_cp << endl;

                /*int slot_max = 0;
        for (int i = 0; i < Crossing_Paths.size(); i++){
                if (i%(degree+1+2*Host_Num) != degree+2*Host_Num && i%(degree+1+2*Host_Num) != degree+2*Host_Num-1)
                if(Crossing_Paths[i].pair_index.size() > slot_max)
                slot_max = Crossing_Paths[i].pair_index.size();
        }
        cout << " slot_max = " << slot_max << endl;	*/

                for (int j = 0; j < ports; j++)
                {
                        vector<Cross_Paths>::iterator elem = Crossing_Paths.begin() + j;
                        elem = max_element(Crossing_Paths.begin(), Crossing_Paths.end());

                        // local IDs are assigned
                        unsigned int path_ct = 0;
                        while (path_ct < elem->pair_index.size())
                        {
                                int t = elem->pair_index[path_ct];
                                // check if IDs are assigned
                                if (pairs[t].Valid == true)
                                {
                                        path_ct++;
                                        continue;
                                }
                                // ID is assigned from 0
                                int id_tmp = 0;
                                bool NG_ID = false;

                        NEXT_ID:
                                // ID is used or not
                                unsigned int s_ct = 0; // channel
                                while (s_ct < pairs[t].channels.size() && !NG_ID)
                                {
                                        int i = pairs[t].channels[s_ct];
                                        //vector<int>::iterator find_ptr;
                                        find_ptr = find(Crossing_Paths[i].assigned_list.begin(), Crossing_Paths[i].assigned_list.end(), id_tmp);
                                        if (path_based && find_ptr != Crossing_Paths[i].assigned_list.end())
                                                NG_ID = true;
                                        if (!path_based && find_ptr != Crossing_Paths[i].assigned_list.end())
                                        {
                                                int tmp = 0;
                                                while (*find_ptr != Crossing_Paths[i].assigned_list[tmp])
                                                {
                                                        tmp++;
                                                }
                                                if (pairs[t].h_dst != Crossing_Paths[i].assigned_dst_list[tmp])
                                                        NG_ID = true;
                                        }
                                        s_ct++;
                                }

                                if (NG_ID)
                                {
                                        id_tmp++;
                                        NG_ID = false;
                                        goto NEXT_ID;
                                }
                                pairs[t].ID = id_tmp;

                                if (id_tmp > ID_max) ID_max = id_tmp;

                                unsigned int a_ct = 0;
                                while (a_ct < pairs[t].channels.size())
                                {
                                        int j = pairs[t].channels[a_ct];
                                        Crossing_Paths[j].assigned_list.push_back(id_tmp);
                                        int t = elem->pair_index[path_ct];
                                        Crossing_Paths[j].assigned_dst_list.push_back(pairs[t].h_dst);
                                        a_ct++;
                                }

                                pairs[t].Valid = true;
                                if (max_id <= id_tmp)
                                        max_id = id_tmp + 1;

                                path_ct++;
                        }
                        elem->Valid = true;
                }

                cout << " === Total number of occupied slots ===" << endl
                     << ID_max + 1 << endl;

        }

        if (Topology == 0 || Topology == 1) // mesh or torus
                show_paths(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree, default_slot, flows);

        if (Topology == 2) // fat-tree
                show_paths_tree(Crossing_Paths, ct, node_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, PORT, default_slot, flows);

        if (Topology == 3) // fully-connected
                show_paths_fullyconnected(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree, default_slot, flows);

        if (Topology == 4) // full mesh connected circles (FCC)
                show_paths_fcc(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree, default_slot, flows);

        if (Topology == 5) // topology file
                show_paths_tf(Crossing_Paths, ct, switch_num, max_id, pairs, hops, Host_Num, max_cp, max_cp_dst, path_based, degree, Switch_Topo, topo_sws_uni, default_slot, flows);

        return 0;
}
