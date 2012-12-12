#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

//#define _XOPEN_SOURCE 500
#include <signal.h>
#include <time.h>

//#define PORT 9004
#define BUFSIZE 8096
#define MAX 1024
#define STR_MAX 258
#define SVR "SERVER"
#define ERR "ERROR"
#define TRUE 1
#define FALSE 0
#define BACKUP_REQUEST 100
#define RECOVERY_REQUEST 101
#define USER_REQUEST 102

#define USER_LIST_PATH "../conf/user.list"
//#define BACKUP_DIR "/tmp/backup"
#define BACKUP_SERVER_LOG_PATH "../log/backup_server.log"
#define CONF_PATH "../conf/server.conf"
//#define MAIL_RCPT "montecast9@gmail.com"
#define MAIL_PATH "../mail/mail.content"

#define DEBUG  0
#define ADAYSEC 86400

int PORT;
char BACKUP_DIR[STR_MAX];
char MAIL_RCPT[STR_MAX];


void logger(char* t, char* l)
{
	char cmd[STR_MAX];
	sprintf(cmd, "./logger \"%s\" \"%s\" \"%s\"", t, l, BACKUP_SERVER_LOG_PATH);
	system(cmd);
}
void set_env()
{
	FILE* fp;
	char buf[STR_MAX], key[STR_MAX], val[STR_MAX];

	if((fp = fopen(CONF_PATH, "r")) == NULL){
		logger(ERR, "conf file open error.");
		exit(1);
	}

	while(fgets(buf, sizeof(buf), fp)){
		if(buf[0] == '\n' || buf[0] == '#') continue;
		sscanf(buf, "%s %s", key, val);
		if(strcmp(key, "@Email") == 0)
			strcpy(MAIL_RCPT, val);
		else if(strcmp(key, "@BackupDir") == 0)
	      strcpy(BACKUP_DIR, val);
		else if(strcmp(key, "@BackupServerPort") == 0)
			sscanf(val, "%d", &PORT);
	}
	if(fp) fclose(fp);
}


int isMatch(char* _user, char* _ip){
	FILE* fp;
	char user[STR_MAX], ip[17];
	char buf[STR_MAX*2];
	int rst = FALSE;
	
	if((fp = fopen(USER_LIST_PATH, "r")) == NULL){
		logger(ERR, "user.list file open file.");
		exit(1);
	}

	while(fgets(buf, sizeof(buf), fp)){
		sscanf(buf, "%s %s", user, ip);
#if DEBUG
		printf("_user : %s, _ip : %s, user: %s ip: %s\n", _user, _ip, user, ip);
#endif

		if(strcmp(user, _user) == 0){
			if(strcmp(ip, _ip) == 0) rst = TRUE;
			break;
		}
	}
	if(fp) fclose(fp);
	return rst;	
}

