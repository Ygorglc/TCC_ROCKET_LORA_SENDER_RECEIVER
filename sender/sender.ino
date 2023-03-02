/*
 * LoRa Sender DEMO
 * board used: LILYGO® TTGO LoRa32 V2.1_1.6
 * Base code by Bernardo Giovanni (https://www.settorezero.com)
*/


#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h> 
#include <SSD1306Wire.h>
#include "board_defs.h" // in this header I've defined the pins used by the module LILYGO® TTGO LoRa32 V2.1_1.6
#include "images.h"
#include "my_fonts.h"
//#include "bmp280.h"
//#include "hmc5883l.h"
//#include "mpu6050.h"
#include "sd_card.h"
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

#include <MechaQMC5883.h>

MechaQMC5883 qmc;
Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;

String packet; // packet to be sent over LoRa
uint32_t counter=0; // a demo variable to be included in the packet
const char PROGMEM demoFileName[]="/demo.txt"; // a demoFileName where the packet will be saved on the SD
bool sdpresent=false; // keep in mind if there is an SD mounted
float battery=0; // battery voltage

SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
SPIClass sdSPI(HSPI); // we'll use the SD card on the HSPI (SPI2) module
SPIClass loraSPI(VSPI); // we'll use the LoRa module on the VSPI (SPI3) module

float ax = 0;
float ay = 0;
float az = 0;
int mx = 0;
int my = 0;
int mz = 0;
float gx = 0;
float gy = 0;
float gz =0;
float bar = 0;
//temperatura
float tem = 0;
//tempo
float t = 0;
float idT = 0;
//posição
float p =0;
//altitude
float a = 0;

float time_to_action = millis();
int modo_lora = 1;
String packSize = "--";
String rssi = "RSSI --";

void setup() 
  {
  // Begins some pins
  pinMode(LED,OUTPUT); // Green Led
  pinMode(VBAT,INPUT); // Input for Battery Voltage

  // Begins UART
  Serial.begin(115200);
  while (!Serial);
  delay(500);
  Serial.println();
  Serial.println("LoRa Sender Test By Giovanni Bernardo");
  Serial.println("https://www.settorezero.com");

  // Begins OLED
  display.init();
  // following setting will allow to read the display frontal keeping the antenna on top
  display.flipScreenVertically(); 
  display.clear();
  delay(100); 
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Monospaced_bold_11);

  // Begins LoRa Module
  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  LoRa.setSPI(loraSPI);
  if (!LoRa.begin(LORA_BAND)) 
    {
    Serial.println("LoRa failed");
    display.drawString(0,0,"LoRa failed");
    display.display();
    while (1);
    }
  Serial.println("LoRa OK");
  display.drawString(0,0,"LoRa OK");
  display.display();
  // set a sync word to be used with both modules for synchronization
  LoRa.setSyncWord(0xF3); // ranges from 0-0xFF, default 0x34, see API docs
  //LoRa.receive();
  delay(1000);

  // Begins SD Card
  sdSPI.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
  // the board does not differentiate No SD card or mounting failed
  // probably for doing this would be necessary a switch for detecting SD Card insertion
  if (!SD.begin(SDCARD_CS, sdSPI)) 
    {
    Serial.println("No SDCard OR Failed");
    display.drawString(0,11,"No SDCard OR FAIL");
    display.display();
    } 
  else 
    {
    uint8_t cardType = SD.cardType();
    sdpresent=true;
    uint32_t SDCardSize = SD.cardSize()/(1024*1024);
    Serial.print("SDCard OK");
    Serial.print(" - Size: ");
    Serial.print(SDCardSize);
    Serial.println("MB");
    display.drawString(0,11,"Card size: "+String(SDCardSize) + "MB");
    display.display();
      
    Serial.print("SD Card Type: ");
    switch(cardType)
      {
      case CARD_MMC:
      Serial.println("MMC");
      break;
      case CARD_SD:
      Serial.println("SDSC");
      break;
      case CARD_SDHC:
      Serial.println("SDHC");
      break;
      // a 4th case is CARD_NONE
      // but is not evaluated with this TFreader
      default:
      Serial.println("UNKNOWN");
      break;
      }
    // create the demofile if don't exists or truncate it to zero
    writeFile(SD, demoFileName, "LoRa Sender Demo\nBy Bernardo Giovanni\nhttps://www.settorezero.com\n");  
    } // SD Mounted and Present
  delay(1000);
  
  // check battery
  battery=checkBattery(); // first battery check at start-up
  Serial.print("Battery Voltage: ");
  Serial.print(battery);
  Serial.println("V");
  display.drawString(0,22,"Battery: "+String(battery)+"V");
  display.display();
  delay(1000);
  
  Serial.println("Init OK");
  display.drawString(0,33,"Init OK");
  display.display();
  delay(1000);
  display.clear();

  // Draw logos
  display.drawXbm(0, 0, SZ_logo_width, SZ_logo_height, SZ_logo);
  display.display();
  delay(1500);
  display.clear();
  
  display.drawXbm(0, 18, Digitspace_logo_width, Digitspace_logo_height, Digitspace_logo);
  display.display();
  delay(1500);
  display.clear();

  setup_qmc5883();
  setup_bmp280();
  setup_MPU6050();
}
void setup_qmc5883(){
   Wire.begin();
   qmc.init();
}
void setup_bmp280(){
  while(!bmp.begin(0x77)){
    Serial.println(F("Sensor BMP280 não foi identificado! Verifique as conexões."));
  }
}

