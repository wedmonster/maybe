#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

#include "web_server.h"

void logger(char* t, char* l)
{
	char cmd[MAX];
	sprintf(cmd, "./logger \"%s\" \"%s\" \"%s\"", t, l, WEB_LOG_PATH);
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
		if(strcmp(key, "@WebServerPort") == 0)
		   sscanf(val, "%d", &PORT);
	}
	if(fp) fclose(fp);
}

char* sgetline( char* buffer, char* dest ){
	int idx = 0, d_idx = 0;;
	int i, len;
	
	
	while(buffer[idx] == '\n' || buffer[idx] == ' ')
		idx++;
	
	if(buffer[idx] == '\0') return NULL;
	
	while(buffer[idx] != '\n' && buffer[idx] != '\0'){
		dest[d_idx] = buffer[idx];
		idx++; d_idx++;
	}

	dest[d_idx] = '\0';
	//printf("dest %s\n", dest);

	len = strlen(buffer);
	for(i = idx; i <= len; i++){
		buffer[i-idx] = buffer[i];
	}
	
		
	return buffer;
}

char key[KEY_MAX][STR_MAX];
char val[KEY_MAX][STR_MAX];
int key_idx, val_idx;

int isPair(char* line)
{
	unsigned int i;
	int cnt = 0;
	for(i = 0; i < strlen(line);i++)
		if(line[i] == '=') cnt++;
	if(cnt == 1) return TRUE;
	return FALSE;
}

void print_message(HTTP_MESSAGE* hm)
{
	int i;
	printf("TYPE : %s\n", (hm->type == 0)?"GET":"POST");
	printf("URI : %s\n", hm->uri);
	printf("VERSION : %s\n", hm->http_version);
	printf("Host : %s\n", hm->host);
	printf("Connection : %s\n", hm->connection);
	printf("User-Agent : %s\n", hm->user_agent);
	printf("Accept : %s\n", hm->accept);
	printf("key = value\n");
	for(i = 0; i < key_idx; i++){
		printf("%s = %s\n", key[i], val[i]);
	}
}

void push_map(char* pair, int n)
{
	int i, j;
	int flag = TRUE;
	for(i = 0, j = 0; i < n; i++){
		if(pair[i] == '=') {
			flag = FALSE;
			key[key_idx++][i] = 0;
		}
		else if(flag) key[key_idx][i] = pair[i];
		else val[val_idx][j++] = pair[i];
	}
	val[val_idx++][j] = 0;
}

void query_parse(char* query)
{
	char* after;
	//char* tkey, *tval;
	after = strtok(query, "&");

	while(after != NULL){
		push_map(after, strlen(after));
		after = strtok(NULL, "&");
	}
	
}

void uri_parse(char* uri){
	char* after;
	//char uri_tmp[STR_MAX];
	//char tmp[STR_HMAX];

	// handling /
	if(strlen(uri) == 1) return;

	after = strtok(uri, "?");
	
	strcpy(uri, &after[1]);
	after = strtok(NULL, "&");

	query_parse(after);
	
}

int isContent(char* line, int n)
{
	int i;
	for(i = 0; i < n; i ++)
		if(line[i] == ':') return FALSE;
	
	return TRUE;
}

