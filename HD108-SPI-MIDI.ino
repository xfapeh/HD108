#include <SPI.h>
// Clock and data pins are whatever are SPI defaults for your board (SCK, MOSI)
// Arduino Mega 2560, Clock 52, Data 51
// Arduino Uno, Clock 13, Data/MOSI 11
// ESP32 DEV: V-Clock D18, V-Data/MOSI D23, H-Clock D14, H-Data/MOSI D13
#include "FastLED.h"
#include <MIDI.h>


//#define NUM_LEDS 177
#define NUM_LEDS 212
uint16_t color[NUM_LEDS*3];
uint16_t change = 0;
uint8_t  hue = 0;
    uint8_t prehue = 0;
    uint8_t bendhue = 0;          // pitch number for pitchbend start
    uint8_t bendprehue = 0;       // previous pitch number for pitchbend start, 2-polyfony
    uint8_t bendvel = 0;
    uint8_t bendprevel = 0;
    uint8_t pitchbendup   =  2;   // adjust in Synth to match
    uint8_t pitchbenddown = 12;   // adjust in Synth to match
bool sustain = 0;
bool note[127] = {};
int minbright = 2; 
int brightness[NUM_LEDS];      // TO-DO convert to 16 bit !
uint16_t hueglobal[NUM_LEDS];

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  SPI.begin();
  //Serial.begin(9600);
  //Serial.println("Start");
  // turn off all LEDs
    for (int i=0; i<NUM_LEDS; i++) {
      setLEDcolor(i, 0, 0 ,0);
      brightness[i]=minbright;
    }
  MIDI.setHandleNoteOn(handleNoteOn16);
  MIDI.setHandleNoteOff(handleNoteOff16);
  MIDI.setHandleControlChange(handleControlChange16);
  MIDI.setHandlePitchBend(pitchbend16);
  MIDI.begin(0);
}

void loop() {
  /*
  for (int i = 0; i < NUM_LEDS; i++) {
    //setLEDcolor(i,i*458,i*458,i*458);
    setLEDcolor(i,i,i,i);
  }
  setLEDcolor(20,1000,change,0000);
  */

  pride16();  
  //colorchange();
  //rainbow();
  //fadein();
  //ledtail();
  //ledfade();
  //fastcycle();
  
  MIDI.read();
  updateLEDs();

  //delay(1);
  /*
  if(change<=65534)
  change +=100;
  else if (change==65535)
  change=0;
  */
}

void updateLEDs() {
  SPI.beginTransaction(SPISettings(25000000, MSBFIRST, SPI_MODE3));   // Arduino Uno 40000000 is OK, ESP32 max ~25000000
  // Start frame
  for (int i = 0; i <= 4; i++) {SPI.transfer16(0);}
  // LEDs
/*
  SPI.transfer(0xFF);
  SPI.transfer(0xFF);
  SPI.transfer16(0);  // RED (16-bit)
  SPI.transfer16(0);  // GREEN (16-bit)
  SPI.transfer16(0);  // BLUE (16-bit)
*/

  for (int i = 0; i < NUM_LEDS; i++) {
    // LED frame
    //SPI.transfer(0b10000000); // Start of LED Frame (1bit 1)    // LOW POWER
    SPI.transfer(0b10111001); // Start of LED Frame (1bit 1)    // RED CORRECTION
    //SPI.transfer(0b11111111); // Start of LED Frame (1bit 1)  // HIGH POWER
    //             1RRRRRGG
    SPI.transfer(0b01001010); // & Brightness as (5bit)(5bit)(5bit) brightnesses    // LOW POWER
    //SPI.transfer(0b11111111); // & Brightness as (5bit)(5bit)(5bit) brightnesses  // HIGH POWER
    //             GGGBBBBB
    SPI.transfer16(color[i*3+0]);  // RED (16-bit)
    SPI.transfer16(color[i*3+1]);  // GREEN (16-bit)
    SPI.transfer16(color[i*3+2]);  // BLUE (16-bit)
  }

  // End Frame
  for (int i = 0; i <= 4; i++) {SPI.transfer16(0xFFFF);}
  SPI.endTransaction();

}

void setLEDcolor(int LEDnum, uint16_t LEDr, uint16_t LEDg, uint16_t LEDb) {
  color[LEDnum*3+0]=LEDr;
  color[LEDnum*3+1]=LEDg;
  color[LEDnum*3+2]=LEDb;
}  

