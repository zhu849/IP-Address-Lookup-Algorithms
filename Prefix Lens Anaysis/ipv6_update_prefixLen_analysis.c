#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define READ_STR_BUF 100

struct ENTRY{
	unsigned long long int ip_upper;//64 bits
	unsigned long long int ip_lower;//64 bits
	unsigned char len;
	unsigned char port;
};
struct ENTRY *table;

/*global variables*/
int num_entry = 0;
int counter[129] = {0};//initialize with 0

//split one line and reassembly with ip 
void read_table(char *str, unsigned long long int *ip_prefix, int *len, unsigned int *nexthop) {
	int i, j = 0;
	int seg_count = 0;//record number of segment having value in one entry 
	int gap;
	char tok[] = "/";
	char tok2[] = ":";
	char buf[READ_STR_BUF];
	char temp[READ_STR_BUF];
	char *p;

	sprintf(buf, "%s\0", strtok(str, tok));
	strcpy(temp,buf);//store part of ip in temp array
	//get prefix length
	//strtok will start at last sprintf pointer
	sprintf(buf, "%s\0", strtok(NULL, tok));
	*len = atoi(buf);

	//count number of segment
	p = strtok(temp, tok2);
	while (p != NULL){
		seg_count++;
		p = strtok(NULL, tok2);
	}

	//ip expandation
	memset(temp, '\0', READ_STR_BUF);
	sprintf(buf, "%s\0", strtok(str, tok));
	char *now = buf, *next = buf;

	while (*next != '\0'){//line end
		next++;
		//check double colon, padding 0 by (8 - (# of segment))
		if (*now == ':' && *next == ':') {
			for (i = j; i < j + (8 - seg_count) * 4; i++)
				temp[i] = '0';
			j += (8 - seg_count) * 4;
			now += 2;
			next++;
		}
		else {
			while (*next != ':' && *next != '\0')//next pointer point to : position or line end
				next++;
			while (*now == ':')//now pointer point to one segment begin position
				now++;
			gap = next - now;
			//alignment by padding zero with same length of zero
			for (i = 4 - gap; i > 0; i--, j++)
				temp[j] = '0';
			for (i = 0; i < gap; i++, j++, now++)
				temp[j] = *now;
		}
	}

	//convert to byte expression
	*ip_prefix = 0;
	for (i = 0; i < 16; i++) {
		*ip_prefix <<= 4;
		*ip_prefix += (temp[i] > '9') ? (temp[i] - 'a' + 10) : (temp[i] - '0');
	}
	*nexthop = temp[5];//assign nexthop value with temp[5]
}

void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int nexthop;
	unsigned long long int ip_upper;
	unsigned long long int ip_lower;
	fp=fopen(file_name,"r");
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string,&ip_upper,&len,&nexthop);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string, &ip_upper, &len, &nexthop);
		table[num_entry].ip_upper = ip_upper;
		//table[num_entry].ip_lower = ip_lower;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}

void count_distribution() {
	int index;
	for (int i = 0; i < num_entry; i++) {
		index = table[i].len;
		counter[index]++;
	}
}

int main(int argc,char *argv[]){
	int i,j;
	set_table(argv[1]);
	count_distribution();
	printf("prefix length : # of nodes : percentage\n");
	for (i = 0; i < 129; i++)
		printf("%13d : %10d : %8.2f\n", i, counter[i], (counter[i]/(float)num_entry)*100);
	printf("Total nodes: %d\n", num_entry);
	return 0;
}
