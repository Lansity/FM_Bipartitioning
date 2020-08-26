#include <iostream>
#include <vector>
#include <map>
#include <iterator>
#include <string>
#include <string.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <time.h>

using namespace std;
/*Define function*/

class Cell;
class Net;

void readNet(char net_file_path[100]);
void readCell(char cell_file_path[100]);
void initialPartition();
//void initialGain();
void computeCutsize();
//void changePartition();
void computeGain();
void updateGain();
bool Movecell();
void recordPartition();               //record the best partition of all cells
bool freecell(Cell *base_cell);
void savebestsolution();
void recallbestsolution();
void out_fm(){
    cout << "**********************************" <<endl;
    cout << "***                            ***" <<endl;
    cout << "***            F  M            ***" <<endl;
    cout << "***                            ***" <<endl;
    cout << "**********************************" <<endl;
    cout << "\n" <<endl;
}



multimap <int, Cell*> gain_bucket[2];//produce two buckets to
                                //store the gain of cells
vector <Cell*> cells;           //vector to store Cells' pointers
vector <Net* > nets;            //store Nets' pointers

map <string,Cell*> cells_map;   //map to search cell by cells' name
int partition_area[2]={0,0};             //used to record the area of different partition
int cutsize=0;                  //used to record the current cutsize
int min_cutsize=0;              //used to record the minimum cutsize
int area_sum;                   //sum of cell size
float r=0.01;                   //the rate of unbalance between two partitions
float ubf=0.45;                 //unbalance factor

bool gain_positive;
int base_partition=0;


class Net{
    public:
    char net_name[20];             //name of current cell
    int num_pins=0;                //num of pins of current cell
    bool cutstate;                 //whether the net been cut, if cut, true
    bool critical;
    bool lock_partition[2];         //whether two partition has locked cells
    int num_cell_Partition[2];      //the num of cell in two partition
    vector <Cell*> cell_list;       //cells' list of current net
};
class Cell{
    public:
    char cell_name[20];             //name of cell
    int cell_size;                  // size of cell
    bool locked;                    //whether cell been locked, if locked, true
    int gain;                         //the gain of the cell if moved
    int partition;                    //the partition of the cell belongs to
    int best_Partition;               //the best partition the cell belongs to
    multimap <int,Cell*>::iterator gain_itr;  
                        //a itr for searching cell in gain buckets
    vector <Net*> net_list;           //the net list of current cell
    
    //change the Partition of current cell
    void changePartition()
    {
        //change the attribute of cell
        locked = true;              //after moving, cell is locked
        gain_bucket[partition].erase(gain_itr); //clear the iterator
        cutsize -= gain;            //change the cut size by gain
        
        //change partition for the cell
        partition_area[partition] -= cell_size;
        partition_area[!partition] += cell_size;
        int base_partition = partition; 
        
        if (gain<0)
            gain_positive = false;
        
        //Loop to change the gain of connected cells
        for ( int i=0; i<net_list.size(); i++)
        {
            //if one net have locked cell in both area, the net is dead
            //any move can't change the cutstate of a dead net
            if(net_list[i]->lock_partition[0] && net_list[i]->lock_partition[1])
                continue;
            
            int from = net_list[i]->num_cell_Partition[partition];
            int to = net_list[i]->num_cell_Partition[!partition];

            if(to == 0)
            {
                //if T(n)=0,gain(all cells on this net) ++
                for (int k=0; k<net_list[i]->cell_list.size(); k++)
                {
                    //unlocked cells' gain + 1
                    if((!net_list[i]->cell_list[k]->locked))
                    {
                        net_list[i]->cell_list[k]->updateGain(1);
                    }
                }
            }
            else if (to == 1)
            {
                //if T(n)=1, gain of "to" cell --
                for(int k=0; k<net_list[i]->cell_list.size(); k++)
                {
                    if(!net_list[i]->cell_list[k]->locked && base_partition!=net_list[i]->cell_list[k]->partition)
                    {
                        //not locked and not in the current cell's partition
                        net_list[i]->cell_list[k]->updateGain(-1);
                        break;
                    }
                }
            }
            
            //F(n)-- and T(n)++
            from--;
            net_list[i]->num_cell_Partition[partition]--;
            to++;
            net_list[i]->num_cell_Partition[!partition]++;

            if(from == 0)
            {
                //cells on this net gain--
                for(int k=0; k<net_list[i]->cell_list.size(); k++)
                {
                    if(!net_list[i]->cell_list[k]->locked)
                        net_list[i]->cell_list[k]->updateGain(-1);
                }
            }
            else if (from == 1)
            {
                for(int k=0; k<net_list[i]->cell_list.size(); k++)
                {
                    //gain( F cell on the net) ++
                    if(!net_list[i]->cell_list[k]->locked && base_partition== net_list[i]->cell_list[k]->partition)
                    {
                        net_list[i]->cell_list[k]->updateGain(1);
                        break;
                    }
                }
            }
            //after moving, the net's T partition has locked cell
            net_list[i]->lock_partition[!base_partition] = true;
        }
        partition=!partition;
    }
    void updateGain(int gain_change)
    {
        if (locked) return;
        gain_bucket[partition].erase(gain_itr);
        gain+=gain_change;
        gain_itr=gain_bucket[partition].insert(pair <int,Cell*> (gain,this));
    }

};

