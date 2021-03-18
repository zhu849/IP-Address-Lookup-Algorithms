#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>


////////////////////////////////////////////////////////////////////////////////////
struct ENTRY {
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};
/////////////////////**********golbal varible***********////////////////////////////
struct ENTRY *table;
int num_entry = 0;
int counter[33] = {0};
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str, unsigned int *ip, int *len, unsigned int *nexthop) {
	char tok[] = "./";//token use to split 
	char buf[100], *str1;
	unsigned int n[4];
	//split first four ip
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
	//input prefix length's long
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
	//input ip value
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
void count_distribute() {
	int i;
	int index;
	for (i = 0; i < num_entry; i++) {
		index = table[i].len;
		counter[index]++;
	}
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
	int i, j;
	char filename[50] = "rrc_all_update.txt";
	set_table(filename);
	count_distribute();
	printf("distrbute status:\n");
	for (i = 0; i < 35; i++)
		printf("i:%d = %d\n", i, counter[i]);
	return 0;
}
