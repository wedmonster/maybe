#!/bin/bash
export LANG=en
cd /tmp/backup
TODAY=`date +"%d %b %Y"`
PREVIOUS=`cat FULL`
echo $PREVIOUS
tmp_tar=/tmp/backup/inc/tmp_$( date +%Y%m%d ).tar.gz
tar cvzpfG $tmp_tar --newer=$PREVIOUS /home/montecast/UNIX/project/scripts/tmp/*
chmod 700 *.tar.gz

