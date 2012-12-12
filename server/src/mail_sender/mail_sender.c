#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#define STR_MAX 256
#define MAIL_FILE "../mail/mail.content"

void putHTML(FILE*fp, char* str)
{
	if(fp) fputs(str, fp);
}
void check_create_dir(char* path)
{
	DIR* dp;
	// char log_buf[MAX];

	if((dp = opendir(path)) == NULL){
		mkdir(path, 0744);
	 }
	if(dp) closedir(dp);
}

void getWeekDay(char* date, char* day)
{
	struct tm now_tm;
	char buf[4];
	system("export LANG=en");
	memset(&now_tm, 0, sizeof(now_tm));
	strptime(date, "%Y-%m-%d", &now_tm);
	strftime(buf, sizeof(buf), "%a", &now_tm);
	strcpy(day, buf);
}


int main(int argc, char* argv[])
{
	char user[STR_MAX];
	char clientIP[STR_MAX];
	char date[STR_MAX];
	char buf[STR_MAX];
	char tmp[STR_MAX];
	char day[4];
	FILE* fp;
	FILE* fp2;

	if(argc != 4){
		printf("argument error. ./mail_sender user clientip date");
		exit(1);
	}
	strcpy(user, argv[1]);
	strcpy(clientIP, argv[2]);
	strcpy(date, argv[3]);


	check_create_dir("../mail");
	getWeekDay(date, day);
	if((fp = fopen(MAIL_FILE, "w")) == NULL){
		perror("fopen");
		exit(1);
	}

	sprintf(buf, "/tmp/backup/%s/%s/backup.%s.log", user, day, date);
	//printf("%s\n", buf);
	if((fp2 = fopen(buf, "r")) == NULL){
		perror("fopen");
		exit(1);
	}

	sprintf(buf, "<H1>The backup of client-(%s, %s) is completed</H1>", user, clientIP);
	putHTML(fp, buf);
	putHTML(fp, "<b>The backup log: </b>");
	putHTML(fp, "<ul>");

	while(fgets(buf, sizeof(buf), fp2)){
		sprintf(tmp, "<li>%s</li>", buf);
		putHTML(fp, tmp);
	}

	putHTML(fp, "</ul>");
	sprintf(buf, "<b>You can check the log at <a href='http://210.117.182.158:8088/%s/log/%s.html'>Here.</a></b>", user, date);
	putHTML(fp, buf);

	if(fp) fclose(fp);
	if(fp2) fclose(fp2);
	return 0;
}