uint32_t hsv2rgb16r( uint16_t hue, uint16_t sat, uint16_t bri ) {
  uint16_t maxbri = bri;
  if      ( hue >= 0     && hue < 21845 ) {
    return (65535-3*hue);
  }
  else if ( hue >= 21845 && hue < 43691 ) {
    return 0;
  }
  else if ( hue >= 43691 && hue < 65536 ) {
    return 3*(hue-43690);
  }
  else return 0;
}
uint32_t hsv2rgb16g( uint16_t hue, uint16_t sat, uint16_t bri ) {
  if      ( hue >= 0     && hue < 21845 ) {
    return 3*hue;
  }
  else if ( hue >= 21845 && hue < 43691 ) {
    return (65535-3*(hue-21844));
  }
  else if ( hue >= 43691 && hue < 65536 ) {
    return 0;
  }
  else return 0;
}
uint32_t hsv2rgb16b( uint16_t hue, uint16_t sat, uint16_t bri ) {
  if      ( hue >= 0     && hue < 21845 ) {
    return 0;
  }
  else if ( hue >= 21845 && hue < 43691 ) {
    return 3*(hue-21844);
  }
  else if ( hue >= 43691 && hue < 65536 ) {
    return (65535-3*(hue-43690));
  }
  else return 0;
}

void fastcycle() {
  int j = 0;
  while (1) {
    turnLED(j);
    j = (j + 1) % NUM_LEDS;
  updateLEDs();
  }
}

void turnLED(int j) {
  for (int i=0; i<NUM_LEDS; i++)
      setLEDcolor(i, 0, 0 ,0);
  setLEDcolor(j, 20000, 0 ,0);
}

void pride16() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  //static uint16_t sHue16 = 0;
// test1 settings
//  uint8_t sat8 = beatsin88( 87, 220, 250);
//  uint8_t brightdepth = beatsin88( 0, 5, 10);
//  uint16_t brightnessthetainc16 = beatsin88( 100, (2 * 256), (10 * 256));
//  uint8_t msmultiplier = beatsin88(47, 10, 30);
// PianoLED setttings
  uint8_t sat8 = beatsin88( 87, 220, 250);
  //uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint8_t brightdepth = beatsin88( 120, 10, 10);   // higher lowlevel to prevent gaps, minbright can then be lower than 20
  //uint16_t brightnessthetainc16 = beatsin88( 50, (2 * 256), (15 * 256)); // brightness variation wavelength
  //uint8_t msmultiplier = beatsin88(147, 13, 50); // brightness variation speed
  
  //Serial.print(brightdepth);
  //Serial.print("\t");

  uint16_t brightnessthetainc16 = beatsin88( 203, (5 * 256), (15 * 256));
  uint8_t msmultiplier = beatsin88(147, 3, 4);

  //Serial.print(brightnessthetainc16);
  //Serial.print(" \t");
  //Serial.print(msmultiplier);
  //Serial.print(" \t");
  
  uint16_t hue16 = 0;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier /10;
  //sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    //Serial.print(i);
    //Serial.print(" \t");
    
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    //Serial.print(b16);
    //Serial.print(" \t");

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint16_t bri8 = (uint32_t)(((((uint32_t)bri16) * brightdepth)/1 / 65536)+1)/1.1;
    bri8 += (65535 - brightdepth)/(95536)+2;

    //Serial.print(bri8);
    //Serial.print(" \t");
  
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    // hue16, sat8*256, bri8
    //CRGB newcolor = CHSV( hue16, sat8*256, bri8);
    uint16_t R16=hsv2rgb16r(hue16, sat8*256, brightness[pixelnumber]);
    uint16_t G16=hsv2rgb16g(hue16, sat8*256, brightness[pixelnumber]);
    uint16_t B16=hsv2rgb16b(hue16, sat8*256, brightness[pixelnumber]);

    hueglobal[pixelnumber]=hue16;
