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
	unsigned int start_index;
	unsigned int end_index;
	unsigned int port;
};
//node?Ø‰?Á®Ælist?ÑÊì¥?ÖÂ???
typedef struct list node;
typedef struct record record_node;
//btrie?Ñtype??node
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;//?®Êñº?áÂ??¥Ê£µÊ®πÁ?root
//query?åtable?öÁõ∏?åÁ?‰∫ãÊ?ÔºåÂîØ‰∏Ä‰∏çÂ??ÑÊòØquery?ÉË¢´suffle
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
record_node *rangearray;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int num_leave = 0;
unsigned int now_index = 0;
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
        //maskÂæåË©≤bit == 1ÔºåÂ??ÄÈ´ò‰??ÉÈ?Âßãcompare
		if(newnode.ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); // Create Node
			ptr=ptr->right;
			if((i==newnode.len - 1)&&(ptr->node_info.port== 256))//lonest matching prefix
				ptr->node_info = newnode;
		}
        //maskÂæåË©≤bit == 0
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
	char tok[]="./";//?®Êñº?áÂâ≤?Ñtoken
	char buf[100],*str1;
	unsigned int n[4];
    //Â∞áÂ??¢Â??ãip‰ΩçÂ??áÂá∫‰æ?
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
    //Ëº∏ÂÖ•prefix length?ÑÈï∑Â∫?
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
    //Ëº∏ÂÖ•ip value
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
	int left = 0;
	int right = num_leave;
	int index = (left + right) / 2;

	while(right - left >= 0){
		if (ip > rangearray[index].end_index)
			left = index;
		else if (ip < rangearray[index].start_index)
			right = index;
		else{
			//printf("ip:%x, port:%d\n", ip, rangearray[index].port);
			break;
		}
		index = (right + left) / 2;
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
		//Â∞álen, ip, nexthop?ÉÊï∏?≥Â??≥ÂÖ•ÔºåÈÄèÈ?read table function?ñÂ??º‚Ä?
		//ÊØèÊ¨°Ëº∏ÂÖ•txtÊ™îÂÖß?Ñ‰?Ë°ådata
		read_table(string, &ip, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	//Ë®òÊÜ∂È´îÈ?ÁΩ?
	input = (unsigned int *)malloc(num_input * sizeof(struct ENTRY));//?®‰?Â≠òÊîæquery
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
        //Â∞álen, ip, nexthop?ÉÊï∏?≥Â??≥ÂÖ•ÔºåÈÄèÈ?read table function?ñÂ??º‚Ä?
        //ÊØèÊ¨°Ëº∏ÂÖ•txtÊ™îÂÖß?Ñ‰?Ë°ådata
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
    //Ë®òÊÜ∂È´îÈ?ÁΩ?
	query=(unsigned int *)malloc(num_query*sizeof(struct ENTRY));//?®‰?Â≠òÊîæquery
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));//?®‰?Â≠òÊîæÊØèËº™?ÇÈ?
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
void leaf_pushing(btrie r)
{
	if (r == NULL)
		return;
	btrie ptr = r;
	//this node have port need to push
	if (r->node_info.port != 256) 
	{
		//this node is leave
		if (r->left == NULL && r->right == NULL)
		{
			int start_index = r->node_info.ip & (0xFFFFFFFF << (32 - r->node_info.len));
			int end_index = r->node_info.ip | (0xFFFFFFFF >> r->node_info.len);
			if (start_index > now_index)
				num_leave++;
			num_leave++;
			now_index = end_index;
			return;
		}
		if (r->left == NULL)
		{
 			r->left = create_node();
			r->left->node_info.ip = r->node_info.ip & (0xFFFFFFFE << (31 - r->node_info.len));
			r->left->node_info.len = r->node_info.len + 1;
			r->left->node_info.port = r->node_info.port;
		}
		else if (r->left->node_info.port == 256) 
		{
			r->left->node_info.ip = r->node_info.ip & (0xFFFFFFFE << (31 - r->node_info.len));
			r->left->node_info.len = r->node_info.len + 1;
			r->left->node_info.port = r->node_info.port;
		}
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
		r->node_info.port = 256;
	}
	else {
		leaf_pushing(r->left);
		leaf_pushing(r->right);
	}
}
void inorder_search(btrie r) {
	if (r == NULL)
		return;
	inorder_search(r->left);
	unsigned int start_index = r->node_info.ip & (0xFFFFFFFF << (32 - r->node_info.len));
	unsigned int end_index = r->node_info.ip | (0xFFFFFFFF >> r->node_info.len);
	if (now_index < start_index) 
	{
		rangearray[num_leave].start_index = now_index;
		rangearray[num_leave].end_index = start_index - 1;
		rangearray[num_leave].port = 256;
		//printf("i:%d, start_index:%x, end index:%x, port;%d\n", num_leave, rangearray[num_leave].start_index, rangearray[num_leave].end_index, rangearray[num_leave].port);
		num_leave++;
	}
	if (r->node_info.port != 256) {
		rangearray[num_leave].start_index = start_index;
		rangearray[num_leave].end_index = end_index;
		rangearray[num_leave].port = r->node_info.port;
		//printf("i:%d, start_index:%x, end index:%x, port;%d\n", num_leave, rangearray[num_leave].start_index, rangearray[num_leave].end_index, rangearray[num_leave].port);
		num_leave++;
		now_index = end_index + 1;
	}
	
	inorder_search(r->right);
}
void create(){
	int i;
	root=create_node();
	begin=rdtsc();
	for(i=0;i<num_entry;i++)
		add_node(table[i]);
	leaf_pushing(root);
	rangearray = malloc((num_leave) * sizeof(record_node));
	num_leave = 0;
	now_index = 0;
	inorder_search(root);
	end=rdtsc();
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
	printf("array size: %d\n",num_leave);
  printf("support memoy%d KB\n", (sizeof(node) * num_node)/1024);
	printf("Total memory requirement: %d KB\n",((num_leave*(sizeof(record_node) - sizeof(unsigned int)))/1024));

	shuffle(query, num_query);//Ê¥ó‰?query?ÜÂ?
	////////////////////////////////////////////////////////////////////////////
    //?????00Ê¨°Search?ñÂπ≥?áÊ???
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
	CountClock();//Ë®àÁ??óË≤ª?ÄÂ§öÊ??ìÔ??ÄÂ∞ëÊ??ìÔ?Ë®àÁ??Ä?Äcycle?∏Á??Ü‰??ÖÊ?
	////////////////////////////////////////////////////////////////////////////
	return 0;
}
