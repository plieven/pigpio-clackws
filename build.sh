#!/bin/bash -e

echo Checking for required binaries...
for BIN in pigs timeout curl; do
 which $BIN >/dev/null && true
 if [ $? -ne 0 ]; then
  echo ERR: required binary $BIN not found in PATH
  exit 1
 fi
done

echo
echo Building Target...
gcc -o pigpio-clackws pigpio-clackws.c -lpigpio -lrt -Wall -Werror

echo
echo Installing Binary...
sudo cp -v ./pigpio-clackws /usr/local/bin

echo
echo Installing Push Service...
sudo cp -v ./pigpio-clackws-submitData.sh /usr/local/bin

if [ ! -e /etc/systemd/system/pigpio-clackws.service ]; then
 echo Installing systemd service...
 sudo cp -v ./pigpio-clackws.service /etc/systemd/system/
fi

echo
echo Enabling pigpio-clackws systemd Service...
sudo systemctl enable pigpio-clackws.service
sudo service pigpio-clackws start

echo Enabling pigpio-clackws Push Service...
sudo sh -c "echo '* * * * * pi /usr/local/bin/pigpio-clackws-submitData.sh' >/etc/cron.d/pigpio-clackws-submitData"

echo
echo Installation complete!
echo
echo You may want to modify the GPIO pins that are monitored by editing:
echo /etc/systemd/system/pigpio-clackws.service
echo
echo You can run pigpio-clackws in monitor mode to check if you receive interrupts:
echo ./pigpio-clackws -m

