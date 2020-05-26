#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <string.h> 
#include <dirent.h>
#include <unistd.h>
#include "md5_ssl.h"

typedef struct{ 
    uint32_t number_commit; 
    uint32_t number_file; 
    uint32_t number_add; 
    uint32_t number_modify; 
    uint32_t number_copy; 
    uint32_t number_del; 
    uint32_t commit_size; 
}Commit_header; 

void INITCommit_header(Commit_header *target){
    target->number_commit=0;
    target->number_file=0;
    target->number_add=0;
    target->number_modify=0;
    target->number_copy=0;
   	target->number_del=0;
    target->commit_size=0;
}

typedef struct node{
	char filename[256];
	uint8_t *file_md5;
	struct node *next;
	struct node *prev;
}file_listNode;


file_listNode *INIT_filelist();
file_listNode *addNext_(file_listNode *node, char *directname, char *filename);
void DEL_filelist(file_listNode *head);


void status(char* directname);
void commit(char* filename);
void log_MP1(int num, char* filename);
void test(char* filename);

int main(int argc, char *argv[]){
	if(argc < 2 || strcmp(argv[1], "--help")==0){
		fprintf(stderr, "usage: ./loser command directory\n");
	}

	if(strcmp("log", argv[1])==0)
		log_MP1(atoi(argv[2]), argv[3]);
	else if(strcmp("status", argv[1])==0){
		status(argv[2]);
	}
	else if(strcmp("commit", argv[1])==0){
		commit(argv[2]);
	}
	else if(strcmp("test", argv[1])==0){
		test(argv[2]);
	}
}

file_listNode *INIT_filelist(){
	return NULL;
}
file_listNode *addNext(file_listNode *head, char *directname, char *filename){
	char *filename_buffer = malloc(256);
	memset(filename_buffer, '\0', 256);
	strcat(filename_buffer, directname);
	filename_buffer[strlen(filename_buffer)] = '/';
	strcat(filename_buffer, filename);
	//fprintf(stderr, "in the addPrev, filename=%s\n", filename_buffer);

	file_listNode *newnode = (file_listNode*)malloc(sizeof(file_listNode));
	memset(newnode->filename, '\0', 256);
	
	if(head==NULL){
		newnode->next = newnode;
		newnode->prev = newnode;
		head=newnode;
	}
	else{
		newnode->prev = head->prev;
		newnode->next = head;
		head->prev->next = newnode;
		head->prev = newnode;
	}
	/*single linked list
	newnode->next = head;
	head=newnod;
	*/
	strcat(newnode->filename, filename);
	newnode->file_md5 = md5_ssl(filename_buffer);
	free(filename_buffer);
	//fprintf(stderr, "%s\n", newnode->filename);
	return head;
}

void DEL_filelist(file_listNode *head){
	head->prev->next = NULL;
	for(file_listNode *ptr=head; ptr!=NULL; ptr=ptr->next){
		free(ptr);
	}
}