/*
 brightness debug
    if (pixelnumber == 67)   {  brightness[pixelnumber]=255; 
    //Serial.print(B16);Serial.print(" ");Serial.print(R16);Serial.print(" ");Serial.print(G16);Serial.print(" ");Serial.print(hue16);Serial.println(""); 
    Serial.print(hueinc16*20);Serial.print(" ");Serial.print(hue16);Serial.print(" ");Serial.print("0");Serial.print(" ");Serial.print("65536");Serial.println(""); 
    //Serial.print(hueinc16);Serial.print(" ");Serial.print("0");Serial.print(" ");Serial.print("3000");Serial.println(""); 
    }
    //if (pixelnumber == 97)     brightness[pixelnumber]=255;
    //if (pixelnumber == 127)     brightness[pixelnumber]=255;
 */
    
    //bri8=2; // switch off brightness variation
    setLEDcolor(pixelnumber, (R16*((bri16/512)+10)*brightness[pixelnumber]/256)/256, (G16*((bri16/512)+10)*brightness[pixelnumber]/256)/256 ,(B16*((bri16/512)+10)*brightness[pixelnumber]/256)/256);
    
    //Serial.print(R16*bri8*brightness[pixelnumber]/255/10);
    //Serial.print(" \t");
    //Serial.print(G16*bri8*brightness[pixelnumber]/255/10);
    //Serial.print(" \t");
    //Serial.print(B16*bri8*brightness[pixelnumber]/255/10);
    //Serial.print(" \t");
    //Serial.println(" ");
  }
}

void colorchange() {
  uint16_t wavenumber = 1000; // 2000 = 32 LEDs, 4000 = 16 LEDs
  uint16_t frequency  = 13;    // BPM, 2/60 = 1/30 Hz, 60/60 = 1 Hz

  //dynamic wavenumber and frequency
  wavenumber = beatsin16 ( 1 , 200 , 750 );  // works
  ////frequency = beatsin16 ( 2 , 10 , 20 );       // fail, phase glitch
  
  uint16_t hue16 = 0;
  //hue16 = beatsin16( frequency , 0 , 65535 , beatsin( 3 , 2 , 5 )*millis() , 64); // 16bit hue value, (BPM, min, max, timebase controls freq, phase)
  //hue16 = beatsin16( frequency , 0 , 65535 ); // 16bit hue value, sin (BPM, min, max)
  hue16 = 65535 - beat16( frequency );                // 16bit hue value, saw (BPM, min, max)

  for ( int i=0 ; i<NUM_LEDS ; i++ ) {
    hue16 += wavenumber;

    uint16_t R16=hsv2rgb16r(hue16, 255, 255);
    uint16_t G16=hsv2rgb16g(hue16, 255, 255);
    uint16_t B16=hsv2rgb16b(hue16, 255, 255);

    uint16_t bri=5;
    setLEDcolor(i, R16/bri, G16/bri ,B16/bri);
  } 
}

void rainbow() {
      for(uint16_t i=0; i<NUM_LEDS; i++)
      {
      uint16_t val=i*455;
      uint16_t bri=1;
      uint16_t R16=hsv2rgb16r(val, 255, 255);
      uint16_t G16=hsv2rgb16g(val, 255, 255);
      uint16_t B16=hsv2rgb16b(val, 255, 255);
      setLEDcolor(i, R16/bri*2*brightness[i]/255, 1*G16/bri*brightness[i]/255 ,1*B16/bri*brightness[i]/255);
      }
}

void fadein() {
  // does not yet work great at all
  uint16_t hue16 = 0;
  hue16 = 65535 - beat16( 2 ) ;
  for(uint16_t i=0; i<NUM_LEDS/3; i++) {
    uint16_t R16=hsv2rgb16r(0, 255, 255);
    setLEDcolor(i, R16*(hue16+i*64), 0 ,0);
  }
  hue16 = 0;
  hue16 = 65535 - beat16( 2 ) ;
  for(uint16_t i=NUM_LEDS/3; i<NUM_LEDS/3*2; i++) {
    uint16_t G16=hsv2rgb16g(21845, 255, 255);
    setLEDcolor(i, 0, G16*(hue16+(i-NUM_LEDS/3)*64*0) ,0);
  }
  hue16 = 0;
  hue16 = 65535 - beat16( 2 ) ;
  for(uint16_t i=NUM_LEDS/3*2; i<NUM_LEDS; i++) {
    uint16_t B16=hsv2rgb16b(43691, 255, 255);
    setLEDcolor(i, 0, 0 ,B16*(hue16+(i-NUM_LEDS/3*2)*64));
  }
}

