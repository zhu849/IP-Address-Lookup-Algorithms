#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include "multilayer_ipv4.h"

/* Swap macro */
#define SWAP(x, y) \
    x ^= y;\
    y ^= x;\
    x ^= y;

/* Time analysis func*/
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

/* Global var */
struct ENTRY *table, *insert_table, *delete_table;
struct segment_array *segment_16;
struct Layer0_BTreeNode *btree_root;
struct Upper_BTreeNode * top_root, * allBtree_root;
IP_Inf *layer0_table, *toplayer_table; 
Top_struct *Upper_Btree_root;
btrie root;
tentry * segement_table;

unsigned long long int begin,end,total=0;
unsigned long long int *clock1;

unsigned int mask_value[33];
unsigned int *query;

int layerArray[10] = {0};// Use to count how many nodes in different layer
int num[65536] = {0};
int num_entry = 0;
int num_insert = 0;
int num_query = 0;
int num_delete = 0; 
int layer0_count = 0;
int match_rule = 0;
int no_match = 0;
int layer0_found = 0;
int segment_16_found = 0;
int Insert2Up = 0;
int Cover_layer0 = 0;
int Replcae_Layer0 = 0;
int replace_flag = 0;
int insertValue_flag = 0;
int splitNode_flag = 0;
int ltable_c = 0; // Use to count layer table
int num_layer_node = 0;
int root_num = 0; // Use to count toplayer-1 node table //?????? like toplayer_array[1]
int node_count[30] = {0};
int toplayer_array[30] = {0}; // Use to count nodes of different top layer
int count_n = 0;
int match_flag=0;
int case1 = 0, case2 = 0,case3 = 0;
int mem_access = 0;
int top_layer_num = 0;//?????? like toplayer_array[1]
/*Just test*/
int level_node_num[10][10] = {0};
int key_dis[10000] = {0};

/* Initial table with build, search, insert, delete operation*/
/* Format data of an entry */
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";
	char buf[100],*str1;
	unsigned int n[4];
	sprintf(buf,"%s\0",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[3]=atoi(buf);
	*nexthop=n[2];
	str1=(char *)strtok(NULL,tok);
	if(str1!=NULL){
		sprintf(buf,"%s\0",str1);
		*len=atoi(buf);
	}
	else{
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
	*ip=n[0];
	*ip<<=8;
	*ip+=n[1];
	*ip<<=8;
	*ip+=n[2];
	*ip<<=8;
	*ip+=n[3];
}
/* These tables are read from outside */
void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;

	fp=fopen(file_name,"r");

	/* Count # of table entry */
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_entry++;
	}
	rewind(fp);
	printf("Number of build table entry = %d\n", num_entry);

	table = (struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	segment_16 = (struct segment_array *)malloc(65536*sizeof(struct segment_array));
	
	/* Segment table initialize*/
	for( int i = 0; i < 65536; i++ ){
		segment_16[i].port = 256;
		segment_16[i].len = 0;
		segment_16[i].len_list = 0;
	}

	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		if( len > 16 ){
			table[num_entry].ip=ip;
			table[num_entry].port=nexthop;
			table[num_entry++].len=len;
		}
		else if(len == 16){
			ip = ip>>16;
			segment_16[ip].port=1;/* When segment table 16's port is 1 mean this blank have some node prefix length <= 16 */
			segment_16[ip].len=16;
			segment_16[ip].len_list |= (1<<16); 
		}
		else{
			unsigned int boundary_begin = ip >> 16;
			unsigned int boundary_end = boundary_begin + (1<<(16-len))-1;

			for( int i = boundary_begin; i <= boundary_end; i++ ){
				segment_16[i].port=1;/* When segment table 16's port is 1 mean this blank have some node prefix length <= 16 */
				segment_16[i].len_list |= (1<<(32-len) );
			}
		}
	}
}

void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;

	fp=fopen(file_name,"r");

	/* Count # of table entry */
	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
	printf("Number of query table entry = %d\n", num_query);

	query=(unsigned int *)malloc(num_query*sizeof(unsigned int));
	clock1=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));

	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		query[num_query]=ip;
		clock1[num_query++]=10000000;
	}
}

void set_insert(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;

	fp=fopen(file_name,"r");

	/* Count # of table entry */
	num_insert=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_insert++;
	}
	rewind(fp);
	printf("Number of insert table entry = %d\n", num_insert);

	insert_table=(struct ENTRY *)malloc(num_insert*sizeof(struct ENTRY));

	num_insert=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		insert_table[num_insert].ip=ip;
		insert_table[num_insert].port=nexthop;
		insert_table[num_insert++].len=len;
	}
}

void set_delete(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;

	fp=fopen(file_name,"r");

	/* Count # of table entry */
	num_delete=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_delete++;
	}
	rewind(fp);
	printf("Number of delete table entry = %d\n", num_delete);

	delete_table=(struct ENTRY *)malloc(num_delete*sizeof(struct ENTRY));
	
	num_delete=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		if( len > 16 ){
			delete_table[num_delete].ip=ip;
			delete_table[num_delete].port=nexthop;
			delete_table[num_delete++].len=len;
		}
	}
}

/* Shuffle entry content with time random*/
void shuffle(unsigned int *array, int n){   
    srand((unsigned)time(NULL));
    for (int i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        SWAP(array[i],array[j]);
    }
}

