/**
Thermomin. MLX90621 simple, clear and robust daemon for Raspberry Pi.
It writes a fifo file with the 64 pixels data in float (32bits) of the Celsius temperature values registered by MLX90621.
run $sudo ./thermomin &
Based on simple demonstration daemon for the MLX9062x 16x4 thermopile array Copyright (C) 2015 Chuck Werbick
modified by https://github.com/terickson/mlxd
i2c comms by Reinoso G. 09/02/2017 https://github.com/electronicayciencia/wPi_soft_i2c
It doesn't require BCM2835 library
Rodo Lillo 01/06/2017

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "thermomin.h"

byte refreshRate;         //Set this value to your desired refresh frequency. See thermomin.h
float temperatures[64],   //Contains the calculated temperatures of each pixel in the array as float
Tambient,             //Tracks the changing ambient temperature of the sensor
Tmax, Tmin;               //Keep the current maximum minimum temperature
int ic;
byte eepromData[256];     //Contains the full EEPROM reading from the MLX90621

int16_t k_t1_scale,
k_t2_scale,
resolution = 0,
cpix = 0,
ptat = 0,
configuration = 0,
irData[64];               //Contains the raw IR data from the sensor

i2c_t my_bus;
const char mlxFifo[] = "/var/run/mlx9062x.sock" ; //Fifo pipe with the 64 float values temperatures array

int main (int argc, char **argv) {
  int fd;
  #ifdef _DEBUG_
  int k; //Test purposes
  #endif

  mkfifo(mlxFifo, 0666);
  if (setup()) {
    printf("Setup error. Probably an i2c error.");
    return 1;
  }

  while (1)
  {
    if (checkConfig())
    {
      readEEPROM();
      writeTrimmingValue();
      setConfiguration();
      readRes();
    }
    for (ic = 0; ic < 16; ic++)    //Every 16 readings check that the POR flag is not set
    {
      readPTAT();
      calculateTA();
      readCPIX();
      readIR();
      calculateTO();

      fd = open(mlxFifo, O_WRONLY);
      write(fd, temperatures, sizeof(temperatures));
      close(fd);
      usleep(5000);

      #ifdef _DEBUG_       //Test purposes
      for (k = 0; k < 64; k++) {
        printf("[%.2f]", temperatures[k]);
      }
      printf("\n");
      #endif
    }
  }
  return 0;
}

int setup() {
  /* You have to select i2c Freq. in soft_12c.h #define I2C_FREQ 400000
    (I use 400K). The frecuency has to be the same as you selected in OS.
    Run raspi-config and activate i2c. Test it running $i2cdetect -y 1
    Edit /boot/config.txt and add at the end: dtparam=i2c_baudrate=400000
    see: https://projects.drogon.net/raspberry-pi/wiringpi/i2c-library/
  */
  if (wiringPiSetup () == -1) return 1;
  my_bus = i2c_init(9, 8);
  i2c_reset(my_bus);
  refreshRate = Hz_LSB_32; // see thermomin.h
  usleep(5000);
  readEEPROM();
  writeTrimmingValue();
  setConfiguration();
  readRes();
  readPTAT();
  calculateTA();
  printf("Service running. Ta=%f\n",Tambient);
  return 0;
}

void readEEPROM()
{
  int j;
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x50 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x00);
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x50 << 1 | I2C_READ);
  for (j = 0; j < 256; j ++)
  {
    eepromData[j] = i2c_read_byte(my_bus);
    i2c_send_bit(my_bus, I2C_ACK);
  }
  i2c_stop(my_bus);
}

void writeTrimmingValue()
{
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x04);
  i2c_send_byte(my_bus, (byte)eepromData[OSC_TRIM_VALUE] - 0xAA);
  i2c_send_byte(my_bus, eepromData[OSC_TRIM_VALUE]);
  i2c_send_byte(my_bus, 0x56);
  i2c_send_byte(my_bus, 0x00);
  i2c_stop(my_bus);
}

