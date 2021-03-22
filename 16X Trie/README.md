# Compressed 16-X Multibit Trie

## How to use
1. Complie with gcc. Use `$ gcc 16X_trie.c -lm` or `$ gcc 16X_trie.c -lm -o2` command.
2. Run executable file. Format is like: `$ ./a.out "table build file", "search file", "update file"` 
    * Example：`./a.out ipv4_90build.txt ipv4_90build.txt ipv4_10insert.txt`
* If you want time consumption by failure search. Building the trie with 90% raw data and search trie with 100% raw data. This method is efficient when 90% raw data are not overlap with 100% raw data.
    * Example：`./a.out ipv4_90build.txt ipv4_100build.txt ipv4_10inert.txt`, This situation can get 10% miss search with your data input. 

## Time Analysis
|Item|Result(clocks)|
|-----|-----|
|Avg. Build Time Per Entry|14957|
|Avg. Search Time Per Entry(All hit)|12|
|Avg. Search Time Per Entry(10% miss)|12|
|(Max Clock, Min Clock)|(294, 12)|
|Avg. Update Time Per Entry|20716|

## Clock Distribution
|Clock Range|# of nodes|Percentage(%)|
|-----|-----|-----|
|0-100|996963|99.90|
|100-200|868|0|
|200-300|121|0|
|300-400|0|0|
|400-500|0|0|
|500-600|0|0|
|600-700|0|0|
|700-800|0|0|
|800-900|0|0|
|900-1000|0|0|
|1000-1100|0|0|
|1100-1200|0|0|
|1200-1300|0|0|
|1300-1400|0|0|
|1400-1500|0|0|
|1500-1600|0|0|
|1600-1700|0|0|
|1700-1800|0|0|
|1800-1900|0|0|
|1900-2000|0|0|
|2000-2100|0|0|
|2100-2200|0|0|
|2200-2300|0|0|
|2300-2400|0|0|
|2400-2500|0|0|
|2500-2600|0|0|
|2600-2700|0|0|
|2700-2800|0|0|
|2800-2900|0|0|
|2900-3000|0|0|
|3000-3100|0|0|
|3100-3200|0|0|
|3200-3300|0|0|
|3300-3400|0|0|
|3400-3500|0|0|
|3500-3600|0|0|
|3600-3700|0|0|
|3700-3800|0|0|
|3800-3900|0|0|
|3900-4000|0|0|
|4000-4100|0|0|
|4100-4200|0|0|
|4200-4300|0|0|
|4300-4400|0|0|
|4400-4500|0|0|
|4500-4600|0|0|
|4600-4700|0|0|
|4700-4800|0|0|
|4800-4900|0|0|
|4900-5000|0|0|
|5000+|0|0|
|**Total**|**997952**|**99.9**|

## Space Analysis
|Item|Result|
|-----|-----|
|# of subnodes created|80386512|
|# of subnodes created have nexthop value|18047|
|# of segnodes created have nexthop value|65698430|
|Total memory requirement(before insert)|314649(KB)|
|# of subnodes after insert operation|84028182|
|# of subnodes after insert operation have nexthop value|19886|
|# of segnodes after insert operation have nexthop value|65738427|
|Total memory requirement(after insert)|328875(KB)|

### Space Computation Method
* In this structure, have fixed number(65536) segment block. And every segment block have 1byte record height, 1byte for port, 8byte for two pointer.
* Every segment block pointer point to a subnodes array, subnodes array is always power of 2. Every node use 1byte to record nexthop.
* Calculate method: 65536*(1+1+4+4) + Total subnodes\*1= TotalSpace Consumption
