/*
 * LoRa Sender DEMO
 * board used: LILYGO® TTGO LoRa32 V2.1_1.6
 * Base code by Bernardo Giovanni (https://www.settorezero.com)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h> 
#include <SSD1306Wire.h>
#include "board_defs.h" // in this header I've defined the pins used by the module LILYGO® TTGO LoRa32 V2.1_1.6
#include "images.h"
#include "my_fonts.h"
#include <ArduinoJson.h>
String packet; // packet to be sent over LoRa
uint32_t counter=0; // a demo variable to be included in the packet
String rssi = "RSSI --";
String packSize = "--";
const char PROGMEM demoFileName[]="/receiver.txt"; // a demoFileName where the packet will be saved on the SD
bool sdpresent=false; // keep in mind if there is an SD mounted
float battery=0; // battery voltage

SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
SPIClass sdSPI(HSPI); // we'll use the SD card on the HSPI (SPI2) module
SPIClass loraSPI(VSPI); // we'll use the LoRa module on the VSPI (SPI3) module

float time_to_action = millis();
int modo_lora = 1;

const char* ssid = "Link Start";  // Enter SSID here
const char* password = "1234#563";  //Enter Password here

AsyncWebServer server(80);
//WebServer server(80);

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

//
String axs = "0";
String ays = "0";
String azs = "0";
String mxs = "0";
String mys = "0";
String mzs = "0";
String gxs = "0";
String gys = "0";
String gzs = "0";
String baros = "0";
String tems = "0";
String als = "0";

void setup() 
  {
    // Begins UART
  Serial.begin(115200);
    ////////////////////////
  IPAddress local_ip(192,168,1,1);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
//WiFi.begin(ssid, password);
//check wi-fi is connected to wi-fi network
//while (WiFi.status() != WL_CONNECTED) {
//delay(1000);
//Serial.print(".");
//}
//Serial.println("");
//Serial.println("WiFi connected..!");
Serial.print("Got IP: ");
Serial.println(WiFi.localIP());

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  ///////////////////////////////
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
//  server.on("/temperaturex", HTTP_GET, [](AsyncWebServerRequest *request){
//    request->send_P(200, "text/plain", "1");
//  });
//  server.on("/humidityx", HTTP_GET, [](AsyncWebServerRequest *request){
//    request->send_P(200, "text/plain", ays.c_str());
//  });
//  server.on("/pressurex", HTTP_GET, [](AsyncWebServerRequest *request){
//    request->send_P(200, "text/plain", axs.c_str());
//  });
//  
  server.on("/aceleracao-x", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", axs.c_str()
    );
  });
  
  server.on("/aceleracao-y", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", ays.c_str()
    );
  });
  
  server.on("/aceleracao-z", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", azs.c_str());
  });  
  server.on("/magnetometro-x", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", mxs.c_str());
  });
  
  server.on("/magnetometro-y", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", mys.c_str());
  });
  
  server.on("/magnetometro-z", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", mzs.c_str());
  });
  
  server.on("/giroscopio-x", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", gxs.c_str());
  });
  
  server.on("/giroscopio-y", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", gys.c_str());
  });
  
  server.on("/giroscopio-z", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", gzs.c_str());
  });
  
  server.on("/barometro", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", baros.c_str());
  });
  
  server.on("/temperatura", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", tems.c_str());
  });

  // Start server
  server.begin();
  // Begins some pins
  pinMode(LED,OUTPUT); // Green Led
  pinMode(VBAT,INPUT); // Input for Battery Voltage

 
  while (!Serial);
  delay(500);
  Serial.println();
  Serial.println("LoRa Receiver Test By Giovanni Bernardo");
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
    writeFile(SD, demoFileName, "LoRa Receiver Demo\nBy Bernardo Giovanni\nhttps://www.settorezero.com\n");  
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
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Monospaced_bold_11);
  display.display();
  }

void loop() 
  {
  // variables used for checking the battery and give an average value
  static uint16_t chkbat=0;
  static float tempbatt=0;
//   Serial.println(millis()- time_to_action);
  if(modo_lora==1||(millis()- time_to_action>350))
  {
    //Serial.println("Solicitando dados!");
    LoRa.beginPacket();
    LoRa.print("*-*");
    LoRa.endPacket();
    time_to_action = millis();    
    modo_lora = 2;
  }else if (modo_lora==2){
    //Serial.println("Esperando dados!");
    display.clear();
  
  display.drawString(0,0,"LoRa RECEIVER Demo");
  display.drawString(0,11, "Battery: "+String(battery)+"V");  
  display.drawString(0, 22, rssi);
  display.drawString(0, 33, "Got "+ packSize + " bytes");
  display.drawStringMaxWidth(0, 44, 128, packet);
  display.display();
  
  int packetSize=LoRa.parsePacket();
  if (packetSize) getLoRaPacket(packetSize);
  
  // battery voltage as average over 2000 readings
  tempbatt+=checkBattery();
  chkbat++;
  if (chkbat==2000)
    {
    chkbat=0;
    battery=(tempbatt/2000);
    Serial.print("Battery voltage: ");
    Serial.println(battery);
    tempbatt=0;
    }
  }else if(modo_lora==3){
    LoRa.beginPacket();
    LoRa.print("#-#");
    LoRa.endPacket();
    time_to_action = millis() - time_to_action;    
    modo_lora = 2;
    Serial.println("botão acionado");
    modo_lora = 1; 
  }
}

// Re-construct LoRa Packet
void getLoRaPacket(int packetSize) 
  {
  DynamicJsonDocument doc(1024);
  LedOn();
  packet="";
  packSize=String(packetSize,DEC);
  for (int i=0; i<packetSize; i++) 
    {
    packet +=(char)LoRa.read(); // re-construct packet char by char
    }
  deserializeJson(doc, packet);
  JsonObject obj = doc.as<JsonObject>();
  rssi="RSSI:"+String(LoRa.packetRssi(), DEC);
  Serial.print("Packet received: ");
  Serial.print(packet);
  Serial.print(" ");
  Serial.println(rssi);
  // packet starts with @ and ends with #
  if ((packet.startsWith("#") && packet.endsWith("#")))
    {
      
    }
  if ((packet.startsWith("{") && packet.endsWith("}")))
    {
    ax = obj[String("ax")];
    ay = obj[String("ay")];
    az = obj[String("az")];
    mx = obj[String("mx")];
    my = obj[String("my")];
    mz = obj[String("mz")];
    gx = obj[String("gx")];
    gy = obj[String("gy")];
    gz = obj[String("gz")];
    bar = obj[String("bar")];
    tem = obj[String("tem")];

    axs = String(ax);
    ays = String(ay);
    azs = String(az);
    mxs = String(mx);
    mys = String(my);
    mzs = String(mz);
    gxs = String(gx);
    gys = String(gy);
    gzs = String(gz);
    baros = String(bar);
    tems = String(tem);
    Serial.println(axs.c_str());
    Serial.println(axs+","+ays+","+azs+","+mxs+","+mys+","+mzs+","+gxs+","+gys+","+gzs+","+baros+","+tems+";");
    Serial.println("Salvou dados!");
     modo_lora = 1;
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
