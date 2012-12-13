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
#define DEBUG 1 

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
	FILE* fp;
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
	int fid, fsize, fsize2, nsize, fpsize, fpsize2;
	int tlen, i, rlen;

	int Q;
		
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
	request = htonl(request);	
	write(server_fd, &request, sizeof(request));
	
	//printf("%s\n", user);
	//Write user
	len = strlen(user) + 1;
	tlen = htonl(len);//
	write(server_fd, &tlen, sizeof(tlen));	
	write(server_fd, user, len);

	read(server_fd, &response, sizeof(response));
	response = ntohl(response);

	if(response){	
		//send file in directory
		len = strlen(day) + 1;
		tlen = htonl(len);//
		write(server_fd, &tlen, sizeof(tlen));//
		write(server_fd, day, len);
		len = strlen(date) + 1;
		tlen = htonl(len);//
		write(server_fd, &tlen, sizeof(tlen));//
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
			tlen = htonl(len); //
			write(server_fd, &tlen, sizeof(tlen));
			write(server_fd, dent->d_name, len);

			fp = fopen(filepath, "rb");
			fseek(fp, 0, SEEK_END);
			fsize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fsize2 = htonl(fsize);
			printf("file size :%d\n", fsize);
			if( (len = send(server_fd, &fsize2, sizeof(fsize), 0) < 0)){
				perror("send");
				exit(1);
			}
			i = 0;
			nsize = 0;
			while(nsize != fsize){
				if((len = fread(buf, 1, 4048, fp)) == 0){
					//perror("fread");
					break;
				}
				nsize += len;
				printf("file size = %d, seq : %d, now_size : %d, send_size : %d\n", fsize, i++, nsize, len);

				//fpsize2 = htonl(fpsize);
				//send(server_fd, &fpsize2, sizeof(fpsize), 0);

				if((len = send(server_fd, buf, len, 0)) < 0){
					perror("send2");
					exit(1);
				}
			}
			Q = htonl(endQ);
			send(server_fd, &Q, sizeof(Q), 0);
			fclose(fp);
			//exit(1);
			/*
			if((fid = open(filepath, O_RDONLY)) < 0){
				sprintf(log_buf, "open file fail : %s", filepath);
				logger(ERR, log_buf);
				continue;
			}
			
			sprintf(log_buf, "file is ready : %s", filepath);
			logger(STL, log_buf);

			
			
			i = 0;
			while( (len = read(fid, buf, sizeof(buf))) > 0){
#if DEBUG
				printf("len : %d %d %d\n", i++, len, htonl(len));
#endif
				rlen = len;
				tlen = htonl(len);//
					
				if(write(server_fd, &tlen, sizeof(tlen)) < 0){
					perror("write");
					exit(1);
				}		
				if(write(server_fd, buf, len) < 0){
					perror("write");
					exit(1);
				}
				if( (len = send(server_fd, &tlen, sizeof(tlen), 0)) != sizeof(tlen)){
					perror("send");
					exit(1);
				}


				while( (len = send(server_fd, buf, rlen, 0)) != rlen){
					perror("send");
					//exit(1);
				}
			}
		

			tlen = htonl(endQ);//
			//write(server_fd, &tlen, sizeof(tlen));//
			send(server_fd, &tlen, sizeof(tlen), 0);
			
			if(fid) close(fid);
			sprintf(log_buf, "file is uploaded : %s", filepath);
			logger(STL, log_buf);
			*/
		}

		if(dp) closedir(dp);
		tlen = htonl(endQ);
		write(server_fd, &tlen, sizeof(tlen));
	}else{		
		//authen fail
		logger(ERR, "user is invalid. backup shuttle fail.");
	}
	
	
	logger(STL, "BACKUP REQUEST END");

	if(server_fd) close(server_fd);

	return 0;
}

