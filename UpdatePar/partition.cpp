//-------------------------------------------//
//author: xbzhao, lffan
//-------------------------------------------//

#include "partition.h"
#include <string>
//===----------------------------------------------------------------------===//
//                               Cell Implementation
//===----------------------------------------------------------------------===//
Cell::Cell(string name, int cell_size)
    : name_(name),
      cell_size_(cell_size),
      gain_(0),
      locked_(false),
      dirty_(false),
      driver_(nullptr),
      owner_(0),
      best_(0) {}


void Cell::changePartition(int target){
  locked_ = true;
  driver_->cut_size_ -= gain_;
  auto gain_bucket = driver_->gain_bucket_;
  gain_bucket[owner_].erase(gain_itr_);

  driver_->partition_area_[owner_] -= cell_size_;
  driver_->partition_area_[target] += cell_size_;

  if(gain_< 0) 
    driver_->gain_positive_ = false;
  for(int i = 0; i < nets_.size();i++){
    if(nets_[i]->lock_partition_[0] && nets_[i]->lock_partition_[1])
      continue;
    
    int from = nets_[i]->partition_cell_num_[owner_];
    int to = nets_[i]->partition_cell_num_[target];

    if(to == 0){
      for(int k = 0; k < nets_[i]->cells_.size(); k++){
        if(!nets_[i]->cells_[k]->locked_)
          nets_[i]->cells_[k]->updateGain(nets_[i]->weight_);
      }
    }
    else if(to == 1){
      for(int k = 0; k < nets_[i]->cells_.size(); k++){
        if(!nets_[i]->cells_[k]->locked_ 
            && nets_[i]->cells_[k]->owner_ == target){
          nets_[i]->cells_[k]->updateGain(-nets_[i]->weight_);
          break;
        }
      }
    }
    from--;
    to++;
    nets_[i]->partition_cell_num_[owner_]--;
    nets_[i]->partition_cell_num_[target]++;

    if(from == 0){
      for(int k = 0; k < nets_[i]->cells_.size();k++){
        if(!nets_[i]->cells_[k]->locked_)
          nets_[i]->cells_[k]->updateGain(-nets_[i]->weight_);
      }
    }	
    else if(from == 1){
      for(int k = 0; k < nets_[i]->cells_.size();k++){
        if(!nets_[i]->cells_[k]->locked_ 
            && nets_[i]->cells_[k]->owner_ == owner_){
          nets_[i]->cells_[k]->updateGain(nets_[i]->weight_);
          break;
        }
      }
    }
    //after moving, the net's To partition is locked
    nets_[i]->lock_partition_[target] = true;
  }
  owner_ = target;
}

void Cell::updateGain(int gain_value){
  if(!locked_){
    auto gain_bucket = driver_->gain_bucket_;
    gain_bucket[owner_].erase(gain_itr_);
    gain_ += gain_value;
        //cout << "Update Gain: "<< cell_name <<" gain change: " << gain_value<<endl;
    gain_itr_ = gain_bucket[owner_].insert(pair<int,Cell*>(gain_,this));
  }
}


//===----------------------------------------------------------------------===//
//                               Net Implementation
//===----------------------------------------------------------------------===//
Net::Net(string name, int weight)
    : name_(name),
      weight_(weight),
      pin_num_(0),
      cut_state_(false),
      dirty_(false),
      critical_(false){
  for(int i = 0; i < K; ++i)
    lock_partition_[i] = false;
}


//===----------------------------------------------------------------------===//
//                               Driver Implementation
//===----------------------------------------------------------------------===//
Driver::Driver(int pass_num, int initial_type, float ubf)
    : pass_num_(pass_num),
      initial_type_(initial_type),
      unblanced_factor_(ubf),
      cut_size_(0),
      mini_cut_size_(0),
      gain_positive_(false),
      best_factor_(0) {
  for(int i = 0; i < K; ++i)
    partition_area_[i] = 0;
}

