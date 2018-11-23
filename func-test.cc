#include <unistd.h> // getopt 
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <algorithm>
#include <math.h>

using namespace std;


int main(int argc, char *argv[])
{
        int groups = 25;
        int group_switch_num = 4;
        int degree = 8;
        int Host_Num = 1;
        int inter_group = 6;
        int switch_num = 100;
        vector<int> Switch_Topo((degree+1+2*Host_Num)*switch_num);
        
        // vector<int> Switch_Inter_Group(switch_num); 
        // for(int g=0; g<groups; g++){
        //         int i = 0;
        //         for(int s=0; s<group_switch_num; s++){
        //                 int sw = g*group_switch_num+s;
        //                 if(s==0) Switch_Topo[sw*(degree+1+2*Host_Num)+1] = sw+(group_switch_num-1);
        //                 else Switch_Topo[sw*(degree+1+2*Host_Num)+1] = sw-1;
        //                 if(s==group_switch_num-1) Switch_Topo[sw*(degree+1+2*Host_Num)+2] = sw-(group_switch_num-1);
        //                 else Switch_Topo[sw*(degree+1+2*Host_Num)+2] = sw+1;
        //                 // if(g==i) i++;
        //                 // if(i==groups) i=0;
        //                 for(int j=3; j<=degree; j++){
        //                         if (Switch_Inter_Group[sw]==inter_group) break;
        //                         while(Switch_Inter_Group[i*group_switch_num+s]==inter_group || i==g){
        //                                 i++;
        //                                 if(i==groups) i=0;
        //                         }
        //                         Switch_Topo[sw*(degree+1+2*Host_Num)+j] = i*group_switch_num+s;
        //                         Switch_Topo[(i*group_switch_num+s)*(degree+1+2*Host_Num)+j] = sw;
        //                         Switch_Inter_Group[sw] = Switch_Inter_Group[sw]+1;
        //                         Switch_Inter_Group[i*group_switch_num+s] = Switch_Inter_Group[i*group_switch_num+s]+1;
        //                         i++;
        //                         if(i==groups) i=0;
        //                 }
        //         }
        // }

        // return group switch offset (fcc)
        //
        // int get_group_switch_offset (int groups, int inter_group, int group_src, int group_dst)
        // {
        //         int offset = 0;
        //         int sws = 0;
        //         for(int g=0; g<groups; g++)
        //         {
        //                 if(g==group_src) continue;
        //                 if(g==group_dst) return offset;
        //                 sws++;
        //                 if(sws%inter_group == 0) offset++;
        //         }
        // }

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

        // for (int i=0; i<switch_num; i++)
        // cout << Switch_Inter_Group[i] << " ";
        // cout << endl;

        for (int i=0; i<(degree+1+2*Host_Num)*switch_num; i++)
        {
                if (i%(degree+1+2*Host_Num) == 0) cout << endl;
                cout << Switch_Topo[i] << " ";
        }

        // vector<int> a(2); 
        // a[0] = 2;
        // a[0] = a[0] + 1;
        // cout << a[0] << endl;
        // cout << a[1] << endl;


        int src = 3;
        int dst = 0;
        int current = src;
        int t;

        // src --> dst   
        int src_group = src/group_switch_num;
        int dst_group = dst/group_switch_num;

        int src_offset = src%group_switch_num;
        int dst_offset = dst%group_switch_num;

        int diff_group = dst_group-src_group;
        int src_gw_offset = -1;
        int dst_gw_offset = -1;
        int src_gw = -1;
        int dst_gw = -1;
        int src_gw_port = -1;
        int dst_gw_port = -1;
        cout << current << " --> " << t << " --> " << endl;
        if (diff_group==0){ //intra-group routing
                if(dst_offset-src_offset==0) ;
                else if(dst_offset-src_offset>group_switch_num/2 || (src_offset-dst_offset>0 && src_offset-dst_offset<group_switch_num/2)){
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+1;
                                current = current - 1;
                                if(current<src_group*group_switch_num) current = current+group_switch_num;
                                cout << current << " --> " << t << " --> " << endl;
                        }
                }
                else{
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+2;
                                current = current + 1;
                                if(current>=(src_group+1)*group_switch_num) current = current-group_switch_num;
                                cout << current << " --> " << t << " --> " << endl;
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

                cout << diff_group << endl;
                cout << src_gw_offset << endl;
                cout << dst_gw_offset << endl;
                cout << src_gw << endl;
                cout << dst_gw << endl;
                cout << src_gw_port << endl;
                cout << dst_gw_port << endl;

                //intra-src-group
                if(src_gw_offset-src_offset==0) ;
                else if(src_gw_offset-src_offset>group_switch_num/2 || (src_offset-src_gw_offset>0 && src_offset-src_gw_offset<group_switch_num/2)){
                        while ( current != src_gw ){
                                t = current*(degree+1+2*Host_Num)+1;
                                current = current - 1;
                                if(current<src_group*group_switch_num) current = current+group_switch_num;
                                cout << current << " --> " << t << " --> " << endl;
                        }
                }
                else{
                        while ( current != src_gw ){
                                t = current*(degree+1+2*Host_Num)+2;
                                current = current + 1;
                                if(current>=(src_group+1)*group_switch_num) current = current-group_switch_num;
                                cout << current << " --> " << t << " --> " << endl;
                        }
                }
                //inter-src-dst-group
                t = current*(degree+1+2*Host_Num)+src_gw_port;
                current = dst_gw;
                cout << current << " --> " << t << " --> " << endl;
                //intra-dst-group
                if(dst_offset-dst_gw_offset==0) ;
                else if(dst_offset-dst_gw_offset>group_switch_num/2 || (dst_gw_offset-dst_offset>0 && dst_gw_offset-dst_offset<group_switch_num/2)){
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+1;
                                current = current - 1;
                                if(current<dst_group*group_switch_num) current = current+group_switch_num;
                                cout << current << " --> " << t << " --> " << endl;
                        }
                }
                else{
                        while ( current != dst ){
                                t = current*(degree+1+2*Host_Num)+2;
                                current = current + 1;
                                if(current>=(dst_group+1)*group_switch_num) current = current-group_switch_num;
                                cout << current << " --> " << t << " --> " << endl;
                        }
                }                
        }        

        return 0;
}