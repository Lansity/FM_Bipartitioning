#include <iostream>
#include <fstream>
#include <map>
#include <string.h>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <random>

using namespace std;


class NET;
class CELL;
int num_way=2;
int partition_area[2]={0,0};
vector <CELL*> cells;
vector <NET*> nets;
map <string,CELL*> cells_map;
int cutsize=0;
int min_cutsize=0;
float ubf=0.45;
float best_balance=0.0;
multimap <int,CELL*> gain_bucket[2];
bool gain_positive;

//title print out
void out_fm(){
    cout << "*****************************************" <<endl;
    cout << "***                                   ***" <<endl;
    cout << "***             F    M                ***" <<endl;
    cout << "***                                   ***" <<endl;
    cout << "*****************************************" <<endl;
    cout << "\n" <<endl;
}

void printpartition();

class NET{
    public:
    char net_name[20];             //name of current cell
    int num_pins=0;                //num of pins of current cell
    int weight;
    bool cutstate;                 //whether the net been cut, if cut, true
    bool dead;
    bool critical;
    bool lock_partition[2];         //whether two partition has locked cells
    int num_cell_Partition[2];      //the num of cell in two partition
    vector <CELL*> cell_list;       //cells' list of current net
};

class CELL{
    public:
    char cell_name[20];             //name of cell
    int cell_size;                  // size of cell
    bool locked;                    //whether cell been locked, if locked, true
    bool dead;
    int gain;                         //the gain of the cell if moved
    int partition;                    //the partition of the cell belongs to
    int best_Partition;               //the best partition the cell belongs to
    multimap <int,CELL*>::iterator gain_itr;  
    vector <NET*> net_list;  

    void changepartition(int to_partition)
    {
        locked=true;
        cutsize-=gain;
        gain_bucket[partition].erase(gain_itr);

        partition_area[partition]-=cell_size;
        partition_area[to_partition]+=cell_size;

        if(gain<0) gain_positive=false;

        for(int i=0; i<net_list.size(); i++)
        {
            //if a net has locked cell in both partition, the net is dead
            //if k-way, the "dead" condition is more restrict
            if(net_list[i]->lock_partition[0] && net_list[i]->lock_partition[1])
            {   /*cout <<"\t!!!!!Wraning!!!! \t GG WP!" <<endl;*/ continue;}
            //define the from/to to store the num of cells of both partition
            int from = net_list[i]->num_cell_Partition[partition];
            int to = net_list[i]->num_cell_Partition[to_partition];

            if(to == 0)
            {
                for( int k=0; k< net_list[i]->cell_list.size(); k++)
                {   
                    //update the gain of unlocked cells in this net
                    if(!net_list[i]->cell_list[k]->locked)
                        net_list[i]->cell_list[k]->updateGain(net_list[i]->weight);
                }
            }
            else if(to == 1)
            {
                for( int k=0; k< net_list[i]->cell_list.size(); k++)
                {
                    if(!net_list[i]->cell_list[k]->locked && net_list[i]->cell_list[k]->partition == to_partition)
                    {
                        net_list[i]->cell_list[k]->updateGain(-net_list[i]->weight);
                        break;
                    }
                }
            }

            //F(n)-- and T(n)++
            from--;
            to++;
            net_list[i]->num_cell_Partition[partition]--;
            net_list[i]->num_cell_Partition[to_partition]++;

            if(from == 0)
            {
                for(int k=0; k<net_list[i]->cell_list.size(); k++)
                {
                    if(!net_list[i]->cell_list[k]->locked)
                        net_list[i]->cell_list[k]->updateGain(-net_list[i]->weight);
                }
            }
            else if(from == 1)
            {
                for(int k=0; k<net_list[i]->cell_list.size(); k++)
                {
                    if(!net_list[i]->cell_list[k]->locked && net_list[i]->cell_list[k]->partition==partition)
                    {
                        net_list[i]->cell_list[k]->updateGain(net_list[i]->weight);
                        break;
                    }
                }
            }
            //after moving, the net's To partition is locked
            net_list[i]->lock_partition[to_partition] = true;
        }
        partition = to_partition;
    }

    void updateGain(int gain_value)
    {
        if(locked) return;
        gain_bucket[partition].erase(gain_itr);
        gain+=gain_value;
        //cout << "Update Gain: "<< cell_name <<" gain change: " << gain_value<<endl;
        gain_itr=gain_bucket[partition].insert( pair<int,CELL*> (gain,this) );
    }
};

