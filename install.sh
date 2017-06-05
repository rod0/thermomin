#!/bin/bash
sudo cp ./thermomin.service /lib/systemd/system/
sudo cp ./thermomin /usr/bin/
sudo chmod 644 /lib/systemd/system/thermomin.service
sudo systemctl daemon-reload
sudo systemctl enable thermomin.service
sudo systemctl start thermomin.service
sudo systemctl status thermomin.service
#Check status
#sudo systemctl status thermomin.service

#Start service
#sudo systemctl start thermomin.service

#Stop service
#sudo systemctl stop thermomin.service

#Check service's log
#sudo journalctl -f -u thermomin.service

#Para ejecutar de forma automatica el programa openCV
#sudo nano ~/.config/lxsession/LXDE-pi/autostart
#añadir una ultima linea
#@lxterminal -e /home/pi/projects_ocv/Thermomin/lanzatermica.sh

#Para quitar el cursor
#sudo apt-get install unclutter
#Añadir en sudo nano ~/.config/lxsession/LXDE-pi/autostart
#@unclutter