void log_MP1(int num, char *filename){
	char *filename_buffer = malloc(256);
	strcat(filename_buffer, filename);
	strcat(filename_buffer, "/.loser_record");

    FILE *fp = fopen(filename_buffer, "rb");
    if(fp==NULL){ 
        perror("error");
		return;
    }
	free(filename_buffer);

	Commit_header FileLog_read;
	for(int i=0; i<num; i++){
		if(fread(&FileLog_read, sizeof(Commit_header), 1, fp)==0)
			break;
		fprintf(stdout, "#commit %u\n", FileLog_read.number_commit);
			
		fprintf(stdout, "[new_file]\n");
        for(uint32_t i=0; i<FileLog_read.number_add; i++){ 
            uint8_t filename_size; 
            char name[256];
			memset(name, '\0', 256);
            fread(&filename_size, sizeof(uint8_t), 1, fp); 
            fread(name, filename_size, 1, fp); 
            fprintf(stdout, "%s\n", name);
        }
		fprintf(stdout, "[modified]\n");
        for(uint32_t i=0; i<FileLog_read.number_modify; i++){ 
            uint8_t filename_size; 
            char name[256];
            fread(&filename_size, sizeof(uint8_t), 1, fp); 
            fread(name, filename_size, 1, fp); 
            fprintf(stdout, "%s\n", name); 
        }
		fprintf(stdout, "[copied]\n");
        for(uint32_t i=0; i<FileLog_read.number_copy; i++){ 
            uint8_t filename_size; 
            char name1[256], name2[256];
			memset(name1, '\0', 256);
			memset(name2, '\0', 256);	

            fread(&filename_size, sizeof(uint8_t), 1, fp); 
            fread(name1, filename_size, 1, fp); 
            fread(&filename_size, sizeof(uint8_t), 1, fp); 
            fread(name2, filename_size, 1, fp); 
            fprintf(stdout, "%s=>%s\n", name1, name2); 
        }
		fprintf(stdout, "[deleted]\n");
        for(uint32_t i=0; i<FileLog_read.number_del; i++){ 
            uint8_t filename_size; 
            char name[256];
            fread(&filename_size, sizeof(uint8_t), 1, fp); 
            fread(name, filename_size, 1, fp); 
	        fprintf(stdout, "%s\n", name);
        }
		fprintf(stdout, "(MD5)\n");
        for(uint32_t i=0; i<FileLog_read.number_file; i++){
            uint8_t filename_size;
            char name[256];
            uint8_t *file_md5 = malloc(sizeof(uint8_t)*16);
            fread(&filename_size, sizeof(uint8_t), 1, fp);
            fread(name, filename_size, 1, fp);
            fread(file_md5, sizeof(uint8_t), 16, fp);
            fprintf(stdout, "%s ", name);
            for(int i=0; i<16; i++){
                if(i==15)
                    fprintf(stdout, "%2.2x\n", file_md5[i]);
                else    
                    fprintf(stdout, "%2.2x", file_md5[i]);
            }
            free(file_md5);
        }
		if(i==num-1);
		else
			printf("\n");
	}
}
int md5_check(uint8_t *md5_prev, uint8_t *md5_curr){
	for(int i=0; i<16; i++){
		if(md5_prev[i]!=md5_curr[i])
			return 0;
	}
	return 1;
}

int status_newfile(file_listNode *prev_dir, char *curr_filename, uint8_t *curr_md5){
	/*for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
		if(strcmp(ptr->filename, curr_filename)==0 || 
				md5_check(ptr->file_md5, curr_md5))
			return 0;
	}*/
	file_listNode *ptr=prev_dir;
	do{
		if(strcmp(ptr->filename, curr_filename)==0||md5_check(ptr->file_md5, curr_md5))
			return 0;
		ptr=ptr->next;
	}
	while(ptr!=prev_dir);
	return 1;
}
int status_modified(file_listNode *prev_dir, char *curr_filename, uint8_t *curr_md5){
	/*for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
		if(strcmp(ptr->filename, curr_filename)==0 && 
				md5_check(prev_dir->file_md5, md5)==0)
			return 1;
	}*/
	file_listNode *ptr=prev_dir;
	do{
		if(strcmp(ptr->filename, curr_filename)==0
				&&md5_check(ptr->file_md5, curr_md5)==0)
			return 1;
		ptr=ptr->next;
	}
	while(ptr!=prev_dir);
	return 0;
}

int status_oldfile(file_listNode *prev_dir, char *curr_filename){
	/*for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
		if(strcmp(ptr->filename, curr_filename)==0)
			return 1;
	}*/
	file_listNode *ptr=prev_dir;
	do{
		if(strcmp(ptr->filename, curr_filename)==0)
			return 1;
		ptr=ptr->next;
	}
	while(ptr!=prev_dir);
	return 0;
}
int status_copied(file_listNode *prev_dir, char *curr_filename, uint8_t *curr_md5){
	//int result=0;
	//file_listNode *temp = NULL;
	if(status_oldfile(prev_dir, curr_filename))
		return 0;
	/*
	for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
		if(md5_check(ptr->file_md5, md5) &&
				strcmp(ptr->filename, curr_filename)!=0){
			result=1;
			temp = ptr;
			//fprintf(stderr, "debug: in copied filename=%s\n", temp->filename);
		}
	}*/
	file_listNode *ptr=prev_dir;
	do{
		if(md5_check(ptr->file_md5, curr_md5)&&strcmp(ptr->filename, curr_filename)!=0){
			fprintf(stdout, "%s", ptr->filename);
			return 1;
		}
			/*result=1;
			temp=ptr;*/
		ptr=ptr->next;
	}
	while(ptr!=prev_dir);
	/*if(temp!=NULL)
		fprintf(stdout, "%s", temp->filename);*/
	return 0;
}
int status_del(file_listNode *curr_dir, char *prev_filename){
	/*for(file_listNode *ptr=curr_dir; ptr!=NULL; ptr=ptr->next){
		if(strcmp(ptr->filename, prev_filename)==0)
			return 0;
	}
	*/
	file_listNode *ptr=curr_dir;
	do{
		if(strcmp(ptr->filename, prev_filename)==0)
			return 0;
		ptr=ptr->next;
	}
	while(ptr!=curr_dir);
	return 1;
}

