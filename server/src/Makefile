all: clean LOGGER WEB_SERVER HTML_GEN BACKUP_SERVER MAIL_SENDER

clean:
	rm -f ./bin/logger ./bin/backup_server ./bin/web_server ./bin/HTMLgen
	
LOGGER:
	gcc -W -Wall -o ./bin/logger ./logger/logger.c

BACKUP_SERVER:
	gcc -W -Wall -o ./bin/backup_server ./backup_server/backup_server.c -D _XOPEN_SOURCE=500 

WEB_SERVER:
	gcc -W -Wall -o ./bin/web_server ./web_server/web_server.c

HTML_GEN:
	gcc -W -Wall -o ./bin/HTMLgen ./HTMLgen/HTMLgen.c -D _XOPEN_SOURCE

MAIL_SENDER:
	gcc -W -Wall -o ./bin/mail_sender ./mail_sender/mail_sender.c -D _XOPEN_SOURCE

release: all
	rm -rf ../html ../bin ../mail ../conf
	mkdir ../html ../bin ../mail ../conf
	\cp -rf ./html/index.html ../html
	\cp -rf ./conf/* ../conf/
	\cp -f ./bin/*	../bin/