//Output partition of all cell
void Partition_output(char filepath[100]){
    ofstream outfile(filepath);
    for(int i=0; i< cells.size(); i++)
    {
        outfile << cells[i]->cell_name <<"\t" << cells[i]->partition<<endl;
    }
    outfile.close();
    return ;
}

//read cell
void readCell(char cell_file_path[100]){
    //read cell file 

	ifstream cell_file(cell_file_path);
    //FILE *cell_file;
    //char file_path[100];
    //strcpy_s(file_path,cell_file_path);
    //cell_file=fopen_s(cell_file,file_path,"r");
    //if(cell_file==NULL) {cout<<"Can't Open!"<<endl;exit(1);}

    cout << "Reading cells...... "<< endl;
    //int i=0;
    while(!cell_file.eof())
    {
        char node_name[100];        //current cell's name
        int node_size;              //current cell's size
        //fscanf_s(cell_file,"%s %d",&node_name,&node_size);
		cell_file >> node_name >> node_size;
        if(cell_file.eof()) break;

        Cell* current_cell;             //used to store current cell
        current_cell = new Cell;  

        vector <Net*> current_net_list; //represent the cell's net_list
        strcpy(current_cell->cell_name,node_name);
        current_cell->cell_size=node_size;
        current_cell->best_Partition = 0;
        current_cell->gain = 0;
        current_cell->partition = 0;
        current_cell->locked = false;
        current_cell->net_list =current_net_list;

        partition_area[0]=partition_area[0]+current_cell->cell_size;

        cells.push_back(current_cell);
        cells_map[current_cell->cell_name] = current_cell;  //put cell in map(name as track)
        //cout << "The "<<i++<<"th cell :  " << current_cell->cell_name <<endl;
    }
	cell_file.close();
	cout << "Read complete!" <<endl;
    return;
}

//read net
void readNet(char net_file_path[100]){

	ifstream net_file(net_file_path);
    int i=1;
    cout << "Reading Net....." <<endl;
    while(!net_file.eof()){
        //cout <<"still reading ########"<<endl;
        Net* current_net;
        current_net = new Net;

        char current_net_name[50],driver_[20]; //used to store name and driver_
        int num_cell_current;             //num of cell in current net
        //cout  << net_file.eof() <<endl;
		net_file >> current_net_name >> driver_ >> num_cell_current;//get net & node
		if(net_file.eof()) break;

        //cout <<"reading reading \t\t reading " <<endl;
        strcpy(current_net->net_name,current_net_name);
        current_net->cutstate=false;
        current_net->critical-false;
        current_net->num_pins=num_cell_current;

        char node_net_name[50];
        strcpy(node_net_name,current_net_name);
        if(cells_map[node_net_name]->net_list.empty()||
        cells_map[node_net_name]->net_list.back()!=current_net)
        {
            cells_map[node_net_name]->net_list.push_back(current_net);
            current_net->cell_list.push_back(cells_map[node_net_name]);
        }

        //cout << "######  The "<<i++<<"th Net :" <<current_net->net_name<<endl;
        //we should store cell at the same time
        for(int k=0;k<num_cell_current;k++){

            char node_name[30];
			net_file >> node_name;
            //printf("node_name  :%s ; " , node_name);
            if(cells_map[node_name]->net_list.empty()||     //if the cell
            cells_map[node_name]->net_list.back()!=current_net){

                cells_map[node_name]->net_list.push_back(current_net);
                current_net->cell_list.push_back(cells_map[node_name]);
            }
        }
        //printf("\n");
        nets.push_back(current_net);
    }
    net_file.close();
    cout << "Read End!"<<endl;
    return;
}