void readCell(char cell_file_path[100]){

    //read cell file 

	ifstream cell_file(cell_file_path);
    if(!cell_file) {cout << "Can't find cell file!" <<endl; exit(1);}

    cout << "Reading cells...... "<< endl;
    //int i=0;
    while(!cell_file.eof())
    {
        //i++;
        //cout <<"^^^^^^^^^^^  THE NODE :" << i <<endl;
        char node_name[100];        //current cell's name
        int node_size;              //current cell's size
		cell_file >> node_name >> node_size;

        CELL* current_cell;             //used to store current cell
        current_cell = new CELL;  
        //cout << " Node : " << node_name <<endl;
        vector <NET*> current_net_list; //represent the cell's net_list
        strcpy(current_cell->cell_name,node_name);
        current_cell->cell_size=node_size;
        current_cell->best_Partition = 0;
        current_cell->gain = 0;
        current_cell->dead =false;
        current_cell->partition = 0;
        current_cell->locked = false;
        current_cell->net_list =current_net_list;

        partition_area[0]=partition_area[0]+current_cell->cell_size;
        //cout <<"Total area : "<< partition_area[0]<<endl;
        cells.push_back(current_cell);
        cells_map[current_cell->cell_name] = current_cell;  //put cell in map(name as track)
    }
	cell_file.close();
	cout << "Read Cell complete!" <<endl;
    return;
}

void readNet(char net_file_path[100])
{
    ifstream netfile(net_file_path);
    if(!netfile) {cout << "Can't find net file!" <<endl; exit(1);}
    cout <<"Reading Net......" <<endl;
    char cell_name[20];
    char driver_;
    int weight;
    netfile >> cell_name >> driver_;
    //Loop to store all net and node
    while(!netfile.eof())
    {
        netfile >> weight;
        NET* current_net;
        current_net = new NET;
        int pin_num=1;
        //cout << "name: "<<cell_name << "   driver: " << driver_ <<endl;
        //build a net and assign arttribute
        strcpy(current_net->net_name, cell_name);
        current_net->critical = false;
        current_net->cutstate = false;
        current_net->dead = false;
        current_net->weight = weight;
        current_net->lock_partition[0]=false;
        current_net->lock_partition[1]=false;
        //put current driver cell into the cellsmap
        //and put current cell into net's cell_list
        if(cells_map[cell_name]->net_list.empty()||
            cells_map[cell_name]->net_list.back()!=current_net)
        {
            cells_map[cell_name]->net_list.push_back(current_net);
            current_net->cell_list.push_back(cells_map[cell_name]);
        }
        if(netfile.eof()) break;
        netfile >> cell_name >> driver_ ;       //read next line and judge its driver type
        //cout << "name: "<<cell_name << "   driver: " << driver_ <<endl;
        //Loop to read undriver cell
        while(driver_=='l')
        {
            pin_num++;
            //if current cell is undirver, put it into cells_map 
            if(cells_map[cell_name]->net_list.empty()||
                cells_map[cell_name]->net_list.back()!=current_net)
            {
                cells_map[cell_name]->net_list.push_back(current_net);
                current_net->cell_list.push_back(cells_map[cell_name]);
            }
            //if the cell is last cell, push net in nets and break
            if(netfile.eof()){nets.push_back(current_net); break;}
            netfile >> cell_name >> driver_;
            //if the read cell is a driver cell, jump out the loop
            //put the current net into nets and apply next loop
        }
        current_net->num_pins = pin_num;
        nets.push_back(current_net);
    }
    netfile.close();
    nets.pop_back();
    cout << "Complete reading Net!"<<endl;
    return;
}

void InitialPartition()
{
    srand(time(0));
    int index;
    cout <<"Initial Partition......." <<endl;
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
    //cout <<"Partition area 0/1 :" <<partition_area[0] <<"/" <<partition_area[1]<<endl;
    cout << "Partition Initial Complete!"<<endl;
}

