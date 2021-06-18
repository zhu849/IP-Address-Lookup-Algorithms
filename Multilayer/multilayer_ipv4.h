#ifndef MULTILAYER_H
#define MULTILAYER_H

/* Use to check B-tree boundary */
// #define MAX 256  
// #define MIN 128 
// #define MAX 128  
// #define MIN 64
 #define MAX 64 
 #define MIN 32 
 //#define MAX 32
 //#define MIN 16
//#define MAX 16
//#define MIN 8
 //#define MAX 8
 //define MIN 4

/* Structure of Binary Trie */
typedef struct list{
	unsigned int ip;
	unsigned int port;
	unsigned int len;
	struct list *left,*right,*parent;
	unsigned int layer;
	unsigned int layer0_count;
	int level;
	int toplayer;
}node;
typedef node *btrie;

typedef struct IP_information{
	unsigned int ip;
	unsigned int len;
	unsigned int layer;
}IP_Inf;

struct Layer0_BTreeNode {
  IP_Inf *item;
  int count;
  struct Layer0_BTreeNode *link[MAX + 1]; 
};

struct Upper_BTreeNode {
  IP_Inf *item;
  IP_Inf *eset;
  int count;
  int eset_num;
  int max_len;
  struct Upper_BTreeNode *nextlevel_root[MAX + 1];
  struct Upper_BTreeNode *link[MAX + 1];
};

typedef struct Top_S{
	unsigned int ip;
	unsigned int len;
	int num;
	btrie trie_node;
	struct Top_S * nextlevel;
	struct Upper_BTreeNode *nextlevel_root;
}Top_struct;

struct ENTRY{
	unsigned int ip;
	unsigned int len;
	unsigned int port;
	unsigned int layer;
	btrie root;
};

struct segment_array{
	unsigned int ip;
	unsigned int len;
	unsigned int port;
	unsigned int len_list;
};

typedef struct list_element{
	struct Layer0_BTreeNode *B_root;
	struct ENTRY * list_table;
	int num;
} tentry;


#endif 