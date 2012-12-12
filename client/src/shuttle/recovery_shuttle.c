//recovery_shuttle.c

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

//#define PORT 9004
#define BUF_MAX 8094
#define STR_MAX 256
#define BACKUP_REQUEST 100
#define RECOVERY_REQUEST 101
#define USER_REQUEST 102
#define TRUE 1
#define FALSE 0
#define STL "RECOVERY_SHUTTLE"
#define ERR "ERROR"
#define DEBUG 1

#define BACKUP_CONF "../conf/backup.conf"
#define RECOV_DIR "../rec"
#define RECOV_LIST_PATH "../rec/rec.list"
#define SHUTTLE_LOG_PATH "../log/recovery_shuttle.log"

char serverIP[17] = {0,};
char backup_dir[STR_MAX] = {0, };
int PORT;

void logger(char* t, char* l){
	char cmd[STR_MAX];
	sprintf(cmd, "./logger \"%s\" \"%s\" \"%s\"", t, l, SHUTTLE_LOG_PATH);
	system(cmd);
}


void remove_create_dir(char* path)
{
	DIR* dp;
	char cmd[STR_MAX];

	if((dp = opendir(path)) != NULL){
		sprintf(cmd, "rm -rf %s", path);
		system(cmd);
	}

	mkdir(path, 0744);

   if(dp) closedir(dp);
}


void set_env()
{
	FILE* fp;
	char buf[STR_MAX], key[STR_MAX], val[STR_MAX];

	if((fp = fopen(BACKUP_CONF, "r")) == NULL){
		logger(ERR, "conf file open error.");
		exit(1);
	}

	while(fgets(buf, sizeof(buf), fp)){
		if(buf[0] == '\n' || buf[0] == '#') continue;
		sscanf(buf, "%s %s", key, val);
		if(strcmp(key, "@ServerIP") == 0){
			strcpy(serverIP, val);
			if(strlen(serverIP) < 7){
				printf("You have to set Backup Server IP in ../conf/backup.conf\n");
				exit(1);
	      }

		}
		else if(strcmp(key, "@BackupDir") == 0)
			strcpy(backup_dir, val);
		else if(strcmp(key, "@Port") == 0)
			sscanf(val, "%d", &PORT);
	}
	fclose(fp);
}

int main(int argc, char* argv[]){
	struct sockaddr_in serv_addr;
	int port;
	int len;
	int server_fd, request, response;
	char user[STR_MAX];
	char filename[STR_MAX];
	char filepath[STR_MAX*2];
	//char log_buf[STR_MAX];
	char buf[BUF_MAX];
	char date[12];
	//DIR* dp;
	FILE* fp;
	//struct dirent* dent;
	const int endQ = -1;
	int fid;
		
	if(argc != 3){
		fprintf(stderr, "Argument Error : ./recovery_shuttle user yyyy-mm-dd\n");
		exit(1);
	}

	set_env();
	port = PORT;	
	strcpy(user, argv[1]);
	strcpy(date, argv[2]);
	
	memset((char*)&serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = inet_addr(serverIP);

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		logger(ERR, "socket open fail.");
		exit(1);
	}

	if(connect(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		logger(ERR, "connect fail.");
		exit(1);
	}

	logger(STL, "RECOVERY REQUEST START");
	request = RECOVERY_REQUEST;
	
	//RECOVERY_REQUEST START
	write(server_fd, &request, sizeof(request));
	
	//printf("%s\n", user);
	//Write user
	len = strlen(user) + 1;
	write(server_fd, &len, sizeof(len));	
	write(server_fd, user, len);

	read(server_fd, &response, sizeof(response));

	if(response){	
		//send file in directory
		
		remove_create_dir(RECOV_DIR);

		fp = fopen(RECOV_LIST_PATH, "w");

		len = strlen(date) + 1;
		write(server_fd, &len, sizeof(len));
		write(server_fd, date, len);
		
		while(TRUE){
			read(server_fd, &len, sizeof(len));
			if(len == endQ) break;
			read(server_fd, filename, len);
			fprintf(fp, "%s\n", filename);
			//printf("get file : %s\n", filename);
			sprintf(filepath, "%s/%s", RECOV_DIR, filename);


			if( (fid = open(filepath, O_CREAT | O_WRONLY | O_TRUNC, 0600)) < 0){
				logger(ERR, "open error");
				exit(1);
			}

			while(TRUE){
				read(server_fd, &len, sizeof(len));
				if(len == endQ) break;
				
				read(server_fd, buf, len);
				write(fid, buf, len);
			}
		}
		if(fp) fclose(fp);
	}else{		
		//authen fail
		logger(ERR, "user is invalid. backup shuttle fail.");
	}
	
	
	logger(STL, "RECOVERY REQUEST END");

	if(server_fd) close(server_fd);

	return 0;
}

