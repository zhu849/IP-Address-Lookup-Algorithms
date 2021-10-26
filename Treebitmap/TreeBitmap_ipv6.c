#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned long long int ip_upper; 
	unsigned long long int ip_lower;
	unsigned char len;
	unsigned char port;
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
//size = 5 byte
struct linklist_node {
	unsigned int port;
	struct linklist_node *next;
};
typedef struct linklist_node list_node;
////////////////////////////////////////////////////////////////////////////////////
//size = 72byte
struct superlist {
	//can using unsigned char = 2byte size solution
	unsigned int internal_bitmap[17];
	struct linklist_node *nexthop;
	//can using unsigned char = 2byte size solution
	unsigned int external_bitmap[17];
	struct supertire *pointer[17];//point to next level supernode
};
typedef struct superlist supernode;
typedef supernode *supertrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;
supertrie super_root;
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
int num_super_node = 0;
int num_listnode = 0;
int memory_access[33] = { 0 };
int memory_access_time;
////////////////////////////////////////////////////////////////////////////////////
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
list_node* create_listnode() {
	list_node *temp = malloc(sizeof(list_node));
	num_listnode++;
	temp->next = NULL;
	temp->port = 256;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
supertrie create_supernode() {
	supertrie temp;
	num_super_node++;
	temp = (supertrie)malloc(sizeof(supernode));
	for (int i = 0; i < 17; i++) {
		temp->internal_bitmap[i] = 0;
		temp->external_bitmap[i] = 0;
		temp->pointer[i] = NULL;
	}
	temp->nexthop = create_listnode();
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void insert(unsigned long long int ip_upper, unsigned char len, unsigned char nexthop) {
	supertrie current = super_root;
	int index;
	int record_nexthop = -1;
	int next_level_index;

	for (int i = 60, j = 0; i >= -4 && j < len; i -= 4, j += 4) {
		//last level finded
		if (j + 4 > len) {
			index = (ip_upper&((unsigned long long int)0x000000000000000E << i)) >> (i + 1);
			//insert level 3
			if (j + 3 == len) {
				//update
				if (current->internal_bitmap[8 + index] == 1) {
					list_node *temp = current->nexthop;
					for (int i = 1; i <= (8 + index); i++) {
						if (current->internal_bitmap[i] == 1)
							temp = temp->next;
					}
					temp->port = nexthop;
				}
				//insert
				else {
					list_node *temp = current->nexthop;
					list_node *t_next;
					for (int i = 1; i <= (8 + index); i++) {
						if (current->internal_bitmap[i] == 1)
							temp = temp->next;
					}
					t_next = temp->next;
					temp->next = create_listnode();
					temp->next->port = nexthop;
					temp->next->next = t_next;
					current->internal_bitmap[8 + index] = 1;
				}
				break;
			}
			//insert level 2
			index >>= 1;
			if (j + 2 == len) {
				if (current->internal_bitmap[4 + index] == 1) {
					list_node *temp = current->nexthop;
					for (int i = 1; i <= (4 + index); i++) {
						if (current->internal_bitmap[i] == 1)
							temp = temp->next;
					}
					temp->port = nexthop;
				}
				//insert
				else {
					list_node *temp = current->nexthop;
					list_node *t_next;
					for (int i = 1; i <= (4 + index); i++) {
						if (current->internal_bitmap[i] == 1)
							temp = temp->next;
					}
					t_next = temp->next;
					temp->next = create_listnode();
					temp->next->port = nexthop;
					temp->next->next = t_next;
					current->internal_bitmap[4 + index] = 1;
				}
				break;
			}
			//insert level 1
			index >>= 1;
			if (j + 1 == len) {
				if (current->internal_bitmap[2 + index] == 1) {
					list_node *temp = current->nexthop;
					for (int i = 1; i <= (2 + index); i++) {
						if (current->internal_bitmap[i] == 1)
							temp = temp->next;
					}
					temp->port = nexthop;
				}
				//insert
				else {
					list_node *temp = current->nexthop;
					list_node *t_next;
					for (int i = 1; i <= (2 + index); i++) {
						if (current->internal_bitmap[i] == 1)
							temp = temp->next;
					}
					t_next = temp->next;
					temp->next = create_listnode();
					temp->next->port = nexthop;
					temp->next->next = t_next;
					current->internal_bitmap[2 + index] = 1;
				}
				break;
			}
			//insert level 0
			index >>= 1;
			if (j == len) {
				if (current->internal_bitmap[1] == 1) {
					list_node *temp = current->nexthop;
					temp = temp->next;
					temp->port = nexthop;
				}
				//insert
				else {
					list_node *temp = current->nexthop;
					list_node *t_next;
					temp = temp->next;
					t_next = temp->next;
					temp->next = create_listnode();
					temp->next->port = nexthop;
					temp->next->next = t_next;
					current->internal_bitmap[1] = 1;
				}
				break;
			}
		}
		next_level_index = (ip_upper&((unsigned long long int)0x000000000000000F << i)) >> i;
		//none next level node, build it
		if (current->pointer[next_level_index] == NULL) {
			current->pointer[next_level_index] = create_supernode();
			current->external_bitmap[next_level_index] = 1;
		}
		//to next level find
		current = current->pointer[next_level_index];
	}
}
////////////////////////////////////////////////////////////////////////////////////
int find_newest_hop(supertrie current, int index) {
	list_node *temp = current->nexthop;//first nexthop not saved anything
	if (temp == NULL)return 256;
	for (int j = 1; j <= index; j++) {
		if (current->internal_bitmap[j] == 1)
			temp = temp->next;
	}
	return temp->port;
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned long long int ip_upper, unsigned long long int ip_lower){
	supertrie current = super_root;
	int index;
	int record_nexthop = 256;
	int next_level_index;
	memory_access_time = 0;
	//need more search one times to check level-8 supernode's external node
	for (int i = 60; i >= 0; i -= 4) {
		if (current == NULL)break;
		index = (ip_upper&((unsigned long long int)0x000000000000000E << i)) >> (i + 1);
		memory_access_time++;
		//fore three bit
		if (current->internal_bitmap[8 + index] == 1)
			record_nexthop = find_newest_hop(current, 8 + index);
		else {
			//fore two bit
			index = index >> 1;
			if (current->internal_bitmap[4 + index] == 1)
				record_nexthop = find_newest_hop(current, 4 + index);
			else {
				//fore one bit
				index = index >> 1;
				if (current->internal_bitmap[2 + index] == 1)
					record_nexthop = find_newest_hop(current, 2 + index);
				else {
					//root
					if (current->internal_bitmap[1] == 1)
						record_nexthop = find_newest_hop(current, 1);
				}
			}
		}
		next_level_index = (ip_upper&((unsigned long long int)0x000000000000000F << i)) >> i;
		if (current->external_bitmap[next_level_index] == 1)
			current = current->pointer[next_level_index];
		else
			break;
	}

	for (int i = 60; i >= -4; i -= 4) {
		if (current == NULL)break;
		index = (ip_lower&((unsigned long long int)0x000000000000000E << i)) >> (i + 1);
		memory_access_time++;
		//fore three bit
		if (current->internal_bitmap[8 + index] == 1)
			record_nexthop = find_newest_hop(current, 8 + index);
		else {
			//fore two bit
			index = index >> 1;
			if (current->internal_bitmap[4 + index] == 1)
				record_nexthop = find_newest_hop(current, 4 + index);
			else {
				//fore one bit
				index = index >> 1;
				if (current->internal_bitmap[2 + index] == 1)
					record_nexthop = find_newest_hop(current, 2 + index);
				else {
					//root
					if (current->internal_bitmap[1] == 1)
						record_nexthop = find_newest_hop(current, 1);
				}
			}
		}
		next_level_index = (ip_lower&((unsigned long long int)0x000000000000000F << i)) >> i;
		if (current->external_bitmap[next_level_index] == 1)
			current = current->pointer[next_level_index];
		else
			break;
	}
	memory_access[memory_access_time]++;
	/*
	if (record_nexthop == 256)
		printf("**************ERROR******************\n");
	else
		printf("search number:%d\n", record_nexthop);
		*/
}
////////////////////////////////////////////////////////////////////////////////////
void build_tree_bitmap(btrie r, int index, supertrie s_root) {
	//now r is NULL 
	if (r == NULL)
		return;
	//root pointer
	if (index == 1) {
		if (r->port != 256) {
			list_node *temp = s_root->nexthop;
			//point to last listnode of that have hop
			while (temp->next != NULL)
				temp = temp->next;
			temp->next = create_listnode();
			temp->next->port = r->port;
			temp = temp->next;//temp until point to last node of that have hop
			s_root->internal_bitmap[1] = 1;
		}
		else
			s_root->internal_bitmap[1] = 0;
	}
	//internal node from index 1 to index 7(level 0 1 2)
	if (index < 8) {
		if (r->left != NULL) {
			if (r->left->port != 256) {
				list_node *temp = s_root->nexthop;
				while (temp->next != NULL)
					temp = temp->next;
				temp->next = create_listnode();
				temp->next->port = r->left->port;
				temp = temp->next;
				s_root->internal_bitmap[2 * index] = 1;
			}
			else
				s_root->internal_bitmap[2 * index] = 0;
			//recursive build left tree
			build_tree_bitmap(r->left, 2 * index, s_root);
		}
		else
			s_root->internal_bitmap[2 * index] = 0;
		if (r->right != NULL) {
			if (r->right->port != 256) {
				list_node *temp = s_root->nexthop;
				while (temp->next != NULL)
					temp = temp->next;
				temp->next = create_listnode();
				temp->next->port = r->right->port;
				temp = temp->next;
				s_root->internal_bitmap[2 * index + 1] = 1;
			}
			else
				s_root->internal_bitmap[2 * index + 1] = 0;
			//recursive build right tree
			build_tree_bitmap(r->right, 2 * index + 1, s_root);
		}
		else
			s_root->internal_bitmap[2 * index + 1] = 0;
	}
	//external node from index 8 to index 15(level 3)
	else {
		if (r->left != NULL) {
			s_root->pointer[2 * index - 16] = create_supernode();
			//build next level supernode
			build_tree_bitmap(r->left, 1, s_root->pointer[(2 * index) - 16]);
			s_root->external_bitmap[2 * index - 16] = 1;
		}
		else
			s_root->external_bitmap[2 * index - 16] = 0;

		if (r->right != NULL) {
			s_root->pointer[2 * index - 16 + 1] = create_supernode();
			//build next level supernode
			build_tree_bitmap(r->right, 1, s_root->pointer[(2 * index) - 16 + 1]);
			s_root->external_bitmap[2 * index - 16 + 1] = 1;
		}
		else
			s_root->external_bitmap[2 * index - 16 + 1] = 0;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned long long int ip_upper, unsigned long long int ip_lower,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for (i = 0; i < len && i< 64; i++) {
		if(ip_upper&((unsigned long long int)0x0000000000000001<<(63-i))){
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
	for (i = 0; i < len - 64; i++) {
		if(ip_lower&((unsigned long long int)0x0000000000000001<<(63-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); 
			ptr=ptr->right;
			if ((i + 4 >= len - 64) && (ptr->port == 256))
				ptr->port=nexthop;
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if ((i + 4 >= len - 64) && (ptr->port == 256))
				ptr->port=nexthop;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str, unsigned long long int *ip_upper, unsigned long long int *ip_lower, int *len, unsigned int *nexthop) {
	int i, j = 0;
	int seg_count = 0;
	int gap;
	char tok[] = "/";
	char tok2[] = ":";
	char buf[100];
	char temp[100];
	char *p;

	sprintf(buf, "%s\0", strtok(str, tok));
	strcpy(temp,buf);
	// get len
	sprintf(buf, "%s\0", strtok(NULL, tok));
	*len = atoi(buf);

	//count have number segment
	p = strtok(temp, tok2);
	while (p != NULL){
		seg_count++;
		p = strtok(NULL, tok2);
	}

	//ip expend
	memset(temp, '\0', 100);
	sprintf(buf, "%s\0", strtok(str, tok));
	char *now = buf, *next = buf;

	while (*next != '\0'){
		next++;
		//check double colon
		if (*now == ':' && *next == ':') {
			for (i = j; i < j + (8 - seg_count) * 4; i++)
				temp[i] = '0';
			j += (8 - seg_count) * 4;
			now += 2;
			next++;
		}
		else {
			while (*next != ':' && *next != '\0')
				next++;
			while (*now == ':')
				now++;
			gap = next - now;
			for (i = 4 - gap; i > 0; i--, j++)
				temp[j] = '0';
			for (i = 0; i < gap; i++, j++, now++)
				temp[j] = *now;
		}
	}
	//convert to byte expression
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
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		table[num_entry].ip_upper = ip_upper;
		table[num_entry].ip_lower = ip_lower;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		num_query++;
	}
	rewind(fp);
	query=(struct ENTRY *)malloc(num_query*sizeof(struct ENTRY));
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
        query[num_query].ip_upper=ip_upper;
		query[num_query].ip_lower = ip_lower;
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
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	input = (struct ENTRY *)malloc(num_input * sizeof(struct ENTRY));
	num_input = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		input[num_input].ip_upper = ip_upper;
		input[num_input].ip_lower = ip_lower;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create() {
	int i;
	begin = rdtsc();
	root = create_node();
	super_root = create_supernode();
	//build binary trie
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip_upper, table[i].ip_lower, table[i].len, table[i].port);
	//root start from index 1
	build_tree_bitmap(root, 1, super_root);
	end = rdtsc();
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
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	int i, j;
	char filename[50] = "ipv6_table.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();

	printf("Avg. build: %llu\n", (end - begin) / num_entry);
	count_node(root);
	printf("There are %d nodes in binary trie\n", N);
	printf("number of super_node: %d\n", num_super_node);
	printf("number of list node :%d\n", num_listnode);

	printf("Binary Trie memory requirement: %d KB\n", (num_node * 9) / 1024);
	printf("super Trie memory requirement: %d KB\n", (num_super_node * 72) / 1024);
	printf("list node memory requirement: %d KB\n", (num_listnode * 5) / 1024);
	shuffle(query, num_entry);
	////////////////////////////////////////////////////////////////////////////
	for (j = 0; j < 100; j++) {
		for (i = 0; i < num_query; i++) {
			begin = rdtsc();
			search(query[i].ip_upper,query[i].ip_lower);
			end = rdtsc();
			if (clock[i] > (end - begin))
				clock[i] = (end - begin);
		}
	}
	total = 0;
	for (j = 0; j < num_query; j++)
		total += clock[j];
	printf("Avg. Search: %llu\n", total / num_query);
	CountClock();
	////////////////////////////////////////////////////////////////////////////
	printf("memory access time table:\n");
	double total_mem = 0;
	for (i = 1; i < 33; i++) {
		printf("%d times = %d\n", i, memory_access[i] / 100);
		total_mem += memory_access[i] * i;
	}
	printf("avg. memory access time:%f\n", total_mem / (num_query * 100));
	////////////////////////////////////////////////////////////////////////////

	begin = rdtsc();
	for (int j = 0; j < num_input; j++)
		insert(input[j].ip_upper, input[j].len, input[j].port);
	end = rdtsc();
	printf("Avg Update Time: %llu\n", (end - begin) / num_input);
	printf("number of super_node After update: %d\n", num_super_node);
	printf("number of list node after insert: %d\n", num_listnode);

	return 0;
}
