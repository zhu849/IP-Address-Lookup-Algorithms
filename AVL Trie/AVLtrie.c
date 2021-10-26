#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
	unsigned int ip;
	unsigned char len;
	unsigned char port;//實際上是longest matching 的 prefix node name
};
////////////////////////////////////////////////////////////////////////////////////

static __inline__ unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

////////////////////////////////////////////////////////////////////////////////////
struct list {//structure of binary trie
	struct ENTRY node_info;
	struct list *left, *right;
	int operation;
};
//node是一種list的擴充型態
typedef struct list node;
//btrie的type為 node
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root[32];//用於指向所有layer樹的root
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int N = 0;//number of nodes
unsigned long long int begin, end, total = 0;
unsigned long long int *clock;//用於記錄時間
int num_node = 0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////
btrie create_node() {
	btrie temp;
	num_node++;
	temp = (btrie)malloc(sizeof(node));
	temp->node_info.ip = 0;
	temp->node_info.len = 0;
	temp->node_info.port = 256;//default port, 256 meaning no name
	temp->right = NULL;
	temp->left = NULL;
	temp->operation = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
btrie do_rotation(btrie p,int dir)
{
	btrie temp;
	//p->left do rotation
	if (dir == 0) 
	{
		switch (p->left->operation) 
		{
		//LL
		case 1:
			//printf("%d do %d Rotation!\n", dir, p->left->operation);
			p->left->operation = 0;
			temp = p->left->left;
			p->left->left = temp->right;
			temp->right = p->left;
			p->left = temp;
			break;
		//LR
		case 2:
			//printf("%d do %d Rotation!\n", dir, p->left->operation);
			p->left->operation = 0;
			temp = p->left->left->right;
			p->left->left->right = temp->left;
			temp->left = p->left->left;
			p->left->left = temp->right;
			temp->right = p->left;
			p->left = temp;
			break;
		//RR
		case 3:
			//printf("%d do %d Rotation!\n", dir, p->left->operation);
			p->left->operation = 0;
			temp = p->left->right;
			p->left->right = temp->left;
			temp->left = p->left;
			p->left = temp;
			break;
		//RL
		case 4:
			//printf("%d do %d Rotation!\n", dir, p->left->operation);
			p->left->operation = 0;
			temp = p->left->right->left;
			p->left->right->left = temp->right;
			temp->right = p->left->right;
			p->left->right = temp->left;
			temp->left = p->left;
			p->left = temp;
			break;
		default:
			printf("left rotation number is error!%d\n", p->left->operation);
			break;
		}
	}
	//p->right do rotation
	else if(dir == 1)
	{
		switch (p->right->operation)
		{
			//LL
		case 1:
			//printf("%d do %d Rotation!\n", dir, p->right->operation);
			p->right->operation = 0;
			temp = p->right->left;
			p->right->left = temp->right;
			temp->right = p->right;
			p->right = temp;
			break;
			//LR
		case 2:
			//printf("%d do %d Rotation!\n", dir, p->right->operation);
			p->right->operation = 0;
			temp = p->right->left->right;
			p->right->left->right = temp->left;
			temp->left = p->right->left;
			p->right->left = temp->right;
			temp->right = p->right;
			p->right = temp;
			break;
			//RR
		case 3:
			//printf("%d do %d Rotation!\n", dir, p->right->operation);
			p->right->operation = 0;
			temp = p->right->right;
			p->right->right = temp->left;
			temp->left = p->right;
			p->right = temp;
			break;
			//RL
		case 4:
			//printf("%d do %d Rotation!\n", dir, p->right->operation);
			p->right->operation = 0;
			temp = p->right->right->left;
			p->right->right->left = temp->right;
			temp->right = p->right->right;
			p->right->right = temp->left;
			temp->left = p->right;
			p->right = temp;
			break;
		default:
			printf("right rotation number is error!%d\n", p->right->operation);
			break;
		}
	}
	return p->right;
}
////////////////////////////////////////////////////////////////////////////////////
int count_height(btrie p, struct  ENTRY new_node)
{
	if (p == NULL)
		return 0;
	int max_left_height = count_height(p->left, new_node);
	int max_right_height = count_height(p->right, new_node);
	int balance = max_left_height - max_right_height;

	//do rotation operation
	if (p->left != NULL && p->left->operation != 0)
		do_rotation(p, 0);

	if (p->right != NULL && p->right->operation != 0)
		do_rotation(p, 1);

	if (balance > 1) 
	{
		//LL
		if (p->left->right == NULL)
			p->operation = 1;
		//LR
		else
			p->operation = 2;
	}
	else if (balance < -1) 
	{
		//RR
		if (p->right->left == NULL)
			p->operation = 3;
		//RL
		else
			p->operation = 4;
	}
	return max_left_height > max_right_height ? max_left_height + 1 : max_right_height + 1;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(struct ENTRY new_node) {
	//use to store prefix with this node's ip and current pointer ip
	int ip_dontcare;
	int current_ip_dontcare;
	btrie current = root[0];
	//start on layer-0
	for (int i = 0; i < 32;)
	{
		if (current->node_info.ip == 0)
		{
			current->node_info = new_node;
			//插入要做旋轉檢查
			count_height(root[i], new_node);
			//root need rotation case
			if (root[i]->operation != 0)
			{
				btrie temp = (btrie)malloc(sizeof(node));
				temp->right = root[i];
				root[i] = do_rotation(temp, 1);
			}
			break;
		}
		//A是搜尋點，Ｂ是被搜尋點
		//1. 若Ａ要包含Ｂ，Ａ的長度一定比Ｂ短，先檢查長度再檢查匹配
		//2. 若Ｂ包含Ａ，則要往下搜尋
		//first case
		if (current->node_info.len >= new_node.len)
		{
			ip_dontcare = (new_node.ip >> (32 - new_node.len));
			current_ip_dontcare = (current->node_info.ip >> (32 - new_node.len));
			if (ip_dontcare == current_ip_dontcare)
			{
				//goto next layer to find
				i++;
				current = root[i];
			}
			else if (ip_dontcare > current_ip_dontcare)
			{
				if (current->right == NULL)
					current->right = create_node();
				current = current->right;
			}
			else
			{
				if (current->left == NULL)
					current->left = create_node();
				current = current->left;
			}
		}
		//second case
		else if (current->node_info.len < new_node.len)
		{
			ip_dontcare = (new_node.ip >> (32 - current->node_info.len));
			current_ip_dontcare = (current->node_info.ip >> (32 - current->node_info.len));
			if (ip_dontcare == current_ip_dontcare)
			{
				//挪動當前點到下一個layer
				i++;
				struct ENTRY temp = current->node_info;
				//覆蓋過這個點
				current->node_info = new_node;
				new_node = temp;
				current = root[i];
			}
			else if (ip_dontcare > current_ip_dontcare)
			{
				if (current->right == NULL)
					current->right = create_node();
				current = current->right;
			}
			else
			{
				if (current->left == NULL)
					current->left = create_node();
				current = current->left;
			}
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
void search(unsigned int ip) {
	int target = -1;
	int ip_dontcare;
	int current_ip_dontcare;
	btrie current = root[0];

	for (int layer = 0; layer < 32;) 
	{
		if (root[layer] == NULL)
			break;
		if (current == NULL)
		{
			layer++;
			current = root[layer];
		}
		else {
			//先和目前的點比較
			ip_dontcare = ip >> (32 - current->node_info.len);
			current_ip_dontcare = (current->node_info.ip >> (32 - current->node_info.len));
			if (ip_dontcare == current_ip_dontcare)
			{
				target = current->node_info.port;
				break;
			}
			//再和左右子點比較
			else if (ip_dontcare > current_ip_dontcare)
				current = current->right;
			else
				current = current->left;
		}
	}

	//printf("search number:%d\n", target);
}

////////////////////////////////////////////////////////////////////////////////////
void create() {
	int i;
	begin = rdtsc();
	//每一個layer都先initial root
	for (i = 0; i < 32; i++)
		root[i] = create_node();
	//insert every node
	for (i = 0; i < num_entry; i++)
	{
		add_node(table[i]);
		if (i % 50000 == 0)
			printf("builded : %d\n", i);
	}
	end = rdtsc();
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
	clock = (unsigned long long int *)malloc(num_input * sizeof(unsigned long long int));//用來存放每輪時間
	num_input = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input].len = len;
		clock[num_input++] = 10000000;
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
	//char filename[50] = "5k_data.txt";
	set_table(argv[1]);
	set_query(argv[2]);
	
	create();

	printf("Avg. Build: %llu\n", (end - begin) / num_entry);
	printf("number of nodes: %d\n", num_node);
	printf("Total memory requirement: %d KB\n", ((num_node * sizeof(btrie)) / 1024));

	shuffle(query, num_entry);//洗亂query順序
	////////////////////////////////////////////////////////////////////////////
	//連續做100次Search取平均時間
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
	CountClock();//計算耗費最多時間，最少時間，計算所需cycle數的分佈情況
	////////////////////////////////////////////////////////////////////////////
	for (int i = 0; i < 32; i++)
	{
		N = 0;
		count_node(root[i]);
		printf("There are %d nodes in layer-%d trie\n", N, i);
	}
	
	set_input(argv[3]);
	begin = 0, end = 0;
	begin = rdtsc();
	for (int i = 0; i < num_input; i++)
		add_node(input[i]);
	end = rdtsc();
	printf("Avg. insert Time: %llu\n", (end - begin) / num_input);
	return 0;
}
