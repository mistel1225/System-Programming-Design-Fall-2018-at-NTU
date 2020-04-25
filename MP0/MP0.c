#include <stdio.h>

int main(int argc, char* argv[]){
	unsigned char char_set[128];
	FILE *fp;
	if(argc == 3){
		int i=0;
		while(argv[1][i]!='\0'){
			char_set[(int)argv[1][i]] = 1;
			i++;
		}
		fp = fopen(argv[2], "r");
		if(fp == NULL){
			fprintf(stderr, "error\n");
			return 0;
		}
	}
	
	int c;
	int count = 0;
	while((c = fgetc(fp)) !=EOF){
		if(c=='\n'){
			printf("%d\n", count);
			count = 0;
		}
		else if(char_set[c]==1)
			count++;
	}
	return 0;
}
