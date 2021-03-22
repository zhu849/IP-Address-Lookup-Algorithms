#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define READ_STR_BUF 100

struct ENTRY{
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};

static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

struct list{
	unsigned int port;
    struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;

//Node structure of segmentaion table
struct segnode {
	char max_height;
	unsigned int port; 
	btrie pointer;
	unsigned int *subarray;
};
typedef struct segnode segmentation_node;

/*global variables*/
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int num_subnode = 0;
int num_have_val_subnode = 0;
int num_have_val_segnode = 0;
segmentation_node *segment_table;


btrie create_node(){
    btrie temp;
    temp=(btrie)malloc(sizeof(node));
    temp->right=NULL;
    temp->left=NULL;
    temp->port=256;
    return temp;
};

//Recursive find trie height, level start with 0
int count_trie_height(btrie p){
    if(p == NULL) return -1;
    else{
        int left_max = count_trie_height(p->left);
        int right_max = count_trie_height(p->right);
        return (left_max >= right_max)?left_max+1:right_max+1;
    }
}
//Insert new node in segment table and trie structure
void insert_node(unsigned int ip, unsigned char len, unsigned char nexthop) {
	add_node(ip,len,nexthop);
	int prefix = (ip&(0xFFFF0000)) >> 16;

	if (segment_table[prefix].pointer != NULL){//have trie
		int old_height = segment_table[prefix].max_height;
		int new_height = count_trie_height(segment_table[prefix].pointer);
		
		if (new_height > old_height) {
			segment_table[prefix].max_height = new_height;
			segment_table[prefix].subarray = realloc(segment_table[prefix].subarray, sizeof(unsigned int) * ((int)pow(2, segment_table[prefix].max_height)));
			
			for(int i = ((int)pow(2, old_height));i<((int)pow(2, new_height));i++)
				segment_table[prefix].subarray[i] = 256;

		}
		//Set new node's nexthop
		int insert_index = (ip & 0x0000FFFF) >> (16 - segment_table[prefix].max_height);
		segment_table[prefix].subarray[insert_index] = nexthop;	
	}	
}
//Add one entry info into trie
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
    btrie ptr=NULL;
	int prefix = (ip&(0xFFFF0000)) >> 16;//16 bits prefix of ip, prefix will be index to find segment node

    //if prefix length small than 16bits, port number direct store in segment table 
	if (len <= 16) segment_table[prefix].port = nexthop;
    else{
        //create segment table[i] root
        if(segment_table[prefix].pointer == NULL) segment_table[prefix].pointer = create_node();
        //point to the segment table block's root
        ptr = segment_table[prefix].pointer;
        //start construct binary trie with this segment[i] node
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

//Split one line and reassembly for ip format
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";
	char buf[READ_STR_BUF],*str1;
	unsigned int n[4];//store ip value
	sprintf(buf,"%s\0",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[3]=atoi(buf);
	*nexthop=n[2];//nexthop = n[2] just mean random nexthop value
	//deal with prefix length
	str1=(char *)strtok(NULL,tok);
	if(str1!=NULL){
		sprintf(buf,"%s\0",str1);
		*len=atoi(buf);
	}
	else{//exception situation
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
	//assign to ip with correct format
	*ip=n[0];
	*ip<<=8;
	*ip+=n[1];
	*ip<<=8;
	*ip+=n[2];
	*ip<<=8;
	*ip+=n[3];
}
//Search in segment table down trie with level 16 to 32
unsigned int trie_search(unsigned int segment_nexthop, int subnode_index, btrie r, int max_height) {
	btrie current = r;
	unsigned int nexthop = segment_nexthop;
	for (int j = max_height; j >= 0; j--) {
		if (current == NULL) break;
		if (current->port != 256) nexthop = current->port; // change longest matching prefix nexthop
		// Find next level
		current = (subnode_index&(0x00000001 << (j - 1)))?current->right:current->left;
	}
	return nexthop;
}

void search(unsigned int ip){
    int target = -1;
    int prefix = (ip&(0xFFFF0000))>>16;
	if (segment_table[prefix].pointer == NULL) target = segment_table[prefix].port;
    else{
		int index = (ip&(0x0000FFFF)) >> (16 - segment_table[prefix].max_height);
        target = segment_table[prefix].subarray[index];
    }
    
    /*
	if(target==-1) 
		printf("default\n");
    else
	  printf("%u\n",target);
	*/
}

void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp=fopen(file_name,"r");
	//count wanted table size
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string, &ip, &len, &nexthop);
		num_entry++;
	}
	rewind(fp);
	//allocate table space
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	//get table info
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}