Driver::~Driver(){
	for(int i = 0; i < nets_.size();i++)
		delete nets_[i];
	for(int i = 0; i < cells_.size();i++)
		delete cells_[i];
}

void Driver::readCell(string cell_file){
  ifstream read_cell(cell_file);
  assert(read_cell.is_open() && "Failed to open cell_file");
  cout<<"Reading cells......"<<endl;
  while (!read_cell.eof()){
    string cell_name = "";
    int cell_size = 0;
    read_cell >> cell_name >> cell_size;
    assert(cell_name != "" && cell_size > 0);
    Cell* cell = new Cell(cell_name, cell_size);
    cell->driver_ = this;

    partition_area_[0] += cell_size;
    cells_.push_back(cell);
    cell_map_[cell_name] = cell;
  }
  read_cell.close();
  cout<<"Read Cell complete!"<<endl;
}

void Driver::readNet(string net_file){
  ifstream read_net(net_file);
  assert(read_net.is_open() && "Failed to open net_file");
  cout<<"Reading nets......"<<endl;
  string cell_name;
  string driver;
  int weight = 0;
  read_net >> cell_name >> driver;
  while (!read_net.eof())
  {
    read_net >> weight;
    Net* net = new Net(cell_name, weight);
    int pin_num = 1;

    if(cell_map_[cell_name]->nets_.empty() 
       || cell_map_[cell_name]->nets_.back() != net){
      cell_map_[cell_name]->nets_.push_back(net);
      net->cells_.push_back(cell_map_[cell_name]);
    }

    read_net >> cell_name >> driver;
    
    while (driver == "l")
    {
      pin_num++;
      if(cell_map_[cell_name]->nets_.empty() 
         || cell_map_[cell_name]->nets_.back() != net){
        cell_map_[cell_name]->nets_.push_back(net);
        net->cells_.push_back(cell_map_[cell_name]);
      }
      if(read_net.eof()){
        nets_.push_back(net);
        break;
      }
      read_net >> cell_name >> driver;
    }
    net->pin_num_ = pin_num;
    nets_.push_back(net);
  }
  read_net.close();
  nets_.pop_back();
  cout << "Complete reading Net!"<<endl;
}

void Driver::RandomPartition(){
  srand(time(0));
  int index = 0;
  cout <<"Initial Partition......." <<endl;
  while((float)partition_area_[1] / (float)(partition_area_[0]+partition_area_[1]) < 0.55){
    index = rand() % cells_.size();
    if(cells_[index]->owner_ == 0){
      cells_[index]->owner_ = 1;
      cells_[index]->best_ = 1;
      partition_area_[0] -= cells_[index]->cell_size_;
      partition_area_[1] += cells_[index]->cell_size_;
    }
  }
  cout<<"Partition Initial Complete!"<<endl;
}

void Driver::InputOrderPartition(){
  for(int i = 0; i < cells_.size(); ++i){
    if( i < (cells_.size()/2)){
      cells_[i]->owner_ = 1;
      cells_[i]->best_ = 1;
      partition_area_[0] -= cells_[i]->cell_size_;
      partition_area_[1] += cells_[i]->cell_size_;
    }
    else
      break;
  }
  cout<< "Input initialing Partition complete! " <<endl;
}

