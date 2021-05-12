#include "Arduino.h"
#include <avr/sleep.h>

// The loop just blinks an LED when not in sleep mode
void sleep_mode_01(){

  // Just blink LED twice to show we're running
  doBlink();

  // Is the "go to sleep" pin now LOW?
  if (digitalRead(sleepPin) == HIGH) {

    // Disable the ADC (Analog to digital converter, pins A0 [14] to A5 [19])
    static byte prevADCSRA = ADCSRA;
    ADCSRA = 0;

    set_sleep_mode (SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    MCUCR = bit (BODS) | bit (BODSE);

    // The BODS bit is automatically cleared after three clock cycles so we better get on with it
    MCUCR = bit (BODS);

    noInterrupts();
    attachInterrupt(digitalPinToInterrupt(wakePin), sleepISR, HIGH);

    // Send a message just to show we are about to sleep
    Serial.println("Good night!");
    Serial.flush();

    // Allow interrupts now
    interrupts();

    // And enter sleep mode as set above
    sleep_cpu();

    Serial.println("I'm awake!");

    // Re-enable ADC if it was previously running
    ADCSRA = prevADCSRA;
  }
}


// When wakePin is brought LOW this interrupt is triggered FIRST (even in PWR_DOWN sleep)


void sleepISR() {
  // Prevent sleep mode, so we don't enter it again, except deliberately, by code
  sleep_disable();

  // Detach the interrupt that brought us out of sleep
  detachInterrupt(digitalPinToInterrupt(wakePin));

  // Now we continue running the main Loop() just after we went to sleep
}


// Double blink just to show we are running. Note that we do NOT
// use the delay for final delay here, this is done by checking
// millis instead (non-blocking)
void doBlink() {
  static unsigned long lastMillis = 0;

  if (millis() > lastMillis + 1000) {
    digitalWrite(ledPin, HIGH);
    delay(10);
    digitalWrite(ledPin, LOW);
    delay(200);
    digitalWrite(ledPin, HIGH);
    delay(10);
    digitalWrite(ledPin, LOW);
    lastMillis = millis();
  }
}
