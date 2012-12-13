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
#define BUFSIZE 8094
#define BUF_MAX 8094
#define RBUFMAX 9000
#define MAX 1024
#define STR_MAX 258
#define SVR "SERVER"
#define ERR "ERROR"
#define TRUE 1
#define FALSE 0
#define BACKUP_REQUEST 100
#define RECOVERY_REQUEST 101
#define USER_REQUEST 102
#define FILE_SENDER_REQUEST 103
#define FILE_GETTER_REQUEST 104
#define BACKUP_COMPLETE_REQUEST 105

#define USER_LIST_PATH "../conf/user.list"
//#define BACKUP_DIR "/tmp/backup"
#define BACKUP_SERVER_LOG_PATH "../log/backup_server.log"
#define CONF_PATH "../conf/server.conf"
//#define MAIL_RCPT "montecast9@gmail.com"
#define MAIL_PATH "../mail/mail.content"

#define DEBUG 1 
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
		if(strcmp(key, "@Email") == 0){
			if(strlen(val) < 2){
				printf("If you wanna get backup log mail, config your email in ../conf/server.con\n");	
				printf("And you have to install and configure sendmail to use mail feature.\n");
				printf("You can reference the way at http://m2k2.tistory.com/43\n");
			}
			strcpy(MAIL_RCPT, val);
		}
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


void handler_file_getter_req(int sd, char* clientIP){
	char user[STR_MAX], buf[BUF_MAX], date[12], day[4];
	char filename[STR_MAX];
	char backup_path[STR_MAX];
	FILE* fp;
	int rlen, len, slen, nlen;
	int nfsize, fsize, cfsize;
	int nresp, resp;
	/******************************************************
	**	start getting user
	*******************************************************/
	if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
		perror("get user's len| recv : ");
		exit(1);
	}

	len = ntohl(nlen);
	if((rlen = recv(sd, buf, len, 0)) < 0){
		perror("get user str| recv : ");
		exit(1);
	}
	strcpy(user, buf);
#if DEBUG
	printf("ip : %s, user : %s, len : %d\n", clientIP, user, len);
#endif

	/******************************************************
	** end getting user, check user
	*******************************************************/

	resp = isMatch(user, clientIP);
		
	nresp = htonl(resp);
	if((slen = send(sd, &nresp, sizeof(nresp), 0)) < 0){
		perror("send resp | send : ");
		exit(1);
	}

	if(resp){
		/***************************************************
		** get date, filename and filezie
		****************************************************/
		//date lenth
		if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
			perror("get date's lne | recv : ");
			exit(1);
		}
		len = ntohl(nlen);

		//date
		if((rlen = recv(sd, buf, len, 0)) < 0){
			perror("get date | recv : ");
			exit(1);
		}
		strcpy(date, buf);

		//filename length
		if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
			perror("get filename's len | recv : ");
			exit(1);
		}
		len = ntohl(nlen);

		//filename
		if((rlen = recv(sd, buf, len, 0)) < 0){
			perror("get filename | recv : ");
			exit(1);
		}

		strcpy(filename, buf);

#if DEBUG
		printf("date : %s, len : %d, filename : %s, len : %d.\n", date, strlen(date), filename, strlen(filename));
#endif

		getWeekDay(date, day);

		/**************************************************
		** set filepath and check existence
		***************************************************/
		sprintf(backup_path, "%s/%s/%s/%s", BACKUP_DIR, user, day, filename);

		resp = TRUE;
		if((fp = fopen(backup_path, "r")) == NULL){
			resp = FALSE;
			perror("file open fail | open : ");
		}

		nresp = htonl(resp);
		if((slen = send(sd, &nresp, sizeof(nresp), 0)) < 0){
			perror("resp send | send : ");
			exit(1);
		}
		
		/***************************************************
		** if file exists, then send file
		***************************************************/
		if(resp){			
			fseek(fp, 0, SEEK_END);
			fsize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			nfsize = htonl(fsize);
			if((slen = send(sd, &nfsize, sizeof(nfsize), 0)) < 0){
				perror("filesize send | send : ");
				exit(1);
			}
			
			
			cfsize = 0;
			while(cfsize < fsize){
				if((len = fread(buf, 1, sizeof(buf), fp)) == 0){
					perror("read fail | fread : ");
					exit(1);
				}
				cfsize += len;
				if((slen = send(sd, buf, len, 0)) < 0){
					perror("file send | send : ");
					exit(1);
				}
			}

			/**********************************************
			** Send file complete
			***********************************************/
#if DEBUG
			printf("Send file Complete\n");
#endif
		}else{
#if DEBUG
			printf("No file in this server : %s\n", filename);
#endif
		}
		
	}

	/*******************************************************
	** if the user is invalid or the work is done, close.
	*******************************************************/
	if(sd) close(sd);
	if(fp) fclose(fp);
	exit(1);



}