void Driver::RegionGrowPartition(){
  srand(time(0));
  int rand_cell = rand() % cells_.size();
  vector<Cell*> cell_region;
  vector<Net*> net_region;
  cell_region.push_back(cells_[rand_cell]);

  int index = 0;
  while (cell_region.size() < cells_.size()/2){
    while(!cell_region.empty()){
      int k = cell_region.size()-1;
      for(int j = 0; j < cell_region[k]->nets_.size(); ++j){
        if((cell_region[k]->nets_)[j]->dirty_ == 1)
          continue;
        net_region.push_back(cell_region[k]->nets_[j]);
        cell_region[k]->nets_[j]->dirty_ = 1;
      }
      cell_region.pop_back();
    }
    while(!net_region.empty()){
      int k = net_region.size() - 1;
      for(int j = 0; j < net_region[k]->cells_.size(); ++j){
        if(net_region[k]->cells_[j]->dirty_ == 1)
          continue;
        cell_region.push_back(net_region[k]->cells_[j]);
        net_region[k]->cells_[j]->dirty_ = 1;
        index++;
      }
      net_region.pop_back();
      if(index >= cells_.size()/2)
        break;
    }
    if(index >= cells_.size()/2)
        break;
  }
  for(int i = 0; i < cells_.size(); ++i){
    if(cells_[i]->dirty_ == 1){
      cells_[i]->owner_ = 1;
      cells_[i]->best_ = 1;
      partition_area_[0] -= cells_[i]->cell_size_;
      partition_area_[1] += cells_[i]->cell_size_;
    }
  }
}

void Driver::doInitialPartition(){
  switch(initial_type_){
    case 0:
      RandomPartition();
      break;
    case 1:
      InputOrderPartition();
      break;
    case 2:
      RegionGrowPartition();
      break;
    default:
      assert(0);
  }
}

void Driver::computeCutSize(){
  cut_size_ = 0;
  for(int i = 0; i < nets_.size(); ++i){
    nets_[i]->partition_cell_num_[0] = 0;
    nets_[i]->partition_cell_num_[1] = 0;
    for(int j = 0;j < nets_[i]->cells_.size(); j++){
      nets_[i]->partition_cell_num_[nets_[i]->cells_[j]->owner_]++;
    }
    if(nets_[i]->partition_cell_num_[0] > 0 && nets_[i]->partition_cell_num_[1] > 0)
      cut_size_ += nets_[i]->weight_;
  }
}

void Driver::computeGain(){
  gain_bucket_[0].clear();
  gain_bucket_[1].clear();
  for(int i = 0; i < cells_.size(); i++){
    cells_[i]->gain_ = 0;
    int target_partition = 1 - cells_[i]->owner_;
    
    for(int j = 0; j < cells_[i]->nets_.size(); j++){
      if(cells_[i]->nets_[j]->partition_cell_num_[cells_[i]->owner_] == 1){
        int new_gain = cells_[i]->gain_ + cells_[i]->nets_[j]->weight_;
        cells_[i]->gain_ = new_gain;
        cells_[i]->nets_[j]->critical_ = true;      
      }
      if(cells_[i]->nets_[j]->partition_cell_num_[target_partition] == 0){
        int new_gain =  cells_[i]->gain_-cells_[i]->nets_[j]->weight_ ;
        cells_[i]->gain_ = new_gain;
        cells_[i]->nets_[j]->critical_ = true;
      }
    }
    auto it = gain_bucket_[cells_[i]->owner_].insert(pair<int,Cell*>(cells_[i]->gain_, cells_[i]));
    cells_[i]->gain_itr_ = it;
  }
}

bool Driver::freeCell(Cell* cell){
  assert(cell != nullptr);//if cell is a nullptr then won't return any value
  bool balance;
  float rate_partition = (float)(partition_area_[cell->owner_] - cell->cell_size_)
   / (float)(partition_area_[0]+partition_area_[1]);
  balance = ( rate_partition >= unblanced_factor_);
  return balance;
}

