#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


#define USER_CONF "../conf/user.conf"
#define RECOV_LIST_PATH "../rec/rec.list"
#define BACKUP_SHUTTLE "./backup_shuttle"
#define USER_SHUTTLE "./user_shuttle"
#define RECOVERY_SHUTTLE "./recovery_shuttle"
#define TRUE 1
#define FALSE 0
#define STR_MAX 256

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
	char day[4], cmd[STR_MAX];;
	struct tm t;

	if((fp = fopen(USER_CONF, "r")) == NULL){
		printf("You have to register this host to the backup server\n");
		exit(1);
	}

	fscanf(fp, "%s", user);

	if(fp) fclose(fp);

	strptime(date, "%Y-%m-%d", &t);
	strftime(day, sizeof(day), "%a", &t);

	sprintf(cmd, "%s %s %s %s", BACKUP_SHUTTLE, user, day, date);
	system(cmd);
}

void handle_u(char* _user)
{
	char user[STR_MAX], cmd[STR_MAX];
	strcpy(user, _user);

	sprintf(cmd, "%s %s", USER_SHUTTLE, user);
	system(cmd);
}

void handle_r(char* _date){
	char date[STR_MAX];
	char user[STR_MAX];
	char buf[STR_MAX];
	char cmd[STR_MAX];
	int cnt;
	FILE* fp;
	strcpy(date, _date);

	if((fp = fopen(USER_CONF, "r")) == NULL){
		printf("You have to register this host to the backup server. : use -u [id]\n");
		exit(1);
	}

	fscanf(fp, "%s", user);
	printf("%s\n", user);
	if(fp) fclose(fp);

	sprintf(cmd, "%s %s %s", RECOVERY_SHUTTLE, user, date);
	system(cmd);

	if((fp = fopen(RECOV_LIST_PATH, "r")) == NULL){
		printf("There is no files to recover. check date : %s\n", date);
		exit(1);
	}
	cnt = 0;
	while(fgets(buf, sizeof(buf), fp)){
		cnt++;
	}
	if(cnt == 0) printf("There is no files to recover. check date : %s\n", date);
	else{
		printf("The recovery files have been downloaded. To recover them, type 'maybe -x'.\n");
	}
	if(fp) fclose(fp);
}

int main(int argc, char* argv[])
{
	int n;
	char resp;
	extern char* optarg;
//	extern int optind;

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
