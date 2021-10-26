#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};
////////////////////////////////////////////////////////////////////////////////////

static __inline__ unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
};

////////////////////////////////////////////////////////////////////////////////////
//size = 12byte
struct list {
	unsigned int port;
	struct list *left, *right;
};
////////////////////////////////////////////////////////////////////////////////////
//size = 5 byte
struct linklist_node {
	unsigned int port;
	struct linklist_node *next;
};
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
////////////////////////////////////////////////////////////////////////////////////
typedef struct list node;
typedef node *btrie;
typedef struct superlist supernode;
typedef supernode *supertrie;
typedef struct linklist_node list_node;
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
int N = 0;
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;
int num_node = 0;
int num_super_node = 0;
int num_listnode = 0;
int memory_access[64] = {0};
int memory_access_time;
////////////////////////////////////////////////////////////////////////////////////
btrie create_node() {
	btrie temp;
	num_node++;
	temp = (btrie)malloc(sizeof(node));
	temp->right = NULL;
	temp->left = NULL;
	temp->port = 256;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
list_node* create_listnode()
{
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
	for (int i = 0; i < 17; i++){
		temp->internal_bitmap[i] = 0;
		temp->external_bitmap[i] = 0;
		temp->pointer[i] = NULL;
	}
	temp->nexthop = create_listnode();
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void insert(unsigned int ip, unsigned char len, unsigned char nexthop){
	supertrie current = super_root;
	int index;
	int record_nexthop = -1;
	int next_level_index;

	for (int i = 28, j = 0; i >= -4 && j < len; i -= 4, j += 4){
		//last level finded
		if (j + 4 > len){
			index = (ip&(0x0000000E << i)) >> (i + 1);
			//insert level 3
			if (j + 3 == len){
				//update
				if (current->internal_bitmap[8 + index] == 1) {
					list_node *temp = current->nexthop;
					for (int i = 1; i <= (8 + index); i++){
						if(current->internal_bitmap[i] == 1)
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
		next_level_index = (ip&(0x0000000F << i)) >> i;
		//none next level node, build it
		if (current->pointer[next_level_index] == NULL){
			current->pointer[next_level_index] = create_supernode();
			current->external_bitmap[next_level_index] = 1;
		}
		//to next level find
		current = current->pointer[next_level_index];
	}
}
////////////////////////////////////////////////////////////////////////////////////
int find_newest_hop(supertrie current,int index) {
	list_node *temp = current->nexthop;//first nexthop not saved anything
	if (temp == NULL)return 256;
	for (int j = 1; j <= index; j++) {
		if (current->internal_bitmap[j] == 1)
			temp = temp->next;
	}	
	return temp->port;
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip) {
	supertrie current = super_root;
	int index;
	int record_nexthop = 256;
	int next_level_index;
	memory_access_time = 0;
	//need more search one times to check level-8 supernode's external node
	for (int i = 28; i >= -4; i -= 4){
		if (current == NULL)break;
		index = (ip&(0x0000000E << i)) >> (i + 1);
		memory_access_time++;
		//fore three bit
		if (current->internal_bitmap[8 + index] == 1) {
			record_nexthop = find_newest_hop(current, 8 + index);
			memory_access_time++;
		}
		else {
			//fore two bit
			index = index >> 1;
			if (current->internal_bitmap[4 + index] == 1) {
				record_nexthop = find_newest_hop(current, 4 + index);
				memory_access_time++;
			}
			else {
				//fore one bit
				index = index >> 1;
				if (current->internal_bitmap[2 + index] == 1) {
					record_nexthop = find_newest_hop(current, 2 + index);
					memory_access_time++;
				}
				else {
					//root
					if (current->internal_bitmap[1] == 1) {
						record_nexthop = find_newest_hop(current, 1);
						memory_access_time++;
					}
				}
			}
		}
		next_level_index = (ip&(0x0000000F << i)) >> i;
		if (current->external_bitmap[next_level_index] == 1)
			current = current->pointer[next_level_index];
		else
			break;
	}
	memory_access[memory_access_time]++;
	/*
	if (record_nexthop == 256)
		printf("**************ERROR******************\n");
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
		if (r->port != 256){
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
void add_node(unsigned int ip, unsigned char len, unsigned char nexthop) {
	btrie ptr = root;
	int i;
	for (i = 0; i < len; i++) {
		if (ip&(1 << (31 - i))) {
			if (ptr->right == NULL)
				ptr->right = create_node(); 
			ptr = ptr->right;
			if ((i == len - 1) && (ptr->port == 256))
				ptr->port = nexthop;
		}
		else {
			if (ptr->left == NULL)
				ptr->left = create_node();
			ptr = ptr->left;
			if ((i == len - 1) && (ptr->port == 256))
				ptr->port = nexthop;
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
void set_table(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		num_entry++;
	}
	rewind(fp);
	table = (struct ENTRY *)malloc(num_entry * sizeof(struct ENTRY));
	num_entry = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		table[num_entry].ip = ip;
		table[num_entry].port = nexthop;
		table[num_entry++].len = len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		//將len, ip, nexthop參數傳址傳入，透過read table function取得值‘
		//每次輸入txt檔內的一行data
		read_table(string, &ip, &len, &nexthop);
		num_query++;
	}
	rewind(fp);
	//記憶體配置
	query = (unsigned int *)malloc(num_query * sizeof(struct ENTRY));//用來存放query
	clock = (unsigned long long int *)malloc(num_query * sizeof(unsigned long long int));//用來存放每輪時間
	num_query = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		query[num_query].ip = ip;
		query[num_query].port = nexthop;
		query[num_query].len = len;
		clock[num_query++] = 10000000;
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
void create() {
	int i;
	begin = rdtsc();
	root = create_node();
	super_root = create_supernode();
	//build binary trie
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
	//root start from index 1
	build_tree_bitmap(root, 1, super_root);
	end = rdtsc();
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r) {
	if (r == NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int*)malloc(50 * sizeof(unsigned int));
	for (i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for (i = 0; i < num_query; i++)
	{
		if (clock[i] > MaxClock) MaxClock = clock[i];
		if (clock[i] < MinClock) MinClock = clock[i];
		if (clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);

	for (i = 0; i < 50; i++)
	{
		printf("%d\n", NumCntClock[i]);
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////////
void shuffle(struct ENTRY *array, int n) {
	srand((unsigned)time(NULL));
	struct ENTRY *temp = (struct ENTRY *)malloc(sizeof(struct ENTRY));

	for (int i = 0; i < n - 1; i++) {
		size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
		temp->ip = array[j].ip;
		temp->len = array[j].len;
		temp->port = array[j].port;
		array[j].ip = array[i].ip;
		array[j].len = array[i].len;
		array[j].port = array[i].port;
		array[i].ip = temp->ip;
		array[i].len = temp->len;
		array[i].port = temp->port;
	}
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	int i, j;
	char filename[50] = "ipv4_build.txt";
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
	shuffle(query, num_query);
	////////////////////////////////////////////////////////////////////////////
	for (j = 0; j < 100; j++) {
		for (i = 0; i < num_query; i++) {
			begin = rdtsc();
			search(query[i].ip);
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
	for (i = 1; i < 64; i++) {
		printf("%d times = %d\n", i, memory_access[i] / 100);
		total_mem += memory_access[i] * i;
	}
	printf("avg. memory access time:%f\n", total_mem / (num_query * 100));
	////////////////////////////////////////////////////////////////////////////

	begin = rdtsc();
	for (int j = 0; j < num_input; j++) 
		insert(input[j].ip, input[j].len, input[j].port);
	end = rdtsc();
	printf("Avg Update Time: %llu\n", (end - begin) / num_input);
	printf("number of super_node After update: %d\n", num_super_node);
	printf("number of list node after insert: %d\n", num_listnode);

	return 0;
}
