//----------------------------------------------------*- C++ -*-------------------//
// file: partition.h
// author: xbzhao, lffan
// purpose: to implement the circuit partition with Fiduccia Mattheyses Algorithm
//--------------------------------------------------------------------------------//

#ifndef PARTITION_H_
#define PARTITION_H_

#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <ctime>
#include <random>

using namespace std;
#define K 2 //the k-way partition

//forward declaration
class Net;
class Partition;
class Driver;
//===----------------------------------------------------------------------===//
//                               Cell class
//===----------------------------------------------------------------------===//
class Cell{
public:
  Cell(string name, int cell_size);
  ~Cell(){}
public:
  void changePartition(int target);
  void updateGain(int gain);
	int getgain() { return gain_;}
  friend class Net;
  friend class Driver;
private:
  string name_;
  int gain_;
  int cell_size_;//now use cell size to replace resource
  bool locked_;
  bool dirty_;//dead
  int owner_;//the partition this obj now belong to
  int best_;
  Driver* driver_;
  multimap<int,Cell*>::iterator gain_itr_;
  vector<Net*> nets_;
};

//===----------------------------------------------------------------------===//
//                               Net class
//===----------------------------------------------------------------------===//
class Net{
public:
  Net(string name, int weight);
  ~Net(){}

  vector<Cell*> getCell(){ return cells_; }//api for main 
  
  friend class Cell;
  friend class Driver;
private:
  string name_;
  int pin_num_;
  int weight_;
  bool cut_state_;
  bool dirty_;
  bool critical_;
  bool lock_partition_[K];
  int partition_cell_num_[K];//the num of cells in partition x    
  vector<Cell*> cells_;
};


//===----------------------------------------------------------------------===//
//                               Resource class
//===----------------------------------------------------------------------===//
//the base class of all resource in cell
//TODO: do NOT support now
class Resource{};

//===----------------------------------------------------------------------===//
//                              Partition class
//===----------------------------------------------------------------------===//
//the base class of all constraint
class Constraint{};

//===----------------------------------------------------------------------===//
//                               Dirver class
//===----------------------------------------------------------------------===//
class Driver{
public:
  Driver(int pass_num, int initial_type, float ubf);
  ~Driver();
public:
  void readCell(string cell_file);
  void readNet(string net_file);

  vector<Net*> getNet(){ return nets_; }
	vector<Cell*> getCell() { return cells_;}
  void doInitialPartition();
  void computeCutSize();
  void computeGain();

  void saveBest();
  void recallbestsolution();
  bool LoopPass(int pass_num);
public:
  void print_partition(string path);//partition_out
  void printFile(string file_name);
private:
  void RandomPartition();
  void InputOrderPartition();
  void RegionGrowPartition();

  bool freeCell(Cell* cell);
  bool MoveCell();

  friend class Cell;
  friend class Net;
public:
  int pass_num_;
  int initial_type_;
  bool gain_positive_;
  int partition_area_[K];
  multimap<int, Cell*> gain_bucket_[2];
  int cut_size_;
private:
  int mini_cut_size_;
  float unblanced_factor_;
  float best_factor_;
private:
  vector<Cell*> cells_;
  vector<Net*> nets_;
  unordered_map<string, Cell*> cell_map_;
};

#endif //!PARTITION_H_
