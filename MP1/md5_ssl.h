#include <stdlib.h>
#include <stdio.h>
#include <openssl/md5.h>


uint8_t* md5_ssl(char *filename)
{
	uint8_t *c = malloc(MD5_DIGEST_LENGTH);
    int i;
    FILE *inFile = fopen (filename, "rb");
	if(inFile==NULL){
		perror("error");
		return 0;
	}
    MD5_CTX mdContext;
    int bytes;
    uint8_t data[1024];

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
        return 0;
    }

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    
	fclose(inFile);
	return c;
}
