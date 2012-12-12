#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdlib.h>
#include <stdio.h>

struct {
	char* ext;
	char* filetype;
} extenstions [] = {
	
	{"gif", "image/gif" },
	{"jpg", "image/jpg" },
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{0,0} 
};

//#define PORT 8088
#define BUFSIZE 8096
#define VERSION 23
#define ERROR 		42
#define LOG 		44
#define FORBIDDEN 403
#define NOTFOUND	404
#define MAX 1024
#define KEY_MAX 1024
#define STR_MAX 258
#define STR_HMAX 128
#define STR_HHMAX 64
#define CONF_PATH "../conf/server.conf"
#define WEB_LOG_PATH "../log/web_server.log"
#define TRUE 1
#define FALSE 0
#define SVR "SERVER"
#define ERR "ERROR"
#define HOME "../html"

#define GET 0
#define POST 1

/*void logger(char* TAG, char *LOG)
{
	char cmd[MAX];
	sprintf(cmd, "./logger %s %s %s", TAG, LOG, WEB_LOG_PATH);
	system(cmd);
}
*/

typedef struct HTTP_MESSAGE
{
  int type;
  int content_length;
  char uri[MAX];
  char http_version[STR_HMAX];
  char host[STR_MAX];
  char connection[STR_HHMAX];
  char user_agent[STR_MAX];
  char accept[STR_MAX];
  char referer[STR_HMAX];
  char accept_encoding[STR_HMAX];
  char accept_language[STR_HMAX];
  char accept_charset[STR_HMAX];
  char content_type[STR_HMAX];
  char origin[STR_MAX];
  char cache_control[STR_HMAX];
}HTTP_MESSAGE;

int PORT;

#endif