void status(char *directname){
	struct dirent **namelist;
	int n;
	n = scandir(directname, &namelist, NULL, alphasort);
	if(n==-1){
		perror("scandir");
		return;
	}
	
	file_listNode *curr_dir = INIT_filelist();
	for(int i=0; i<n; i++){
		/*char *filename = malloc(256);
		strcat(filename, directname);
		filename[strlen(filename)] = '/';
		strcat(filename, namelist[i]->d_name);*/
		
		if(strcmp(namelist[i]->d_name, ".")==0 || strcmp(namelist[i]->d_name, "..")==0 
				|| strcmp(namelist[i]->d_name, ".loser_record")==0);
		else{
			curr_dir = addNext(curr_dir, directname, namelist[i]->d_name);
		}
		//free(filename);
		free(namelist[i]);
	}
	free(namelist);
	/*for(file_listNode *ptr = curr_dir; ptr!=NULL; ptr=ptr->next){
		fprintf(stdout, "debug: currlist filename=%s\n", ptr->filename);
		fprintf(stdout, "debug: currlist md5=");
		for(int i=0; i<16; i++){
			if(i==15)
				fprintf(stdout, "%02x\n", ptr->file_md5[i]);
			else
				fprintf(stdout, "%02x", ptr->file_md5[i]);
		}
	}*/
	
	char *filename_buffer = malloc(256);
	strcat(filename_buffer, directname);
	strcat(filename_buffer, "/.loser_record");
	FILE *loser_in = fopen(filename_buffer, "rb");
	if(loser_in ==NULL){
		perror("error");
		return;
	}
	free(filename_buffer);
	Commit_header CommitLog;
	int32_t commit_size=0 , commit_size_tmp = 0;
	while(1){
		if(fread(&CommitLog, sizeof(Commit_header), 1, loser_in)==0){
			fseek(loser_in, (commit_size_tmp*-1)+sizeof(Commit_header), SEEK_END);
			//fprintf(stderr, "in the loop i=%d\n", commit_size);
			break;
		}
		else{
			commit_size_tmp = CommitLog.commit_size;
			commit_size += commit_size_tmp;
			fseek(loser_in, commit_size, SEEK_SET);
		}
	}
	//fseek(loser_in, sizeof(Commit_header), SEEK_CUR);
	//file_listNode *prev_dir = INIT_filelist();
	for(uint32_t i=0; i<CommitLog.number_add; i++){
		uint8_t filename_size;
		fread(&filename_size, sizeof(uint8_t), 1, loser_in);
		fseek(loser_in, filename_size, SEEK_CUR);
	}
    for(uint32_t i=0; i<CommitLog.number_modify; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
    }
    for(uint32_t i=0; i<CommitLog.number_copy; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
		fread(&filename_size, sizeof(uint8_t), 1, loser_in);
		fseek(loser_in, filename_size, SEEK_CUR);
    }
    for(uint32_t i=0; i<CommitLog.number_del; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
    }
	file_listNode *prev_dir = INIT_filelist();
	for(uint32_t i=0; i<CommitLog.number_file; i++){
		file_listNode *newnode = malloc(sizeof(file_listNode));
		newnode->file_md5 = malloc(sizeof(uint8_t)*16);
		memset(newnode->filename, '\0', 256);
		uint8_t filename_size;
		fread(&filename_size, sizeof(uint8_t), 1, loser_in);
		fread(newnode->filename, filename_size, 1, loser_in);
	   	fread(newnode->file_md5, sizeof(uint8_t), 16, loser_in);
		
		if(prev_dir==NULL){
			newnode->prev=newnode;
			newnode->next=newnode;
			prev_dir=newnode;
		}
		else{
			newnode->prev=prev_dir->prev;
			newnode->next=prev_dir;
			prev_dir->prev->next=newnode;
			prev_dir->prev=newnode;
		}
	}
	/*for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
		fprintf(stderr, "debug: prevlist filename=%s\n", ptr->filename);
		fprintf(stderr, "debug: prevlist file_md5=");
		for(int i=0; i<16; i++){
			if(i==15){
				fprintf(stderr, "%02x\n", ptr->file_md5[i]);
			}
			else
				fprintf(stderr, "%02x", ptr->file_md5[i]);
		}
	}*/
	fprintf(stdout, "[new_file]\n");
	file_listNode *ptr=curr_dir;
	/*for(file_listNode *ptr=curr_dir; ptr!=NULL; ptr=ptr->next){
		if(status_newfile(prev_dir, ptr->filename, ptr->file_md5)){
			fprintf(stdout, "%s\n", ptr->filename);
		}
	}*/
	do{
		if(status_newfile(prev_dir, ptr->filename, ptr->file_md5)){
			fprintf(stdout, "%s\n", ptr->filename);
		}
		ptr=ptr->next;	
	}
	while(ptr!=curr_dir);
	fprintf(stdout, "[modified]\n");
	ptr=curr_dir;
	/*
    for(file_listNode *ptr=curr_dir; ptr!=NULL; ptr=ptr->next){
        if(status_modified(prev_dir, ptr->filename, ptr->file_md5))
            fprintf(stdout, "%s\n", ptr->filename);
    }
	*/
	do{
		if(status_modified(prev_dir, ptr->filename, ptr->file_md5))
            fprintf(stdout, "%s\n", ptr->filename);
		ptr=ptr->next;
	}
	while(ptr!=curr_dir);
	fprintf(stdout, "[copied]\n");
	ptr=curr_dir;
	/*
    for(file_listNode *ptr=curr_dir; ptr!=NULL; ptr=ptr->next){
        if(status_copied(prev_dir, ptr->filename, ptr->file_md5))
            fprintf(stdout, "=>%s\n", ptr->filename);
    }
	*/
	do{
       if(status_copied(prev_dir, ptr->filename, ptr->file_md5))
           fprintf(stdout, "=>%s\n", ptr->filename);
	   ptr=ptr->next;
	}
	while(ptr!=curr_dir);
	fprintf(stdout, "[deleted]\n");
	ptr=prev_dir;
	/*
    for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
        if(status_del(curr_dir, ptr->filename))
            fprintf(stdout, "%s\n", ptr->filename);
    }*/
	do{
        if(status_del(curr_dir, ptr->filename))
            fprintf(stdout, "%s\n", ptr->filename);
		ptr=ptr->next;	
	}
	while(ptr!=prev_dir);
	DEL_filelist(prev_dir);
	DEL_filelist(curr_dir);
}