bool Driver::MoveCell(){
  if(gain_bucket_[0].size()==0 && gain_bucket_[1].size()==0)
    return false;
  Cell* base_cell = nullptr;
  multimap<int, Cell*>::iterator itr[2];
  if(gain_bucket_[0].size() != 0)
    itr[0] = --gain_bucket_[0].end();    //choose the highest gain
  if(gain_bucket_[1].size() != 0)
    itr[1] = --gain_bucket_[1].end();
  
  int from_partition = 0;

  if(gain_bucket_[0].size()==0)    //if bucket 0 has no cell
    from_partition = 1;         //choose cell from bucket1
  else if(gain_bucket_[1].size()==0)
    from_partition = 0;
  else if(itr[0]->first >= itr[1]->first)
    from_partition = 0;
  else from_partition = 1;
    
  base_cell = itr[from_partition]->second;

  if(!freeCell(base_cell)){
    from_partition = !from_partition;       //this equality should change when k-way 
    if(gain_bucket_[from_partition].size()==0)
      return false;
    base_cell = itr[from_partition]->second;

    if(!freeCell(base_cell))
      return false;  
  }
  int to_partition = !base_cell->owner_;
  base_cell->changePartition(to_partition);
  return true;
}
//int save_time=0;
//float save_t=0;
void Driver::saveBest(){
  mini_cut_size_ = cut_size_;
  for(int i = 0; i < cells_.size(); ++i){
    cells_[i]->best_ = cells_[i]->owner_;
  }
  best_factor_ = 0.5+abs(0.5-(float)partition_area_[0]/(float)(partition_area_[0]+partition_area_[1]));
  //et=clock();
  //save_time++;
  //save_t+=(float)(et-st)/CLOCKS_PER_SEC;
  //if(save_time%1000==0)
  //{
  //  cout<<save_time << " th save cost time : " <<save_t <<"s"<<endl;
  //}
}

void Driver::recallbestsolution(){
  cut_size_ = mini_cut_size_;
  for(int i = 0; i < cells_.size(); ++i){
    cells_[i]->owner_ = cells_[i]->best_;
    cells_[i]->locked_ = 0;//0 = false
  }
  for(int i = 0; i < nets_.size(); ++i){
    nets_[i]->partition_cell_num_[0] = 0;
    nets_[i]->partition_cell_num_[1] = 0;

    nets_[i]->lock_partition_[0] = false;
    nets_[i]->lock_partition_[1] = false;

    for(int k = 0; k < nets_[i]->cells_.size(); ++k){
      int par = nets_[i]->cells_[k]->owner_;
      nets_[i]->partition_cell_num_[par]++;
    } 
  }
  computeGain();
  partition_area_[0] = 0;
  partition_area_[1] = 0;
  
  // recall the best solution's area
  for(int i = 0; i < cells_.size(); i++){
    partition_area_[cells_[i]->owner_] += cells_[i]->cell_size_;
  }
}

bool Driver::LoopPass(int pass_num){
  int start_size = cut_size_;
    //int current_cutsize = cutsize;
  int move_time=0;        //the moving times
  saveBest();
	int save_time=0;
  while(MoveCell()){   
    move_time++;
    float balance = 0.5+abs(0.5 - (float)partition_area_[0]/(float)(partition_area_[0]+partition_area_[1]));
    if( (cut_size_ < mini_cut_size_  )||(cut_size_ == mini_cut_size_ && balance < best_factor_))
      saveBest();
			save_time++;
  }
  recallbestsolution();
  computeCutSize();
  computeGain();
  cout << "The "<<pass_num << "th pass cut size :" <<cut_size_<<endl;
  return true;
}

void Driver::printFile(string file_name){
  ofstream outfile(file_name);
  outfile << "The netlist has net :" << nets_.size() <<endl;
  outfile << "\t\t\t" <<"cell: " <<cells_.size() <<endl;
  outfile << "The Final cutsize is : " << cut_size_ <<endl;
  outfile << "Partition 0 area : " << partition_area_[0] <<endl;
  outfile << "Partition 1 area : " << partition_area_[1] <<endl;
  outfile.close();
}

void Driver::print_partition(string path){
  string str = path + ".info";
  ofstream fout(str);
  for(int i = 0; i < cells_.size();i++){
    fout << cells_[i]->name_<<" "<<cells_[i]->owner_<<endl;
  }
  fout.close();
}
