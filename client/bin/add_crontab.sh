#!/bin/bash

PWD=`pwd`;


echo "0 1-3 * * * $PWD/backup.sh" | crontab - e