void handler_file_sender_req(int sd, char* clientIP){
	char user[STR_MAX];
	char buf[BUF_MAX], filename[STR_MAX], date[12], day[4];
	char backup_path[STR_MAX];
	FILE* fp;
	int rlen, len, slen, nlen;
	int nfsize, fsize, csize;
	int nresp, resp;
	//int fid;

	/******************************************************
	**	start getting user
	*******************************************************/
	if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
		perror("get user's len| recv : ");
		exit(1);
	}

	len = ntohl(nlen);
	if((rlen = recv(sd, buf, len, 0)) < 0){
		perror("get user str| recv : ");
		exit(1);
	}
	strcpy(user, buf);
#if DEBUG
	printf("ip : %s, user : %s, len : %d\n", clientIP, user, len);
#endif

	/******************************************************
	** end getting user, check user
	*******************************************************/

	resp = isMatch(user, clientIP);
		
	nresp = htonl(resp);
	if((slen = send(sd, &nresp, sizeof(nresp), 0)) < 0){
		perror("send resp | send : ");
		exit(1);
	}

	/*******************************************************
	** if the user is valid, then get a file.
	********************************************************/
	if(resp){
		/***************************************************
		** get date, filename and filezie
		****************************************************/
		//date lenth
		if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
			perror("get date's lne | recv : ");
			exit(1);
		}
		len = ntohl(nlen);

		//date
		if((rlen = recv(sd, buf, len, 0)) < 0){
			perror("get date | recv : ");
			exit(1);
		}
		strcpy(date, buf);

		//filename length
		if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
			perror("get filename's len | recv : ");
			exit(1);
		}
		len = ntohl(nlen);

		//filename
		if((rlen = recv(sd, buf, len, 0)) < 0){
			perror("get filename | recv : ");
			exit(1);
		}

		strcpy(filename, buf);

		//filesize
		if((rlen = recv(sd, &nfsize, sizeof(nfsize), 0)) < 0){
			perror("get filesize | recv : ");
			exit(1);
		}
		fsize = ntohl(nfsize);
#if DEBUG
		printf("date : %s, len : %d, filename : %s, len : %d, filesize : %d.\n", date, strlen(date), filename, strlen(filename), fsize);
#endif

		getWeekDay(date, day);

		/****************************************************
		** check directory and set file path
		****************************************************/
		//check /tmp/backup
		sprintf(backup_path, "%s", BACKUP_DIR);
		check_create_dir(backup_path);

		//check /tmp/backup/user
		sprintf(backup_path, "%s/%s", backup_path, user);
		check_create_dir(backup_path);

		//check /tmp/backup/user/day
		sprintf(backup_path, "%s/%s", backup_path, day);
		check_create_dir(backup_path);

		sprintf(backup_path, "%s/%s", backup_path, filename);
#if DEBUG
		printf("filepath : %s\n", backup_path);
#endif
		/****************************************************
		** open the file and get the file from cli
		****************************************************/

		if((fp = fopen(backup_path, "w")) == NULL){
			perror("file open : ");
			exit(1);
		}

		//if((fid = open(backup_path, O_CREAT | O_WRONLY | O_TRUCATE | 0644)) < 0){
		//	perror("file open : ");
		//	exit(1);
		//}

		csize = 0;
		while(csize < fsize){
			if((rlen = recv(sd, buf, sizeof(buf), 0)) < 0){
				perror("get file buf | recv : ");
				exit(1);
			}
			csize += rlen;
			fwrite(buf, 1, rlen, fp);
			
		}
		/***************************************************
		** getting file is finish
		****************************************************/
#if DEBUG
		printf("File transmission complete\n");
#endif

		if(fp) fclose(fp);
	}
	
	/*******************************************************
	** if the user is invalid or the work is done, close.
	*******************************************************/
	if(sd) close(sd);
	exit(1);

}

