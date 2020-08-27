# Update
new code is upload in new file

update the region-grow algorithm initializer and delete the hill-climp argv

### using RG initializer the cutsize can be decrease to ~400

# FM_Bipartitioning
This pj has mainly two edition
fisrt edition is to debug with many notes

second edition is used to implement in commandline without notes

how to use the second to partition a graph

1: g++ code.cpp -o code.exe

2: ./code.exe cellfile netfile ubf pass_num init_part

#### input argv notes:
cellfile & netfile is the input file of cells(nodes) and nets

ubf: unbalance factor, balance constraint should be 0~0.5

pass_num: number of pass, suggest num: 5~15

init_part: should be 0, 1 or 2

  0: initial randomly
  
  1: initial by the input order
  
  2: initial with the region-grow algorithm

eg: ./tmp.exe ibm01.are ibm01.net 0.45 10 2

### testfm.cpp is the original edition with many notes
