
all: CLEAN LOGGER BACKUP.SH MAYBE USER_SHUTTLE FILE_SENDER FILE_GETTER BACKUP_COMPLETE

CLEAN:
	rm -rf ./bin
	mkdir ./bin

LOGGER:
	gcc -W -Wall -o ./bin/logger ./logger/logger.c

BACKUP.SH:
	cp ./scripts/backup.sh ./bin/backup.sh
	cp ./scripts/backup.list ./bin/backup.list
	cp ./scripts/add_crontab.sh ./bin/add_crontab.sh
MAYBE:
	gcc -W -Wall -o ./bin/maybe ./maybe/maybe.c -D _XOPEN_SOURCE

USER_SHUTTLE:
	gcc -W -Wall -o ./bin/user_shuttle ./shuttle/user_shuttle.c

BACKUP_SHUTTLE:
	gcc -W -Wall -o ./bin/backup_shuttle ./shuttle/backup_shuttle.c

RECOVERY_SHUTTLE:
	gcc -W -Wall -o ./bin/recovery_shuttle ./shuttle/recovery_shuttle.c

FILE_SENDER:
	gcc -W -Wall -o ./bin/file_sender ./file_sender/file_sender.c -D _XOPEN_SOURCE

FILE_GETTER:
	gcc -W -Wall -o ./bin/file_getter ./file_getter/file_getter.c -D _XOPEN_SOURCE

BACKUP_COMPLETE:
	gcc -W -Wall -o ./bin/backup_complete ./backup_complete/backup_complete.c -D _XOPEN_SOURCE

release: all
	rm -rf ../conf ../bin ../rec ../log
	mkdir ../conf ../bin ../log
	\cp -f ./tmp/backup2.conf ../conf/backup.conf
	\cp -rf ./bin/* ../bin
	rm -rf ../bin/backup.list
	\cp -f ./tmp/backup2.list ../bin/backup.list
	