void parse_message(HTTP_MESSAGE* hm, char* buffer)
{
	char line[MAX], buf[STR_MAX], buf2[STR_MAX];
	char type[10];
	char* after;
	char* post;
	int cnt = 0;
	key_idx = val_idx = 0;
	hm->content_length = -1;

	while(sgetline(buffer, line)){
						
		if(strlen(line) != 1){
			if(cnt == 0) {
				sscanf(line, "%s %s %s", type, hm->uri, hm->http_version);
				if(strcmp(type, "GET") == 0 || strcmp(type, "get") == 0)
					hm->type = GET;
				else if(strcmp(type, "POST") == 0 || strcmp(type, "post") == 0)
					hm->type = POST;
				uri_parse(hm->uri);
			}else if(isContent(line, strlen(line))){
				query_parse(line);
			}else{
				after = strtok(line, ":");
				post = strtok(NULL, "");
				strcpy(buf, post);
				strcpy(buf2, after);
				//printf("%s %s\n", buf2, buf);
				if(strcmp(buf2, "Host") == 0)
					sscanf(buf, "%s", hm->host);
				else if(strcmp(buf2, "Connection") == 0)
					sscanf(buf, "%s", hm->connection);
				else if(strcmp(buf2, "Content-Length") == 0)
					sscanf(buf, "%d", &(hm->content_length));
				else if(strcmp(buf2, "Cache-Control") == 0)
					sscanf(buf, "%s", hm->cache_control);
				else if(strcmp(buf2, "Origin") == 0)
					sscanf(buf, "%s", hm->origin);
				else if(strcmp(buf2, "User-Agent") == 0)
					sscanf(buf, "%s", hm->user_agent);
				else if(strcmp(buf2, "Content-Type") == 0)
					sscanf(buf, "%s", hm->content_type);
				else if(strcmp(buf2, "Referer") == 0)
					sscanf(buf, "%s", hm->referer);
				else if(strcmp(buf2, "Accept") == 0)
					sscanf(buf, "%s", hm->accept);
				else if(strcmp(buf2, "Accept-Encoding") == 0)
					sscanf(buf, "%s", hm->accept_encoding);
				else if(strcmp(buf2, "Accept-Language") == 0)
					sscanf(buf, "%s", hm->accept_language);
				else if(strcmp(buf2, "Accept-Charset") == 0)
					sscanf(buf, "%s", hm->accept_charset);
				else{
					;
				}
			}
		}
		cnt++;
	}
	printf("-------------------------\n");
	print_message(hm);
}

void web(int fd)
{
	long ret, len, uri_len;
	char *fstr, *estr;
	int file_fd;
	unsigned long i = 0;
	char buf[STR_MAX];
	static char buffer[BUFSIZE + 1];
	HTTP_MESSAGE hm;

	ret = read(fd, buffer, BUFSIZE);
	if(ret <= 0){
		logger(ERR, "read error.");
		close(fd);
		exit(2);
	}
	printf("%s\n", buffer);	
	parse_message(&hm, buffer);

	//check ..
	
	for(i = 0; i < strlen(hm.uri)-1; i++){
		if(hm.uri[i] == '.' && hm.uri[i+1] == '.'){
			//error	
		}
	}

	if(strncmp(hm.uri, "/\0", 1) == 0){
		strcpy(hm.uri, "index.html");
	}
	
	
	uri_len = strlen(hm.uri);

	for(i = 0; extenstions[i].ext != 0; i++){
		len = strlen(extenstions[i].ext);
		if(!strncmp(&hm.uri[uri_len - len], extenstions[i].ext, len)){
			estr = extenstions[i].ext;
			fstr = extenstions[i].filetype;
		}
	}

	if(fstr == 0){
		//error
	}
	sprintf(buf, "%s/%s", HOME, hm.uri);
	if((file_fd = open(buf, O_RDONLY)) == -1){
		//error open fail
	}

	len = (long) lseek(file_fd, 0, SEEK_END);
	lseek(file_fd, 0, SEEK_SET);
	sprintf(buffer, "HTTP/1.1 200 OK\nServer: mwep/%d.0\nContent-Lenth: %ld\nConnection:close\nContent-Type:%s\n\n", VERSION, len, fstr);
	write(fd, buffer, strlen(buffer));

	while( (ret = read(file_fd, buffer, BUFSIZE)) > 0){
		write(fd, buffer, ret);
	}

	sleep(1);
	close(fd);
	exit(1);
}

int main()
{
	int pid, port;
	int listenfd, socketfd;
	char buf[MAX];
	int optval;
	socklen_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;

	//child become a deamon
	if(fork() != 0) return 0; //parent is terminated.
	
	//ogging	logger(TAG, LOG);
	(void) signal(SIGCLD, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void) setpgrp();

	set_env();
	printf("The maybe webserver is operating.\n");
	logger(SVR, "The maybe lightweight webserver is operating.");
	sprintf(buf, "The current pid id %d.", getpid());
	logger(SVR, buf);

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

	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
	      perror("bind");
			logger(ERR, "bind error");
			close(listenfd);
	      exit(1);
	 }

	if(listen(listenfd, 64) < 0){
		logger(ERR, "listen error.");
		exit(1);
	}

	logger(SVR, "This server is listening.");

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
			web(socketfd);
		}else{
			close(socketfd);
		}
	}
	
	return 0;
}
