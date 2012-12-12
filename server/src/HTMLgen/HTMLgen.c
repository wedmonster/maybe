#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <time.h>

#define MAX 1024
#define HTML_LOG_PATH "../log/HTMLgen.log"
#define BACKUP_DIR "/tmp/backup"


FILE* html_fid;
FILE* list_fid;
FILE* log_fld;

void check_create_dir(char* path)
{
	DIR* dp;
//	char log_buf[MAX];

	if((dp = opendir(path)) == NULL){
		mkdir(path, 0644);
	}
	if(dp) closedir(dp);
}

void logger(char *TAG, char* LOG)
{
	char buf[MAX];
	sprintf(buf, "./logger %s %s %s", TAG, LOG, HTML_LOG_PATH);
	system(buf);
}

void putHTML(FILE*fp, char* str)
{
	if(fp) fprintf(fp, str);
}

int main(int argc, char* argv[])
{
	FILE *log_fid;
//	char *HTML_PATH, *LOG_PATH;
	char buf[MAX];
	char user[128];
	char date[11];
	char backup_log_path[MAX];
	char backup_list_path[MAX];
	char backup_html_path[MAX];
	char backup_dir[MAX];
	char day[4];
	struct tm t;

	/**
	* have to add-on logger
	* Make HTML Page from log
	* output file append ./html/user/list.html
	* output file new ./html/user/log/%Y-%m-%d.html < /tmp/backup/user/Sun/backup.2012-12-12.log
	* argument user date
	* 
	* <HTML>
		<HEAD></HEAD>
		<BODY>
					
		<BODY>
	* </HTML>
	*/
	if(argc != 3){
		fprintf(stderr, "./HTMLgen user date(Y-m-d)\n");
		//logger("ERR", "Argument error.");
		exit(1);
	}
	system("export LANG=en");
	strcpy(user, argv[1]);
	strcpy(date, argv[2]);
	
	sprintf(backup_dir, "../html");
	check_create_dir(backup_dir);
	sprintf(backup_dir, "%s/%s", backup_dir, user);
	check_create_dir(backup_dir);

	sprintf(backup_list_path, "%s/list.html", backup_dir);
	
	strptime(date, "%Y-%m-%d", &t);
	strftime(day, sizeof(day), "%a", &t);
	//printf("%s %s %s\n", user, date, day);

	sprintf(backup_log_path, "%s/%s/%s/backup.%s.log", BACKUP_DIR, user, day, date);

	sprintf(backup_dir, "%s/log", backup_dir);
	check_create_dir(backup_dir);
	sprintf(backup_html_path, "%s/%s.html", backup_dir, date);

	if((list_fid = fopen(backup_list_path, "a+")) == NULL){
			
	}
	if((log_fid = fopen(backup_log_path, "r")) == NULL){

	}
	if((html_fid = fopen(backup_html_path, "w")) == NULL){

	}	
	sprintf(buf, "<p><a href=\"./log/%s.html\">%s bakcup log</a></p>", date, date);
	fprintf(list_fid, "%s", buf);
	putHTML(html_fid, "<HTML>");
	putHTML(html_fid, "<BODY>");
	while(fgets(buf, sizeof(buf), log_fid) != NULL){
		putHTML(html_fid, "<p>");
		putHTML(html_fid, buf);
		putHTML(html_fid, "</p>");
	}
	putHTML(html_fid, "</BODY>");
	putHTML(html_fid, "</HTML>");

	/*
	LOG_PATH = argv[1];
	HTML_PATH = argv[2];

	if((log_fid = fopen(LOG_PATH, "r")) == NULL){
		sprintf(buf, "There is no log file:%s", LOG_PATH);
		logger("ERR", buf);
	}

	if((html_fid = fopen(HTML_PATH, "w+")) == NULL){
		sprintf(buf, "File Open file:%s", HTML_PATH);
		logger("ERR", buf);
	}

	putHTML("<HTML>");
	putHTML("<BODY>");
	while(fgets(buf, sizeof(buf), log_fid) != NULL){
		putHTML("<p>");
		putHTML(buf);
		putHTML("</p>");
	}
	putHTML("</BODY>");
	putHTML("</HTML>");

	if(log_fid) fclose(log_fid);
	if(html_fid) fclose(html_fid);
	*/
	if(log_fid) fclose(log_fid);
	if(list_fid) fclose(list_fid);
	if(html_fid) fclose(html_fid);
	return 0;
}