void handler_backup_complete_req(int sd, char* clientIP){
	char user[STR_MAX], subject[STR_MAX];
	char date[12], day[4], cmd[STR_MAX], buf[BUF_MAX];;
	int resp, nresp;
	int len, nlen, rlen, slen;
	/******************************************************
	**	start getting user
	*******************************************************/
	if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
		perror("get user's len| recv : ");
		exit(1);
	}

	len = ntohl(nlen);
	if((rlen = recv(sd, buf, len, 0)) < 0){
		perror("get user str| recv : ");
		exit(1);
	}
	strcpy(user, buf);
#if DEBUG
	printf("ip : %s, user : %s, len : %d\n", clientIP, user, len);
#endif

	/******************************************************
	** end getting user, check user
	*******************************************************/

	resp = isMatch(user, clientIP);
		
	nresp = htonl(resp);
	if((slen = send(sd, &nresp, sizeof(nresp), 0)) < 0){
		perror("send resp | send : ");
		exit(1);
	}

	if(resp){
		/***************************************************
		** get date, filename and filezie
		****************************************************/
		//date lenth
		if((rlen = recv(sd, &nlen, sizeof(nlen), 0)) < 0){
			perror("get date's lne | recv : ");
			exit(1);
		}
		len = ntohl(nlen);

		//date
		if((rlen = recv(sd, buf, len, 0)) < 0){
			perror("get date | recv : ");
			exit(1);
		}
		strcpy(date, buf);

		getWeekDay(date, day);
#if DEBUG
		printf("user %s date %s.\n", user, date);
#endif

		sprintf(cmd, "./HTMLgen %s %s", user, date);
		system(cmd);

		sprintf(cmd, "./mail_sender %s %s %s", user, clientIP, date);
		system(cmd);

		sprintf(subject, "The backup of %s is finish.", user);
		sprintf(cmd, "mail -s \"$(echo -e \"%s\\nContent-Type: text/html\")\" %s < %s", subject, MAIL_RCPT, MAIL_PATH);
		system(cmd);

	}

	if(sd) close(sd);
}

void handle_user_req(int sd, char* clientIP){
	char ip[17], user[STR_MAX];
	char _user[STR_MAX];
	char buf[STR_MAX];
	FILE* fp;
	int resp, len, nresp, nlen;
	int tlen;
	const int ALREADY = 3;
	const int DUP = 4;
	tlen = 0;

	logger(SVR, "USER REQUEST START");
	
	if((fp = fopen(USER_LIST_PATH, "a+")) == NULL){
		logger(ERR, "user.list file open fail");
		exit(1);
	}
	fseek(fp, 0, SEEK_SET);
	
	read(sd, &nlen, sizeof(nlen));
	len = ntohl(nlen);
	read(sd, user, len);

	resp = TRUE;
	while(fgets(buf, sizeof(buf), fp)){
		sscanf(buf, "%s %s", _user, ip);
		if(strcmp(ip, clientIP) == 0){
			printf("The client is already enrolled. %s\n", _user);
			logger(SVR, "The client is already enrolled.");
			resp = ALREADY;
			break;
		}
		if(strcmp(user, _user) == 0){
			printf("ID is duplicated. %s\n", user);
			resp = DUP;
			break;
		}		
	}
	//true or false response
	nresp = htonl(resp);//
	write(sd, &nresp, sizeof(nresp));

	if(resp == TRUE){
		fseek(fp, 0, SEEK_END);
		fprintf(fp, "%s %s\n", user, clientIP);		
	}else if(resp == ALREADY){
		len = strlen(_user) + 1;
		nlen = htonl(len);
		write(sd, &nlen, sizeof(nlen));
		write(sd, _user, len);
	}
	
	logger(SVR, "USER REQIESET END");
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
			request = ntohl(request);

			switch(request){
			case USER_REQUEST:				
				handle_user_req(socketfd, clientIP);
				break;
			case FILE_SENDER_REQUEST:
				handler_file_sender_req(socketfd, clientIP);
				break;
			case FILE_GETTER_REQUEST:
				handler_file_getter_req(socketfd, clientIP);
				break;
			case BACKUP_COMPLETE_REQUEST:
				handler_backup_complete_req(socketfd, clientIP);
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
