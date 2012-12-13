#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>

#define USER_CONF "../conf/user.conf"
#define BACKUP_CONF "../conf/backup.conf"
#define RECOV_LIST_PATH "../rec/rec.list"
#define BACKUP_SHUTTLE "./backup_shuttle"
#define USER_SHUTTLE "./user_shuttle"
#define RECOVERY_SHUTTLE "./recovery_shuttle"
#define FILE_SENDER "file_sender"
#define FILE_GETTER "file_getter"
#define BACKUP_COMPLETE "backup_complete"
#define REC_DIR "../rec"
#define REC_LIST_PATH "../rec/rec.list"
#define TRUE 1
#define FALSE 0
#define STR_MAX 256
#define ADAYSEC 86400

char BACKUP_DIR[STR_MAX];
char key[STR_MAX], val[STR_MAX];

void set_env()
{
	FILE* fp;
	char buf[STR_MAX], key[STR_MAX], val[STR_MAX];

	if((fp = fopen(BACKUP_CONF, "r")) == NULL){
		exit(1);
	}

	while(fgets(buf, sizeof(buf), fp)){
		if(buf[0] == '\n' || buf[0] == '#') continue;
		sscanf(buf, "%s %s", key, val);
		if(strcmp(key, "@BackupDir") == 0)
			strcpy(BACKUP_DIR, val);
	}

	if(fp) fclose(fp);
}


int checkDateType(char* date)
{
	int len = strlen(date);	
	int i;
	for(i = 0; i < len; i++){
		if(i != 4 && i != 7){
			if(!('0'<= date[i] && date[i] <= '9'))
				return FALSE;
		}else if(date[i] != '-'){
				return FALSE;
		}
	}
	if(len > 10) return FALSE;
	return TRUE;
}

void handle_s(char* date)
{
	//get user from 
	//calc week of day
	//./backup_shuttle user day date
	FILE* fp;
	char user[STR_MAX];
	char day[4], cmd[STR_MAX];
	char backup_dir[STR_MAX];
	struct tm t;
	DIR* dp;
	struct dirent* dent;

	if((fp = fopen(USER_CONF, "r")) == NULL){
		printf("You have to register this host to the backup server\n");
		exit(1);
	}

	fscanf(fp, "%s", user);

	if(fp) fclose(fp);
	fp = NULL;
	
	strptime(date, "%Y-%m-%d", &t);
	strftime(day, sizeof(day), "%a", &t);

//	sprintf(cmd, "%s %s %s %s", BACKUP_SHUTTLE, user, day, date);
//	system(cmd);
	
	sprintf(backup_dir, "%s/%s", BACKUP_DIR, day);

	if((dp = opendir(backup_dir)) == NULL){
		perror("invaild dir | opendir : ");
		exit(1);
	}
	while((dent = readdir(dp))){
		if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
			continue;
		sprintf(cmd, "./%s %s %s %s", FILE_SENDER, user, date, dent->d_name);
		system(cmd);
	}

	if(dp) closedir(dp);

	sprintf(cmd, "./%s %s %s", BACKUP_COMPLETE, user, date);
	system(cmd);
}

void handle_u(char* _user)
{
	char user[STR_MAX], cmd[STR_MAX];
	strcpy(user, _user);

	sprintf(cmd, "%s %s", USER_SHUTTLE, user);
	system(cmd);
}

int getDateList(char* date, char list[][12])
{
	int i = 0;
	struct tm now_tm;
	struct tm *new_tm;
	char buf[12];
	time_t t_now;
	time_t t_new;
	int n;

	memset(&now_tm, 0, sizeof(now_tm));
	strptime(date, "%Y-%m-%d", &now_tm);
	t_now = mktime(&now_tm);
 	strftime(buf, sizeof(buf), "%w", &now_tm);
	n = atoi(buf);
#if DEBUG
	//printf("%d\n", n);
#endif
	for(i = 0; i <= n+1; i++){
		t_new = t_now - (n - i)*ADAYSEC;
		new_tm = localtime(&t_new);
		strftime(buf, sizeof(buf), "%Y-%m-%d", new_tm);
		strcpy(list[i], buf);
	}
 	return n;
}
void remove_create_dir(char* path)
{
	DIR* dp;
	char cmd[STR_MAX];

	if((dp = opendir(path)) != NULL){
		sprintf(cmd, "rm -rf %s", path);
		system(cmd);
	}
	mkdir(path, 0755);
	if(dp) closedir(dp);
}


void handle_r(char* _date){
	char date[STR_MAX];
	char user[STR_MAX];
//	char buf[STR_MAX];
	char cmd[STR_MAX], rec_file_path[STR_MAX];;
	char datelist[8][12];
	int cnt, nidx, i;
	FILE* fp, *rfp;
	strcpy(date, _date);

	if((fp = fopen(USER_CONF, "r")) == NULL){
		printf("You have to register this host to the backup server. : use -u [id]\n");
		exit(1);
	}

	fscanf(fp, "%s", user);
	//printf("%s\n", user);
	if(fp) fclose(fp);
	fp = NULL;

	remove_create_dir(REC_DIR);

	if( (rfp = fopen(REC_LIST_PATH, "w") ) == NULL){
		perror("fopen");
		exit(1);
	}
	
	nidx = getDateList(date, datelist);

	for(i = 0; i <= nidx; i++){
		sprintf(cmd, "./%s %s %s %s.tgz", FILE_GETTER, user, datelist[i], datelist[i]);
		system(cmd);
		printf(rec_file_path, "%s/%s", REC_DIR, datelist[i]);
		if((fp = fopen(rec_file_path, "r")) != NULL){
			cnt++;
			fprintf(fp, "%s.tgz\n", datelist[i]);
			if(fp) fclose(fp);
			fp = NULL;
		}
	}

	if(cnt == 0) printf("There is no files to recover. check date : %s\n", date);
	else{
		sprintf(cmd, "ls -al %s", REC_DIR);
		system(cmd);
		printf("The recovery files have been downloaded. To recover them, type 'maybe -x'.\n");
	}
	if(fp) fclose(fp);
	if(rfp) fclose(rfp);
	rfp = NULL;
}

int main(int argc, char* argv[])
{
	int n;
	char resp;
	extern char* optarg;
//	extern int optind;
	//strcpy
	set_env();

	if(argc == 1){
		printf("Do you want to backup now?(y/n) : ");
		scanf("%c", &resp);
		
		if(resp == 'y' || resp == 'Y'){
			printf("=====Start backup.=====\n");
			system("./backup.sh");
			printf("=====End backup.=======\n");
			return 0;
		}else {
			printf("No Backup.\n");
			return 0;
		}
		
	}

	while( (n = getopt(argc, argv, "u:s:r:x")) != -1){
		switch(n){
		case 'u':
			handle_u(optarg);
			break;
		case 's':
			//printf("Option : s, %s\n", optarg);
			if(checkDateType(optarg)==FALSE){
				printf("Input date format should be yyyy-mm-dd.\n");
				return 0;
			}
			//handle_s
			handle_s(optarg);
			
			break;
		case 'r':
			if(checkDateType(optarg) == FALSE){
				printf("Input date format should be yyyy-mm-dd.\n");
				return 0;
			}
			handle_r(optarg);
			break;
		case 'x':
			//not yet implemented.
			break;
		}
	}
	return 0;
}