/* Function about binary trie */
/* Create binary trie node */
btrie create_node(){
	btrie temp;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;//default port=256
	temp->layer = 0;
	temp->ip = 0;
	temp->len = 0;
	temp->layer0_count = 0;
	temp->parent = NULL;
	temp->level = 0;
	temp->toplayer = 0;
	return temp;
}

/* Insert a node to binary trie */
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node();
			ptr->right->parent = ptr;
			ptr=ptr->right;
			if((i==len-1)&&(ptr->port==256)){
				ptr->port = nexthop;
				ptr->ip = ip;
				ptr->len = len;
			}
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr->left->parent = ptr;
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256)){
				ptr->port = nexthop;
				ptr->ip = ip;
				ptr->len = len;
			}
		}
	}
}

/* Build binary trie */
void create(){
	int i;
	root=create_node();
	for(i=0;i<num_entry;i++)
		add_node(table[i].ip,table[i].len,table[i].port);
}

//////////////////////////////////////////////////////////////////////////////

/* Segment table initialize */
/* What are these function doing? */
void inition_mask(){
	mask_value[0]=0;
	mask_value[32]=-1;
	for(int i=1;i<32;i++)
		mask_value[i] = ~( (1 << (32-i) )-1);
}

/* Assign btrie layer infomation and count layer nodes by recursive method */
int Set_Trie_Layer(btrie current){
	if(current == NULL)
		return 0;
	
	int leftlayer = Set_Trie_Layer(current->left);
	int rightlayer = Set_Trie_Layer(current->right);
	
	//Check whether this mode need to record
	if(current->port != 256){
		if(leftlayer > rightlayer){
			current->layer = leftlayer + 1;		
			layerArray[current->layer]++;
			return leftlayer + 1;
		}
		else{
			current->layer = rightlayer + 1;
			layerArray[current->layer]++;
			return rightlayer + 1;
		}
	}
	else
		return (leftlayer>rightlayer)?leftlayer:rightlayer;
}

/* Count biggest of node in same layer by pre-order */
/*
void cal_node( btrie current ){
	// Just return if this node is NULL or this node is leaf
	if(current == NULL) return;
	if(current->right == NULL && current->left == NULL) return;
	
	if(current->port != 256)
		num_layer_node++;
	cal_node(current->left);
	cal_node(current->right);
}
*/

/* Count toplayer's node by pre-order*/ 
void Set_Toplayer(btrie current, int toplayer){
	if(current == NULL)
		return;

	// This node is layer-0(leaf) node
	if(current->right == NULL && current->left == NULL){
		current->toplayer = 0;
		toplayer_array[0]++;
		return;
	}

	// This node need to record but not layer-0 node, it is in other toplayer
	if(current->port != 256){
		// Store nodes those in top layer
		toplayer_table[ltable_c].ip = current->ip;
		toplayer_table[ltable_c++].len = current->len;
		/*
			num_layer_node = 0;
			cal_node(current);
		*/
		current->toplayer = toplayer;
		toplayer_array[toplayer]++;
		toplayer++;
	}

	Set_Toplayer(current->left, toplayer);
	Set_Toplayer(current->right, toplayer);
}

//////////////////////////////////////////////////////////////////////////////

/* Find all toplayer-1 node input to upper_btree_root triversal full trie */
void push_toplayer1_prefix(btrie current){
	// Just return if this node is NULL or this node is leaf
	if(current == NULL)
		return;
	if(current->right == NULL  && current->left == NULL)
		return;

	if(current->port != 256){
		if(current->toplayer == 1){
			Upper_Btree_root[root_num].ip = current->ip;
			Upper_Btree_root[root_num].trie_node = current;
			Upper_Btree_root[root_num++].len = current->len;
		}
	}
	push_toplayer1_prefix(current->left);
	push_toplayer1_prefix(current->right);
}

/* Find out all nextlevel nodes store in Btree_current */
void build_btree_layerarray(btrie current, Top_struct * Btree_current){
	if(current == NULL)
		return;
	if(current->right == NULL  && current->left == NULL){
		return;
	}
	// Find out all toplayer node under this binary trie node
	if(current->port != 256){
		Btree_current[count_n].ip = current->ip;
		Btree_current[count_n].trie_node = current;
		Btree_current[count_n++].len = current->len;
		return;// Just find next level toplayer node
	}

	build_btree_layerarray(current->left, Btree_current);
	build_btree_layerarray(current->right, Btree_current);
}

void upper_splitNode(unsigned int ip, unsigned int *pval,
         unsigned int len, unsigned int *pval2,  int pos, struct Upper_BTreeNode *node,struct Upper_BTreeNode *child, struct Upper_BTreeNode **newNode) 
{
  int median, j;
  if (pos > MIN)
    median = MIN + 1;
  else
    median = MIN;

  *newNode = (struct Upper_BTreeNode *)malloc(sizeof(struct Upper_BTreeNode));
  (*newNode)->item = (IP_Inf*)malloc( (MAX+1)*sizeof(IP_Inf) );
  for( int i = 0; i < MAX + 1; i++ ){
    (*newNode)->nextlevel_root[i] = NULL;
  }
  j = median + 1;
  while (j <= MAX) {
    (*newNode)->item[j-median].ip = node->item[j].ip;
    (*newNode)->item[j-median].len = node->item[j].len;
    (*newNode)->nextlevel_root[j-median] = node->nextlevel_root[j];
    (*newNode)->link[j-median] = node->link[j];
    j++;
  }
  node->count = median;
  (*newNode)->count = MAX - median;

  if (pos <= MIN) {
    addValToNode(ip, len, pos, node, child, 1);
  } else {
    addValToNode(ip, len,  pos-median, *newNode, child, 1);
  }
  *pval = node->item[node->count].ip;
  *pval2 = node->item[node->count].len;
  (*newNode)->link[0] = node->link[node->count];
  node->count--;
}

