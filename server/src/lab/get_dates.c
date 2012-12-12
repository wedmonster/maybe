#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <memory.h>

int main()
{
	time_t ct2, ct3;
	//struct tm* mtime2;
	//mtime2 = malloc(sizeof(struct tm));
	char buf[100];
	struct tm mtime2;
	struct tm mtime3;
	struct tm *tm2;
	int itv, i;
	memset(&mtime2, 0, sizeof(mtime2));
	memset(&mtime3, 0, sizeof(mtime3));

	strptime("2012-12-09", "%Y-%m-%d", &mtime2);
	strptime("2012-12-13", "%Y-%m-%d", &mtime3);

	ct2 = mktime(&mtime2);
	ct3 = mktime(&mtime3);
	printf("%d\n", (ct3 - ct2)/86400);

	strftime(buf, sizeof(buf), "%w", &mtime3);
	
	itv = atoi(buf);
	
	for(i = 0; i < itv; i++){
		ct3 -= 86400;
		tm2 = localtime(&ct3);
		strftime(buf, sizeof(buf), "%Y-%m-%d", tm2);
		printf("%s\n", buf);
	}
	//free(mtime2);
	return 0;
}