void setConfiguration()
{
  byte defaultConfig_H = 0b01000100;            //0x54=0b00000100;
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x03);
  i2c_send_byte(my_bus, (byte)refreshRate - 0x55);
  i2c_send_byte(my_bus, refreshRate);
  i2c_send_byte(my_bus, defaultConfig_H - 0x55);
  i2c_send_byte(my_bus, defaultConfig_H);
  i2c_stop(my_bus);
}

void readRes()
{
  resolution = (readConfig() & 0x30) >> 4;      //Read the resolution from the config register
}

void readPTAT()                     //Absolute ambient temperature data of the device can be read by using the following function.
{
  byte ptatLow = 0, ptatHigh = 0;
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x02);
  i2c_send_byte(my_bus, 0x40);
  i2c_send_byte(my_bus, 0x00);
  i2c_send_byte(my_bus, 0x01);
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_READ);
  ptatLow=i2c_read_byte(my_bus);
  i2c_send_bit(my_bus, I2C_ACK);
  ptatHigh=i2c_read_byte(my_bus);
  i2c_send_bit(my_bus, I2C_ACK);
  i2c_stop(my_bus);
  ptat = ((uint16_t) (ptatHigh << 8) | ptatLow);
}

void readCPIX()                     //Compensation pixel data of the device can be read by using the following function.
{
  byte cpixLow = 0, cpixHigh = 0;
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x02);
  i2c_send_byte(my_bus, 0x41);
  i2c_send_byte(my_bus, 0x00);
  i2c_send_byte(my_bus, 0x01);
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_READ);
  cpixLow=i2c_read_byte(my_bus);
  i2c_send_bit(my_bus, I2C_ACK);
  cpixHigh=i2c_read_byte(my_bus);
  i2c_send_bit(my_bus, I2C_ACK);
  i2c_stop(my_bus);
  cpix = ((int16_t) (cpixHigh << 8) | cpixLow);
  if (cpix >= 32768)    cpix -= 65536;
}

bool checkConfig()           //Poll the MLX90621 for its current status. Returns true if the POR/Brown out bit is set
{
  bool check = !((readConfig() & 0x0400) >> 10);
  return check;
}

uint16_t readConfig()
{
  byte configLow = 0, configHigh = 0;
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x02);
  i2c_send_byte(my_bus, 0x92);
  i2c_send_byte(my_bus, 0x00);
  i2c_send_byte(my_bus, 0x01);
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_READ);
  configLow=i2c_read_byte(my_bus);
  i2c_send_bit(my_bus, I2C_ACK);
  configHigh=i2c_read_byte(my_bus);
  i2c_send_bit(my_bus, I2C_ACK);
  i2c_stop(my_bus);
  configuration = ((uint16_t) (configHigh << 8) | configLow);
  return configuration;
}

void readIR()     //IR data of the device that it is read as a whole.
{
  int j;

  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_WRITE);
  i2c_send_byte(my_bus, 0x02);
  i2c_send_byte(my_bus, 0x00);
  i2c_send_byte(my_bus, 0x01);
  i2c_send_byte(my_bus, 0x40);
  i2c_start(my_bus);
  i2c_send_byte(my_bus, 0x60 << 1 | I2C_READ);

  for (j = 0; j < 64; j++)
  {
    byte pixelDataLow = i2c_read_byte(my_bus);
    i2c_send_bit(my_bus, I2C_ACK);
    byte pixelDataHigh = i2c_read_byte(my_bus);
    i2c_send_bit(my_bus, I2C_ACK);
    irData[j] = (int16_t) ((pixelDataHigh << 8) | pixelDataLow);  //Each pixel value takes up two bytes thus NUM_PIXELS * 2
  }
  i2c_stop(my_bus);
}

