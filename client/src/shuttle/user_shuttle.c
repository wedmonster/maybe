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
#include <fcntl.h>

//#define PORT 9004
#define BUFMAX 8094
#define STR_MAX 256
#define BACKUP_REQUEST 100
#define RECOVERY_REQUEST 101
#define USER_REQUEST 102
#define TRUE 1
#define FALSE 0
#define DEBUG 0
#define STL "USER_SHUTTLE"
#define ERR "ERROR"

#define BACKUP_CONF "../conf/backup.conf"
#define USER_CONF "../conf/user.conf"
#define SHUTTLE_LOG_PATH "../log/user_shuttle.log"

char serverIP[17] = {0,};
char backup_dir[STR_MAX] = {0, };
int PORT;

void logger(char* t, char* l){
	char cmd[STR_MAX];
	sprintf(cmd, "./logger \"%s\" \"%s\" \"%s\"", t, l, SHUTTLE_LOG_PATH);
//	printf("%s\n", cmd);
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
         if(strlen(serverIP) < 8){
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
	int port, len;
	int server_fd, request, response;
	FILE* fp;
	char user[STR_MAX];
		
	if(argc != 2){
		fprintf(stderr, "Argument Error : ./user_shuttle user\n");
		exit(1);
	}
	
	set_env();
#if DEBUG
	printf("%s\n", serverIP);
#endif

	strcpy(user, argv[1]);
	port = PORT;
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

	logger(STL, "USER REQUEST START");
	request = USER_REQUEST;
	
	write(server_fd, &request, sizeof(request));

	//READ FROM SERVER
	read(server_fd, &response, sizeof(response));

	if(response == FALSE){
		printf("You are already enrolled in the host.\n");
		logger(STL, "This host is already enrolled.");
		logger(STL, "USER REQUEST END");
		if(server_fd) close(server_fd);
		exit(1);
	}

	len = strlen(user)+1;
	write(server_fd, &len, sizeof(len));
	write(server_fd, user, len);
	
	read(server_fd, &response, sizeof(response));

	if(response == TRUE){
		logger(STL, "Complete to add user.");
		fp = fopen(USER_CONF, "w");
		fprintf(fp, "%s", user);
		fclose(fp);
		printf("Complete to add user. Your ID is %s\n", user);
	}
	
	logger(STL, "USER REQUEST END");
	if(server_fd) close(server_fd);
	return 0;
}

