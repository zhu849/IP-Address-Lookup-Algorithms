#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define READ_STR_BUF 100

struct ENTRY {
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};
struct ENTRY *table;

//***global varible***

int num_entry;
int counter[33] = {0};//initialize with 0

//split one line and reassembly with ip 
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";
	char buf[READ_STR_BUF],*str1;
	unsigned int n[4];//store ip value
	sprintf(buf,"%s\0",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[3]=atoi(buf);
	*nexthop=n[2];//nexthop = n[2] just mean random nexthop value
	//deal with prefix length
	str1=(char *)strtok(NULL,tok);
	if(str1!=NULL){
		sprintf(buf,"%s\0",str1);
		*len=atoi(buf);
	}
	else{//exception situation
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
	//assign to ip with correct format
	*ip=n[0];
	*ip<<=8;
	*ip+=n[1];
	*ip<<=8;
	*ip+=n[2];
	*ip<<=8;
	*ip+=n[3];
}

void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp=fopen(file_name,"r");
	//count wanted table size
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string, &ip, &len, &nexthop);
		num_entry++;
	}

	rewind(fp);
	//allocate table space
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	//get table info
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
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

int main(int argc, char *argv[]) {
	int i, j;
	set_table(argv[1]);
	count_distribution();
	printf("Distrbute status:\n");
	for (i = 0; i < 33; i++)
		printf("prefix length:%d = %d\n", i, counter[i]);
	return 0;
}
