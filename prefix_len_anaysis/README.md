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
prefix length : # of nodes
             0 :        0
             1 :        0
             ...
            28 :      951
            29 :     1369
            30 :      618
            31 :       69
            32 :    10253
            33 :     2257
            34 :     2291
            35 :     1067
            36 :     4232
            37 :       75
            38 :      275
            39 :        0
            40 :      896
            41 :       58
            42 :      398
            43 :       39
            44 :      669
            45 :       57
            46 :     2616
            47 :      143
            48 :     5647
            49 :        0
            50 :        0
            51 :        0
            ...
            128:		0
```