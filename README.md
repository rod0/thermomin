# thermomin
MLX90621 simple, clear and robust service for raspberry Pi .<h4>Stable but not finished.</h4><p> 
TODO:
finish Readme.md

# Installation
First of all activate i2c

```sudo raspi-config -> interfacing options -> i2c ->enable```

Select 400k as i2c baudrate 

```nano /boot/config.txt``` and add at the end:```dtparam=i2c_baudrate=400000```

```cd thermomin

make

chmod +x install.sh

sudo ./install.sh```

