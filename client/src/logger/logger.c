#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define MAX 1024
#define DATE_MAX 21

int main(int argc, char* argv[])
{
	int fd;
	char* TAG, *LOG, *FILE_PATH;
	char DATE[DATE_MAX];
	char buf[MAX];
	time_t t;
	struct tm *tm;

	/**
	* Log form
	* [2000-10-10 10:10:10] [TAG] LOG. > To File
	* arg : TAG, LOG, FILE_PATH
	*/
	if(argc != 4){
		fprintf(stderr, "Argument error : logger TAG LOG FILEPATH");
		exit(1);
	}
	
	TAG = argv[1];
	LOG = argv[2];
	FILE_PATH = argv[3];

	if((fd = open(FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0600)) < 0){
		perror("open");
		exit(1);
	}
	
	time(&t);
	tm = localtime(&t);

	strftime(DATE, sizeof(DATE), "%F %T", tm);

	sprintf(buf, "[%s] [%s] %s\n", DATE, TAG, LOG);

	if(write(fd, buf, strlen(buf)) < 0){
		perror("write");
		exit(1);
	}

	close(fd);

	return 0;
}
