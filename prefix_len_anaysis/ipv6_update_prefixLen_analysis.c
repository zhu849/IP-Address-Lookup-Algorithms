#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned long long int ip_upper; 
	unsigned long long int ip_lower;
	unsigned char len;
	unsigned char port;
};
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
struct ENTRY *table;
int num_entry = 0;
int counter[130] = { 0 };
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str, unsigned long long int *ip_upper, int *len, unsigned int *nexthop) {
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
	/*
	*ip_lower = 0;
	for (i = 16; i < 32; i++) {
		*ip_lower <<= 4;
		*ip_lower += (temp[i] > '9') ? (temp[i] - 'a' + 10) : (temp[i] - '0');
	}
	*/
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
		read_table(string,&ip_upper,&len,&nexthop);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string, &ip_upper, &len, &nexthop);
		table[num_entry].ip_upper = ip_upper;
		//table[num_entry].ip_lower = ip_lower;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void count_distribute() {
	int i;
	int index;
	for (i = 0; i < num_entry; i++) {
		index = table[i].len;
		counter[index]++;
	}
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	int i,j;
	char filename[50] = "rrc_ipv6_update.txt";
	set_table(filename);
	count_distribute();
	for (i = 0; i < 130; i++)
		printf("i:%d = %d\n", i, counter[i]);
	return 0;
}
