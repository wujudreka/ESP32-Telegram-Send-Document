//=========================telegram==============================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Wifi network station credentials
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;          // last time messages' scan has been done
bool Start = false;
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
//=========================load cell==============================
#include "HX711.h" 
const int scale1_DOUT_PIN = 16;
const int scale1_SCK_PIN = 4;
HX711 scale;
int setpoint = 5;
int count = 0;
//=========================smoothing components==============================
const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
float total = 0;                  // the running total
int averageMass = 0;                // the averageMass
float mass;
//=========================millis==============================
unsigned long previousMillisSerial = 0;
const long intervalSerial = 1000;
//=========================relay==============================
int relayPin = 5;
//=========================SPIFFS==============================
#include "FS.h"
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true
File myFile;
bool isMoreDataAvailable();
byte getNextByte();
SemaphoreHandle_t xMutex = NULL;  // Create a mutex object
boolean sendDataToTelegram = false;
boolean sendDataMeow = false;

//=========================Dummy==============================
int buttonTare = 32;
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

TaskHandle_t Task1, Task2;
String msg = "";

bool isMoreDataAvailable()
{
  return myFile.available();
}

byte getNextByte()
{
  return myFile.read();
}
void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";
    if (text == "/download"){
      bot.sendChatAction(chat_id, "meow meow");
      int size;
      myFile = SPIFFS.open("/Data.csv");
      size = myFile.size();
      String response=bot.sendMultipartFormDataToTelegram("sendDocument", "document",
                          "Data.csv","",CHAT_ID,size,isMoreDataAvailable,getNextByte, 
                          nullptr, nullptr);
    }
    if (text == "/start"){
      String welcome = "Welcome...\n";
      welcome += "Click download to download your data!\n\n";
      welcome += "/download : to send test chat action message\n";
      bot.sendMessage(chat_id, welcome);
    }
  }
}
void setup() {
  xMutex = xSemaphoreCreateMutex();  // crete a mutex object
  Serial.begin(9600);
  pinMode(buttonTare, INPUT_PULLUP);
//  load cell
  float m,c;
  scale.begin(scale1_DOUT_PIN, scale1_SCK_PIN);
  scale.set_scale(29.50);
  scale.set_offset(50441.00); 
  scale.tare();       
//  relay
  pinMode(relayPin, OUTPUT);
  xTaskCreatePinnedToCore(
             Task1code, /* Task function. */
             "Task1",   /* name of task. */
             10000,     /* Stack size of task */
             NULL,      /* parameter of the task */
             1,         /* priority of the task */
             &Task1,    /* Task handle to keep track of created task */
             0);        /* pin task to core 0 */                
  delay(500); 
  xTaskCreatePinnedToCore(
            Task2code,   /* Task function. */
            "Task2",     /* name of task. */
            10000,       /* Stack size of task */
            NULL,        /* parameter of the task */
            1,           /* priority of the task */
            &Task2,      /* Task handle to keep track of created task */
            1);          /* pin task to core 1 */                
  delay(500); 
  
}
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    delay(10);
    Serial.println("main loop 1");
    unsigned long currentMillis = millis();
    smoothing();
    if(averageMass > setpoint){
        if(sendDataToTelegram == true){
          digitalWrite(relayPin, LOW);
          count++;
          String currentTime = takeTime();
          Serial.print(currentTime); 
          Serial.print(", ");
          Serial.print("Count : ");
          Serial.println(count);
          msg = currentTime;
          msg += " Jumlah: " + String(count) + "\r\n";
    //      appendFile(SPIFFS, "/data.csv", String(count));
          if (xSemaphoreTake (xMutex, portMAX_DELAY)) {
            sendDataMeow = true;
            xSemaphoreGive (xMutex);  // release the mutex  
          }
          sendDataToTelegram = false;
        }
    }
    if(digitalRead(buttonTare) == LOW){
      scale.tare();
    }
//    if(digitalRead(buttonTare) == LOW){
//      if(sendDataToTelegram == true){
//        digitalWrite(relayPin, LOW);
//        count++;
//        String currentTime = takeTime();
//        Serial.print(currentTime); 
//        Serial.print(", ");
//        Serial.print("Count : ");
//        Serial.println(count);
//        msg = currentTime;
//        msg += " Jumlah: " + String(count) + "\r\n";
//  //      appendFile(SPIFFS, "/data.csv", String(count));
//        if (xSemaphoreTake (xMutex, portMAX_DELAY)) {
//          sendDataMeow = true;
//          xSemaphoreGive (xMutex);  // release the mutex  
//        }
//        sendDataToTelegram = false;
//      }
//    }else{
//      digitalWrite(relayPin, HIGH);    
//      if (currentMillis - previousMillisSerial >= intervalSerial) {
//        previousMillisSerial = currentMillis;
//        Serial.print("Berat: ");
//        Serial.print(averageMass);
//        Serial.println(" kg");
//        sendDataToTelegram = true;
//      }
//    }
  } 
}
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
//  telegram
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Bot started up", "");
//  time
  timeClient.begin();
  timeClient.setTimeOffset(7*3600);
//  SPIFFS
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  for(;;){
    delay(10);
    Serial.println("main loop 2");
    if (millis() - bot_lasttime > BOT_MTBS){
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while (numNewMessages)
      {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      bot_lasttime = millis();      
    }else{
      if (xSemaphoreTake (xMutex, portMAX_DELAY)) {
        if(sendDataMeow == true){
          bot.sendMessage(CHAT_ID, msg, "");
          sendDataMeow = false;
        }
        xSemaphoreGive (xMutex);  // release the mutex  
      }      
    }    
  }
}
void loop() {
}
void smoothing() {
  total = total - readings[readIndex];
  mass = scale.get_units(1);  
  readings[readIndex] = mass;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  averageMass = total / numReadings;
  averageMass /= 1000;
}
String takeTime(){
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    String formattedTime = timeClient.getFormattedTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime); 
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    String currentMonthName = months[currentMonth-1];
    int currentYear = ptm->tm_year+1900;
    String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
    currentDate += " " + formattedTime;
    return currentDate;
}