void check_create_dir(char* path)
{
	DIR* dp;
	char log_buf[MAX];

	if((dp = opendir(path)) == NULL){
		sprintf(log_buf, "there is no directory : %s", path);
		logger(SVR, log_buf);	
		mkdir(path, 0755);
	}

	if(dp) closedir(dp);
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
	printf("%d\n", n);
#endif

	for(i = 0; i <= n+1; i++){
		t_new = t_now - (n - i)*ADAYSEC;
		new_tm = localtime(&t_new);
		strftime(buf, sizeof(buf), "%Y-%m-%d", new_tm);
		strcpy(list[i], buf);
	}
	
	return n;
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

//@see Design-page 9
void handle_recovery_req(int sd, char* clientIP){
	char buf[BUFSIZE];
	char log_buf[STR_MAX];
	char user[STR_MAX];
	char filename[STR_MAX];
	char filepath[STR_MAX];
	char backup_dir_path[STR_MAX];
	char day[4];
	char date[12];
	char date_list[8][12];
	unsigned int d_list_idx;
	unsigned int i;
	//char cmd[STR_MAX];
	int response, len, fid;
	const int endQ = -1;

	logger(SVR, "RECOVERY REQUEST START");

	read(sd, &len, sizeof(len));
	read(sd, user, len);
	
	response = isMatch(user, clientIP);
	write(sd, &response, sizeof(response));

	sprintf(backup_dir_path, "%s/%s", BACKUP_DIR, user);
	logger(SVR, "set path");

	if(response){
		//get file
		read(sd, &len, sizeof(len));
		read(sd, date, len);
		
		d_list_idx = 0;
		d_list_idx = getDateList(date, date_list);
		
		for(i = 0; i <= d_list_idx; i++){
			getWeekDay(date_list[i], day);
			sprintf(filename, "%s.tgz", date_list[i]);
			sprintf(filepath, "%s/%s/%s", backup_dir_path, day, filename);
			
			if( (fid = open(filepath, O_RDONLY)) < 0){
				sprintf(log_buf, "There is no File : %s\n", filepath);
				logger(SVR, log_buf);
				continue;
			}

			//transmit file if it exist

			len = strlen(filename) + 1;
			write(sd, &len, sizeof(len));
			write(sd, filename, len);

			while( (len = read(fid, buf, sizeof(buf))) > 0 ){
				write(sd, &len, sizeof(len));
				write(sd, buf, len);
			}
			write(sd, &endQ, sizeof(endQ));

			if(fid) close(fid);

		}
		write(sd, &endQ, sizeof(endQ));		
	}

	logger(SVR, "RECOVERY REQUEST END");
	if(sd) close(sd);
	exit(1);

}

void handle_backup_req(int sd, char* clientIP){
	char buf[BUFSIZE];
	char log_buf[STR_MAX];
	char user[STR_MAX];
	char filename[STR_MAX];
	char filepath[STR_MAX];
	char backup_dir_path[STR_MAX];
	char day[4];
	char date[12];
	char subject[STR_MAX];
	char cmd[STR_MAX];
	int response, len, fid;
	const int endQ = -1;

	logger(SVR, "BACKUP REQUEST START");

	read(sd, &len, sizeof(len));
	read(sd, user, len);
	
	response = isMatch(user, clientIP);
	write(sd, &response, sizeof(response));

	if(response){
		//get file
		read(sd, &len, sizeof(len));
		read(sd, day, len);

		read(sd, &len, sizeof(len));
		read(sd, date, len);

		check_create_dir(backup_dir_path);
		sprintf(backup_dir_path, "%s/%s", BACKUP_DIR, user);
		check_create_dir(backup_dir_path);
		sprintf(backup_dir_path, "%s/%s", backup_dir_path, day);
		check_create_dir(backup_dir_path);
		sprintf(log_buf, "Set Backup directory : %s", backup_dir_path);
		logger(SVR, log_buf);
		
		//check and create directory
		
		while(TRUE){
			read(sd, &len, sizeof(len));
			if(len == endQ) break;
			read(sd, filename, len);
#if DEBUE
			printf("filename : %s\n", filename);		
#endif
			sprintf(log_buf, "Get filename from %s[%s] : %s\n", user, clientIP, filename);
			logger(SVR, log_buf);

			sprintf(filepath, "%s/%s", backup_dir_path, filename);
			if( (fid = open(filepath, O_CREAT | O_WRONLY | O_TRUNC, 0700)) < 0 ){
				sprintf(log_buf, "file open fail : %s", filepath);
				logger(ERR, log_buf);
				exit(1);
			}
			
			while(TRUE){
				read(sd, &len, sizeof(len));
#if DEBUF
				printf("len %d\n", len);
#endif
				if(len == endQ) break;
				if(read(sd, buf, len) > 0)
					write(fid, buf, len);
			}
			if(fid) close(fid);
			sprintf(log_buf, "file[%s] is uploaded.", filepath);
			logger(SVR, log_buf);
			
			sprintf(log_buf, "Call: HTMLgen %s %s", user, date);
			logger(SVR, log_buf);

			sprintf(cmd, "./HTMLgen %s %s", user, date);
#if DEBUF
			printf("%s\n", cmd);
#endif	
			system(cmd);

			sprintf(cmd, "./mail_sender %s %s %s", user, clientIP, date);
			system(cmd);

			sprintf(subject, "The backup of %s is finish.", user);
			sprintf(cmd, "mail -s \"$(echo -e \"%s\\nContent-Type: text/html\")\" %s < %s", subject, MAIL_RCPT, MAIL_PATH); 

			system(cmd);
		}
	}

	logger(SVR, "BACKUP REQUEST END");
	if(sd) close(sd);
	exit(1);

}
void handle_user_req(int sd, char* clientIP){
	char ip[17], user[STR_MAX];
	char buf[STR_MAX];
	FILE* fp;
	int response, len;

	logger(SVR, "USER REQUEST START");
	
	if((fp = fopen(USER_LIST_PATH, "a+")) == NULL){
		logger(ERR, "user.list file open fail");
		exit(1);
	}
	fseek(fp, 0, SEEK_SET);
	response = TRUE;
	while(fgets(buf, sizeof(buf), fp)){
		sscanf(buf, "%s %s", user, ip);
		if(strcmp(ip, clientIP) == 0){
			logger(SVR, "The client is already enrolled.");
			response = FALSE;
			break;
		}
	}
	//true or false response
	write(sd, &response, sizeof(response));

	if(response == TRUE){
		read(sd, &len, sizeof(len));
		read(sd, user, len);

		fseek(fp, 0, SEEK_END);
		fprintf(fp, "%s %s\n", user, clientIP);
		response = TRUE;
		write(sd, &response, sizeof(response));
	}
	
	if(fp) fclose(fp);
	if(sd) close(sd);
}

int listenfd, socketfd;

void handler(int signo){
	//printf("this process will be terminated.\n");
	//psignal(signo, "Recevied Signal : ");
	char log_buf[STR_MAX];
	sprintf(log_buf, "this process will be terminated by %d.", signo);
	logger(SVR, log_buf);
	if(listenfd) close(listenfd);
	if(socketfd) close(socketfd);
	exit(1);
}

int main()
{
	int pid, port;
	//int listenfd, socketfd;
	char buf[MAX], clientIP[17];
	int request, optval;
	socklen_t length;
	struct sockaddr_in cli_addr;
	struct sockaddr_in serv_addr;

	//child become a deamon
	if(fork() != 0) return 0; //parent is terminated.

	(void) signal(SIGCLD, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void) setpgrp();

	sigset(SIGINT, handler);
	
	set_env();

	printf("The maybe backup server is operating.\n");
	logger(SVR, "The maybe backup server is operating.");

	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		logger(ERR, "socket open error.");
		exit(1);
	}

	optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	port = PORT;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		perror("bind");
		logger(ERR, "bind error");
		close(listenfd);
		exit(1);
	}

	if(listen(listenfd, 64) < 0){
		logger(ERR, "listen error.");
		exit(1);
	}
	
	logger(SVR, "This servier is listening.");

	while(TRUE){
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){
			logger(ERR, "accept error.");
			exit(1);
		}

		if((pid = fork()) < 0){
			logger(ERR, "fork error.");
			exit(1);
		}else if(pid == 0){
			close(listenfd);
			strcpy(clientIP, inet_ntoa(cli_addr.sin_addr));
			
			sprintf(buf, "client[%s] is connected.\n", clientIP);
			logger(SVR, buf);
	

			read(socketfd, &request, sizeof(request));
			switch(request){
			case USER_REQUEST:				
				handle_user_req(socketfd, clientIP);
				break;
			case BACKUP_REQUEST:
				handle_backup_req(socketfd, clientIP);
				break;
			case RECOVERY_REQUEST:
				handle_recovery_req(socketfd, clientIP);
				break;
			}
			
			if(socketfd) close(socketfd);
			exit(1);

			//get Request
			//authentication
			//if the user is authenticated
			//		get Request
			//		if The rquest is BACKUP then do handle_backup_req
			//		else if The request is RECOVERY then do handle_recovery_req
			//else return no and close
		}else close(socketfd);
	}

	return 0;
}