void addValToNode(unsigned int ip,unsigned int len , int pos, void *node_p, void *child_p, int indicate) {

	if(indicate){
		struct Upper_BTreeNode *node = (struct Upper_BTreeNode *) node_p;
		struct Upper_BTreeNode *child = (struct Upper_BTreeNode *) child_p;
		int j = node->count;
		while (j > pos) {
			
			node->item[j+1].ip = node->item[j].ip;
			node->item[j+1].len = node->item[j].len;
			node->nextlevel_root[j+1] = node->nextlevel_root[j];
			node->link[j+1] = node->link[j];
			j--;
		}
		node->item[j+1].ip = ip;
		node->item[j+1].len = len;
		node->nextlevel_root[j+1] = NULL;
		node->link[j+1] = child;
		node->count++;
	}
	else{
		struct Layer0_BTreeNode *node = (struct Layer0_BTreeNode *) node_p;;
		struct Layer0_BTreeNode *child = (struct Layer0_BTreeNode *) child_p;
		int j = node->count;
		while (j > pos) {
			node->item[j+1].ip = node->item[j].ip;
			node->item[j+1].len = node->item[j].len;
			node->link[j+1] = node->link[j];
			j--;
		}
		node->item[j+1].ip = ip;
		node->item[j+1].len = len;
		node->link[j+1] = child;
		node->count++;
	}
}

/* Set infomation to this node */
int upper_setValueInNode(unsigned int ip, unsigned int len, 
  unsigned int *pval, unsigned int *pval2, struct Upper_BTreeNode *node, struct Upper_BTreeNode **child){
  int pos;

  if (!node) {
    *pval=ip;
    *pval2=len;
    *child=NULL;
    return 1;
  }
  
  if (ip < node->item[1].ip)
    pos = 0;
  else{
    for (pos = node->count; (ip < node->item[pos].ip && pos > 1); pos--);
    if (ip == node->item[pos].ip && len >= node->item[pos].len)
      return 0;
  }

  if (upper_setValueInNode(ip, len, pval, pval2, node->link[pos], child)) {
    if (node->count < MAX)
      addValToNode(*pval, *pval2, pos, node, *child, 1);
    else{
      upper_splitNode(*pval, pval, *pval2, pval2, pos, node, *child, child);
      return 1;
    }
  }
  return 0;
}

/* Create a upper layer btree node */
struct Upper_BTreeNode *upper_createNode( unsigned int ip, unsigned int len, struct Upper_BTreeNode *child ) {
  struct Upper_BTreeNode *newNode;
  newNode = (struct Upper_BTreeNode *)malloc(sizeof(struct Upper_BTreeNode));
  newNode->item = (IP_Inf*)malloc( (MAX+1)* sizeof(IP_Inf) );
  newNode->item[1].ip = ip;
  newNode->item[1].len = len;
  newNode->count = 1;
  newNode->link[0] = top_root;
  newNode->link[1] = child;
  for( int i = 0; i < MAX + 1; i++ )
    newNode->nextlevel_root[i] = NULL;
 
  return newNode;
}


void insertion( unsigned int ip, unsigned int len) {
  int flag;
  unsigned long long int i, i2;
  struct Upper_BTreeNode *child;

  /* If flag = 1, mean need create this btree node*/
  flag = upper_setValueInNode(ip, len, &i, &i2, top_root, &child);
  if(flag)
    top_root = upper_createNode(i, i2, child);
}

void Searching(struct Upper_BTreeNode *myNode, unsigned int ip ,int len, int idxl,int idxr, struct Upper_BTreeNode *nextlevel_root){
	int mid = 0;
	int bndl = 0;

	if( myNode == NULL )
		return;
	while( idxl <= idxr ){
		mid = (idxl + idxr)/2;
		if( ip == myNode->item[mid].ip && len == myNode->item[mid].len ){
			myNode->nextlevel_root[mid] = nextlevel_root;
			return;
		}
		if( ip < myNode->item[mid].ip ){
			idxr = mid-1;
			bndl = mid-1;
		}
		else{
			idxl = mid + 1;
			bndl = mid;
		}
		if(idxl > idxr && myNode->link[bndl]!=NULL){
			myNode = myNode->link[bndl];
			mid=0;
			bndl=0;
			idxl=1;
			idxr=myNode->count;
		}
	}
}


/* Build all top layer structure by recusive method */
void traversal_build_tree(int level, struct Upper_BTreeNode * current_root, Top_struct * current, int num) {
  int i, j;
  int top_count= current_root->count;

  if (current) {
    for (i = 0; i < num; i++) {
    	if(current[i].nextlevel != NULL){	
    		top_root = NULL;
    		for(j = 0; j < current[i].num; j++)
    			insertion(current[i].nextlevel[j].ip, current[i].nextlevel[j].len);
    		Searching(current_root, current[i].ip , current[i].len , 1, top_count, top_root);
    		current[i].nextlevel_root =  top_root;
    		traversal_build_tree(level+1, current[i].nextlevel_root, current[i].nextlevel, current[i].num);
    	}
    }
  }
}

