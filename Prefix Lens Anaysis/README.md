# Prefix Lens Analysis

## Purpose
Analysis IPv4 and IPv6 prefix distribution.

## How to use(ipv4)
1. complie with `gcc. Use $ gcc ipv4_update_prefixLen_analysis.c` 
2. Run executable file. `./a.out "target file"`
* Provide an example file named "rrc_ipv4_update.txt" to test and input shoudl following this format.

## How to use(ipv6)
1. complie with `gcc. Use $ gcc ipv6_update_prefixLen_analysis.c` 
2. Run executable file. `./a.out "target file"`
* Provide an example file named "rrc_ipv6_update.txt" to test and input shoudl following this format.
* In this codes, it can deal with all case with ipv6 address count. Because it expandition "::" to full address format.
    * `2404:5780:3::/48` will turn to `2404:5780:0003:0000:0000:0000:0000:0000` and do table set operation. 
    * `2404:5780::3/48` will turn to `2404:5780:0000:0000:0000:0000:0000:0003` and do table set operation. 
    * `::3/48` will turn to `0000:0000:0000:0000:0000:0000:0000:0003` and do table set operation. 

## Prefix Length Distribution(IPv4)
```
$ ./a.out rrc_ipv4_update.txt
prefix length : # of nodes : percentage
            0 :          1 :     0.00
            1 :          0 :     0.00
            2 :          0 :     0.00
            3 :          0 :     0.00
            4 :          0 :     0.00
            5 :          0 :     0.00
            6 :          0 :     0.00
            7 :          0 :     0.00
            8 :          3 :     0.00
            9 :          3 :     0.00
           10 :         11 :     0.01
           11 :         29 :     0.01
           12 :        117 :     0.06
           13 :        193 :     0.10
           14 :        341 :     0.17
           15 :        552 :     0.28
           16 :       2454 :     1.25
           17 :       1880 :     0.96
           18 :       3792 :     1.93
           19 :       6761 :     3.44
           20 :       9161 :     4.66
           21 :      11054 :     5.62
           22 :      22947 :    11.66
           23 :      19978 :    10.16
           24 :     117227 :    59.59
           25 :          5 :     0.00
           26 :          4 :     0.00
           27 :          3 :     0.00
           28 :          2 :     0.00
           29 :          5 :     0.00
           30 :         13 :     0.01
           31 :          5 :     0.00
           32 :        186 :     0.09
Total nodes: 196727
```

## Prefix Length Distribution(IPv6)
```
prefix length : # of nodes : percentage
            0 :          0 :     0.00
            1 :          0 :     0.00
            2 :          0 :     0.00
...
           25 :          0 :     0.00
           26 :          0 :     0.00
           27 :          0 :     0.00
           28 :        951 :     2.80
           29 :       1369 :     4.03
           30 :        618 :     1.82
           31 :         69 :     0.20
           32 :      10253 :    30.17
           33 :       2257 :     6.64
           34 :       2291 :     6.74
           35 :       1067 :     3.14
           36 :       4232 :    12.45
           37 :         75 :     0.22
           38 :        275 :     0.81
           39 :          0 :     0.00
           40 :        896 :     2.64
           41 :         58 :     0.17
           42 :        398 :     1.17
           43 :         39 :     0.11
           44 :        669 :     1.97
           45 :         57 :     0.17
           46 :       2616 :     7.70
           47 :        143 :     0.42
           48 :       5647 :    16.62
           49 :          0 :     0.00
           50 :          0 :     0.00
...
```