//regiongrow algorithm initial partition
void RegiongrowInitialPartition()
{
    //choose a cell randomly
    srand(time(0));
    int rand_cell;
    rand_cell = rand() % cells.size();
    vector <CELL*> cell_region;     //a vector to store chosen cell
    vector <NET*> net_region;       //a vector to store chosen net
    cell_region.push_back(cells[rand_cell]);

    int index = 0;                  //to record the num of dead cell

    //when the num of dead cell less than half of total, do region grow
    while(cell_region.size() < cells.size()/2)
    {

        //do the loop until the vector is empty
        while(!cell_region.empty())   
        {
            //use k to represent the end element in the cell vector
            int k = cell_region.size() - 1;
            
            //put all the net has current cell into net-region
            for(int j=0; j<cell_region[k]->net_list.size();j++)
            {
                //if a net is dead, jump over it 
                if(cell_region[k]->net_list[j]->dead) 
                    continue;
                net_region.push_back(cell_region[k]->net_list[j]);
                //once a net is pushed into netregion, it was killed and dead
                cell_region[k]->net_list[j]->dead = true;
            }

            cell_region.pop_back();         //after complete a cell. pop it
        }

        //Loop the net-region
        while(!net_region.empty())
        {

            int k=net_region.size()-1;      //used to find last net in region

            for(int j=0; j<net_region[k]->cell_list.size();j++)
            {

                if(net_region[k]->cell_list[j]->dead) continue; //jump over dead cell
                //push chosen cell into cell region and kill it
                cell_region.push_back(net_region[k]->cell_list[j]);
                net_region[k]->cell_list[j]->dead = true;
                index++;
            }
            net_region.pop_back();
            // if dead cells >= totalcell/2 , end loop
            if(index>=cells.size()/2) break;   
        }
        if(index>=cells.size()/2) break;
    }

    //find dead cell and initial their partition
    for(int i=0;i<cells.size();i++)
    {
        if(cells[i]->dead)
        {
            cells[i]->partition = 1;
            cells[i]->best_Partition = 1;
            partition_area[0]-=cells[i]->cell_size;
            partition_area[1]+=cells[i]->cell_size;
        }
    }
    printpartition();
    return ;
}

void computeCutsize()
{
    cutsize=0;

    for(int i = 0; i <nets.size() ; i++)
    {
        //initial every net
        nets[i]->num_cell_Partition[0] = 0;
        nets[i]->num_cell_Partition[1] = 0;
        //count every net's cells' number in different part
        for(int j = 0; j<nets[i]->cell_list.size(); j++)
        {
            nets[i]->num_cell_Partition[nets[i]->cell_list[j]->partition]++;
        }
        //if a net have cell in both part, cutsize increase by net's weight
        if(nets[i]->num_cell_Partition[0] > 0 && nets[i]->num_cell_Partition[1]>0)
            cutsize=cutsize+nets[i]->weight;
    }
}

void computeGain()
{
    gain_bucket[0].clear();
    gain_bucket[1].clear();

    for( int i=0; i<cells.size();i++)
    {
        cells[i]->gain=0;
        //define the moving to partition
        //if two-way, to_partition=!partition
        //if k-way, to_partition should pair with gain bucket's index
        int to_partition=!cells[i]->partition;

        for(int j=0;j<cells[i]->net_list.size();j++)
        {
            //F(n)=1,g=g+weight
            if(cells[i]->net_list[j]->num_cell_Partition[cells[i]->partition]==1)
            {
                cells[i]->gain+=cells[i]->net_list[j]->weight;
                cells[i]->net_list[j]->critical=true;
            }
            //T(n)=0,g=g-weight
            if(cells[i]->net_list[j]->num_cell_Partition[to_partition]==0)
            {
                cells[i]->gain-=cells[i]->net_list[j]->weight;
                cells[i]->net_list[j]->critical=true;
            }
        }
        //put gain-cell pair into the gain bucket
        cells[i]->gain_itr=gain_bucket[cells[i]->partition].insert( pair <int,CELL*> (cells[i]->gain,cells[i]) );
    }

}

//print out the partition result
void partitionout(char outpath[100])
{
    string str=outpath;
    str+=".info";
    ofstream fout(str);
    for (int i=0; i<cells.size();i++)
    {
        fout << cells[i]->cell_name<<" "<<cells[i]->partition<<endl;
    }
    fout.close();
    return ;
}

