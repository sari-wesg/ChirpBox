#include "SimpleFIFO.h"
#include "sti_gnss_lib.h"
#include "GNSS.h"

const uint8_t blue_led = 0;
const uint8_t output_pin = 4;

char buf[64];

void setup() {
  GnssConf.init(); /* do initialization for GNSS */
  GnssInfo.timestamp.setTrigCapture(TS_TRIG_ON, TS_TRIG_RISING, (void*)userFuncforTimeStamp);//set GPIO10 on RISING tri
  gnss_gpio_set_output(blue_led);
  pinMode(output_pin, OUTPUT); 
  Serial.config(STGNSS_UART_8BITS_WORD_LENGTH, STGNSS_UART_1STOP_BITS, STGNSS_UART_NOPARITY);//set uart at 8bits, 1 bit stop
  Serial.begin(115200);//initialize the UART2 console port with baud rate 115200
}

void loop() 
{
  while (GnssInfo.timestamp.numRecord() != 0) //check fifo, default depth is 10
  {
    if (GnssInfo.timestamp.pop() == true)     //get a new unread record from fifo
    {
      gnss_gpio_low(output_pin);
      GnssInfo.timestamp.convertTimeStampToUTC(); //convert to UTC time
//      GnssInfo.timestamp.formatGPSString(buf);  //convert GPS data to string
//      Serial.print(buf);                        //print
//      Serial.print("; ");
      GnssInfo.timestamp.formatUTCString(buf);    //convert UTC time to string
      Serial.print(buf);    
      Serial.print("\r\n");
    }
  }
}

void task_called_after_GNSS_update(void) {
  char buf[64];
  GnssInfo.update();
//  Serial.print("NumGPSInView = ");
//  Serial.print(GnssInfo.satellites.numGPSInView(NULL));
//  Serial.print(", NumGPSInUse = ");
//  Serial.println(GnssInfo.satellites.numGPSInUse(NULL));
  
  if(GnssInfo.satellites.numGPSInUse(NULL)>=4){
    gnss_gpio_low(blue_led);
  }
  else{
    gnss_gpio_high(blue_led);
  }
}

void userFuncforTimeStamp(TIME_STAMPING_STATUS_T ts) {
  if (ts.time_is_valid) {
    if (GnssInfo.timestamp.push(ts) == true) {
      // add necessary code for successful data push here, but note the more
      // code added, the higher possibility to loss GNSS lock
    }
  }
}