//Initial Partition
void initialPartition(){
    srand((int)time(0));    //random num seed create
    int index;
    //the initial partition should satisfy the unbalance rate
    while((float)partition_area[1] / (float)(partition_area[0]+partition_area[1]) < 0.55 )
       // (float)partition_area[0]/ (float)(partition_area[0]+partition_area[1])<0.55) 
    {
        //move random cell to partition 1
        index= rand() % cells.size();
        if(cells[index]->partition == 0){
            cells[index]->partition = 1;
            cells[index]->best_Partition = 1;
            partition_area[0]-=cells[index]->cell_size;
            partition_area[1]+=cells[index]->cell_size;    
        }
    }

}

//compute the cut size of current partitioning
void computeCutsize()  
{
    cutsize=0;
    
    //loop to compute the cells' area in each net
    for(int i=0; i <nets.size(); i++)
    {
        //initial cell number of every net
        nets[i]->num_cell_Partition[0] = 0;
        nets[i]->num_cell_Partition[1] = 0;
        for(int j = 0; j < nets[i]->cell_list.size(); j++)
        {
            //count the num of cell in two partition and record in num_cell_Partition
            nets[i]->num_cell_Partition[nets[i]->cell_list[j]->partition]++;
        }
        //if the num of cell in each partition > 0, cutstate is TRUE
        if(nets[i]->num_cell_Partition[0] > 0 && nets[i]->num_cell_Partition[1] > 0)
            cutsize ++; 
    }
}

//compute Gain of cells
void computeGain()
{
    gain_bucket[0].clear();
    gain_bucket[1].clear();

    for (int i = 0 ; i < cells.size() ; i++)
    {
        cells[i]->gain = 0;

        for (int j = 0 ; j < cells[i]->net_list.size() ; j++)
        {
            Net* current_net = cells[i]->net_list[j];
            //if F(n)=1, g++
            if(current_net->num_cell_Partition[cells[i]->partition] == 1)
            {
                cells[i]->gain++;
                current_net->critical = true;
            }
            //if T(n)=0, g--
            if(current_net->num_cell_Partition[!cells[i]->partition] == 0)
            {
                cells[i]->gain--;
                current_net->critical = true;
            }
        }
        //put the gain-cell pair into the gain bucket
        cells[i]->gain_itr = gain_bucket[cells[i]->partition].insert( pair <int,Cell*> (cells[i]->gain,cells[i]));
    }
}

//to judge a cell is free to move or not 
bool freecell(Cell* base_cell)
{
    if(base_cell == NULL) { cout << "NULL cell!"<<endl; return false;}
    bool balance;
    float rate_partition;
    //the balance factor should satisfy the constraint                 
    rate_partition= (float)(partition_area[base_cell->partition]-base_cell->cell_size) / (float) (partition_area[0]+partition_area[1]);
    //cout << rate_partition <<endl;
    balance = ( rate_partition >= ubf);
    return balance;
}

//select a free cell from gain bucket and move it to the other partition
bool Movecell()
{
    if(gain_bucket[0].size()==0 && gain_bucket[1].size()==0)
    { 
        //cout << " Bucket empty!" << endl;
        return false;
    }
    Cell* base_cell;
    multimap <int,Cell*>::iterator itr[2];

    if(gain_bucket[0].size() != 0)  //choose the unempty bucket
        itr[0] = --gain_bucket[0].end();    //choose the highest gain
    if(gain_bucket[1].size() != 0)
        itr[1] = --gain_bucket[1].end();
    
    int from_partition;

    if(gain_bucket[0].size()==0)    //if bucket 0 has no cell
        from_partition = 1;         //choose cell from bucket1
    else if(gain_bucket[1].size()==0)
        from_partition = 0;
    else
        from_partition = base_partition;
    
    base_partition = !base_partition;

    base_cell = itr[from_partition]->second;

    if(!freecell(base_cell))
    {
        //cout << "Can't move (imbalance)!"<<endl;
        from_partition = !from_partition;
        if(gain_bucket[from_partition].size()==0)
        {
            cout << "Last cell in bucket will break balance"<<endl;
            return false;
        }
        base_cell = itr[from_partition]->second;

        if(!freecell(base_cell))
        {
            cout << " Both partition can't be moved due to the unbalance factor!" <<endl;
            return false;
        }
    }
    (*base_cell).changePartition();
    
    return true;
}

//save the best solution
void savebestsolution()
{
    min_cutsize = cutsize;
    //cout << "\t\tthe min cut size: "<<min_cutsize <<endl;
    for(int i=0; i<cells.size();i++)
    {
        cells[i]->best_Partition=cells[i]->partition;
    }
}