void ledtail() {
  uint16_t taillength = 13; // tail = taillength-1 !
  uint16_t tailbright = 41850;
  for (int16_t i=0; i<NUM_LEDS; i++) {
    for (int16_t j=0; j<taillength+1; j++)  {
      uint16_t tailcolor = (uint16_t)floor((float)tailbright-(tailbright/(taillength)*(j)));
  //  uint16_t tailcolor =                        tailbright-(tailbright/(taillength)*(j));
      if( j==taillength ) tailcolor = 0;
      if( i < NUM_LEDS && i-j < NUM_LEDS && i >= 0+taillength) {
        setLEDcolor(i-j,          tailcolor, 0 ,tailcolor);
      }
      else if ( i < 0+taillength && i-j < NUM_LEDS && i-j >= 0) {
        setLEDcolor(i-j,          tailcolor, 0 ,tailcolor);
        //setLEDcolor(i-j+NUM_LEDS, tailcolor, 0 ,tailcolor);
      }
      else if ( i < 0+taillength && i-j < NUM_LEDS && i-j < 0) {
        //setLEDcolor(j-i,          tailcolor, 0 ,tailcolor);
        setLEDcolor(i-j+NUM_LEDS, tailcolor, 0 ,tailcolor);
      }
    }
    updateLEDs();
    delay(200);
  }
}


void ledfade() {
  //Serial.println("Start");
  
  uint16_t maxbright = 16384;
  float maxbrightrel = (float)maxbright / 65536.;
  int16_t steps = 16;
  int16_t neighbors = 10;
  float fre=10;
  float res=neighbors*fre * 1;
  float stepheight = (float)maxbright / (float)steps;
  uint16_t colorval = 0;
  
  for (int16_t i=0-neighbors; i<NUM_LEDS+neighbors; i++) {
  //int16_t i=10;
    for (int16_t j=0; j<steps; j++)  {
        for (int16_t n=-neighbors; n<=neighbors; n++) {
        // FAIL colorval = ((float)sin16((float)65535/(float)((steps-1)*neighbors)*(-(neighbors+n)*(steps-1)/neighbors+j))+(float)32767)*maxbrightrel 
        // FAIL                  *(sin16((float)65535/(float)(neighbors)*0.5*(n+neighbors-2))+(float)32767)/65535;
        // sin(x)
        float sinx=                1*PI/2    - 1.*2*PI /steps/(2*neighbors)*j + .5      *fre*2*PI*(float)n/(float)res;
        // sin16(x) -> FAIL ab neighbors > 5
        //float sin16x=              1*16383 - 1.*65535/steps/(2*neighbors)*j + .5*65535*fre     *(float)n/(float)res;
        if(sinx >= -1*PI/2 && sinx < 2*PI) {
        //if(sin16x >= -16383 && sin16x < 65536) {
          colorval =( (float)sin(  sinx  )/2.    +.5 ) * maxbright;
        //colorval =( (float)sin16(sin16x)/65535.+.5 ) * maxbright;
        }
        else if ( sinx < -1*PI/2 ) {
        //else if ( sin16x < -16383 ) {
          colorval =0;
        }
        //delay(10);
        /*
        Serial.print(i+n);
        Serial.print("\t");
        Serial.print(sin16x);
        Serial.print("\t");
        Serial.print(colorval);
        Serial.println("");
        */
        uint16_t huemult = (i+n-30)*256/127*400;
        uint16_t R16=hsv2rgb16r(huemult, 0, 0);
        uint16_t G16=hsv2rgb16g(huemult, 0, 0);
        uint16_t B16=hsv2rgb16b(huemult, 0, 0);
      if( i+n >= 0 && i+n < NUM_LEDS ) {
        setLEDcolor(i+n,         R16*(float)colorval/65535., G16*(float)colorval/65535., B16*(float)colorval/65535.);
        //setLEDcolor(i+n,         colorval, colorval, colorval);
        //setLEDcolor(i,         20000, 0, 0);
      }

        //setLEDcolor(i,        5000,0,0);
        //setLEDcolor(i-neighbors-1,        0,0,0);
        //setLEDcolor(i+neighbors+1,        0,0,0);
        }
        delay(0);
       updateLEDs();
    }
  }
  //for (int i=0; i<NUM_LEDS; i++) {
  //    setLEDcolor(i, 0, 0 ,0);
  //  }
}


