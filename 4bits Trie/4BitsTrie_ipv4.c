#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<chrono>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};
////////////////////////////////////////////////////////////////////////////////////
//rdtsc

static __inline__ unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

////////////////////////////////////////////////////////////////////////////////////
// rdtscp
inline unsigned long long int rdtscp(void)
{
	unsigned int lo, hi;
	// take time stamp counter, rdtscp does serialize by itself, and is much cheaper than using CPUID
	__asm__ __volatile__(
		"rdtscp" : "=a"(lo), "=d"(hi)
	);
	return ((unsigned long long int)lo) | (((unsigned long long int)hi) << 32);
}
////////////////////////////////////////////////////////////////////////////////////
struct list {//structure of binary trie
	unsigned int port;
	struct list *bitsPointer[16];//from 0000 to 1111 subtrie node
};
typedef struct list node;
typedef node *mtrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
mtrie root;
struct ENTRY *query;
struct ENTRY *table;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int N = 0;//number of nodes
unsigned long long int begin, end, total = 0;
std::chrono::steady_clock::time_point c_begin, c_end;
unsigned long long int *clocks;
int num_node = 0;
int memory_access[33] = { 0 };
int memory_access_time;
////////////////////////////////////////////////////////////////////////////////////
mtrie create_node() {
	mtrie temp;
	num_node++;
	temp = (mtrie)malloc(sizeof(node));
	//initialize all subnode point to NULL
	for (int i = 0; i < 16; i++)
		temp->bitsPointer[i] = NULL;
	temp->port = 256;//default port number 256 meaning no name
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip, unsigned char len, unsigned char nexthop) {
	mtrie ptr = root;
	unsigned int stridePrefix;//get the wanted 4-bit part of prefix
	for (int i = 0; i < len; i += 4) {
		//相當於將每個4bit拿出來, 然後再往右邊推到底當作index
		stridePrefix = (ip&(0xF0000000 >> i)) >> (28 - i);
		//haven't create node, 所有的子節點都要Created
		if (ptr->bitsPointer[stridePrefix] == NULL) {
			ptr->bitsPointer[stridePrefix] = create_node();
		}
		// if (i+4 > len) mean it is last stride and still not named port
		if ((i + 4 >= len) && (ptr->port == 256))
			ptr->port = nexthop;
		ptr = ptr->bitsPointer[stridePrefix];
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
	//判斷prefix length的長度
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
	//assign ip value
	*ip = n[0];
	*ip <<= 8;
	*ip += n[1];
	*ip <<= 8;
	*ip += n[2];
	*ip <<= 8;
	*ip += n[3];
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip) {
	int j;
	int prefix;
	memory_access_time = 0;
	mtrie current = root, temp = NULL;

	for (j = 28; j > (-1); j -= 4) {
		memory_access_time++;
		//this node is leave
		if (current == NULL)
			break;
		//record longest prefix port
		if (current->port != 256)
			temp = current;
		//找到往下搜尋的subnode index
		prefix = (ip&(0x0000000F << j)) >> j;
		if (current->bitsPointer[prefix] == NULL)
			break;
		current = current->bitsPointer[prefix];
	}
	memory_access[memory_access_time]++;
	//找到最適合的prefix node name
	/*
	if(temp==NULL)
	  printf("default\n");
	else
	  printf("%d\n",temp->port);
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
		//將len, ip, nexthop參數傳址傳入，透過read table function取得值‘
		//每次輸入txt檔內的一行data
		read_table(string, &ip, &len, &nexthop);
		num_input++;
	}
	rewind(fp);
	//記憶體配置
	input = (struct ENTRY* )malloc(num_input * sizeof(struct ENTRY));//用來存放query
	num_input = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
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
		//將len, ip, nexthop參數傳址傳入，透過read table拿回
		//每次輸入txt檔內的一行data
		read_table(string, &ip, &len, &nexthop);
		num_query++;
	}
	rewind(fp);
	//記憶體配置
	query = (struct ENTRY*)malloc(num_query * sizeof(struct ENTRY));
	clocks = (unsigned long long int *)malloc(num_query * sizeof(unsigned long long int));
	num_query = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		query[num_query].ip = ip;
		query[num_query].port = nexthop;
		query[num_query].len = len;
		clocks[num_query++] = 10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create() {
	int i;
	//begin = rdtscp();
	c_begin = std::chrono::steady_clock::now();
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
	c_end = std::chrono::steady_clock::now();
	//end = rdtscp();
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
		if (clocks[i] > MaxClock) MaxClock = clocks[i];
		if (clocks[i] < MinClock) MinClock = clocks[i];
		if (clocks[i] / 100 < 50) NumCntClock[clocks[i] / 100]++;
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
	srand((unsigned)time_t(NULL));
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
	//char filename[50] = "400k_IP_build.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();

	printf("Avg. build: %llu\n", (c_end - c_begin) / num_entry);
	printf("number of nodes: %d\n", num_node);
	printf("Total memory requirement: %d KB\n", ((num_node * 65) / 1024));

	shuffle(query, num_entry);
	////////////////////////////////////////////////////////////////////////////
	for (j = 0; j < 100; j++) {
		for (i = 0; i < num_query; i++) {
			//begin = rdtscp();
			c_begin = std::chrono::steady_clock::now();
			search(query[i].ip);
			c_end = std::chrono::steady_clock::now();
			//end = rdtscp();
			if (clocks[i] > (c_end - c_begin).count())
				clocks[i] = (c_end - c_begin).count();
		}
	}
	total = 0;
	for (j = 0; j < num_query; j++)
		total += clocks[j];
	printf("Avg. Search: %llu\n", total / num_query);
	CountClock();
	////////////////////////////////////////////////////////////////////////////
	printf("memory access time table:\n");
	double total_mem;
	for (i = 1; i < 32; i++) {
		printf("%d times = %d\n", i, memory_access[i] / 100);
		total_mem += memory_access[i] * i;
	}
	printf("avg. memory access time:%f\n", total_mem / (num_query * 100));
	////////////////////////////////////////////////////////////////////////////
	//begin = rdtscp();
	c_begin = std::chrono::steady_clock::now();
	for (i = 0; i < num_input; i++)
		add_node(input[i].ip, input[i].len, input[i].port);
	c_end = std::chrono::steady_clock::now();
	//end = rdtscp();
	printf("Avg. insert time:%llu\n", (c_end - c_begin) / num_input);
	printf("number of nodes after insert: %d\n", num_node);
	printf("Total memory requirement after insert: %d KB\n", ((num_node * 65) / 1024));
	return 0;
}
