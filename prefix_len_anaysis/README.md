# Prefix Lens Analysis

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
Distrbute status:
prefix length:0 = 1
prefix length:1 = 0
prefix length:2 = 0
prefix length:3 = 0
prefix length:4 = 0
prefix length:5 = 0
prefix length:6 = 0
prefix length:7 = 0
prefix length:8 = 3
prefix length:9 = 3
prefix length:10 = 11
prefix length:11 = 29
prefix length:12 = 117
prefix length:13 = 193
prefix length:14 = 341
prefix length:15 = 552
prefix length:16 = 2454
prefix length:17 = 1880
prefix length:18 = 3792
prefix length:19 = 6761
prefix length:20 = 9161
prefix length:21 = 11054
prefix length:22 = 22947
prefix length:23 = 19978
prefix length:24 = 117227
prefix length:25 = 5
prefix length:26 = 4
prefix length:27 = 3
prefix length:28 = 2
prefix length:29 = 5
prefix length:30 = 13
prefix length:31 = 5
prefix length:32 = 186
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