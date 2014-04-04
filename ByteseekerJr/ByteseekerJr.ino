// Byteseeker Jr.
// Adapted from Michael Smith's PCM Audio sketch on the Arduino Playground

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#define SAMPLE_RATE 8000

int ledPin = 13;
int speakerPin = 11; // 11 is connected to Timer 2
int t=0;
unsigned int lastTime = 0;
unsigned int thisTime = 0;
volatile int a, b, c, d;
volatile int value;
int col;
int state = 1;
int states = 9;
int buttonPressed = 0;
volatile int aTop=99;
volatile int aBottom=0;
volatile int bTop=99;
volatile int bBottom=0;


void stopPlayback()
{
  // Disable playback per-sample interrupt.
  TIMSK1 &= ~_BV(OCIE1A);

  // Disable the per-sample timer completely.
  TCCR1B &= ~_BV(CS10);

  // Disable the PWM timer.
  TCCR2B &= ~_BV(CS10);

  digitalWrite(speakerPin, LOW);
}

// This is called at 8000 Hz to load the next sample.
ISR(TIMER1_COMPA_vect) {

  switch (state) {
  case 1: 
    value = ((t&((t>>a)))+(t|((t>>b))))&(t>>(a+1))|(t>>a)&(t*(t>>b));  
    aTop = 10;
    aBottom =0;
    bTop = 14;
    bBottom = 0;
    break;
  case 2: 
    value =(t*(t>>(a/10)|t>>(b/10)))>>(t>>((b/10)*2)); 
    aTop = 10;
    aBottom =0;
    bTop = 16;
    bBottom = 0;
    break;
  case 3:
    value = t*(((t>>(a*3))|(t>>(10+a)))&(b&(t>>(a*2))));   
    aTop = 6;
    aBottom =0;
    bTop = 50;
    bBottom = 0;
    break;
  case 4:
    value = t*(((t>>a)&(t>>8))&((b+73)&(t>>3)));  
    aTop = 22;
    aBottom =0;
    bTop = 99;
    bBottom = 0;
    break;
  case 5:
    value = t*(((t>>a)|(t>>(b*2)))&(63&(t>>b)));   
    aTop = 24;
    aBottom = 0;
    bTop = 8;
    bBottom = 0;
    break;
  case 6:
    value = ((t>>a&t)-(t>>a)+(t>>a&t))+(t*((t>>b)&b)); 
    aTop = 10;
    aBottom = 0;
    bTop = 28;
    bBottom = 0;
    break;
  case 7:
    value = ((t%42)*(t>>a)|(0x577338)-(t>>a))/(t>>b)^(t|(t>>a));  
    aTop = 8;
    aBottom = 0;
    bTop = 32;
    bBottom = 0;
    break;
  case 8:
    value = (t>>a|t|t>>(t>>16))*b+((t>>(b+1))&(a+1));   
    aTop = 12;
    aBottom = 0;
    bTop = 20;
    bBottom = 0;
    break;
  case 9:
    value = ((t*(t>>a|t>>(a+1))&b&t>>8))^(t&t>>13|t>>6);   
    aTop = 16;
    aBottom = 0;
    bTop = 86;
    bBottom = 0;
    break;
  case 10:
    value = ((t>>32)*7|(t>>a)*8|(t>>b)*7)&(t>>7);   
    aTop = 8;
    aBottom = 0;
    bTop = 22;
    bBottom = 0;
    break; 

  }

  OCR2A = 0xff & value;
  ++t;
}

void startPlayback()
{
  pinMode(speakerPin, OUTPUT);
  pinMode (10, OUTPUT);  // Pots + side hooked up to this
  digitalWrite(10, HIGH);

  // Set up Timer 2 to do pulse width modulation on the speaker
  // pin.

  // Use internal clock (datasheet p.160)
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));

  // Set fast PWM mode  (p.157)
  TCCR2A |= _BV(WGM21) | _BV(WGM20);
  TCCR2B &= ~_BV(WGM22);

  // Do non-inverting PWM on pin OC2A (p.155)
  // On the Arduino this is pin 11.
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

  // Set initial pulse width to the first sample.
  OCR2A = 0;

  // Set up Timer 1 to send a sample every interrupt.
  cli();

  // Set CTC mode (Clear Timer on Compare Match) (p.133)
  // Have to set OCR1A *after*, otherwise it gets reset to 0!
  TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
  TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));

  // No prescaler (p.134)
  TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);

  // Set the compare register (OCR1A).
  // OCR1A is a 16-bit register, so we have to do this with
  // interrupts disabled to be safe.
  OCR1A = F_CPU / SAMPLE_RATE;    // 16e6 / 8000 = 2000

  // Enable interrupt when TCNT1 == OCR1A (p.136)
  TIMSK1 |= _BV(OCIE1A);

  sei();
}

void blinkNTimes(int n) {
 
  digitalWrite(13, LOW);
  delay(200);
  for (int i=0; i<n; i++) {
    digitalWrite(13, LOW);
    delay(150);
    digitalWrite(13, HIGH);
    delay(30);
  }
   digitalWrite(13, LOW);
  delay(400);
  digitalWrite(13, HIGH);
}
void setup() {
  

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(6, INPUT);
  digitalWrite(6, HIGH);
  pinMode(7, INPUT);
  digitalWrite(7, HIGH); // set pullup

  startPlayback();

  //printProg(0);
  lastTime = millis();
  thisTime = millis();
 Serial.begin(9600);   // Debugging
}

void loop() {
  // Is this working? May be broken by the timer action above
  thisTime = millis();   
  if ((thisTime - lastTime) > 5) {
    //updateScreen();
    lastTime = thisTime;
    a = map(analogRead(0), 0, 1023, aBottom, aTop); 
    b = map(analogRead(1), 0, 1023, bBottom, bTop);   

    if ((digitalRead(6) == LOW) && (digitalRead(7) == LOW) && (buttonPressed = 1)) {
      state = 1;
      delay(20);
      digitalWrite(13, LOW);
      delay(400);
      blinkNTimes(1);
    }
    else if  ((digitalRead(6) == LOW) && (buttonPressed == 0)) {
       buttonPressed = 1;
      int exitFlag = 0;
      delay(2);
      for (int i = 0; i < 3; i++){
        if (digitalRead(6) == HIGH) {
          delay(2);
          exitFlag = 1;
        }
      }
      if (!exitFlag){
        state = (state + 1) % states;
        
        blinkNTimes(state);
      }
    }

    else  if ((digitalRead(7) == LOW) && (buttonPressed == 0)) {
      buttonPressed = 1;
      int exitFlag = 0;
      delay(10);
      for (int i = 0; i < 3; i++){
        if (digitalRead(7) == HIGH) {
          delay(2);
          exitFlag = 1;
        }
      }
      if (!exitFlag){
         Serial.println("7");
        state--;
        if (state <= 0) {
          state = states;
        }
        blinkNTimes(state);
       
      }
    }

    if ((digitalRead(6) == LOW) && (digitalRead(7) == LOW)) {
      state = 1;
    }
    if ((digitalRead(6) == HIGH)  && (digitalRead(7) == HIGH) && (buttonPressed == 1)) {
      buttonPressed = 0;
    }
  }
}

