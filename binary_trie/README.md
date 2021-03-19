# Binary Trie

## How to use
1. complie with gcc. Use `$ gcc Binary_trie.c` or `$ gcc Binary_trie.c -o2` command.
2. Run executable file. Format is like: `$ ./a.out "table build file", "search file", "update file"` 
    * Example：`./a.out ipv4_90build.txt ipv4_90build.txt ipv4_10insert.txt`
* If you want time consumption by failure search. Building the trie with 90% raw data and search trie with 100% raw data. This method is efficient when 90% raw data are not overlap with 100% raw data.
    * Example：`./a.out ipv4_90build.txt ipv4_100build.txt ipv4_10inert.txt`, This situation can get 10% miss search with your data input. 

## Time Analysis
|Item|Result(clocks)|
|-----|-----|
|Avg. Build Time Per Entry|940|
|Avg. Search Time Per Entry(All hit)|956|
|Avg. Search Time Per Entry(10% miss)|942|
|(Max Clock, Min Clock)|(2338, 150)|
|Avg. Update Time Per Entry|1040|

## Clock Distribution
|Clock Range|# of nodes|Percentage(%)|
|-----|-----|-----|
|0-100|0|0|
|100-200|10|0|
|200-300|261|0|
|300-400|3234|0|
|400-500|13586|1|
|500-600|41116|4|
|600-700|78123|8|
|700-800|121885|12|
|800-900|159816|16|
|900-1000|160265|16|
|1000-1100|153864|15|
|1100-1200|114984|12|
|1200-1300|73266|7|
|1300-1400|43836|4|
|1400-1500|19833|2|
|1500-1600|8792|1|
|1600-1700|3323|0|
|1700-1800|1127|0|
|1800-1900|429|0|
|1900-2000|141|0|
|2000-2100|39|0|
|2100-2200|16|0|
|2200-2300|5|0|
|2300-2400|2|0|
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
|**Total**|**997952**|98|

## Space Analysis
|Item|Result|
|-----|-----|
|# of nodes created|2282297|
|Total memory requirement(before insert)|20059(KB)|
|# of nodes after inserted|2420550|
|Total memory requirement(after insert)|21274(KB)|

### Space Computation Method
* Because one node have 2 pointer(4byte) and 1 exact value, port(1byte). Calculate method show like below： 
	* Total Node * (2*4+1) = Total Space Consumption
