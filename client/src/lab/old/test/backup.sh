#!/bin/bash
export LANG=en
cd /tmp/backup
find /tmp/backup -mtime +7 -exec rm -f {} ';'
tmp_tar=/tmp/backup/tmp_$( date +%Y%m%d ).tar.gz
tar cvzpfP $tmp_tar /home/montecast/UNIX/project/scripts/tmp/*
chmod 700 *.tar.gz
echo `date +"%F"` > FULL
#date > FULL


