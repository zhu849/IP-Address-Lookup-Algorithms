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
struct list{//structure of binary trie
	struct ENTRY node_info;
	unsigned int num_hopchild;
	struct list *left,*right;
};
struct m_node {
	unsigned int index;
	unsigned int bucket[BUCKET_SIZE];
	int bucket_size
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
int bucket_gindex;
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
	int i;
	for(i=0;i<new_node.len;i++){
		ptr->num_hopchild++;
		if(new_node.ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); 
			ptr=ptr->right;
			if ((i == new_node.len - 1) && (ptr->node_info.port == 256))
			{
				ptr->num_hopchild++;
				ptr->node_info = new_node;
			}
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if ((i == new_node.len - 1) && (ptr->node_info.port == 256))
			{
				ptr->num_hopchild++;
				ptr->node_info = new_node;
			}
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
	int i;
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i]);
	int storage_size = (2 * num_node) / BUCKET_SIZE;
	storage = malloc(sizeof(memory_node) * storage_size);
	for (i = 0; i < storage_size; i++) {
		storage[i].index = 0;
		storage[i].bucket_size = 0;
		//storage[i].cover_prefix = NULL;
		for(int j =0;j<BUCKET_SIZE;j++)
			storage[i].bucket[j] = NULL;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void subtree_cut(btrie r) {
	if (r == NULL)
		return 0;
	subtree_cut(r->left);
	subtree_cut(r->right);
	// this node have port nunmber
	if (r->node_info.port != 256) {
		storage[memory_gindex].bucket[bucket_gindex] = r->node_info.ip >> (32 - r->node_info.len);
		bucket_gindex++;//let every recursive subfuntion know number of count now
		storage[memory_gindex].bucket_size++;
	}
}
////////////////////////////////////////////////////////////////////////////////////
int subtree_search(btrie r,unsigned int path) {
	if (r == NULL)
		return 0;

	int minus = 0;
	if (r->left != NULL)
	{
		path = path << 1;
        //cut subtree rule
		if (r->left->num_hopchild <= BUCKET_SIZE && r->left->num_hopchild >= (BUCKET_SIZE+1)/2) {
			//record subtree's root path
            storage[memory_gindex].index = path;
			subtree_cut(r->left);
			r->left = NULL;// cut out this subtree
			r->num_hopchild -= storage[memory_gindex].bucket_size;
			return storage[memory_gindex].bucket_size;
		}
		minus = subtree_search(r->left, path);
		//child had cut
        if (minus > 0){
			r->num_hopchild -= minus;
			return minus;
		}
	}
	if (r->right != NULL)
	{
		path = (path << 1) + 1;
		if (r->right->num_hopchild <= BUCKET_SIZE && r->right->num_hopchild >= (BUCKET_SIZE + 1) / 2) {
			storage[memory_gindex].index = path;
			subtree_cut(r->right);
			r->right = NULL;
			r->num_hopchild -= storage[memory_gindex].bucket_size;
			return storage[memory_gindex].bucket_size;
		}
		minus = subtree_search(r->right, path);
		if (minus > 0)
		{
			r->num_hopchild -= minus;
			return minus;
		}
	}
	return minus;
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	int i,j;
	char filename[30] = "400k_IP_build.txt";
	set_table(argv[1]);
	create();
	while (root->num_hopchild > 0)
	{
		subtree_search(root, 0);
		memory_gindex++;
		bucket_gindex = 0;
	}
	printf("Total data:%d\n", num_entry);
	printf("Total TCAM index:%d\n", memory_gindex);
    int count_bucket[BUCKET_SIZE+1];
	for (i = 0; i <= BUCKET_SIZE; i++)
		count_bucket[i] = 0;
	for (i = 0; i < memory_gindex; i++)
		count_bucket[storage[i].bucket_size]++;
	for (i = BUCKET_SIZE/2; i <= BUCKET_SIZE; i++)
		printf("Bucket size:%d, number:%d\n", i, count_bucket[i]);
		
	return 0;
}
