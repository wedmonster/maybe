
all: CLEAN LOGGER BACKUP.SH MAYBE USER_SHUTTLE BACKUP_SHUTTLE RECOVERY_SHUTTLE

CLEAN:
	rm -rf ./bin
	mkdir ./bin

LOGGER:
	gcc -W -Wall -o ./bin/logger ./logger/logger.c

BACKUP.SH:
	cp ./scripts/backup.sh ./bin/backup.sh
	cp ./scripts/backup.list ./bin/backup.list
MAYBE:
	gcc -W -Wall -o ./bin/maybe ./maybe/maybe.c -D _XOPEN_SOURCE

USER_SHUTTLE:
	gcc -W -Wall -o ./bin/user_shuttle ./shuttle/user_shuttle.c

BACKUP_SHUTTLE:
	gcc -W -Wall -o ./bin/backup_shuttle ./shuttle/backup_shuttle.c

RECOVERY_SHUTTLE:
	gcc -W -Wall -o ./bin/recovery_shuttle ./shuttle/recovery_shuttle.c

release: all
	rm -rf ../conf ../bin ../rec ../log
	mkdir ../conf ../bin ../log
	\cp -f ./conf/backup.conf ../conf/backup.conf
	\cp -rf ./bin/* ../bin