void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp=fopen(file_name,"r");
	//count table size
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
	//allocate table space
	query=(unsigned int *)malloc(num_query*sizeof(struct ENTRY));
	//record start timestamp
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	//get table info, initialize clock recorded block
	while(fgets(string, READ_STR_BUF, fp)!=NULL){
		read_table(string, &ip, &len, &nexthop);
        query[num_query].ip=ip;
        query[num_query].port = nexthop;
        query[num_query].len = len;
		clock[num_query++]=10000000;
	}
}

void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	//count table size
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	//allocate space
	input = (unsigned int *)malloc(num_input * sizeof(struct ENTRY));
	num_input = 0;
	//get table info
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
}
//Create subnode structure
void build_subarray(int i){
	if (segment_table[i].pointer != NULL){//have trie
		segment_table[i].max_height = count_trie_height(segment_table[i].pointer);
		segment_table[i].subarray = malloc(sizeof(unsigned int)*((int)pow(2, segment_table[i].max_height) + 1));
		//Assign value to subnode
		for (int j = 0; j < (int)pow(2, segment_table[i].max_height); j++)
			segment_table[i].subarray[j] = trie_search(segment_table[i].port, j, segment_table[i].pointer, segment_table[i].max_height);
	}
}
//Create trie structure and subnode structure
void create(){
	int i;
	begin=rdtsc();
	segment_table = malloc(sizeof(segmentation_node) * 65536);
	//Segment table initialize
	for(i= 0;i<65536;i++){
        segment_table[i].port = 256;
        segment_table[i].pointer = NULL;
		segment_table[i].subarray = NULL;
		segment_table[i].max_height = 0;
    }
    //Build trie with all entry
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
	for (i = 0; i < 65536; i++) 
		build_subarray(i);
	end=rdtsc();
}
//Count distribution of computation time 
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
//Table entry shuffle
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

int main(int argc,int *argv[]){
	int i, j;
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();
	printf("Avg. Build: %llu\n", (end - begin) / num_entry);
	//Search operation
	shuffle(query, num_query);
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
            search(query[i].ip);
			end=rdtsc();
			if(clock[i]>(end-begin))
				clock[i]=(end-begin);
		}
	}
	//Time calculation
	total=0;
	for(j=0;j<num_query;j++)
		total+=clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	CountClock();

	//Space calculation
	for(i =0;i<65536;i++){
		//Count number of subnode
		num_subnode+=(int)pow(2,segment_table[i].max_height);
		//Count number of subnode those have value
		if(segment_table[i].subarray != NULL){
			for(j=0;j<(int)pow(2,segment_table[i].max_height);j++)
				if(segment_table[i].subarray[j]!=256) num_have_val_subnode++;
		}
        //Count segnode those have value
        if(segment_table[i].port != 256) num_have_val_segnode++;
	}
	printf("Number of subnodes: %d\n", num_subnode);
	printf("There are %d segnodes have port value.\n",num_have_val_segnode);
	printf("There are %d subnodes have port value.\n",num_have_val_subnode);
	printf("Total memory requirement: %d KB\n", (10*65536+4*num_subnode)/1024);

	//Update operation
	begin = rdtsc();
	for (int i = 0; i < num_input; i++)
		insert_node(input[i].ip, input[i].len, input[i].port);
	end = rdtsc();
	printf("Avg. Insert Time:%llu\n", (end - begin) / num_input);

	//Space calculation after insert operation
	num_subnode = 0;
	num_have_val_segnode = 0;
	num_have_val_subnode = 0;
	for(i =0;i<65536;i++){
		//Count number of subnode
		num_subnode+=(int)pow(2,segment_table[i].max_height);
		//Count number of subnode those have value
		if(segment_table[i].subarray != NULL){
			for(j=0;j<(int)pow(2,segment_table[i].max_height);j++)
				if(segment_table[i].subarray[j]!=256) num_have_val_subnode++;
		}
        //Count segnode those have value
        if(segment_table[i].port != 256) num_have_val_segnode++;
	}
	printf("Number of subnodes after insert operation: %d\n", num_subnode);
	printf("There are %d segnodes have port value after insert operation.\n",num_have_val_segnode);
	printf("There are %d subnodes have port value after insert operation.\n",num_have_val_subnode);
	printf("Total memory requirement after insert operation: %d KB\n", (10*65536+4*num_subnode)/1024);
	return 0;
}