/* Find out the toplayer structure and construct toplayer btree structure */
void build(){
	int i, j, k;
	int max_index = 0;
	int q_size = 0; // Size of build btree toplayer array queue
	int n_num;// Record how many elements in this btree layer array 
	count_n = 0;

	// Toplayer_array[1] is hierarchical structrue root
	// pre allocate size = 1000
	// Upper_Btree_root = (Top_struct*)malloc((toplayer_array[1] + 1000)*sizeof(Top_struct));
	Upper_Btree_root = (Top_struct*)malloc((toplayer_array[1])*sizeof(Top_struct));
	push_toplayer1_prefix(root);
	printf("btrie_root_num(toplayer_array[1]):%d\n", root_num);

	// Initialize upper_btree_root array
	for(i = 0; i < root_num; i++){
		Upper_Btree_root[i].num = 0;
		Upper_Btree_root[i].nextlevel = NULL;
	}
	
	// Another allocated space to temporary store all toplayer nodes
	// size = 10000 mean that pre allocated space size 10000
	Top_struct ** build_queue = (Top_struct **) malloc((num_entry + 10000)*sizeof(Top_struct*));
	
	// Build first level toplayer
	for(j = 0; j < toplayer_array[1]; j++){
		count_n = 0;
		// Nextlevel point to next level hierarchical btrie node 
		// 3000 mean pre allocated space size 3000
		Upper_Btree_root[j].nextlevel = (Top_struct*)malloc(3000*sizeof(Top_struct));
		// Initialize next level btrie node's array
		for(k = 0; k < 3000; k++){
			Upper_Btree_root[j].nextlevel[k].num = 0;
			Upper_Btree_root[j].nextlevel[k].nextlevel = NULL;
		}

		/* There is a trick method let sub-function will not stop at first round,\
		/* because first round trie_node.port maybe is 256.\
		/* Next three row code is do this trick */
		Upper_Btree_root[j].trie_node->port = 256;
		build_btree_layerarray(Upper_Btree_root[j].trie_node, Upper_Btree_root[j].nextlevel);
		Upper_Btree_root[j].trie_node->port = 1;
		Upper_Btree_root[j].num = count_n;// Number of next level node

		// Check whether have nextlevel
		if(count_n != 0){
			build_queue[q_size++] = Upper_Btree_root[j].nextlevel;
			Upper_Btree_root[j].nextlevel[0].num = count_n;
		}
		else{
			free(Upper_Btree_root[j].nextlevel);
			Upper_Btree_root[j].nextlevel = NULL;
		}
	}
	
	// Build other level toplayer
	for(i = 0; i < q_size; i++){
		n_num = build_queue[i][0].num;
		
		Top_struct * temp = build_queue[i];

		for(j = 0; j < n_num; j++){
			count_n = 0;
			// size = 120 mean that pre allocated space to nextlevel layer array
			temp[j].nextlevel = (Top_struct*)malloc(120*sizeof(Top_struct));
			// Initialize next layer array
			for(k = 0; k < 120; k++){
				temp[j].nextlevel[k].num = 0;
				temp[j].nextlevel[k].nextlevel = NULL;
			}
			// Trick method like layer-1 build above
			temp[j].trie_node->port = 256;
			build_btree_layerarray(temp[j].trie_node, temp[j].nextlevel);
			temp[j].trie_node->port = 1;
			temp[j].num = count_n;// Nextlevel layer array size

			// Check whether have nextlevel
			if(count_n != 0){
				build_queue[q_size++] = temp[j].nextlevel;
				temp[j].nextlevel[0].num = count_n;
			}
			else{
				free(temp[j].nextlevel);
				temp[j].nextlevel = NULL;
			}
		}
	}

	for(i = 0; i < top_layer_num; i++)
		insertion(Upper_Btree_root[i].ip, Upper_Btree_root[i].len);

	allBtree_root =  top_root;
	traversal_build_tree( 1, allBtree_root, Upper_Btree_root, top_layer_num);
}

void inition(){
	Set_Trie_Layer(root);
	toplayer_table = (IP_Inf* )malloc(num_entry*sizeof(IP_Inf));
	Set_Toplayer(root, 1);
	top_layer_num = toplayer_array[1];
	build();
	layer0_table = (IP_Inf* )malloc(layerArray[1]*sizeof(IP_Inf));
}


//////////////////////////////////////////////////////////////////////////////