int commit_copy(file_listNode *prev_dir, char *curr_filename, uint8_t *curr_md5, FILE *fp){
    //int result=0;
    //file_listNode *temp = NULL;
    if(status_oldfile(prev_dir, curr_filename))
        return 0;
    /*
    for(file_listNode *ptr=prev_dir; ptr!=NULL; ptr=ptr->next){
        if(md5_check(ptr->file_md5, md5) &&
                strcmp(ptr->filename, curr_filename)!=0){
            result=1;
            temp = ptr;
            //fprintf(stderr, "debug: in copied filename=%s\n", temp->filename);
        }
    }*/
    file_listNode *ptr=prev_dir;
    do{
        if(md5_check(ptr->file_md5, curr_md5)&&strcmp(ptr->filename, curr_filename)!=0){
            uint8_t filesize = strlen(ptr->filename);
			fwrite(&filesize, sizeof(uint8_t), 1, fp);
			fwrite(ptr->filename, filesize, 1, fp);
            //fprintf(stderr, "%s=>", ptr->filename);
            return 1;
        }   
            /*result=1;
            temp=ptr;*/
        ptr=ptr->next;
    }   
    while(ptr!=prev_dir);
    /*if(temp!=NULL)
        fprintf(stdout, "%s", temp->filename);*/
    return 0;
}

