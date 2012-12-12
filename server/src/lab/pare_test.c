#include <stdio.h>
#include <string.h>

#define MAX 1024

char buffer[MAX] = "POST /test.html HTTP/1.1\nHost: groupn.net:8088\nConnection: keep-alive\nContent-Length: 9\nCache-Control: max-age=0\nOrigin: http://groupn.net:8088\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.95 Safari/537.11\nContent-Type: application/x-www-form-urlencoded\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\nReferer: http://groupn.net:8088/post_test.html\nAccept-Encoding: gzip,deflate,sdch\nAccept-Language: en-US,en;q=0.8\nAccept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\n\n\n\n";

char uri[MAX] = "/post_test.html?test=test&tset=test";

char* sgetline( char* buffer, char* dest ){
	   int idx = 0, d_idx = 0;;
		   int i, len;


			   while(buffer[idx] == '\n')
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


int main()
{
	char line[MAX];
	while(sgetline(buffer, line)){
		printf("line %s\n", line);
	}
	
	char* after = strtok(uri, "?");
	printf("%s\n", after);

	after = strtok(NULL, "&");
	while(after != NULL){
		printf("%s\n", after);
		after = strtok(NULL, "&");
	}
	return 0;
}
