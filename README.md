# FM_Bipartitioning
This pj has mainly two edition
fisrt edition is to debug with many notes

second edition is used to implement in commandline without notes

how to use the second to partition a graph

1: g++ code.cpp -o code.exe

2: ./code.exe cellfile netfile ubf pass_num hl init_part

#### input argv notes:
cellfile & netfile is the input file of cells(nodes) and nets

ubf: unbalance factor, balance constraint should be 0~0.5

pass_num: number of pass, suggest num: 5~15

hl: hill_climping ability, can hardly influence the result, suggest 1.2

init_part: should be 0 or 1, 0: initial by the input order, 1: initial randomly

eg: ./tmp.exe ibm01.are ibm01.net 0.45 10 1.2 0

### testfm.cpp is the original edition with many notes