//recall the best solution
void recallbestsolution()
{
    cutsize=min_cutsize;

    for(int i=0;i<cells.size();i++){
        cells[i]->partition=cells[i]->best_Partition;
        cells[i]->locked=false;
    }

    for(int i=0;i<nets.size();i++)
    {
        nets[i]->num_cell_Partition[0]=0;
        nets[i]->num_cell_Partition[1]=0;

        nets[i]->lock_partition[0]=false;
        nets[i]->lock_partition[0]=false;

        for(int k=0; k<nets[i]->cell_list.size();k++)
        {
            nets[i]->num_cell_Partition[nets[i]->cell_list[k]->partition]++;
        }
    }
    computeGain();
    partition_area[0]=0;
    partition_area[1]=0;
    // recall the best solution's area
    for(int i=0;i<cells.size();i++){
        partition_area[cells[i]->partition]+=cells[i]->cell_size;
    }
}

//do one pass
bool LoopPass(int pass_num)
{

    //int current_cutsize = cutsize;
    int move_time=0;        //the moving times
    savebestsolution();
    while(Movecell())
    {
        move_time++;
        //printf("The %dth pass--- cutsize: %d \n",pass_num,cutsize);
        if( cutsize < min_cutsize)
            savebestsolution();
    }
    recallbestsolution();
    computeCutsize();
    cout << "The "<<pass_num << "th pass cut size :" <<cutsize<<endl;

    return true;
}

//print partition information
void printfile(char printpath[100])
{
    ofstream outfile(printpath);
    outfile << "The netlist has net :" << nets.size() <<endl;
    outfile << "\t\t\t" <<"cell: " <<cells.size() <<endl;
    outfile << "The Final cutsize is : " << cutsize <<endl;
    outfile << "Partition 0 area : " << partition_area[0] <<endl;
    outfile << "Partition 1 area : " << partition_area[1] <<endl;
    outfile.close();
    return ;
}

int main (){
    //if file input controled by cin
    char net_filepath[100];
    char cell_filepath[100];
    float unbalance;
    cout << "Input the netfile path :\t" ;
    cin >> net_filepath;
    cout << "Input the cell file path :\t";
    cin >> cell_filepath;
    cout << "Input the unbalance factor: \t";
    cin >> unbalance;
    ubf=unbalance;
    clock_t stime=clock();
    out_fm();
    readCell(cell_filepath);
    readNet(net_filepath);
    cout <<"\n" ;
    /*
    cout << "******************************" << endl;
    Cell *node;
    node=cells[0];
    cout <<"cell's name:  "<< node->cell_name <<endl;
    cout <<"cell's size:  "<< node->cell_size <<endl;
    cout << node->net_list.empty()<<endl;
    Net *edge;
    edge=node->net_list[0];
    cout << "edge0 name:  "<< edge->net_name <<endl;
    cout << "edge0 pins:  "<< edge->num_pins <<endl;
    Cell *kkk;
    kkk=edge->cell_list[2];
    cout <<"net's cell 3:  " << kkk->cell_name <<endl;
    */
    /*cout <<"Partition 1 : "<< partition_area[0]<<endl;
    cout <<"Partition 2 : "<< partition_area[1]<<endl;*/
    initialPartition();
    /*
    cout <<"\n"<<"After initialization" <<endl;
    cout <<"Partition 1 : "<< partition_area[0]<<endl;
    cout <<"Partition 2 : "<< partition_area[1]<<endl;  
    cout << "\n" ;
    */
    //float rate;
    //area_sum=partition_area[0]+partition_area[1];
    //rate =  (float)partition_area[0] / (float)area_sum;
    //cout <<"Rate of Partition 1: "<< rate <<endl;
    //cout <<"Rate of Partition 2: "<< 1-rate <<endl;

    computeCutsize();
    cout <<"Cut size is : " << cutsize <<endl;
    char file1[100]="./tmp1.info";
    char file2[100]="./tmp2.info";
    char file3[100]="./partiton.slt";
    Partition_output(file1);
    computeGain();

    int pass_num=1;

    while(LoopPass(pass_num) && pass_num<=5)
    {
        pass_num ++;
    }
    /*
    int i=0;
    while(i < cells.size())
    {
        if(cells[i]->gain > 0)
        {
            cells[i]->changePartition();
            break;
        }
        else i++;
    }*/
    Partition_output(file2);
    cout <<"Partition 1 : "<< partition_area[0]<<endl;
    cout <<"Partition 2 : "<< partition_area[1]<<endl; 
    //cout <<"Cut size is : " << cutsize <<endl;
    printfile(file3);
    cout <<"Time used:"<< (clock()-stime)/(double) CLOCKS_PER_SEC << endl;
    /*
    for(int i=0; i< cells.size();i++)
    {
        cout << cells[i]->gain <<endl;
    }*/
    
    return 0;
}