void commit(char *directname){
	struct dirent **namelist;
    int n = scandir(directname, &namelist, NULL, alphasort);
    if(n==-1){
        perror("scandir");
        return;
    }
    file_listNode *curr_dir = INIT_filelist();
    for(int i=0; i<n; i++){
        if(strcmp(namelist[i]->d_name, ".")==0 || strcmp(namelist[i]->d_name, "..")==0
                || strcmp(namelist[i]->d_name, ".loser_record")==0);
        else{
            curr_dir = addNext(curr_dir, directname, namelist[i]->d_name);
        }
        //free(filename);
        free(namelist[i]);
    }
    free(namelist);

	Commit_header CURR_CommitHead;
	INITCommit_header(&CURR_CommitHead);
	CURR_CommitHead.number_file = (uint32_t)n-3;
    
	char *filename_buffer = malloc(256);
    strcat(filename_buffer, directname);
    strcat(filename_buffer, "/.loser_record");
    FILE *loser_in = fopen(filename_buffer, "rb");
    if(loser_in ==NULL){
        perror("error");
        return;
    }

	Commit_header CommitLog;
	INITCommit_header(&CommitLog);
    int32_t commit_size=0 , commit_size_tmp = 0;
    while(1){
        if(fread(&CommitLog, sizeof(Commit_header), 1, loser_in)==0){
            fseek(loser_in, (commit_size_tmp*-1)+sizeof(Commit_header), SEEK_END);
            //fprintf(stderr, "in the loop i=%d\n", commit_size);
            break;
        }
        else{
            commit_size_tmp = CommitLog.commit_size;
            commit_size += commit_size_tmp;
            fseek(loser_in, commit_size, SEEK_SET);
        }
    }
	CURR_CommitHead.number_commit = CommitLog.number_commit+1;
    //fseek(loser_in, sizeof(Commit_header), SEEK_CUR);
    //file_listNode *prev_dir = INIT_filelist();
    for(uint32_t i=0; i<CommitLog.number_add; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
    }
    for(uint32_t i=0; i<CommitLog.number_modify; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
    }
    for(uint32_t i=0; i<CommitLog.number_copy; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
    }
    for(uint32_t i=0; i<CommitLog.number_del; i++){
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fseek(loser_in, filename_size, SEEK_CUR);
    }
    file_listNode *prev_dir = INIT_filelist();
    for(uint32_t i=0; i<CommitLog.number_file; i++){
        file_listNode *newnode = malloc(sizeof(file_listNode));
        newnode->file_md5 = malloc(sizeof(uint8_t)*16);
        memset(newnode->filename, '\0', 256);
        uint8_t filename_size;
        fread(&filename_size, sizeof(uint8_t), 1, loser_in);
        fread(newnode->filename, filename_size, 1, loser_in);
        fread(newnode->file_md5, sizeof(uint8_t), 16, loser_in);

        if(prev_dir==NULL){
            newnode->prev=newnode;
            newnode->next=newnode;
            prev_dir=newnode;
        }
        else{
            newnode->prev=prev_dir->prev;
            newnode->next=prev_dir;
            prev_dir->prev->next=newnode;
            prev_dir->prev=newnode;
        }
    }
    fclose(loser_in);
	FILE *loser_out = fopen(filename_buffer, "ab+");
	if(loser_out==NULL){
		perror("error");
		return;
	}


	fwrite("\0", 1, sizeof(Commit_header), loser_out);
	//fprintf(stderr, "write number:%d\n", wsize);
	CURR_CommitHead.commit_size += 28;
	
	uint8_t filesize;
	file_listNode *ptr=curr_dir;
	//fprintf(stderr, "new\n");
    do{
        if(status_newfile(prev_dir, ptr->filename, ptr->file_md5)){
            CURR_CommitHead.number_add++;
			CURR_CommitHead.commit_size += strlen(ptr->filename)+sizeof(uint8_t);
			filesize = strlen(ptr->filename);
			fwrite(&filesize, sizeof(uint8_t), 1, loser_out);
			//fprintf(stderr, "write number newfile: %d\n", wsize);
			fwrite(ptr->filename, filesize, 1, loser_out);
			//fprintf(stderr, "write number newfile: %d\n", wsize);
			/*fprintf(stderr, "%u\n", CURR_CommitHead.number_add);
			fprintf(stderr, "%u\n", filesize);
			fprintf(stderr, "%s\n", ptr->filename);*/
        }
        ptr=ptr->next;
    }
    while(ptr!=curr_dir);
	//fprintf(stderr, "modify\n");
    ptr=curr_dir;
    do{
        if(status_modified(prev_dir, ptr->filename, ptr->file_md5)){
			CURR_CommitHead.number_modify++;
			CURR_CommitHead.commit_size += strlen(ptr->filename)+sizeof(uint8_t);
			filesize = strlen(ptr->filename);
			fwrite(&filesize, sizeof(uint8_t), 1, loser_out);
			fwrite(ptr->filename, filesize, 1, loser_out);
            /*fprintf(stderr, "%u\n", CURR_CommitHead.number_modify);
            fprintf(stderr, "%u\n", filesize);
            fprintf(stderr, "%s\n", ptr->filename);*/
		}
        ptr=ptr->next;
    }
    while(ptr!=curr_dir);
	
	//fprintf(stderr, "copy\n");
    ptr=curr_dir;
    do{
       if(commit_copy(prev_dir, ptr->filename, ptr->file_md5, loser_out)){
	   		CURR_CommitHead.number_copy++;
			CURR_CommitHead.commit_size += (strlen(ptr->filename)+sizeof(uint8_t))*2;
			filesize = strlen(ptr->filename);
			fwrite(&filesize, sizeof(uint8_t), 1, loser_out);
			fwrite(ptr->filename, filesize, 1, loser_out);
            //fprintf(stderr, "%s\n", ptr->filename);
	   }
       ptr=ptr->next;
    }
    while(ptr!=curr_dir);

	//fprintf(stderr, "del\n");
    ptr=prev_dir;
    do{
        if(status_del(curr_dir, ptr->filename)){
            CURR_CommitHead.number_del++;
			CURR_CommitHead.commit_size+=strlen(ptr->filename)+sizeof(uint8_t);
			filesize = strlen(ptr->filename);
			fwrite(&filesize, sizeof(uint8_t), 1, loser_out);
			fwrite(ptr->filename, filesize, 1, loser_out);
            /*fprintf(stderr, "%u\n", CURR_CommitHead.number_del);
            fprintf(stderr, "%u\n", filesize);
            fprintf(stderr, "%s\n", ptr->filename);*/
		}
        ptr=ptr->next;
    }
    while(ptr!=prev_dir);

    DEL_filelist(prev_dir);
	

	ptr=curr_dir;
	for(uint8_t i=0; i<CURR_CommitHead.number_file; i++){
		filesize=strlen(ptr->filename);
		fwrite(&filesize, sizeof(uint8_t), 1, loser_out);
		fwrite(ptr->filename, sizeof(uint8_t), 1, loser_out);
		fwrite(ptr->file_md5, sizeof(uint8_t), 16, loser_out);
		/*for(int j=0; j<16; j++){
			if(j==15)
				fprintf(stderr, "%02x\n", ptr->file_md5[j]);
			else
				fprintf(stderr, "%02x", ptr->file_md5[j]);
		}*/
		CURR_CommitHead.commit_size += 18;
		ptr=ptr->next;
	}
    DEL_filelist(curr_dir);
	//fprintf(stderr, "old commit:%d\n", commit_size);
	fclose(loser_out);
	loser_out = fopen(filename_buffer, "rb+");
	if(loser_out==NULL){
		perror("error");
		return;
	}
	fseek(loser_out, commit_size, SEEK_SET);
	fwrite(&CURR_CommitHead, sizeof(Commit_header), 1, loser_out);
	fclose(loser_out);
	/*fprintf(stderr, "debug: new commit log\n%u\n%u\n%u\n%u\n%u\n%u\n%u\n",
            CURR_CommitHead.number_commit, CURR_CommitHead.number_file
            , CURR_CommitHead.number_add, CURR_CommitHead.number_modify
            , CURR_CommitHead.number_copy, CURR_CommitHead.number_del
            , CURR_CommitHead.commit_size);*/
	free(filename_buffer);
}

void test(char* filename){
		char *filename_buffer = malloc(256);
	 	strcat(filename_buffer, filename);
	 	strcat(filename_buffer, "/.loser_record");
		FILE *fp = fopen(filename_buffer, "rb");
   	 	if(fp==NULL){
        	perror("error");
        	return;
    	}
    	free(filename_buffer);

    	Commit_header FileLog_read;
        fread(&FileLog_read, sizeof(Commit_header), 1, fp);
		fprintf(stderr, "%u\nnumber of commit: %u\n", FileLog_read.commit_size, 
				FileLog_read.number_commit);
		fseek(fp, FileLog_read.commit_size, SEEK_SET);
		fread(&FileLog_read, sizeof(Commit_header), 1, fp);
		fprintf(stderr, "number of commit: %u\n", FileLog_read.number_commit);
}
