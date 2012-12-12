//backup_shuttle.c

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

//#define PORT 9004
#define BUF_MAX 8094
#define STR_MAX 256
#define BACKUP_REQUEST 100
#define RECOVERY_REQUEST 101
#define USER_REQUEST 102
#define TRUE 1
#define FALSE 0
#define STL "BACKUP_SHUTTLE"
#define ERR "ERROR"
#define DEBUG 0

#define BACKUP_CONF "../conf/backup.conf"
#define SHUTTLE_LOG_PATH "../log/backup_shuttle.log"

char serverIP[17] = {0,};
char backup_dir[STR_MAX] = {0, };
int PORT;

void logger(char* t, char* l){
	char cmd[STR_MAX];
	sprintf(cmd, "./logger \"%s\" \"%s\" \"%s\"", t, l, SHUTTLE_LOG_PATH);
	system(cmd);
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
	if(fp) fclose(fp);
}

int main(int argc, char* argv[]){
	struct sockaddr_in serv_addr;
	int port, len;
	int server_fd, request, response;
	char user[STR_MAX];
	char filepath[STR_MAX*2];
	char log_buf[STR_MAX];
	char buf[BUF_MAX];
	char day[4], date[12];
	DIR* dp;
	struct dirent* dent;
	const int endQ = -1;
	int fid;
		
	if(argc != 4){
		fprintf(stderr, "Argument Error : ./backup_shuttle user Sun yyyy-mm-dd\n");
		exit(1);
	}
	
	set_env();

	port = PORT;
	strcpy(user, argv[1]);
	strcpy(day, argv[2]);
	strcpy(date, argv[3]);
	
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

	logger(STL, "BACKUP REQUEST START");
	request = BACKUP_REQUEST;
	
	//USER_REQUEST START
	write(server_fd, &request, sizeof(request));
	
	//printf("%s\n", user);
	//Write user
	len = strlen(user) + 1;
	write(server_fd, &len, sizeof(len));	
	write(server_fd, user, len);

	read(server_fd, &response, sizeof(response));

	if(response){	
		//send file in directory
		len = strlen(day) + 1;
		write(server_fd, &len, sizeof(len));
		write(server_fd, day, len);
		len = strlen(date) + 1;
		write(server_fd, &len, sizeof(len));
		write(server_fd, date, len);
		
		sprintf(backup_dir, "%s/%s", backup_dir, day);
		sprintf(log_buf, "Set Backup directory : %s\n", backup_dir);
		logger(STL, log_buf);

		if((dp = opendir(backup_dir)) == NULL){
			sprintf(log_buf, "Open directory fail : %s", backup_dir);
			logger(ERR, log_buf);
			exit(1);
		}

		while((dent = readdir(dp))){
			if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
				continue;

			sprintf(filepath, "%s/%s", backup_dir, dent->d_name);
			len = strlen(dent->d_name) + 1;
			write(server_fd, &len, sizeof(len));
			write(server_fd, dent->d_name, len);


			if((fid = open(filepath, O_RDONLY)) < 0){
				sprintf(log_buf, "open file fail : %s", filepath);
				logger(ERR, log_buf);
				continue;
			}
			
			sprintf(log_buf, "file is ready : %s", filepath);
			logger(STL, log_buf);

			while( (len = read(fid, buf, sizeof(buf))) > 0){
#if DEBUG
				printf("len : %d\n", len);
#endif
				write(server_fd, &len, sizeof(len));
				write(server_fd, buf, len);
			}

			write(server_fd, &endQ, sizeof(endQ));
			if(fid) close(fid);
			sprintf(log_buf, "file is uploaded : %s", filepath);
			logger(STL, log_buf);
		}

		if(dp) closedir(dp);
		write(server_fd, &endQ, sizeof(endQ));
	}else{		
		//authen fail
		logger(ERR, "user is invalid. backup shuttle fail.");
	}
	
	
	logger(STL, "BACKUP REQUEST END");

	if(server_fd) close(server_fd);

	return 0;
}