void handleNoteOn16(byte channel, byte pitch, byte velocity)
{
      hue=pitch;
      int i=-42+2*hue+1;
      brightness[i] = min(1*(255*velocity)/127 , 255);             // besser mit floor oder so -> flickert wegen buffer overrun, TO-DO -> 16 bit
      // ab hier alles überflüssig wegen brightness
      uint16_t val=hueglobal[pitch];
      //uint16_t bri=30; // no effect???
      //uint16_t R16=hsv2rgb16r(val, 255, brightness[i]);
      //uint16_t G16=hsv2rgb16g(val, 255, brightness[i]);
      //uint16_t B16=hsv2rgb16b(val, 255, brightness[i]);
      //setLEDcolor(-42+2*hue+1, R16/bri, 1*G16/bri ,1*B16/bri);   // wahrscheinlich überflüssig -> brightness[i] zählt, hue von pride
      // Ende überflüssig
      
      // Test Spark
      //delay(1000);
      setLEDcolor(-42+2*hue+1,65535/2, 65535/2 ,65535/2);
      updateLEDs();
      //delay(1000);
      note[pitch]=true;
}

void handleNoteOff16(byte channel, byte pitch, byte velocity)
{
      hue=pitch;
      int j = -42+2*hue+1;
      if(sustain==0) {
        //setLEDcolor(j, color[j*3+0]*minbright/255, color[j*3+1]*minbright/255 ,color[j*3+2]*minbright/255);   // wahrscheinlich überflüssig -> brightness[i] zählt, hue von pride
        brightness[j]=minbright;
      }  
      note[pitch]=false;
}


void handleControlChange16(byte channel, byte number, byte value)
{
  if(number==64 && value==127) {
      sustain=1;
  }
  if(number==64 && value==0) {
      for(int i=21; i<=108; i++) {    
        if(note[i]==false){
          //setLEDcolor(-42+2*i+1, minbright, minbright ,minbright);
          brightness[-42+2*i+1]=minbright;
        }
      }
    sustain=0;
  }
  updateLEDs();
}

void pitchbend16(byte channel, int bend)
{
  if(bend!=0) {
    // pitchbend
    if(note[hue]==true) {
      brightness[-42+2*hue+1]=minbright;        // primary note LED off
      brightness[-42+1*(bendhue)+1]=minbright;  // switch previous bend led off
  // adjust bend range up and down -------- BEGIN
      if (bend > 0) {
        bendhue=2*hue+pitchbendup*bend/4095;                    // pitch for EVERY LED, 8192=2HT=4LED
      }
      else if (bend < 0) {
        bendhue=2*hue+pitchbenddown*bend/4095;                  // pitch for EVERY LED, 8192=2HT=4LED
      }
  // adjust bend range up and down -------- END
      brightness[-42+1*(bendhue)+1] = (255*abs(bendvel-20))/127;
      updateLEDs();
    }
  }
  else if(note[prehue]==true) {
      brightness[-42+2*prehue+1]=minbright;        // primary note LED off
      brightness[-42+1*(bendprehue)+1]=minbright;  // switch previous bend led off
  // adjust bend range up and down -------- BEGIN
      if (bend > 0) {
        bendprehue=2*prehue+pitchbendup*bend/4095;                    // pitch for EVERY LED, 8192=2HT=4LED
      }
      else if (bend < 0) {
        bendprehue=2*prehue+pitchbenddown*bend/4095;                  // pitch for EVERY LED, 8192=2HT=4LED
      }
  // adjust bend range up and down -------- END
      brightness[-42+1*(bendprehue)+1] = (255*abs(bendprevel-20))/127;
      updateLEDs();
  }
  else {
    if(note[hue]==false) {   // note off -> LED off (+left+right)
      //setLEDcolor(-42+1*(bendhue)+1, minbright, minbright ,minbright);
      brightness[-42+1*(bendhue)+1]=minbright;
    }
    if(note[prehue]==false) {   // note off -> LED off (+left+right)  
      //setLEDcolor(-42+1*(bendprehue)+1, minbright, minbright ,minbright);
      brightness[-42+1*(bendprehue)+1]=minbright;
    }
    updateLEDs();
  }
}
