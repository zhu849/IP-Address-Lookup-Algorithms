#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
////////////////////////////////////////////////////////////////////////////////////
//total size = 6byte
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
	unsigned int port;
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int N=0;
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
int num_node = 0;
int num_space = 0;
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;//default port, 256 meaning no name
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
		if(ip&(1<<(31-i))){
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
////////////////////////////////////////////////////////////////////////////////////
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
    //*******why nexthop = n[2]?**********
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
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip){
	int j;
	btrie current = root, temp = NULL;
	for(j=31;j>=(-1);j--){
		if(current==NULL)
			break;
		if(current->port!=256)
			temp=current;
		if(ip&(1<<j)){
			current=current->right;
		}
		else{
			current=current->left; 
		}
	}
    /*
	if(temp==NULL)
	  printf("default\n");
    else
	  printf("%u\n",temp->port);
    */
     
}
////////////////////////////////////////////////////////////////////////////////////
void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	input = (unsigned int *)malloc(num_input * sizeof(struct ENTRY));//?¨ä?å­˜æ”¾query
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
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
	query=(unsigned int *)malloc(num_query*sizeof(struct ENTRY));
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
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
void count_prefix_node(btrie r) {
	if (r == NULL)
		return;
	count_prefix_node(r->left);
	count_prefix_node(r->right);
	if (r->port != 256)
		N++;
}
////////////////////////////////////////////////////////////////////////////////////
void one_level_push(btrie r,int push) 
{
	if (r == NULL)
		return;
	//if this level had pushed, next level should not push 
	int left_push = 0, right_push = 0;
	//need do push
	if (r->port != 256 && push == 0)
	{
		//both node is NULL, this node is leave
		if (r->left == NULL && r->right == NULL)
			return;
		//left of child node is NULL
		else if (r->left == NULL) {
			//not need to push
			if (r->right->port == 256) {
			}//do nothing
			//push to left
			else {
				r->left = create_node();
				r->left->port = r->port;
				r->port = 256;
				left_push = 1;
				num_space++;
			}
		}
		//right of child node is NULL
		else if (r->right == NULL) {
			// not need to push
			if (r->left->port == 256) {
			}//do nothing
			//push to right
			else {
				r->right = create_node();
				r->right->port = r->port;
				r->port = 256;
				right_push = 1;
				num_space++;
			}
		}
		//both of child is empty,not need to push
		else if (r->left->port == 256 && r->right->port == 256) 
		{//do nothing
		}
		//both of child have port number, not need to push, and delete this node port number
		else if (r->left->port != 256 && r->right->port != 256) {
			r->port = 256;
		}
		//push to right child
		else if (r->left->port != 256 && r->right->port == 256) {
			r->right->port = r->port;
			r->port = 256;
			right_push = 1;
		}
		else if (r->right->port != 256 && r->left->port == 256)
		{
			r->left->port = r->port;
			r->port = 256;
			left_push = 1;
		}
	}
	//recursive function
	one_level_push(r->left, left_push);
	one_level_push(r->right, right_push);
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	begin=rdtsc();
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
	count_prefix_node(root);
	printf("Before one-level push have prefix nodes;%d\n", N);
	one_level_push(root, 0);
	N = 0;
	count_prefix_node(root);
	printf("After one-level push have prefix nodes;%d\n", N);
	printf("add space;%d\n", num_space);
	end = rdtsc();
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
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
	char filename[50] = "400k_IP_build.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();
    
	printf("Avg. Build Time: %llu\n", (end - begin) / num_entry);
	//printf("number of nodes created: %d\n", num_node);
	printf("Total memory requirement: %ld KB\n", ((num_node * sizeof(node)) / 1024));
	count_node(root);
	printf("There are %d nodes in binary trie\n", N);

	shuffle(query, num_query);
	////////////////////////////////////////////////////////////////////////////
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
	
	printf("Avg. Search: %lld\n",total/num_query);
	CountClock();
	////////////////////////////////////////////////////////////////////////////
	begin = rdtsc();
	for (int i =0;i<num_input;i++)
		add_node(input[i].ip, input[i].len, input[i].port);
	end = rdtsc();
	printf("Avg. insert Time:%lld\n", (end - begin) / num_input);
	return 0;
}