void setup_MPU6050(){
      
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
}
void loop() 
  {
  dados_bmp280();
  dados_mpu6050();
  dados_qmc5883();
  // variables used for checking the battery and give an average value
  int packetSize=LoRa.parsePacket();
  if (packetSize) getLoRaPacket(packetSize);
  }


void setLoRaPacket() 
  {
  static uint8_t chkbat=0;
  static float tempbatt=0;
  LedOn();
  display.clear();
  display.drawXbm(37, 0, LoRa_logo_width, LoRa_logo_height, LoRa_logo);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Monospaced_bold_11);

  display.drawString(8,31, "LoRa SENDER DEMO");
  display.drawString(0,42, "Battery: "+String(battery)+"V");
  
  display.drawString(0, 52,"Packet: ");
  display.drawString(48,52, String(counter));
  Serial.println(String(counter));
  display.display();

  String httpRequestDataObjectJson = "{\"ax\": \""+String(ax)+"\",";
                 httpRequestDataObjectJson +="\"ay\": \""+String(ay)+"\",";
                 httpRequestDataObjectJson += "\"az\": \""+String(az)+"\",";
                 httpRequestDataObjectJson += "\"gx\": \""+String(gx)+"\",";
                 httpRequestDataObjectJson += "\"gy\": \""+String(gy)+"\",";
                 httpRequestDataObjectJson += "\"gz\": \""+String(gz)+"\",";
                 httpRequestDataObjectJson += "\"mx\": \""+String(mx)+"\",";
                 httpRequestDataObjectJson += "\"my\": \""+String(my)+"\",";
                 httpRequestDataObjectJson += "\"mz\": \""+String(mz)+"\",";
                 httpRequestDataObjectJson += "\"bar\": \""+String(bar)+"\",";
                 httpRequestDataObjectJson += "\"a\": \""+String(a)+"\",";
                 httpRequestDataObjectJson += "\"tem\": \""+String(tem)+"\",";
                 httpRequestDataObjectJson += "\"t\": \""+String(t)+"\",";
                 httpRequestDataObjectJson += "\"p\": \""+String(p)+"\",";
                 httpRequestDataObjectJson += "\"idT\": \""+String(idT)+"\"}";
                 
  // send packet over LoRa
  packet=httpRequestDataObjectJson;
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
  Serial.print("Packet: ");
  Serial.println(packet);
  // add a newline for the text file
  packet+=String("\n");
  if (sdpresent)
    {
    appendFile(SD, demoFileName, packet.c_str()); // log on the SD card if present
    }
  counter++;
  
  // battery voltage as average over 20 readings
  tempbatt+=checkBattery();
  chkbat++;
  if (chkbat==20)
    {
    chkbat=0;
    battery=(tempbatt/20);
    Serial.print("Battery voltage: ");
    Serial.println(battery);
    tempbatt=0;
    }

  //delay(50);
  LedOff();
  //delay(100);
  }
void getLoRaPacket(int packetSize) 
  {
  LedOn();
  packet="";
  packSize=String(packetSize,DEC);
  for (int i=0; i<packetSize; i++) 
    {
    packet +=(char)LoRa.read(); // re-construct packet char by char
    }
  rssi="RSSI:"+String(LoRa.packetRssi(), DEC);
  Serial.print("Packet received: ");
  Serial.print(packet);
  Serial.print(" ");
  Serial.println(rssi);
  if((packet.startsWith("#") && packet.endsWith("#"))){
    LoRa.beginPacket();
    LoRa.print("#-#");
    LoRa.endPacket();
    Serial.println("Acionar paraquedas!!!");
  }else if ((packet.startsWith("*") && packet.endsWith("*"))){
    setLoRaPacket();
  }
  // packet starts with @ and ends with #
  if ((packet.startsWith("{") && packet.endsWith("}")))
    {
     modo_lora = 2;
    if (sdpresent)
      {
      packet+=" "+rssi+"\n";  
      appendFile(SD, demoFileName, packet.c_str()); // log on the SD card if present
      }
  }else if((packet.startsWith("#") && packet.endsWith("#"))){
    Serial.println("paraquedas ativado");
  }
  else
    {
    Serial.println("Packet error");  
    }
  LedOff();
  }
