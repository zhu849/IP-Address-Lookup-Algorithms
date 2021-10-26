#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int ip;
	unsigned char len;
	unsigned char port;//實際上是longest matching 的 prefix node name
};
////////////////////////////////////////////////////////////////////////////////////
//linux系統用於計算時間

static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

////////////////////////////////////////////////////////////////////////////////////
struct list{
	unsigned int port;
    struct list *left,*right;
};
////////////////////////////////////////////////////////////////////////////////////
//node structure of segmentaion table
typedef struct list node;
typedef node *btrie;
struct segnode {
	int max_height;
	unsigned int port;
	btrie pointer;
	unsigned int *subarray;
	unsigned int *compress_subarray;
	unsigned int *bitmap;
};
typedef struct segnode segmentation_node;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int N = 0;
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
int num_node = 0;
int num_subnode = 0;
int num_compressnode = 0;
segmentation_node *segmentation_Table;
/////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
    btrie temp;
    num_node++;
    temp=(btrie)malloc(sizeof(node));
    temp->right=NULL;
    temp->left=NULL;
    temp->port=256;
    return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void insert_node(unsigned int ip, unsigned char len, unsigned char nexthop) {
	//找到前16bit的prefix
	int prefix = (ip&(0xFFFF0000)) >> 16;

	if (len <= 16)
		segmentation_Table[prefix].port = nexthop;
	else {
		if (segmentation_Table[prefix].pointer == NULL)
			segmentation_Table[prefix].pointer = create_node();
		int now_index = (ip & 0x0000FFFF) >> (16 - segmentation_Table[prefix].max_height);
		//realloc new size
		if (now_index >= (int)pow(2, segmentation_Table[prefix].max_height) - 1) {
			int max_height = count_trie_height(segmentation_Table[prefix].pointer);
			unsigned int *temp = realloc(segmentation_Table[prefix].subarray, sizeof(unsigned int) * ((int)pow(2, max_height) + 1));
			segmentation_Table[prefix].subarray = temp;
		}
		segmentation_Table[prefix].subarray[now_index] = nexthop;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr = NULL;
    //找到前16bit的prefix
	int prefix = (ip&(0xFFFF0000)) >> 16;

    //<=16bit prefix 的port number 直接存在segmentation table
	if (len <= 16)
		segmentation_Table[prefix].port = nexthop;
    else{
        //Create segment table[i] root
		if (segmentation_Table[prefix].pointer == NULL)
		{
			segmentation_Table[prefix].pointer = create_node();
			segmentation_Table[prefix].pointer->port = segmentation_Table[prefix].port;
		}
        //point to the index's root
        ptr = segmentation_Table[prefix].pointer;
        //start construct binary trie with this segment[i] node
        //under code is same as Binary_trie
        for(int i = 16;i<len;i++){
            if(ip&(1<<(31-i))){
                if(ptr->right==NULL)
                    ptr->right=create_node(); 
                ptr=ptr->right;
				if ((i == len - 1) && (ptr->port == 256))
					ptr->port = nexthop;
            }
            else{
                if(ptr->left==NULL)
                    ptr->left=create_node();
                ptr=ptr->left;
                if((i==len-1)&&(ptr->port==256))
                    ptr->port=nexthop;
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str, unsigned int *ip, int *len, unsigned int *nexthop) {
	char tok[] = "./";//用於切割的token
	char buf[100], *str1;
	unsigned int n[4];
	//將前面四個ip位址切出來
	sprintf(buf, "%s\0", strtok(str, tok));
	n[0] = atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok));
	n[1] = atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok));
	n[2] = atoi(buf);
	sprintf(buf, "%s\0", strtok(NULL, tok));
	n[3] = atoi(buf);
	//*******why nexthop = n[2]?**********
	*nexthop = n[2];
	str1 = (char *)strtok(NULL, tok);
	//輸入prefix length的長度
	if (str1 != NULL) {
		sprintf(buf, "%s\0", str1);
		*len = atoi(buf);
	}
	else {
		if (n[1] == 0 && n[2] == 0 && n[3] == 0)
			*len = 8;
		else
			if (n[2] == 0 && n[3] == 0)
				*len = 16;
			else
				if (n[3] == 0)
					*len = 24;
	}
	//輸入ip value
	*ip = n[0];
	*ip <<= 8;
	*ip += n[1];
	*ip <<= 8;
	*ip += n[2];
	*ip <<= 8;
	*ip += n[3];
}
////////////////////////////////////////////////////////////////////////////////////
unsigned int trie_search(unsigned char segment_nexthop, int morebit, btrie r, int max_height) {
	btrie current = r;
	unsigned char nexthop = segment_nexthop;
	for (int j = max_height; j >= 0; j--) {
		if (current == NULL)
			break;
		if (current->port != 256)
			nexthop = current->port;
		if (morebit&(0x00000001 << (j - 1)))
			current = current->right;
		else
			current = current->left;
	}
	return nexthop;
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip){
    int target = 256;
    //first of 16bit
    int prefix = (ip&(0xFFFF0000))>>16;

	target = segmentation_Table[prefix].port;
    if (segmentation_Table[prefix].pointer != NULL){
		int index = (ip&(0x0000FFFF)) >> (16 - segmentation_Table[prefix].max_height);
		int count = -1;
		for (int j = 0; j <= index; j++) 
			if (segmentation_Table[prefix].bitmap[j]) count++;
		target = segmentation_Table[prefix].compress_subarray[count];
    }
    
    //用於確認是否正確找到longest matching prefix
	/*
	if (target >= 256)
		printf("**********default*************\n");
    else
	    printf("%u\n",target);
	*/
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
void initial_segmentationTable(){
    for(int i= 0;i<65536;i++){
        segmentation_Table[i].port = 256;
        segmentation_Table[i].pointer = NULL;
		segmentation_Table[i].subarray = NULL;
		segmentation_Table[i].max_height = 0;
    }
}
////////////////////////////////////////////////////////////////////////////////////
//recursive find trie height, level start with 0
int count_trie_height(btrie p){
    if(p == NULL)
        return -1;
    else{
        int left_max = count_trie_height(p->left);
        int right_max = count_trie_height(p->right);
        if(left_max >= right_max)
            return left_max+1;
        else
            return right_max+1;
    }
}
////////////////////////////////////////////////////////////////////////////////////
void build_subarray(){
    int max_height;
	for (int i = 0; i < 65536; i++) {
		if (segmentation_Table[i].pointer != NULL){
			max_height = count_trie_height(segmentation_Table[i].pointer);
			segmentation_Table[i].max_height = max_height;
			segmentation_Table[i].subarray = malloc(sizeof(unsigned int)*((int)pow(2, max_height) + 1));
			num_subnode += (int)pow(2, max_height);
			//assign value to segment subtable
			for (int j = 0; j < (int)pow(2, max_height); j++) {
				segmentation_Table[i].subarray[j] = trie_search(segmentation_Table[i].port, j, segmentation_Table[i].pointer, max_height);
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create_bitmap() {
	for (int i = 0; i < 65536; i++) {
		//有trie才需要建bitmap
		if (segmentation_Table[i].pointer != NULL) {
			segmentation_Table[i].bitmap = malloc(sizeof(unsigned int)*((int)pow(2, segmentation_Table[i].max_height + 1)));
			segmentation_Table[i].bitmap[0] = 1;
			for (int j = 1; j < pow(2, segmentation_Table[i].max_height); j++) {
				if (segmentation_Table[i].subarray[j] != segmentation_Table[i].subarray[j - 1])
					segmentation_Table[i].bitmap[j] = 1;
				else
					segmentation_Table[i].bitmap[j] = 0;
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create_compress_subarray() {
	int num_bitmap_one;
	//count memory size of compress_subarray
	for (int i = 0; i < 65536; i++) {
		if (segmentation_Table[i].pointer != NULL) {
			num_bitmap_one = 0;
			for (int j = 0; j < pow(2, segmentation_Table[i].max_height); j++) {
				if (segmentation_Table[i].bitmap[j]) num_bitmap_one++;
			}
		}
		segmentation_Table[i].compress_subarray = malloc(sizeof(unsigned int)*num_bitmap_one);
		num_compressnode += num_bitmap_one;
	}
	int k;
	for (int i = 0; i < 65536; i++) {
		if (segmentation_Table[i].pointer != NULL) {
			k = 0;
			for (int j = 0; j < pow(2, segmentation_Table[i].max_height); j++) {
				if (segmentation_Table[i].bitmap[j]) {
					segmentation_Table[i].compress_subarray[k] = segmentation_Table[i].subarray[j];
					k++;
				}
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	begin=rdtsc();
	segmentation_Table = malloc(sizeof(segmentation_node) * 65536);
	initial_segmentationTable();
    //build tree node
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
    //build subtable
    build_subarray();
	create_bitmap();
	create_compress_subarray();
	end=rdtsc();
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	if(r->port != 256)
		N++;
	count_node(r->right);
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
		if (clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
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
int main(int argc,int *argv[]){
	int i, j;
	//char filename[50] = "d2_5k.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();

	printf("Avg. Build: %llu\n", (end - begin) / num_entry);
	printf("number of subnodes: %d\n", num_subnode);
	printf("number of compressed node: %d\n", num_compressnode);
	printf("Total memory requirement: %d KB\n", ((sizeof(segmentation_node) * 65536 + sizeof(unsigned int)* num_compressnode) / 1024));

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
    for(int i =0;i<65536;i++)
        count_node(segmentation_Table[i].pointer);
	printf("There are %d nodes in have port value\n",N);
	return 0;
}