//to judge whether is free(satisfy the balance constraint )
bool freecell(CELL* base_cell)
{
    if(base_cell == NULL) { cout << "NULL cell!"<<endl; return false;}
    bool balance;
    float rate_partition;
    //the balance factor should satisfy the constraint                 
    rate_partition= (float)(partition_area[base_cell->partition]-base_cell->cell_size) / (float) (partition_area[0]+partition_area[1]);
    //cout << rate_partition <<endl;
    balance = (rate_partition >= ubf);
    //cout << balance<<endl;
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
    CELL* base_cell;
    multimap <int,CELL*>::iterator itr[2];

    if(gain_bucket[0].size() != 0)  //choose the unempty bucket
        itr[0] = --gain_bucket[0].end();    //choose the highest gain
    if(gain_bucket[1].size() != 0)
        itr[1] = --gain_bucket[1].end();
    
    int from_partition;

    if(gain_bucket[0].size()==0)    //if bucket 0 has no cell
        from_partition = 1;         //choose cell from bucket1
    else if(gain_bucket[1].size()==0)
        from_partition = 0;
    else if(itr[0]->first >= itr[1]->first)
        from_partition = 0;
    else from_partition = 1;
    
    base_cell = itr[from_partition]->second;

    if(!freecell(base_cell))
    {
        //cout << "Can't move (imbalance)!"<<endl;
        from_partition = !from_partition;       //this equality should change when k-way 
        if(gain_bucket[from_partition].size()==0)
        {
            return false;
        }
        base_cell = itr[from_partition]->second;

        if(!freecell(base_cell))
        {
            return false;
        }
    }
    //if 2-way partition, to_partition= !basepartition
    //if k-way partition, to_partition should match with the gain_itr's index
    int to_partition = !base_cell->partition;
    (*base_cell).changepartition(to_partition);
    
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
    best_balance=0.5+abs(0.5-(float)partition_area[0]/(float)(partition_area[0]+partition_area[1]));
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
    int start_size=cutsize;
    //int current_cutsize = cutsize;
    int move_time=0;        //the moving times
    savebestsolution();
    while(Movecell())
    {   
        move_time++;
        float balance = 0.5+abs(0.5 - (float)partition_area[0]/(float)(partition_area[0]+partition_area[1]));
        //printf("Best balance : %f \t balance : %f \n", best_balance,balance);
        if( (cutsize < min_cutsize  )|| //the hill climp ability
            (cutsize == min_cutsize && balance <best_balance))         //if current solution has better balance,save it
            savebestsolution();
    }
    //after one pass, unlocked all cell and put them in gain bucket
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

void printpartition()
{
    int pp0=0;
    int pp1=0;
    for(int i=0;i<cells.size();i++)
    {
        if(!cells[i]->partition)
        {
            pp0++;
        }
        else
        {
            pp1++;
        }
    }
    cout << "partition 0 :" <<pp0 <<endl;
    cout << "partition 1 :" <<pp1 <<endl;
}

int main()
{
    clock_t start_time,end_time;
    out_fm();
    char cellpath[100];
    char netpath[100];
    cout <<"Input the cell path: " ;
    cin >> cellpath;
    cout <<"Input the net path : ";
    cin >> netpath;
    float unbalance_f;
    cout <<"Input the balance factor :";
    cin >> unbalance_f;
    
    //count time
    start_time=clock();
    ubf = unbalance_f;
    readCell(cellpath);
    readNet(netpath);
    RegiongrowInitialPartition();

    computeCutsize();
    cout << "Cut size after initial : " <<cutsize <<endl;
    computeGain();
    cout << "\n"<<endl;
    int pass_num=1;

    while(pass_num<=4 && LoopPass(pass_num))
    {
        pass_num ++;
    }
    
    recallbestsolution();
    computeCutsize();
    partitionout(netpath);
    cout << "\n" <<endl;
    cout <<"Partition 1 : "<< partition_area[0]<<endl;
    cout <<"Partition 2 : "<< partition_area[1]<<endl; 
    end_time=clock();
    cout << "The run time = " <<(double)(end_time-start_time)/CLOCKS_PER_SEC << "s" <<endl;
    
    int pins=0;
    //for(int i=0;i<cells.size();i++){        cout <<"cell name: " <<cells[i]->cell_name <<endl;}
    for(int i=0;i<nets.size();i++)
    {
        pins+=nets[i]->cell_list.size();
    }

    cout <<"Toal pins' number : " << pins << endl;
    return 0;
}