#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define BACKUP_CONF "../conf/backup.conf"
#define FILE_SENDER_REQUEST 103
#define BUF_MAX 8094
#define STR_MAX 256
#define DEBUG 1
#define TRUE 1
#define FALSE 0
#define STL "BACKUP_SHUTTLE"
#define ERR "ERROR"
#define SHUTTLE_LOG_PATH "../log/backup_shuttle.log"

int PORT;
char BACKUP_DIR[STR_MAX];
char SERVER_IP[20];

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
			strcpy(SERVER_IP, val);
			if(strlen(SERVER_IP) < 7){
				printf("You have to set Backup Server IP in ../conf/backup.conf\n");
			   exit(1);
			}
		}else if(strcmp(key, "@BackupDir") == 0)
			strcpy(BACKUP_DIR, val);
		else if(strcmp(key, "@Port") == 0)
		   sscanf(val, "%d", &PORT);
	}
	if(fp) fclose(fp);
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

int main(int argc, char* argv[]){
	FILE* fp;
	struct sockaddr_in serv_addr;
	int server_fd;
	int req, nreq, resp, nresp;
	char user[STR_MAX], date[12], filename[STR_MAX], day[4];
	char buf[BUF_MAX];
	char backup_path[STR_MAX];
	int fsize, nfsize, cfsize;
	int len, nlen, rlen, slen;

	if(argc != 4){
		fprintf(stderr, "Argument error : %s user date filename\n", argv[0]);
		exit(1);
	}
	strcpy(user, argv[1]);
	strcpy(date, argv[2]);
	strcpy(filename, argv[3]);

	set_env();

	getWeekDay(date, day);

	sprintf(backup_path, "%s/%s/%s", BACKUP_DIR, day, filename);

	if((fp = fopen(backup_path, "r")) == NULL){
		perror("no file | fopen : ");
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	memset((char*)&serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
#if DEBUG
	printf("user : %s, date : %s, filename : %s\n", user, date, filename);
#endif

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}

	if(connect(server_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
		perror("connect");
		exit(1);
	}

	/**********************************************
	**	send request
	**********************************************/

	req = FILE_SENDER_REQUEST;
	nreq = htonl(req);
	if((slen = send(server_fd, &nreq, sizeof(nreq), 0)) < 0){
		perror("send req | send : ");
		exit(1);
	}

	/***********************************************
	** send user
	***********************************************/
	
	len = strlen(user) + 1;
	nlen = htonl(len);
	if((slen = send(server_fd, &nlen, sizeof(nlen), 0)) < 0){
		perror("send user's len | send : ");
		exit(1);
	}

	if((slen = send(server_fd, user, len, 0)) < 0){
		perror("send user | send : ");
		exit(1);
	}

	if((rlen = recv(server_fd, &nresp, sizeof(nresp), 0)) < 0){
		perror("get resp | recv : ");
		exit(1);
	}

	resp = ntohl(nresp);

	/***********************************************
	** if the user is valid, then
	***********************************************/
	if(resp){

		/*******************************************
		** send date, filename, filesize
		********************************************/

		//send date length
		len = strlen(date) + 1;
		nlen = htonl(len);
		if((slen = send(server_fd, &nlen, sizeof(nlen), 0)) < 0){
			perror("send date's len | send : ");
			exit(1);
		}
		//send date
		if((slen = send(server_fd, date, len, 0)) < 0){
			perror("send date | send : ");
			exit(1);
		}

		//send filename length
		len = strlen(filename) + 1;
		nlen = htonl(len);
		if((slen = send(server_fd, &nlen, sizeof(nlen), 0)) < 0){
			perror("send filename's len | send : ");
			exit(1);
		}

		//send filename
		if((slen = send(server_fd, filename, len, 0)) < 0){
			perror("send filename | send : ");
			exit(1);
		}

		//sen filesize
		nfsize = htonl(fsize);
		if((slen = send(server_fd, &nfsize, sizeof(nfsize), 0)) < 0){
			perror("send filesize | send : ");
			exit(1);
		}

		/*********************************************
		** send the file
		*********************************************/
		cfsize = 0;

		while(cfsize < fsize){
			if((len = fread(buf, 1, sizeof(buf), fp)) == 0){
				perror("read fail | fread : ");
				exit(1);
			}

			cfsize += len;

			if((slen = send(server_fd, buf, len, 0)) < 0){
				perror("file send | send : ");
				exit(1);
			}
		}

		/************************************************
		** send complete
		************************************************/
#if DEBUG
		printf("Send file complete.\n");
#endif
		
	}

	/*************************************************
	**	if user is invalid or the work is done, then
	*************************************************/
	if(server_fd) close(server_fd);
	if(fp) fclose(fp);
	return 0;
}

