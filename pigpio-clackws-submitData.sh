#!/bin/bash

TIMEOUT=10
VOLATILE_DIR=/var/run/pigpio-clackws
STATIC_DIR=/var/lib/pigpio-clackws
[ -z "$API" ] && API=http://home.dlhnet.de/api/s0meter

for FILE in $STATIC_DIR/uuid; do
 UUID=$(cat $STATIC_DIR/uuid)
 if [ -e $VOLATILE_DIR/lastPeriod -a -e $VOLATILE_DIR/totalCount ]; then
  LASTPERIOD=$(cat $VOLATILE_DIR/lastPeriod)
  TOTALCOUNT=$(cat $VOLATILE_DIR/totalCount)
  /usr/bin/timeout -s KILL 10 /usr/bin/curl -s -d "totalCount=$TOTALCOUNT&lastPeriod=$LASTPERIOD" -X POST $API/$UUID/submitData 2>&1 | logger -i -t 'pigpio-clackws-submitData.sh'
 fi
done
