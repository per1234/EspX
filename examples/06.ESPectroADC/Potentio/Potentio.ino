/*
  Blink on-board Neopixel RGB LED using ESPectro library
  Turns on-board Neopixel RGB LED on for one second, then off for one second, repeatedly.

  Created by 22 Sep 2016
  by Andri Yadi
*/

#include <ESPectroBase.h>

ESPectroBase base;

int lastAdcPot = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  //Wait Serial to be ready
  while(!Serial);
  
  base.begin();
}

// the loop function runs over and over again forever
void loop() {
  int pot = base.analogRead(ESPECTRO_BASE_ADC_POT_CHANNEL);
  if (pot != -1 && abs(pot - lastAdcPot) > 10) {
      lastAdcPot = pot;
      Serial.printf("Pot value: %d\r\n", pot);
  }
  
  delay(500);
}
