#include <cdcftdi.h>
#include <usbhub.h>

#include "pgmstrings.h"
#include <elapsedMillis.h>
elapsedMillis statusMillis;
// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#include <WiFi.h>
#include <WiFiClient.h>


const byte lenMsg = 64;
uint8_t  buf[lenMsg];

class FTDIAsync : public FTDIAsyncOper
{
public:
    uint8_t OnInit(FTDI *pftdi);
};

uint8_t FTDIAsync::OnInit(FTDI *pftdi)
{
    uint8_t rcode = 0;

    rcode = pftdi->SetBaudRate(115200);

    if (rcode)
    {
        ErrorMessage<uint8_t>(PSTR("SetBaudRate"), rcode);
        return rcode;
    }
    rcode = pftdi->SetFlowControl(FTDI_SIO_DISABLE_FLOW_CTRL);

    if (rcode)
        ErrorMessage<uint8_t>(PSTR("SetFlowControl"), rcode);

    return rcode;
}

USB              Usb;
//USBHub         Hub(&Usb);
FTDIAsync        FtdiAsync;
FTDI             Ftdi(&Usb, &FtdiAsync);


/*Put your SSID & Password*/
const char* ssid      = "WIFI-SSID";  // Enter SSID here
const char* password  = "WIFI-PASSWORD";  //Enter Password here


//Thingspeak
String apiKey       = "API-KEY"; 
const char* server  = "api.thingspeak.com"; 

WiFiClient client;
int initial=1;
int ctt=0;
void setup() {
  
  Serial.begin( 115200 );
  Serial.println("booting up system...");
  delay(2000);
             //Start the BLE server
  
  delay(2000);
  
  #if !defined(__MIPSEL__)
    while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  #endif

  Serial.println("USB Host starting up...");

  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");

        wifiConnect();
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
}
  
void loop()
{
  Usb.Task();

  initial++;

  if (initial>=12){
    ESP.restart();
  }
  
   if( Usb.getUsbTaskState() == USB_STATE_RUNNING ) {
          uint8_t  rcode;
//          char strbuf[] = "rx";
//          rcode = Ftdi.SndData(strlen(strbuf), (uint8_t*)strbuf);
//          if (rcode){
//                    ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
//          }

           if (statusMillis >= 10000) //Longer Time means less data coming in
           {
             char strbuf[] = "rx";
             rcode = Ftdi.SndData(strlen(strbuf), (uint8_t*)strbuf);
             if (rcode){
                     ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
                    }
            statusMillis = 0;
            }
                 
          delay(50); //Wait a while for SQM to respond.

          for (uint8_t i=0; i<lenMsg; i++)
              buf[i] = 0;
          uint16_t rcvd = lenMsg;
          rcode = Ftdi.RcvData(&rcvd, buf);
  
          //if (rcvd > 2)
          //Serial.print("Debug :")
          //Serial.println((char*)buf+20);
          
          if (rcode && rcode != hrNAK)
                //Serial.print("A");
              ErrorMessage<uint8_t>(PSTR("Ret"), rcode);
          //Serial.println(rcvd);

          if (rcvd>=59){
            //Serial.print((char*)buf);
            String front = (char*)buf;
            String whole = front.substring(5,9) + " MAG "+front.substring(52,56)+" C";
            Serial.println(whole);

            float magL = 1.0*front.substring(5,9).toFloat();
            float tempL = 1.0*front.substring(52,56).toFloat();
            
            //Serial.println("MQTT Starman");
            wifiConnect();
            if (client.connect(server,80)) { // "184.106.153.149" or api.thingspeak.com
              Serial.println("Sending Package");  
              String postStr = apiKey;
              postStr +="&field1=";
              postStr += String(magL);
              postStr +="&field2=";
              postStr += String(tempL);
//              postStr +="&field3=";
//              postStr += String(pressure);
          //    postStr +="&field4=";
          //    postStr += String(tpl);
          //    postStr +="&field5=";
          //    postStr += String(litre);
              postStr += "\r\n\r\n";
          
             client.print("POST /update HTTP/1.1\n");
             client.print("Host: api.thingspeak.com\n");
             client.print("Connection: close\n");
             client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
             client.print("Content-Type: application/x-www-form-urlencoded\n");
             client.print("Content-Length: ");
             client.print(postStr.length());
             client.print("\n\n");
             client.print(postStr);
             Serial.println("Uploading…");
             client.stop();
             Serial.println("Data Sent! Yeay~");
             }
            else {
              Serial.println("Cannot Reach Thingspeak");
              delay(1000);
              ESP.restart();
             }



          }
          
          //if (rcvd > 2)
                //Serial.print((char*)(buf+2));
                
          delay(50);
          
      }
      

delay(30000);
}


void wifiConnect(){
  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  ctt++;
  if (ctt>100){
    ESP.restart();
  }
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
}
