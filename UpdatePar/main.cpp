#include <iostream>
#include <string>
#include <ctime>
#include "partition.h"
using namespace std;

int main(int argc, const char* argv[]){
  string cell_file = argv[1];
  string net_file = argv[2];
  float ubf = atof(argv[3]);
  int pass_time = (int)atof(argv[4]);
  int init_part = (int)atof(argv[5]);

  clock_t start_time,end_time;
  cout <<"******************************************************"<<endl;
  cout <<"***************                      *****************"<<endl;
  cout <<"******************************************************"<<endl;
  if(ubf<0 || ubf >=0.5) {
    cout<<"Unbalance Factor should be limited in 0~0.5 " <<endl; 
    exit(1);
  }
  
  start_time=clock();

  Driver driver(pass_time,init_part,ubf);
  driver.readCell(cell_file);
  driver.readNet(net_file);
  driver.doInitialPartition();
  cout << "Initial Partition ";
  driver.computeCutSize();
  cout << "Cut Size : " << driver.cut_size_ <<endl;
  driver.computeGain();
  cout<<endl;
  int pass_num=1;
  while(pass_num<=pass_time && driver.LoopPass(pass_num)){
    pass_num ++;

  }
  driver.recallbestsolution();
  driver.computeCutSize();
  driver.print_partition(cell_file);

  cout << "\n" <<endl;
  cout <<"Partition 1 : "<< driver.partition_area_[0]<<endl;
  cout <<"Partition 2 : "<< driver.partition_area_[1]<<endl; 
  end_time=clock();
  cout << "The run time = " <<(double)(end_time-start_time)/CLOCKS_PER_SEC << "s" <<endl;

  int pins=0;
  //for(int i=0;i<cells.size();i++){cout <<"cell name: " <<cells[i]->cell_name <<endl;}
  for(int i=0;i<driver.getNet().size();i++){
    pins+=driver.getNet()[i]->getCell().size();
  }
  cout <<"Toal pins' number : " << pins << endl;
  //driver.~Driver();
  return 0;
}
