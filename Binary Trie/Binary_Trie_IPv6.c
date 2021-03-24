#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define READ_STR_BUF 100

//Use to count computation time on linux sys 
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

struct ENTRY{
	unsigned long long int ip_upper; 
	//unsigned long long int ip_lower;
	unsigned char len;
	unsigned char port;
};

struct list{
	unsigned int port;
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;

/*global variables*/
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
btrie root;
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int num_node = 0;

btrie create_node(){
	btrie temp;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;
	return temp;
}

void add_node(unsigned long long int ip_upper, unsigned long long int ip_lower, unsigned char len, unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<64 && i<len;i++){
		if(ip_upper&((unsigned long long int)1<<(63-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); 
			ptr=ptr->right;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
		}
	}
	for(i=64;i<len;i++){
		if(ip_lower&((unsigned long long int)1<<(63-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); 
			ptr=ptr->right;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
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

void search(unsigned long long int ip_upper, unsigned long long int ip_lower){
	int i;
	btrie current = root, temp = NULL;
	for(i=63;i>=(-1);i--){
		if(!current) break;
		if(current->port!=256) temp=current;
		current = (ip_upper&((unsigned long long int)1<<i))?current=current->right:current=current->left;
	}
	for(i=63;i>=(-1);i--){
		if(!current) break;
		if(current->port!=256) temp=current;
		current = (ip_lower&((unsigned long long int)1<<i))?current=current->right:current=current->left;
	}
    /*
	if(temp==NULL)
	  printf("default\n");
    else
	  printf("%u\n",temp->port);   
	*/
}

void read_table(char *str, unsigned long long int *ip_upper, unsigned long long int *ip_lower, int *len, unsigned int *nexthop) {
	int i, j = 0;
	int seg_count = 0;
	int gap;
	char tok[] = "/";
	char tok2[] = ":";
	char buf[READ_STR_BUF];
	char temp[READ_STR_BUF];
	char *p, *now, *next;

	//Throw out part of front(IP)
	sprintf(buf, "%s\0", strtok(str, tok));
	strcpy(temp,buf);
	//Get prefix length
	sprintf(buf, "%s\0", strtok(NULL, tok));
	*len = atoi(buf);

	//Count have number segment  
	p = strtok(temp, tok2);
	while (p != NULL){
		seg_count++;
		p = strtok(NULL, tok2);
	}
	//IP expandation
	memset(temp, '\0', READ_STR_BUF);
	sprintf(buf, "%s\0", strtok(str, tok));
	next = now = buf;
	while (*next != '\0'){//line end
		next++;
		//check double colon, padding 0 by (8 - (# of segment))
		if (*now == ':' && *next == ':') {
			for (i = j; i < j + (8 - seg_count) * 4; i++)
				temp[i] = '0';
			j += (8 - seg_count) * 4;
			now += 2;
			next++;
		}
		else {
			while (*next != ':' && *next != '\0')//next pointer point to : position or line end
				next++;
			while (*now == ':')//now pointer point to one segment begin position
				now++;
			gap = next - now;
			//alignment by padding zero with same length of zero
			for (i = 4 - gap; i > 0; i--, j++)
				temp[j] = '0';
			for (i = 0; i < gap; i++, j++, now++)
				temp[j] = *now;
		}
	}
	//Convert to byte expression
	*ip_upper = 0;
	for (i = 0; i < 16; i++) {
		*ip_upper <<= 4;
		*ip_upper += (temp[i] > '9') ? (temp[i] - 'a' + 10) : (temp[i] - '0');
	}
	*ip_lower = 0;
	for (i = 16; i < 32; i++) {
		*ip_lower <<= 4;
		*ip_lower += (temp[i] > '9') ? (temp[i] - 'a' + 10) : (temp[i] - '0');
	}
	*nexthop = temp[5];
}

void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp=fopen(file_name,"r");
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string,&ip_upper, &ip_lower, &len, &nexthop);
		num_entry++;
	}
	rewind(fp);
	//Allocate table space
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		table[num_entry].ip_upper = ip_upper;
		table[num_entry].ip_lower = ip_lower;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}

void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp = fopen(file_name, "r");
	while (fgets(string, READ_STR_BUF-1, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		num_query++;
	}
	rewind(fp);
	//Allocate table space
	query=(struct ENTRY *)malloc(num_query*sizeof(struct ENTRY));
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
        query[num_query].ip_upper = ip_upper;
		query[num_query].ip_lower = ip_lower;
        query[num_query].port = nexthop;
        query[num_query].len = len;
		clock[num_query++]=10000000;
	}
}

void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp = fopen(file_name, "r");
	while (fgets(string, READ_STR_BUF-1, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	input = (struct ENTRY *)malloc(num_input * sizeof(struct ENTRY));
	num_input = 0;
	while (fgets(string, READ_STR_BUF-1, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		input[num_input].ip_upper = ip_upper;
		input[num_input].ip_lower = ip_lower;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
}
//Create binary trie structure
void create(){
	begin=rdtsc();
	root = create_node();
	for (int i = 0; i < num_entry; i++)
		add_node(table[i].ip_upper, table[i].ip_lower, table[i].len, table[i].port);
	end = rdtsc();
}
//count trie node with inorder
void count_node(btrie r){
	if(!r) return;
	count_node(r->left);
	num_node++;
	count_node(r->right);
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

void shuffle(struct ENTRY *array, int n) {
    srand((unsigned)time(NULL));
    struct ENTRY *temp=(struct ENTRY *)malloc(sizeof(struct ENTRY));
    
    for (int i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        temp->ip_upper=array[j].ip_upper;
		temp->ip_lower = array[j].ip_lower;
        temp->len=array[j].len;
        temp->port=array[j].port;
        array[j].ip_upper = array[i].ip_upper;
		array[j].ip_lower = array[i].ip_lower;
        array[j].len = array[i].len;
        array[j].port = array[i].port;
        array[i].ip_upper = temp->ip_upper;
		array[i].ip_lower = temp->ip_lower;
        array[i].len = temp->len;
        array[i].port = temp->port;
    }
}

int main(int argc,char *argv[]){
	int i,j;
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();
    
	printf("Avg. Build Time: %llu\n", (end - begin) / num_entry);
	printf("number of nodes created: %d\n",num_node);
	printf("Total memory requirement: %ld KB\n",((num_node*9)/1024));
	count_node(root);
	printf("There are %d nodes in binary trie have port\n", N);

	shuffle(query, num_query);
	//Search operation
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
			search(query[i].ip_upper);
		    end=rdtsc();
			if(clock[i]>(end-begin))
				clock[i]=(end-begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=clock[j];
	
	printf("Avg. Search: %lld\n",total/num_query);
	CountClock();

	//Update operation
	begin = rdtsc();
	for (i =0;i<num_input;i++)
		add_node(table[i].ip_upper, table[i].len, table[i].port);
	end = rdtsc();
	printf("Avg. insert Time:%lld\n", (end - begin) / num_input);
	printf("number of nodes after inserted: %d\n", num_node);
	return 0;
}
