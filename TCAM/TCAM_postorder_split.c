#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define BUCKET_SIZE 4
////////////////////////////////////////////////////////////////////////////////////
//total size = 6byte
struct ENTRY{
	unsigned int ip; //ip?Ä?∑ÁÇ∫32bits = 4byte
	unsigned char len;//?åÁ?Ôº?byteË∂≥Â?Ë°®È?0~255
	unsigned int port;//ÂØ¶È?‰∏äÊòØlongest matching ??prefix node name,Â§ßÂ???byte = 8bitË∂≥Â?Ë°®È?0~255
};
////////////////////////////////////////////////////////////////////////////////////
struct list{
	struct ENTRY node_info;
	unsigned int num_hopchild;
	struct list *left,*right;
};
struct m_node {
	unsigned int index[BUCKET_SIZE];//index from 0 - 3
	unsigned int bucket[BUCKET_SIZE];//index from 0 - 3
	int bucket_size;
	int index_size;
	//unsigned int cover_prefix;
};
typedef struct list node;
typedef struct m_node memory_node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
struct ENTRY *table;
memory_node *storage;
int num_entry = 0;
int num_node = 0;
int memory_gindex;
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->node_info.port = 256;
	temp->node_info.len = 0;
	temp->node_info.ip = 0;
	temp->num_hopchild = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(struct ENTRY new_node){
	btrie ptr=root;
	ptr->num_hopchild++;
	int i;
	for(i=0;i<new_node.len;i++){
		if(new_node.ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); 
			ptr=ptr->right;
			ptr->num_hopchild++;
			if ((i == new_node.len - 1) && (ptr->node_info.port == 256))
				ptr->node_info = new_node;
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			ptr->num_hopchild++;
			if ((i == new_node.len - 1) && (ptr->node_info.port == 256))
				ptr->node_info = new_node;
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
void create(){
	int i,j;
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i]);

	int storage_size = (2 * num_node) / BUCKET_SIZE;
	storage = malloc(sizeof(memory_node) * storage_size);
	for (i = 0; i < storage_size; i++) {
		for (j = 0; j < BUCKET_SIZE; j++)
			storage[i].index[j] = 0;
		storage[i].bucket_size = -1;
		storage[i].index_size = -1;
		//storage[i].cover_prefix = NULL;
		for (int j = 0; j < BUCKET_SIZE; j++)
			storage[i].bucket[j] = 0;
	}
}
////////////////////////////////////////////////////////////////////////////////////
int subtree_cut(btrie r) {
	if (r == NULL)
		return 0;
	int left_count = subtree_cut(r->left);
	int right_count = subtree_cut(r->right);
	r->num_hopchild -= (left_count + right_count);
	// this node have port nunmber
	if (r->node_info.port != 256) {
		storage[memory_gindex].bucket_size++;
		storage[memory_gindex].bucket[storage[memory_gindex].bucket_size] = r->node_info.ip >> (32 - r->node_info.len);
		r->num_hopchild--;
		return (left_count + right_count + 1);
	}
	return (left_count + right_count);
}
////////////////////////////////////////////////////////////////////////////////////
//path is parent prefix
int postorder_search(btrie r,unsigned int path) {
	//external node
	if (r == NULL)
		return 0;
	//every time shift once
	path = path << 1;
	int minus = 0;
	//have left child
	if (r->left != NULL)
	{
		// if now bucket can contain this subtree's prefix node and this num_hopchild not num of 0
		if (r->left->num_hopchild < (BUCKET_SIZE - storage[memory_gindex].bucket_size)) {
			minus = subtree_cut(r->left);
			storage[memory_gindex].index_size++;
			storage[memory_gindex].index[storage[memory_gindex].index_size] = path;
			r->num_hopchild -= minus;
			r->left = NULL;
			return minus;
		}
		else
		{
			minus = postorder_search(r->left, path);
			if (minus > 0)
			{
				r->num_hopchild -= minus;
				return minus;
			}
		}
	}
	else if (r->right != NULL)
	{
		path = path + 1;
		if (r->right->num_hopchild < (BUCKET_SIZE - storage[memory_gindex].bucket_size)) {
			minus = subtree_cut(r->right);
			storage[memory_gindex].index_size++;
			storage[memory_gindex].index[storage[memory_gindex].index_size] = path;
			r->num_hopchild -= minus;
			r->right = NULL;
			return minus;
		}
		else
		{
			minus = postorder_search(r->right, path);
			if (minus > 0)
			{
				r->num_hopchild -= minus;
				return minus;
			}
		}
	}

	return minus;
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	int i,j;
	char filename[30] = "testd1.txt";
	set_table(argv[1]);
	create();
	printf("created over\n");
	while (root->num_hopchild > 0)
	{
		postorder_search(root, 0);
		if (storage[memory_gindex].bucket_size == BUCKET_SIZE - 1) {
			memory_gindex++;
		}
		//if(memory_gindex == 255)
			//printf("%d\n",memory_gindex);
	}

	printf("Total data:%d\n", num_entry);
	printf("Total TCAM index:%d\n", memory_gindex);
	int count_bucket[BUCKET_SIZE + 1];
	for (i = 0; i <= BUCKET_SIZE; i++)
		count_bucket[i] = 0;
	for (i = 0; i < memory_gindex; i++)
		count_bucket[storage[i].index_size]++;
	for (i = 0; i <= BUCKET_SIZE; i++)
		printf("index size:%d, number:%d\n", i, count_bucket[i]);
	
	return 0;
}
