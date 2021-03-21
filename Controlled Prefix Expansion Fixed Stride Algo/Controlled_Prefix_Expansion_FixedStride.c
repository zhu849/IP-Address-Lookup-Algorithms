#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<limits.h>

#define READ_STR_BUF 100
#define START_LEVEL 0

struct ENTRY{
	unsigned int ip; 
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
btrie root;
struct ENTRY *table;
int num_entry = 0;
int level_node[32] = {0};
int perfect_cut[3];

btrie create_node(){
	btrie temp;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;
	return temp;
}

void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
		// which bit from head, 1 mean right node 0 mean left node
		if(ip&(1<<(31-i))){
			// child is NULL
			if(ptr->right==NULL)
				ptr->right=create_node(); 
			ptr=ptr->right;
			// leaf node
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

void create(){
	root = create_node();
	for (int i = 0; i < num_entry; i++) 
		add_node(table[i].ip, table[i].len, table[i].port);
}
//Count how many node in same level
void count_levelnode(btrie r, int level)
{
	if (!r) return;
	if (r->left || r->right) level_node[level]++;
	count_levelnode(r->left, level + 1);
	count_levelnode(r->right, level + 1);
}

void DP_cut()
{
	int i,j,k;
	unsigned long long int cost[33][5];//Cost will exception when use int and long type declared
	unsigned long long int temp_cost;
	int cut_point[33][5];
	int temp_cutpoint;
	
	//Initialize
	for (i = 0; i < 33; i++){
		for (j = 0; j < 5; j++){
			cost[i][j] = ULLONG_MAX;//Edge value
			cut_point[i][j] = 0;//Mean have'n cut
		}
	}
	//One part situation
	for (i = 1; i < 33; i++)
	{
		cost[i][1] = pow(2, i);	//If level-0 to level-i in same part, cost always pow of 2
		cut_point[i][1] = i;	//First cut position at level-i
	}
	//Dynamic Programming count total cost with twice parts to fourth parts
	for (j = 2; j < 5; j++){//Decision where to cut base on less part situation
		for (i = 1; i < 33; i++){
			//Count cost with cost[i][j], mean partition j parts in level-i
			for (k = j - 1; k < i; k++){//Number of partitionshould sould smaller than number of level
				//If cut at level-k, than partition with two part, first part is remain level-0 to level-k, second part is level-k to level i.
				//In second part, will construct array by height(i-k) that size is always pow of 2.
				temp_cost = cost[k][j - 1] + (unsigned long long int)(level_node[k] * pow(2, i - k));
				temp_cutpoint = k;
				/*For debug and trace
				printf("Now count cost[%d][%d], k:%d, First cost[%d][%d]:%lld, Level_node[%d]:%ld\n", i, j, k, k, j - 1, cost[k][j - 1], k, level_node[k]);
				printf("Pow cost:%d, Tempcost:%lld, Cutpoint:%d\n", (int)pow(2, i - k), temp_cost,cut_point[i][j]);
				*/
				//Update min cost
				if (temp_cost < cost[i][j]){
					cost[i][j] = temp_cost;
					cut_point[i][j] = temp_cutpoint;
				}
			}
		}
	}
	//Display cost table
	printf("\n\n*** Best Cost On Every Level ***\n\n");
	for (j = 1; j < 5; j++) 
		for (i = 1; i < 33; i++) 
			printf("cost[%d][%d] = %llu, cutpoint = %d\n", i, j, cost[i][j], cut_point[i][j]);
	//Set result cut
	perfect_cut[0] = cut_point[32][4];
	perfect_cut[1] = cut_point[perfect_cut[0]][3];
	perfect_cut[2] = cut_point[perfect_cut[1]][2];
}

int main(int argc,char *argv[]){
	set_table(argv[1]);
	create();
	count_levelnode(root, START_LEVEL);

	printf("*** Number of nodes on every level ***\n");
	for (int i = 0; i < 32; i++) printf("Level:%d, Number of nodes:%d\n", i, level_node[i]);
	DP_cut();
	printf("\n*** First cut: %d, Second cut: %d, Third cut: %d ***\n", perfect_cut[0], perfect_cut[1], perfect_cut[2]);
	return 0;
}