struct Layer0_BTreeNode *createNode(struct Layer0_BTreeNode *B_root,unsigned int item,int len, struct Layer0_BTreeNode *child) {
  struct Layer0_BTreeNode *newNode;
  newNode = (struct Layer0_BTreeNode *)malloc(sizeof(struct Layer0_BTreeNode));
  newNode->item = (IP_Inf*)malloc( (MAX+1)* sizeof(IP_Inf) );
  newNode->item[1].ip = item;
  newNode->item[1].len = len;
  newNode->count = 1;
  newNode->link[0] = B_root;
  newNode->link[1] = child;
  return newNode;
}
int cover(unsigned int ip1, unsigned int len, unsigned int ip2, unsigned int len2) {
	
	if (len < len2) 
		return ((ip1 & mask_value[len]) ^ (ip2 & mask_value[len])) ? 0 : 1;
	else return 0;
}
int is_match( unsigned int ip,  unsigned int ip_rule, unsigned int len){
	return (( ip_rule & mask_value[len]) ^ (ip & mask_value[len])) ? 0 : 1;
}
void splitNode(unsigned int item, unsigned int *pval,unsigned int len, unsigned int *pval2, int pos, struct Layer0_BTreeNode *node,struct Layer0_BTreeNode *child, struct Layer0_BTreeNode **newNode) {
  int median, j;
  if (pos > MIN)
    median = MIN + 1;
  else
    median = MIN;

  *newNode = (struct Layer0_BTreeNode *)malloc(sizeof(struct Layer0_BTreeNode));
  (*newNode)->item = (IP_Inf*)malloc( (MAX+1)* sizeof(IP_Inf) );
  j = median + 1;
  while (j <= MAX) {
    (*newNode)->item[j-median].ip = node->item[j].ip;
    (*newNode)->item[j-median].len = node->item[j].len;
    (*newNode)->link[j-median] = node->link[j];
    j++;
  }
  node->count = median;
  (*newNode)->count = MAX - median;

  if (pos <= MIN) {
    addValToNode(item, len, pos, node, child, 0);
  } else {
    addValToNode(item, len, pos-median, *newNode, child, 0);
  }
  *pval = node->item[node->count].ip;
  *pval2 = node->item[node->count].len;
  (*newNode)->link[0] = node->link[node->count];
  node->count--;
}
int setNodeValue(unsigned int item,unsigned int len, unsigned int *pval,unsigned int *pval2,struct Layer0_BTreeNode *node, struct Layer0_BTreeNode **child,int flag) {
  int pos;
  if (!node) {
    *pval=item;
    *pval2=len;
    *child=NULL;
    return 1;
  }
  if(flag==0){
  	if (item < node->item[1].ip) {
	    pos = 0;
	} 
	else {
		for (pos = node->count;(item < node->item[pos].ip && pos > 1); pos--);
		if (item == node->item[pos].ip) {
		  Insert2Up++;
		  return 0;
		}
	}
  }
  else{ 
  	if(item <= node->item[1].ip){
  		if(item < node->item[1].ip)
	  		if(cover(item,len,node->item[1].ip,node->item[1].len)){
	  			Insert2Up++;
	  			Cover_layer0++;
	  			return 0;
	  		}
	  	if(item == node->item[1].ip){
	  		if( len >= node->item[1].len){
				Insert2Up++;
				Replcae_Layer0++;
				replace_flag=1;
				insert_Btree( 1, node->item[1].ip , node->item[1].len, 1, allBtree_root->count, allBtree_root, NULL, 0);
				node->item[1].ip = item;
				node->item[1].len = len;
				return 2;
			}
			else{
				Cover_layer0++;
				Insert2Up++;
				return 0;
			}
	  	}
  		pos = 0;
  	}
  	else if(item >= node->item[node->count].ip){
  		if(item > node->item[node->count].ip)
	  		if(cover(node->item[node->count].ip,node->item[node->count].len,item,len)){
	  			Insert2Up++;
				Replcae_Layer0++;
				replace_flag=1;
				insert_Btree( 1, node->item[node->count].ip, node->item[node->count].len, 1, allBtree_root->count, allBtree_root, NULL, 0);
				node->item[node->count].ip = item;
				node->item[node->count].len = len;
				return 2;
	  		}

	  	if(item == node->item[node->count].ip){
	  		if( len >= node->item[node->count].len){
				Insert2Up++;
				Replcae_Layer0++;
				replace_flag=1;
				insert_Btree( 1, node->item[node->count].ip ,node->item[node->count].len, 1, allBtree_root->count, allBtree_root, NULL, 0);
				node->item[node->count].ip = item;
				node->item[node->count].len = len;
				return 2;
			}
			else{
				Cover_layer0++;
				Insert2Up++;
				return 0;
			}
	  	}
		pos = node->count;
  	}
  	else{
  		int cover_case;
  		int mid=0;
		int idxl=1;
		int idxr=node->count;
		while( idxl <= idxr ){
			mid = (idxl + idxr)/2;
			if(item == node->item[mid].ip){
				if( len >= node->item[mid].len){
					Insert2Up++;
					Replcae_Layer0++;
					replace_flag=1;
					insert_Btree( 1, node->item[mid].ip,  node->item[mid].len, 1, allBtree_root->count, allBtree_root, NULL, 0);
					node->item[mid].ip = item;
					node->item[mid].len = len;
					return 2;
				}
				else{
					Cover_layer0++;
					Insert2Up++;
					return 0;
				}
			}
			else if( item < node->item[mid].ip ){
				cover_case=0;
				idxr = mid-1;
				pos = mid-1;
			}
			else{
				cover_case=1;
				idxl = mid+1;
				pos = mid;
			}
			if(idxl > idxr){
				if(cover_case==0){
					if( cover(item,len,node->item[mid].ip,node->item[mid].len) ){
						Insert2Up++;
						Cover_layer0++;
						return 0;
					}
					else if(cover(node->item[mid-1].ip,node->item[mid-1].len,item,len)){
						Insert2Up++;
						Replcae_Layer0++;
						replace_flag=1;
						insert_Btree( 1, node->item[mid-1].ip, node->item[mid-1].len, 1, allBtree_root->count, allBtree_root, NULL, 0);
						node->item[mid-1].ip = item;
						node->item[mid-1].len = len;
						return 2;
					}
				}
				else{
					if(cover(item,len,node->item[mid+1].ip,node->item[mid+1].len)){
						Insert2Up++;
						Cover_layer0++;
						return 0;
					}
					else if( cover(node->item[mid].ip,node->item[mid].len,item,len) ){
						Insert2Up++;
						Replcae_Layer0++;
						replace_flag=1;
						insert_Btree( 1, node->item[mid].ip,  node->item[mid].len, 1, allBtree_root->count, allBtree_root, NULL, 0);
						node->item[mid].ip = item;
						node->item[mid].len = len;
						return 2;
					}
				}

			}
		}
  	}
  	
  }
  
  if (setNodeValue(item, len, pval, pval2, node->link[pos], child, flag)==1 ) {
    if (node->count < MAX) {
	    insertValue_flag=1;
	    addValToNode(*pval,*pval2, pos, node, *child, 0);
	    return 2;
    } else {
      splitNode_flag=1;
      splitNode(*pval, pval, *pval2, pval2, pos, node, *child, child);
      return 1;
    }
  }

  if(flag==0)
  	return 2;
  if(flag==1){
  	if(insertValue_flag==1 || splitNode_flag==1 || replace_flag==1){
  		return 2;
  	}
  	else
  		return 0;
  }
}
void btree_insertion(struct Layer0_BTreeNode **B_root ,unsigned int item,unsigned int len,int insert_type) {
  int flag;
  replace_flag=0;
  insertValue_flag=0;
  splitNode_flag=0;
  unsigned int i,i2;
  struct Layer0_BTreeNode *child;
  flag = setNodeValue(item,len,&i,&i2,*B_root,&child,insert_type);
  if (flag==1){
  	*B_root = createNode(*B_root, i, i2, child);
  }
  else if(flag==0){
  	insert_Btree( 1, item, len, 1, allBtree_root->count, allBtree_root, NULL, 0);
  }
}
int BtreeSearch(struct Layer0_BTreeNode *myNode, unsigned int ip,int idxl,int idxr){
	int mid = 0;
	int bndl = 0;
	while( idxl <= idxr ){
		mem_access++;
		mid = (idxl + idxr)/2;
		if( is_match( ip, myNode->item[mid].ip, myNode->item[mid].len) ){ //is_match( ip, layer0_table[mid].ip, layer0_table[mid].len) ){ //layer0_table[mid].up_bound >= ip && layer0_table[mid].ip <= ip
			match_rule++;
			layer0_found++;
			return 1;
		}
		if( ip < myNode->item[mid].ip ){
			idxr = mid-1;
			bndl = mid-1;
		}
		else{
			idxl = mid + 1;
			bndl = mid;
		}
		if(idxl > idxr && myNode->link[bndl]!=NULL){
			mem_access++;
			myNode = myNode->link[bndl];
			mid=0;
			bndl=0;
			idxl=1;
			idxr=myNode->count;
		}
	}
	return 0;
}
void Seg_size(btrie current){
	if (current == NULL)
		return;
	Seg_size(current->left);
	Seg_size(current->right);
	if ( current->port != 256 ) {
		if (current-> layer == 1 ) {
			unsigned int temp_min = 0;
			temp_min = current->ip >> 16;
			num[temp_min]++;
			layer0_table[layer0_count].ip = current->ip;
			layer0_table[layer0_count++].len = current->len;
		}
	}
}
void Build_Seg(){
	int max_segment_size = num[0];
	int max_index;
	for(int i=1;i<65536;i++){
		if(num[i] > max_segment_size){
			max_segment_size = num[i];
			max_index=i;
		}
	}
	printf("Max_seg_size =%d, index = %d\n",max_segment_size,max_index);
	segement_table= (tentry*)malloc( 65536* sizeof(tentry) );
	int i;
	for( i = 0; i < 65536; i++ ){
		segement_table[i].num = 0;
		segement_table[i].B_root = NULL;
		segement_table[i].list_table = (struct ENTRY *)malloc( (max_segment_size+1) * sizeof(struct ENTRY));
	}
	for(int i = 0; i < layer0_count; i++ ){
		if( layer0_table[i].len > 16 ){ 
			unsigned int ip = layer0_table[i].ip>>16;
			segement_table[ip].list_table[segement_table[ip].num].len = layer0_table[i].len;
			segement_table[ip].list_table[segement_table[ip].num].ip = layer0_table[i].ip;
			segement_table[ip].num++;
		}

	}
	int non_empty_entry = 0;
	int empty_entry = 0;
	for(int i = 0; i < 65536; i++ ){
		if ( segement_table[i].num == 0 )
			empty_entry++;
		else{
			non_empty_entry++;
			build_segment_create(i);
		}
	}
	printf("empty_entry =%d , non_empty_entry =%d\n",empty_entry,non_empty_entry);
}
void build_segment_create( int pos ){
	for(int i=0;i<segement_table[pos].num;i++){
		btree_insertion( &segement_table[pos].B_root, segement_table[pos].list_table[i].ip, segement_table[pos].list_table[i].len, 0);
	}
}
void search_Btree(int level, struct Upper_BTreeNode *myNode, unsigned int ip ,int idxl,int idxr){
  int mid = 0;
  int bndl = 0;
  if( myNode == NULL )
    return;
  struct Upper_BTreeNode *current_root = myNode;
  while( idxl <= idxr ){
    mid = (idxl + idxr)/2;
    mem_access++;
    if( is_match( ip,  myNode->item[mid].ip,  myNode->item[mid].len ) ){
      mem_access++;
      match_flag = 1;
      if( myNode->nextlevel_root[mid] != NULL ){
        search_Btree(level+1, myNode->nextlevel_root[mid], ip, 1, myNode->nextlevel_root[mid]->count);
        return;
      }
      else
        return;
    }
    if( ip < myNode->item[mid].ip ){
      idxr = mid-1;
      bndl = mid-1;
    }
    else{
      idxl = mid + 1;
      bndl = mid;
    }
    if(idxl > idxr){
    	mem_access++;
		if( myNode->link[bndl]!=NULL ){
			myNode = myNode->link[bndl];
			mid=0;
			bndl=0;
			idxl=1;
			idxr=myNode->count;
		}
		else{
			for( int i = 0; i < current_root->eset_num; i++ ){
				if( is_match( ip,  current_root->eset[i].ip,  current_root->eset[i].len ) ){ 
					match_flag = 1;
					if(current_root->eset[i].len == current_root->max_len)
						break;
				}
			}
		}
    }
  }
}

