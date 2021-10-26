#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int ip;
	unsigned char len;
	unsigned int port;
};
////////////////////////////////////////////////////////////////////////////////////
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
////////////////////////////////////////////////////////////////////////////////////
struct list{
	struct ENTRY node_info;
	struct list *left,*right;
};
struct record {
	unsigned int prefix;
	unsigned char len;
	unsigned int port;
};
//node是一種list的擴充型態
typedef struct list node;
typedef struct record record_node;
//btrie的type為 node
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;//用於指向整棵樹的root
//query和table做相同的事情，唯一不同的是query會被suffle
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
record_node *searcharray;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int arraySize = 0;
struct ENTRY *last_node;
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
int num_node=0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->node_info.port = 256;//default port, 256 meaning no name
	temp->node_info.ip = 0;
	temp->node_info.len = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(struct ENTRY newnode){
	btrie ptr=root;
	int i;
	for(i=0;i<newnode.len;i++){
        //mask後該bit == 1，從最高位元開始compare
		if(newnode.ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); // Create Node
			ptr=ptr->right;
			if((i==newnode.len - 1)&&(ptr->node_info.port== 256))//lonest matching prefix
				ptr->node_info = newnode;
		}
        //mask後該bit == 0
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if ((i == newnode.len -1) && (ptr->node_info.port == 256))
				ptr->node_info = newnode;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";//用於切割的token
	char buf[100],*str1;
	unsigned int n[4];
    //將前面四個ip位址切出來
	sprintf(buf,"%s\0",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[3]=atoi(buf);
    //*******why nexthop = n[2]?**********
	*nexthop=n[2];
	str1=(char *)strtok(NULL,tok);
    //輸入prefix length的長度
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
    //輸入ip value
	*ip=n[0];
	*ip<<=8;
	*ip+=n[1];
	*ip<<=8;
	*ip+=n[2];
	*ip<<=8;
	*ip+=n[3];
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip){
	int left = 0, right = arraySize, index = (left + right) / 2;
	unsigned int prefix, left_prefix, right_prefix;
	while (right - left > 1) 
	{
		//hit on edge node
		if (ip == searcharray[index].prefix)
		{
			//printf("hit edge ip;%x, len;%d, index:%d, port:%d\n", ip, searcharray[index].len, index, searcharray[index].port);
			break;
		}
		else if (ip > searcharray[index].prefix)
			left = index;
		else
			right = index;
		index = (left + right) / 2;
	}
	//just remain left and right, first check longer side then another
	if (searcharray[left].len >= searcharray[right].len) 
	{
		//check if this node is contain in left node prefix
		prefix = ip >> (32 - searcharray[left].len);
		left_prefix = searcharray[left].prefix >> (32 - searcharray[left].len);
		if (prefix == left_prefix)
		{
			//printf("ip;%x, prefix:%x, len;%d, index:%d, port:%d\n", ip, prefix, searcharray[left].len, index, searcharray[left].port);
			return;
		}
		//check if this node is contain in right node prefix
		prefix = ip >> (32 - searcharray[right].len);
		right_prefix = searcharray[right].prefix >> (32 - searcharray[right].len);
		if (prefix == right_prefix)
		{
			//printf("ip;%x, prefix:%x, len:%d, index:%d, port:%d\n", ip, prefix, searcharray[right].len, index, searcharray[right].port);
			return;
		}
	}
	else 
	{
		prefix = ip >> (32 - searcharray[right].len);
		right_prefix = searcharray[right].prefix >> (32 - searcharray[right].len);
		if (prefix == right_prefix)
		{
			//printf("ip;%x, prefix:%x, len:%d, index:%d, port:%d\n", ip, prefix, searcharray[right].len, index, searcharray[right].port);
			return;
		}
		prefix = ip >> (32 - searcharray[left].len);
		left_prefix = searcharray[left].prefix >> (32 - searcharray[left].len);
		if (prefix == left_prefix)
		{
			//printf("ip;%x, prefix:%x, len;%d, index:%d, port:%d\n", ip, prefix, searcharray[left].len, index, searcharray[left].port);
			return;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		//將len, ip, nexthop參數傳址傳入，透過read table function取得值‘
		//每次輸入txt檔內的一行data
		read_table(string, &ip, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	//記憶體配置
	input = (unsigned int *)malloc(num_input * sizeof(struct ENTRY));//用來存放query
	num_input = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
        //將len, ip, nexthop參數傳址傳入，透過read table function取得值‘
        //每次輸入txt檔內的一行data
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
    //記憶體配置
	query=(unsigned int *)malloc(num_query*sizeof(struct ENTRY));//用來存放query
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));//用來存放每輪時間
	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
        query[num_query].ip=ip;
        query[num_query].port = nexthop;
        query[num_query].len = len;
		clock[num_query++]=10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////
//do leaf_push by inorder
void leaf_pushing(btrie r)
{
	//endpoint
	if (r == NULL)
		return;
	btrie ptr = r;
	//this node have port need to push
	if (r->node_info.port != 256) 
	{
		//this node is leave
		if (r->left == NULL && r->right == NULL)
			return;
		//push to new left leave
		if (r->left == NULL)
		{
 			r->left = create_node();
			r->left->node_info.ip = r->node_info.ip & (0xFFFFFFFE << (31 - r->node_info.len));
			r->left->node_info.len = r->node_info.len + 1;
			r->left->node_info.port = r->node_info.port;
		}
		//push to empty left leave
		else if (r->left->node_info.port == 256) 
		{
			r->left->node_info.ip = r->node_info.ip & (0xFFFFFFFE << (31 - r->node_info.len));
			r->left->node_info.len = r->node_info.len + 1;
			r->left->node_info.port = r->node_info.port;
		}
		//else is left have its port number, should not do push
		leaf_pushing(r->left);
		if (r->right == NULL)
		{
			r->right = create_node();
			r->right->node_info.ip = r->node_info.ip | (1 << (31 - r->node_info.len));
			r->right->node_info.len = r->node_info.len + 1;
			r->right->node_info.port = r->node_info.port;
		}
		else if (r->right->node_info.port == 256) 
		{
			r->right->node_info.ip = r->node_info.ip | (1 << (31 - r->node_info.len));
			r->right->node_info.len = r->node_info.len + 1;
			r->right->node_info.port = r->node_info.port;
		}
		leaf_pushing(r->right);
		//push over this node's port should eliminate
		r->node_info.port = 256;
	}
	//this node port number is default, just recursive
	else {
		leaf_pushing(r->left);
		leaf_pushing(r->right);
	}
}
////////////////////////////////////////////////////////////////////////////////////
void do_merge(btrie r) 
{
	struct ENTRY now_node = r->node_info;
	struct ENTRY pre_node = *last_node;
	int i = 0;
	//count how long nonconsistent
	while (now_node.ip != pre_node.ip) {
		now_node.ip = now_node.ip >> 1;
		pre_node.ip = pre_node.ip >> 1;
		i++;
	}
	btrie ptr = root;
	//remain bits is from 31bit to x bit
	for (int j = i; j <32; j++) 
	{
		if (now_node.ip&(1 << (31 - j)))
			ptr = ptr->right;
		else
			ptr = ptr->left;
	}
	//merged node information
	ptr->node_info.ip = r->node_info.ip;
	ptr->node_info.len = 32 - i;
	ptr->node_info.port = r->node_info.port;
	//eliminate two nodes from merged node 
	last_node->ip = 0;
	last_node->len = 0;
	last_node->port = 256;
	r->node_info.ip = 0;
	r->node_info.len = 0;
	r->node_info.port = 256;
	//last_node point to ptr let next to compare
	last_node = ptr;
}
////////////////////////////////////////////////////////////////////////////////////
void inorder_search(btrie r) {
	if (r == NULL)
		return;
	inorder_search(r->left);
	//this node is prefix node
	if (r->node_info.port != 256) 
	{
		//can do merge
		if (r->node_info.port == last_node->port)
			do_merge(r);
		else
			last_node = r;
	}
	inorder_search(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
void rotation(btrie r, int index) {
	btrie ptr = r;
	if (r == NULL)
		return;
	//just on level 31 need to do rotation
	if (index == 31) 
	{
		//level 31 node have value
		if (r->node_info.port != 256)
		{
			//left node have value too
			if (r->left != NULL && r->left->node_info.port != 256)
			{
				if (r->right == NULL)
					r->right = create_node();
				//assign left node_info to right node info
				r->right->node_info.len = 0;
				r->right->node_info.port = r->left->node_info.port;
				r->right->node_info.ip = 0;
				//disable left node
				r->left->node_info.ip = 0;
				r->left->node_info.port = 256;
				r->left->node_info.len = 0;
			}
			if (r->right != NULL && r->right->node_info.port != 256)
				r->right->node_info.ip = 0;
		}
		//level 31 node have no value
		else {
			if (r->right != NULL && r->right->node_info.port != 256)
			{
				r->node_info.ip = r->right->node_info.ip;
				r->node_info.len = r->right->node_info.len - 1;
				r->node_info.port = r->right->node_info.port;
				r->right->node_info.ip = 0;
			}
			if (r->left != NULL && r->left->node_info.port != 256) 
			{
				r->node_info = r->left->node_info;
				r->left->node_info.ip = 0;
				r->left->node_info.len = 0;
				r->left->node_info.port = 0;
				if (r->right == NULL)
					r->right = create_node();
				r->right->node_info.ip = 0;
				r->right->node_info.len = 0;
				r->right->node_info.port = r->node_info.port;
			}
		}
		return;
	}
	rotation(r->left, index + 1);
	rotation(r->right, index + 1);
}
////////////////////////////////////////////////////////////////////////////////////
//count how many node port have value
void count_arraysize(btrie r) 
{
	if (r == NULL)
		return;
	if (r->node_info.port != 256)
		arraySize++;
	count_arraysize(r->left);
	count_arraysize(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
void build_array(btrie r) 
{
	if (r == NULL)
		return;
	build_array(r->left);
	if (r->node_info.port != 256) 
	{
		unsigned int temp_prefix;
		temp_prefix = r->node_info.ip >> (32 - r->node_info.len);
		if (r->node_info.len < 32) 
		{
			//assign dont care = 10000....00
			temp_prefix = (temp_prefix << 1) | 0x00000001;
			temp_prefix = temp_prefix << (31 - r->node_info.len);
		}
		searcharray[arraySize].prefix = temp_prefix;
		searcharray[arraySize].len = r->node_info.len;
		searcharray[arraySize].port = r->node_info.port;
		arraySize++;
	}
	build_array(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	begin=rdtsc();
	//initialize last_node value
	last_node = malloc(sizeof(struct ENTRY));
	last_node->ip = 0;
	last_node->len = 0;
	last_node->port = 256;
	//build binary trie
	root = create_node();
	for(i=0;i<num_entry;i++)
		add_node(table[i]);
	leaf_pushing(root);
	//merge binary trie
	inorder_search(root);
	//adjust bottom level node
	rotation(root, 0);
	//count how many node needed
	count_arraysize(root);
	searcharray = malloc(arraySize * sizeof(record_node));
	arraySize = 0;
	//build search array
	build_array(root);
	 end=rdtsc();
	//for (int i = 0; i < arraySize; i++)
	//printf("i = %d, prefix:%x, len:%d, port:%d\n", i, searcharray[i].prefix,searcharray[i].len, searcharray[i].port);
}
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int* )malloc(50 * sizeof(unsigned int ));
	for(i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for(i = 0; i < num_query; i++)
	{
		if(clock[i] > MaxClock) MaxClock = clock[i];
		if(clock[i] < MinClock) MinClock = clock[i];
		if(clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);
	
	for(i = 0; i < 50; i++)
	{
		printf("%d\n",NumCntClock[i]);
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////////
void shuffle(struct ENTRY *array, int n) {
    srand((unsigned)time(NULL));
    struct ENTRY *temp=(struct ENTRY *)malloc(sizeof(struct ENTRY));
    
    for (int i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        temp->ip=array[j].ip;
        temp->len=array[j].len;
        temp->port=array[j].port;
        array[j].ip = array[i].ip;
        array[j].len = array[i].len;
        array[j].port = array[i].port;
        array[i].ip = temp->ip;
        array[i].len = temp->len;
        array[i].port = temp->port;
    }
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	int i,j;
	//char filename[50] ="d4_75k.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();
	printf("Avg. build: %llu\n",(end-begin)/num_entry);
	printf("array size: %d\n", arraySize);
	printf("Total memory requirement: %d KB\n", ((arraySize * sizeof(record_node)) / 1024));
	printf("Auxiliary memory requirement: %d KB\n", (num_node*(sizeof(node) - 3 ))/1024);
	shuffle(query, num_query);//洗亂query順序
	////////////////////////////////////////////////////////////////////////////
    //連續做100次Search取平均時間
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
            search(query[i].ip);
			end=rdtsc();
			if(clock[i]>(end-begin))
				clock[i]=(end-begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	CountClock();//計算耗費最多時間，最少時間，計算所需cycle數的分佈情況
	////////////////////////////////////////////////////////////////////////////
	return 0;
}
