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

#include <phyphoxBle.h> 

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


void setup() {
  
  Serial.begin( 115200 );
  Serial.println("booting up system...");
  delay(2000);
  PhyphoxBLE::start("SQM-BLE");                //Start the BLE server
  
  delay(2000);
  
  #if !defined(__MIPSEL__)
    while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  #endif

  Serial.println("USB Host starting up...");

  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");
}
  
void loop()
{
  Usb.Task();
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

            PhyphoxBLE::write(magL,tempL);    //Send value to phyphox 

            Serial.println("Phyphox BLE Section Done");
      
          }
          
          //if (rcvd > 2)
                //Serial.print((char*)(buf+2));
                
          delay(50);
          
      }
      

  //delay(1000);
}
