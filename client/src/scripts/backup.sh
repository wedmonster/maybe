#!/bin/bash

export LANG=en

ERR="ERROR"
TAR="TAR"
SHL="SHELL"

BACKUP_DIR=""
BACKUP_CONF="../conf/backup.conf"
BACKUP_LOG="../log/backup.log"
BACKUP_DATE=""
LOGGER="./logger"
TOTAL_PATH=""

#logger : logger function
function logger()
{
	if [ $# != 2 ]
	then
		#echo "logger argument error"
		`"$LOGGER" "$ERR" "logger argument error" "$BACKUP_LOG"`
		exit 1
	fi
	
	DATE=`date +"%F %T"`
	STATE="$1"
	LOG="$2"

	CMD="$LOGGER \"$STATE\" \"$LOG\" \"$BACKUP_LOG\""

	eval "$CMD"

	#printf "[%s %s] %s\n" "$DATE" "$STATE" "$LOG" >> "$BACKUP_LOG"
}

#set_env : set environmental variables from $BACKUP_CONF
function set_env()
{
	line=`sed '/^$/d' $BACKUP_CONF | sed '/#/d' | sed 's/\n/ /g'`
	
	BACKUP_DIR=`echo "$line" | awk /@BackupDir/'{print $2}'`
	BACKUP_LIST=`echo "$line" | awk /@ListFileName/'{print $2}'`

	if [ -z "$BACKUP_DIR" ]
	then
		logger "$ERR"  "Backup Directory is must be setted. Check configuration file."
		#echo "Backup Directory is must be setted. Check configuration file."
		exit 1
	fi

	if [ -z "$BACKUP_LIST" ]
	then
		logger "$ERR" "This shell need a backup list file. Check configuration file."
		#echo "This shell need a backup list file. Check configuration file."
		exit 1
	fi

	if [ ! -f "./$BACKUP_LIST" ]
	then
		logger "$ERR"  "Backup List file dosen't exist. Check the file."
		#echo "Backup List file dosen't exist. Check the file."
		exit 1
	fi
}


#logger "$SHL" "backup.sh is running."

if [ ! -f "$BACKUP_CONF" ]
then
	# LOGGING
	logger "$ERR" "There is no backup.conf."
	exit 1
fi

set_env

#check_dir(path) : check directory path and make directory if the path is invaild.
function check_dir()
{
	_PATH=$1;
	if [ ! -d "$_PATH" ]
	then
		logger "$SHL" "Make directory:$_PATH"
		mkdir "$_PATH"
		chmod 600 "$_PATH"
	fi
}


function remove_dir()
{
	logger "$SHL" "Remove backup directory:$BACKUP_DIR"
	rm -rf "$BACKUP_DIR"
	check_dir "$BACKUP_DIR"
}

function set_total_path()
{
	line=`sed '/^$/d' "./$BACKUP_LIST" | sed '/#/d' | sed 's/\n/ /g'`

	TOTAL_PATH=""
	for arg in $line 
	do
		echo "$arg"
		#LOGGING NO FILE OR DRIECTORY
		if [ -d "$arg" -o -f "$arg" ]
		then
			TOTAL_PATH="$TOTAL_PATH $arg"
		fi
	done
}

function full_backup()
{
	logger "$SHL" "Start Full Backup"
	
	TODAY=`date +%a`
	FILE_PATH="$BACKUP_DIR/$TODAY"
	check_dir "$FILE_PATH"

	set_total_path
	
	DATE=`date +%Y-%m-%d`
	LOG_DATE=`date +%Y-%m-%d`
	BACKUP_DATE="$LOG_DATE"

	LOG_PATH="$FILE_PATH/backup.$LOG_DATE.log"
	#check file
	BACKUP_FILE="$FILE_PATH/$DATE.tgz"
	logger "$SHL" "Backup Path:$BACKUP_FILE"

	#LOGGING
	CMD="tar cvzpfP $BACKUP_FILE $TOTAL_PATH"

	#just execution dosen't work. I have to figure it out.
	eval "$CMD" > "$LOG_PATH"

	chmod 600 "$BACKUP_FILE"

	echo `date +%Y-%m-%d` > "$BACKUP_DIR/FULL"

	logger "$SHL" "Complete Full Backup"
}

function incremental_backup()
{
	logger "$SHL" "Start Incremental Backup"

	TODAY=`date +%a`
	FILE_PATH="$BACKUP_DIR/$TODAY"
	check_dir "$FILE_PATH"

	set_total_path

	DATE=`date +%Y-%m-%d`
	BACKUP_DATE="$DATE";
	LOG_DATE="$DATE"
	LOG_PATH="$FILE_PATH/backup.$LOG_DATE.log"

	if [ -e "$BACKUP_DIR/B_DATE" ]
	then
		PREVIOUS=`cat $BACKUP_DIR/B_DATE`
	elif [ -e "$BACKUP_DIR/FULL" ]
	then
		PREVIOUS=`cat $BACKUP_DIR/FULL`
	else
		logger "$ERR" "Incremental Backup Error : no comp file."
		exit 1
	fi
	
	BACKUP_FILE="$FILE_PATH/$DATE.tgz"
	logger "$SHL" "Backup Path:$BACKUP_FILE"
	CMD="tar cvzpfP $BACKUP_FILE --newer=$PREVIOUS $TOTAL_PATH" > "$LOG_PATH"

	eval "$CMD"

	chmod 600 "$BACKUP_FILE"

	echo `date +%Y-%m-%d` > "$BACKUP_DIR/B_DATE"

	logger "$SHL" "Complete Incremental Backup"

}

check_dir "$BACKUP_DIR"

# if today is sunday
#  Remove Before Backup Files
# 	Do Full Backup
# else
#	Do Incremental Backup

TODAY=`date +%w`
_TODAY=`date +%A`
TODAY_STR=`date +%Y-%m-%d`
logger "$SHL" "Today is $_TODAY"

# No FULL file or Today is sunday
if [ ! -e "$BACKUP_DIR/FULL" -o $TODAY = 0 ]
then
	remove_dir
	full_backup
# else there is B_DATE. its mean is there is INC_BACKUP
elif [ -e "$BACKUP_DIR/B_DATE" ]
then
	PREV_INC_DATE=`cat $BACKUP_DIR/B_DATE`
	
	if [ "$TODAY_STR" = "$PREV_INC_DATE" ]
	then
		full_backup
	elif [ "$TODAY_STR" \> "$PREV_INC_DATE" ]
	then
		incremental_backup
	else
		remove_dir
		full_backdup
	fi
else
	PREV_FULL_DATE=`cat $BACKUP_DIR/FULL`
	if [ "$TODAY_STR" =  "$PREV_FULL_DATE" ]
	then
		full_backup
	elif [ "$TODAY_STR" \> "$PREV_FULL_DATE" ]
	then
		incremental_backup
	else
		remove_dir
		full_backup
	fi
fi

#call File shuttle
./maybe -s "$BACKUP_DATE"

logger "$SHL" ""