void calculateTA()                                          //See Datasheet MLX90621
{
  float v_th = 0, k_t1 = 0, k_t2 = 0;

  k_t1_scale = (int16_t) (eepromData[KT_SCALE] & 0xF0) >> 4;    //KT_SCALE=0xD2[7:4]
  k_t2_scale = (int16_t) (eepromData[KT_SCALE] & 0x0F) + 10;    //KT_SCALE=0xD2[3:0]+10
  v_th = (float) 256 * eepromData[VTH_H] + eepromData[VTH_L];
  if (v_th >= 32768.0)   v_th -= 65536.0;
  v_th = v_th / pow(2, (3 - resolution));
  k_t1 = (float) 256 * eepromData[KT1_H] + eepromData[KT1_L];
  if (k_t1 >= 32768.0)   k_t1 -= 65536.0;
  k_t1 /= (pow(2, k_t1_scale) * pow(2, (3 - resolution)));
  k_t2 = (float) 256 * eepromData[KT2_H] + eepromData[KT2_L];
  if (k_t2 >= 32768.0)   k_t2 -= 65536.0;
  k_t2 /= (pow(2, k_t2_scale) * pow(2, (3 - resolution)));      //0.000768
  Tambient = ((-k_t1 + sqrt((k_t1*k_t1) - (4 * k_t2 * (v_th - (float) ptat)))) / (2 * k_t2)) + 25.0;
}

void calculateTO()                                              //See Datasheet MLX90621
{
  float a_ij[64], b_ij[64], alpha_ij[64];
  float emissivity, tgc, alpha_cp, a_cp, b_cp;
  int16_t a_common, a_i_scale, b_i_scale;
  int i;

  emissivity = (256 * eepromData[CAL_EMIS_H] + eepromData[CAL_EMIS_L]) / 32768.0;
  a_common = (int16_t) 256 * eepromData[CAL_ACOMMON_H] + eepromData[CAL_ACOMMON_L];
  if (a_common >= 32768)    a_common -= 65536;
  alpha_cp = (256 * eepromData[CAL_alphaCP_H] + eepromData[CAL_alphaCP_L]) / (pow(2, CAL_A0_SCALE) * pow(2, (3 - resolution)));
  a_i_scale = (int16_t) (eepromData[CAL_AI_SCALE] & 0xF0) >> 4;
  b_i_scale = (int16_t) eepromData[CAL_BI_SCALE] & 0x0F;
  a_cp = (float) 256 * eepromData[CAL_ACP_H] + eepromData[CAL_ACP_L];
  if (a_cp >= 32768.0)    a_cp -= 65536.0;
  a_cp /= pow(2, (3 - resolution));
  b_cp = (float) eepromData[CAL_BCP];
  if (b_cp > 127.0)     b_cp -= 256.0;
  b_cp /= (pow(2, b_i_scale) * pow(2, (3 - resolution)));
  tgc = (float) eepromData[CAL_TGC];
  if (tgc > 127.0)  tgc -= 256.0;
  tgc /= 32.0;
  float v_cp_off_comp = (float) cpix - (a_cp + b_cp * (Tambient - 25.0));
  float v_ir_off_comp, v_ir_tgc_comp, v_ir_norm, v_ir_comp;
  Tmin = 250;
  Tmax = -250;
  for (i = 0; i < 64; i++)
  {
    a_ij[i] = ((float) a_common + eepromData[i] * pow(2, a_i_scale)) / pow(2, (3 - resolution));
    b_ij[i] = eepromData[0x40 + i];
    if (b_ij[i] > 127)        b_ij[i] -= 256;
    b_ij[i] /= (pow(2, b_i_scale) * pow(2, (3 - resolution)));
    v_ir_off_comp = irData[i] - (a_ij[i] + b_ij[i] * (Tambient - 25.0));
    v_ir_tgc_comp = v_ir_off_comp - tgc * v_cp_off_comp;
    alpha_ij[i] = ((256 * eepromData[CAL_A0_H] + eepromData[CAL_A0_L]) / pow(2, eepromData[CAL_A0_SCALE]));
    alpha_ij[i] += (eepromData[0x80 + i] / pow(2, eepromData[CAL_DELTA_A_SCALE]));
    alpha_ij[i] /= pow(2, 3 - resolution);
    v_ir_norm = v_ir_tgc_comp / (alpha_ij[i] - tgc * alpha_cp);
    v_ir_comp = v_ir_norm / emissivity;
    temperatures[i] = sqrt(sqrt((v_ir_comp + pow((Tambient + 273.15), 4)))) - 273.15;
    if (temperatures[i] < Tmin) Tmin = temperatures[i];
    if (temperatures[i] > Tmax) Tmax = temperatures[i];
  }
}