void dados_qmc5883(){
  qmc.read(&mx,&my,&mz);

  Serial.print("x: ");
  Serial.print(mx);
  Serial.print(" y: ");
  Serial.print(my);
  Serial.print(" z: ");
  Serial.print(mz);
  Serial.println();
}
void dados_mpu6050(){
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    gx = float(g.gyro.x);
    gy = float(g.gyro.y);
    gz = float(g.gyro.z);
}
void dados_bmp280(){
  tem= bmp.readTemperature();
  bar = bmp.readPressure();
  a = bmp.readAltitude(1013.25);
  Serial.print(F("Temperatura: ")); //IMPRIME O TEXTO NO MONITOR SERIAL
    Serial.print(tem); //IMPRIME NO MONITOR SERIAL A TEMPERATURA
    Serial.println(" *C (Grau Celsius)"); //IMPRIME O TEXTO NO MONITOR SERIAL
    
    Serial.print(F("Pressão: ")); //IMPRIME O TEXTO NO MONITOR SERIAL
    Serial.print(bar); //IMPRIME NO MONITOR SERIAL A PRESSÃO
    Serial.println(" Pa (Pascal)"); //IMPRIME O TEXTO NO MONITOR SERIAL

    Serial.print(F("Altitude aprox.: ")); //IMPRIME O TEXTO NO MONITOR SERIAL
    Serial.print(a,0); //IMPRIME NO MONITOR SERIAL A ALTITUDE APROXIMADA
    Serial.println(" m (Metros)"); //IMPRIME O TEXTO NO MONITOR SERIAL
    
    Serial.println("-----------------------------------"); //IMPRIME UMA LINHA NO MONITOR SERIAL
}
void LedOn()
    {
    digitalWrite(LED, HIGH);
    }

void LedOff()
    {
    digitalWrite(LED, LOW);
    }

float checkBattery(void)
  {
  // The ADC value is a 12-bit number (values from 0 to 4095)
  // To convert the ADC integer value to a real voltage you’ll need to divide it by the maximum value of 4095
  // then double it since there is an 1:2 voltage divider on the battery
  // then you must multiply that by the reference voltage of the ESP32 which is 3.3V 
  // and then multiply that again by the ADC Reference Voltage of 1100mV.
  return (float)(analogRead(VBAT))/4095*2*3.3*1.1;
  }

/************************************************************************************************************
 * SD File Functions taken from official Arduino-ESP32 repository, SD examples:
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
 ************************************************************************************************************/
void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
  {
  Serial.printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if(!root)
    {
    Serial.println("Failed to open directory");
    return;
    }
  if(!root.isDirectory())
    {
    Serial.println("Not a directory");
    return;
    }

  File file = root.openNextFile();
  while(file)
    {
    if(file.isDirectory())
      {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels)
        {
        listDir(fs, file.name(), levels -1);
        }
      } 
    else 
      {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
      }
    file = root.openNextFile();
    }
  }

void createDir(fs::FS &fs, const char * path)
  {
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path))
    {
    Serial.println("Dir created");
    } 
  else 
    {
    Serial.println("mkdir failed");
    }
  }

void removeDir(fs::FS &fs, const char * path)
  {
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path))
    {
    Serial.println("Dir removed");
    } 
  else 
    {
    Serial.println("rmdir failed");
    }
  }

void readFile(fs::FS &fs, const char * path)
  {
  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);
  if(!file)
    {
    Serial.println("Failed to open file for reading");
    return;
    }
  Serial.print("Read from file: ");
  while(file.available())
    {
    Serial.write(file.read());
    }
  file.close();
  }

void writeFile(fs::FS &fs, const char * path, const char * message)
  {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file)
    {
    Serial.println("Failed to open file for writing");
    return;
    }
  if(file.print(message))
    {
    Serial.println("File written");
    } 
  else 
    {
    Serial.println("Write failed");
    }
  file.close();
  }

void appendFile(fs::FS &fs, const char * path, const char * message)
  {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if(!file)
    {
    Serial.println("Failed to open file for appending");
    return;
    }
  if(file.print(message))
    {
    Serial.println("Message appended");
    } 
  else 
    {
    Serial.println("Append failed");
    }
  file.close();
  }

void renameFile(fs::FS &fs, const char * path1, const char * path2)
  {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) 
    {
    Serial.println("File renamed");
    } 
  else 
    {
    Serial.println("Rename failed");
    }
  }

void deleteFile(fs::FS &fs, const char * path)
  {
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path))
    {
    Serial.println("File deleted");
    } 
  else 
    {
    Serial.println("Delete failed");
    }
  }
