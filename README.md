# thermomin
MLX90621 simple, clear and robust service for raspberry Pi <p> 
TODO:
finish Readme.md
# Introduction
The purpose of this project was to make a robust service to capture MLX90621 data using WirinPi library. The data are stored in a fifo file to be taken by other applications. In the examples folder there are two python programs to adquire the data. Simpletest.py is a minimum program to test if the service is working rigth. Ocv_camera is a thermal camera running at 15fps using OpenCV.

![alt text](http://i.imgur.com/c2Vv2Rv.png)

# Installation
First of all activate i2c

```sudo raspi-config ->interfacing options ->i2c ->enable```

Select 400k as i2c baudrate 

```sudo nano /boot/config.txt``` and add at the end: ```dtparam=i2c_baudrate=400000```

```sudo reboot```

```git clone https://github.com/rod0/thermomin```

```cd thermomin```

```make```

```chmod +x install.sh```

```sudo ./install.sh```

Test the service

```cd ./examples```

```python ./simpletest.py```
