#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<chrono>
#include<iostream>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	unsigned char len;
	unsigned char port;
};
////////////////////////////////////////////////////////////////////////////////////
// rdtsc
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
struct list {
	unsigned int port;
	struct list *bitPointer[16];
};
typedef struct list node;
typedef node *mtrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
mtrie root;
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int N = 0;
unsigned long long int begin, end, total = 0;
std::chrono::steady_clock::time_point c_begin, c_end;
unsigned long long int *clocks;
int num_node = 0;
int counter = 0;
int memory_access[33] = { 0 };
int memory_access_time;
////////////////////////////////////////////////////////////////////////////////////
mtrie create_node() {
	mtrie temp;
	num_node++;
	temp = (mtrie)malloc(sizeof(node));
	for (int i = 0; i < 16; i++)
		temp->bitPointer[i] = NULL;
	temp->port = 256;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned long long int ip_upper, unsigned long long int ip_lower, unsigned char len, unsigned char nexthop) {
	mtrie ptr = root;
	int i;
	unsigned long long int stridePrefix;//get the wanted 4-bit part of prefix

	for (i = 0; i < len && i < 64; i += 4) {
		stridePrefix = (ip_upper&((unsigned long long int)0xF000000000000000 >> i)) >> (60 - i);
		//next level node hadn't created
		if (ptr->bitPointer[stridePrefix] == NULL) {
			ptr->bitPointer[stridePrefix] = create_node();
		}
		// if (i+4 > len) mean it is last stride and still not named port
		if ((i + 4 >= len) && (ptr->port == 256))
			ptr->port = nexthop;
		ptr = ptr->bitPointer[stridePrefix];
	}
	//find ip_lower
	for (i = 0; i < len - 64; i += 4) {
		stridePrefix = (ip_lower&((unsigned long long int)0xF000000000000000 >> i)) >> (60 - i);
		//next level node hadn't created
		if (ptr->bitPointer[stridePrefix] == NULL) {
			ptr->bitPointer[stridePrefix] = create_node();
		}
		// if (i+4 > len) mean it is last stride and still not named port
		if ((i + 4 >= len - 64) && (ptr->port == 256))
			ptr->port = nexthop;
		ptr = ptr->bitPointer[stridePrefix];
	}
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned long long int ip_upper, unsigned long long int ip_lower) {
	int j;
	memory_access_time = 0;
	mtrie current = root, temp = NULL;
	unsigned long long int prefix;
	for (j = 60; j > (-1); j -= 4) {
		memory_access_time++;
		//this node is leave
		if (current == NULL)
			break;
		if (current->port != 256)
			temp = current;
		prefix = (ip_upper&((unsigned long long int)0x000000000000000F << j)) >> j;
		current = current->bitPointer[prefix];
	}
	for (j = 60; j > (-1); j -= 4) {
		memory_access_time++;
		//this node is leave
		if (current == NULL)
			break;
		if (current->port != 256)
			temp = current;
		prefix = (ip_lower&((unsigned long long int)0x000000000000000F << j)) >> j;
		current = current->bitPointer[prefix];
	}
	memory_access[memory_access_time]++;
	/*
	if(temp==NULL)
	  printf("default\n");
	else
	  printf("%u\n",temp->port);
	  */

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
	strcpy(temp, buf);
	// get len
	sprintf(buf, "%s\0", strtok(NULL, tok));
	*len = atoi(buf);

	//count have number segment
	p = strtok(temp, tok2);
	while (p != NULL) {
		seg_count++;
		p = strtok(NULL, tok2);
	}

	//ip expend
	memset(temp, '\0', 100);
	sprintf(buf, "%s\0", strtok(str, tok));
	char *now = buf, *next = buf;

	while (*next != '\0') {
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
void set_table(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		num_entry++;
	}
	rewind(fp);
	table = (struct ENTRY *)malloc(num_entry * sizeof(struct ENTRY));
	num_entry = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		table[num_entry].ip_upper = ip_upper;
		table[num_entry].ip_lower = ip_lower;
		table[num_entry].port = nexthop;
		table[num_entry++].len = len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void set_query(char *file_name) {
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
	query = (struct ENTRY *)malloc(num_query * sizeof(struct ENTRY));
	clocks = (unsigned long long int *)malloc(num_query * sizeof(unsigned long long int));
	num_query = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip_upper, &ip_lower, &len, &nexthop);
		query[num_query].ip_upper = ip_upper;
		query[num_query].ip_lower = ip_lower;
		query[num_query].port = nexthop;
		query[num_query].len = len;
		clocks[num_query++] = 10000000;
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
	//begin = rdtscp();
	c_begin = std::chrono::steady_clock::now();
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip_upper, table[i].ip_lower, table[i].len, table[i].port);
	c_end = std::chrono::steady_clock::now();
	//end = rdtscp();
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(mtrie r) {
	if (r == NULL)
		return;
	for (int i = 0; i < 16; i++)
		count_node(r->bitPointer[i]);
	if (r->port != 256)
		N++;
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
		temp->ip_upper = array[j].ip_upper;
		temp->ip_lower = array[j].ip_lower;
		temp->len = array[j].len;
		temp->port = array[j].port;
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
	char filename[50] = "ipv6_100build.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();
	std::cout << (double)std::chrono::duration_cast<std::chrono::microseconds>(c_end - c_begin).count()<< "microseconds" << std::endl;
	printf("Avg. Build Time: %llu\n", (std::chrono::microseconds)(c_end - c_begin).count() / num_entry);
	printf("number of nodes created: %d\n", num_node);
	printf("Total memory requirement: %ld KB\n", ((num_node * 65) / 1024));
	count_node(root);
	printf("There are %d nodes in binary trie have port\n", N);

	shuffle(query, num_query);
	////////////////////////////////////////////////////////////////////////////

	for (j = 0; j < 100; j++) {
		for (i = 0; i < num_query; i++) {
			//begin = rdtscp();
			c_begin = std::chrono::steady_clock::now();
			search(query[i].ip_upper, query[i].ip_lower);
			c_end = std::chrono::steady_clock::now();
			//end = rdtscp();
			if (clocks[i] > (c_end - c_begin).count())
				clocks[i] = (c_end - c_begin).count();
		}
	}
	total = 0;
	for (j = 0; j < num_query; j++)
		total += clocks[j];

	printf("Avg. Search: %lld\n", total / num_query);
	CountClock();
	////////////////////////////////////////////////////////////////////////////
	printf("memory access time table:\n");
	double total_mem;
	for (i = 1; i < 33; i++) {
		printf("%d times = %d\n", i, memory_access[i] / 100);
		total_mem += memory_access[i] * i;
	}
	printf("avg. memory access time:%f\n", total_mem / (num_query * 100));
	////////////////////////////////////////////////////////////////////////////
	//begin = rdtscp();
	c_begin = std::chrono::steady_clock::now();
	for (int i = 0; i < num_input; i++)
		add_node(input[i].ip_upper, input[i].ip_lower, input[i].len, input[i].port);
	c_end = std::chrono::steady_clock::now();
	//end = rdtscp();
	printf("Avg. insert Time:%lld\n", (c_end - c_begin).count() / num_input);
	printf("number of nodes after inserted: %d\n", num_node);
	return 0;
}