void insert_Btree( int level, unsigned int ip, int len, int idxl, int idxr, struct Upper_BTreeNode *current, 
				struct Upper_BTreeNode * pre_node, int pre_index){
	int mid = 0;
	int bndl = 0;
	struct Upper_BTreeNode *current_root = current;
	int n_num;
	if( current == NULL )
		return;
	while( idxl <= idxr ){
		mid = (idxl + idxr)/2;
		if( ip < current->item[mid].ip ){
			idxr = mid-1;
			bndl = mid-1;
		}
		else{
			idxl = mid + 1;
			bndl = mid;
		}
		if(idxl > idxr ){
			n_num = current->count;
			if( bndl != 0 && cover(current->item[bndl].ip, current->item[bndl].len, ip, len) ){
				if( current->nextlevel_root[bndl] != NULL ){
					insert_Btree( level+1, ip, len, 1, current->nextlevel_root[bndl]->count, current->nextlevel_root[bndl], current, bndl);
					return;
				}
				else{ 
					case2++;
					top_root = NULL; 
					insertion(ip, len);
					current->nextlevel_root[bndl] = top_root;
					return;
				}
			}
			else {
				if( bndl+1 <= n_num && cover( ip, len, current->item[bndl+1].ip, current->item[bndl+1].len)){
					if( current_root->eset_num == 0 ){
		 				current_root->eset = (IP_Inf*)malloc( 1600*sizeof(IP_Inf) );
		 				current_root->max_len = 0;
		 			}
		 			current_root->eset[current_root->eset_num].ip = ip;
		 			current_root->eset[current_root->eset_num].len = len;
		 			if(len > current_root->max_len ){
		 				current_root->max_len = len;
		 			}
		 			current_root->eset_num++;
		        	case3++;         
					return;
				}
			}
			if( current->link[bndl] != NULL ){
				current = current->link[bndl];
				mid=0;
				bndl=0;
				idxl=1;
				idxr=current->count;

			}
			else{
				case1++;
				top_root = current_root;
				insertion( ip, len);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

void lookup_performance(){
	int upper_match=0, max_access=0;
	no_match=0;
	for(int j=0; j<100; j++){
		for( int i=0;i<num_query;i++){
			unsigned int ip_temp = query[i]>>16;
			int temp_access = mem_access;
			begin=rdtsc();
			mem_access++;   // search segment-table
			if(segement_table[ip_temp].B_root==NULL){
				if( segment_16[ip_temp].port != 256 ){
					match_rule++;
					segment_16_found++;
				}
				else no_match++;
			}
			else{
				int search_flag = BtreeSearch( segement_table[ip_temp].B_root, query[i], 1, segement_table[ip_temp].B_root->count);
				if(!search_flag){
					match_flag = 0;
					search_Btree(1, allBtree_root, query[i],  1, allBtree_root->count);
					if( match_flag ){
						upper_match++;
						match_rule++;
					}
					else{
						if( segment_16[ip_temp].port != 256 ){
							match_rule++;
							segment_16_found++;
						}
						else no_match++;
					}
				}
			}
			if(mem_access-temp_access > max_access) max_access = mem_access - temp_access;
			end=rdtsc();
			if(clock1[i]>(end-begin)) clock1[i]=(end-begin);
		}
	}

	// printf("match_rule = %d , no_match=%d , upper_match = %d ,layer0_found = %d ,segment_16_found = %d \n",match_rule/100,no_match/100,upper_match/100,layer0_found/100,segment_16_found/100);
	total=0;
	for(int j=0;j<num_query;j++)
		total+=clock1[j];
	printf("---------------------------------Avg. Search: %llu---------------------------------\n",total/num_query);
	printf("max_access=%d ,mem_access =%d, Avg mem_access = %f\n",max_access, mem_access,(float)mem_access/(float)num_query );
}
void insert_performance(){
	for(int j=0; j<num_insert; j++) clock1[j]=10000000;
	for(int i=0; i<num_insert; i++){
		unsigned int ip_temp = insert_table[i].ip>>16;
		unsigned int ip_end = ip_temp + (1<<(16-insert_table[i].len))-1;
		begin=rdtsc();
		if(insert_table[i].len>16)
			btree_insertion( &segement_table[ip_temp].B_root ,insert_table[i].ip, insert_table[i].len, 1);  // segment
		else if(insert_table[i].len==16){
			segment_16[ip_temp].port=1;
			segment_16[ip_temp].len=16;
			segment_16[ip_temp].len_list |= (1<<(32-insert_table[i].len) );
		}
		else{
			for( int j = ip_temp; j <= ip_end; j++ ){
				segment_16[j].port=1;
				segment_16[j].len_list |= (1<<(32-insert_table[i].len) );
				if(insert_table[i].len >= segment_16[j].len)
					segment_16[j].len = insert_table[i].len;
			}
		}
		end=rdtsc();
		if(clock1[i]>(end-begin)) clock1[i]=(end-begin);
	}
	total=0;
	for(int j=0;j<num_insert;j++) total += clock1[j];
	printf("---------------------------------Avg. insert: %llu ---------------------------------\n",total/num_insert);
}

//////////////////////////////////////////////////////////////////////////////

/* Inner for */
void search_btree_level(int tree_level, int layer_level, struct Upper_BTreeNode * btree_root){
	if(!btree_root) return;

	if(btree_root->count > 10000)
		key_dis[10000]++;
	else
		key_dis[btree_root->count]++;

	level_node_num[layer_level][tree_level]+=btree_root->count;
	for(int i=0;i<MAX;i++){
		search_btree_level(tree_level+1, layer_level, btree_root->link[i]);
	}
}

/* Just test */
/* Outer for */
void search_toplayer_level(int layer_level, Top_struct * current, int num) {
  int i, j;

  if (current) {
    for (i = 0; i < num; i++) {
    	if(current[i].nextlevel != NULL){
    		search_btree_level(1, layer_level, current[i].nextlevel_root);
    		search_toplayer_level(layer_level+1, current[i].nextlevel, current[i].num);
    	}
    }
  }
}


/* Entring point */
int main(int argc,char *argv[]){
	set_table(argv[1]);
	set_query(argv[2]);
	set_insert(argv[3]); 
	//set_delete(argv[3]);
	shuffle(query, num_query);

	// Data structure initialize  
	inition_mask();
	create(); //Create trie structure
	inition();
	
	// Build Segmentation table
	Seg_size(root); //segment
	Build_Seg(); //segment

	//lookup_performance();
	/*insert_performance();

	printf("case1(Disjoint all): %d\n",case1);
	printf("case2(Build the new tree): %d\n",case2);
	printf("case3(Cover prefix): %d\n", case3);
	printf("Insert2Up = %d , Replcae_Layer0 = %d,Cover_layer0 =%d \n",Insert2Up, Replcae_Layer0, Cover_layer0);
	*/

	search_toplayer_level(1, Upper_Btree_root, top_layer_num);


	for(int i=0;i<10;i++){
		for(int j=0;j<10;j++){
			if(level_node_num[i][j]!=0){
				printf("layer-%d level node num %d:%d\n",i,j,level_node_num[i][j]);
			}
		}
	}

	for(int i=0;i<10000;i++){
		if(key_dis[i]!=0)
			printf("key value[%d] = %d\n",i,key_dis[i]);
	}

}


/*
void leafpushing(btrie current){
	if(current ==NULL)
		return;
	if(current->left==NULL && current->right==NULL)
		return;
	int port_temp = current->port;
	if(current->port != 256 ){
		int is_push = 0;
		if( current->left !=NULL &&  current->right !=NULL){
			if( current->left->port != 256 && current->right ->port != 256){
				current->port = 256;
			} 
			else if( current->left->port != 256 ) {
				current->right->port = current->port;
				current->right->ip = (((current->ip >> (32-current->len))<<1) + 1 )<< (32-current->len-1);
				current->right->len = current->len+1;
				is_push = 1;
			}
			else if( current->right->port != 256 ) {
				current->left->port = current->port;
				current->left->ip = (current->ip >> (32-current->len))<<(32-current->len);
				current->left->len = current->len+1;
				is_push = 1;
			}
		}
		else if(current->left ==NULL && current->right->port!=256 ){
			current->left = create_node();
			current->left ->port = port_temp;
			current->left -> parent = current;
			current->left->ip = (current->ip >> (32-current->len))<<(32-current->len);
			current->left->len = current->len+1;
			is_push = 1;
			
		}
		else if(current->right ==NULL && current->left->port!=256 ){
			current->right = create_node();
			current->right ->port = port_temp;
			current->right -> parent = current;
			current->right->ip = (((current->ip >> (32-current->len))<<1) + 1 )<< (32-current->len-1);
			current->right->len = current->len+1;
			is_push = 1;
		}
		if( is_push ){
			current->ip = 0;
			current->len = 0;
			current->port = 256;
		}
	}
	leafpushing(current->left);
	leafpushing(current->right);
}
*/