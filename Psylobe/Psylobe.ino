#include "FastLED.h"
#include "palettes.h"

#define LED_PIN 13
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define MAX_MILLI_AMPS 3000
#define NUM_LEDS 241

#define BUTTON_PIN A1
#define BUTTON_THRES 500
#define BUTTON_DELAY 300
#define BUTTON_SUSTAIN_DELAY 1000
#define BUTTON_SUSTAIN_INTERVAL 40

#define BTN_IDLE 0
#define BTN_SINGLE 1
#define BTN_SUSTAIN 2
#define BTN_RELEASE 3

#define MODE_NOISE 0
#define MODE_WAVE 1
#define MODE_SPIRAL 2
//#define MODE_FIRE 1
#define MODE_MAX_ MODE_SPIRAL

const uint8_t led_count[] = {60, 48, 40, 32, 24, 16, 12, 8, 1};
#define NUM_RINGS (sizeof(led_count) / sizeof(led_count[0]))

unsigned long btnTime = 0;
uint8_t btnState = BTN_IDLE;
bool btnDouble = false;

CRGB leds[NUM_LEDS];
uint8_t mode = MODE_NOISE;
bool on = true;
uint8_t brightness = 255;
bool configDirection = false;

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLI_AMPS);
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void loop() {
  static long t = millis();
  
  manageButton();

  if (on) {
    switch (mode) {
      case MODE_NOISE:
        runNoise();
        break;

      case MODE_WAVE:
        runWave();
        break;

      case MODE_SPIRAL:
        runSpiral();
        break;

//      case MODE_FIRE:
//        runFire();
//        break;
    }
  }

  FastLED.setBrightness(brightness);

  long m = millis();
  FastLED.delay(max(0, 10 - m + t));
  t = m;
}

void runNoise() {
  static uint16_t k = random16();
  
  uint8_t H[NUM_LEDS];
  memset(H, 0, NUM_LEDS);

  fill_raw_noise8(H, NUM_LEDS, 4, 0, 10, k);

  for (int i = 0; i < NUM_LEDS; ++i) {
    uint8_t index = 255.0 * i / NUM_LEDS;
    uint8_t one = sin8(index * 4 - k * 2);
    uint8_t two = sin8(index * 16 + k);
    leds[i] = CHSV(H[i] + k, 255, one * two / 255.0);
  } 

  k++;
}

// https://gist.github.com/kriegsman/8281905786e8b2632aeb
#define SECONDS_PER_PALETTE 10
void runWave() {
  // Current palette number from the 'playlist' of color palettes
  static uint8_t gCurrentPaletteNumber = 0;
  static CRGBPalette16 gCurrentPalette( CRGB::Black);
  static CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16);
  }

  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( gCurrentPalette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 128);
  }
}

// https://wokwi.com/arduino/projects/287161387106435596
void runSpiral() {
  static CRGBPalette16 rainbowPalette = RainbowHalfStripeColors_p;
  static uint16_t startIndex = 0, phase = 0;
  startIndex -= 320; /* rotation speed */
  phase += 64; /* swirl speed */

  CRGB *led = leds;
  for (uint8_t ring = 0; ring < 9 ; ring++) {
    uint16_t ringIndex = startIndex + sin16(phase + ring * 1024);
    uint8_t count = led_count[ring];
    for (uint8_t i = 0; i < count ; i++) {
      uint16_t hue = ringIndex + i * 65535 / count;
      *led++ = ColorFromPaletteExtended(rainbowPalette, hue, 255, LINEARBLEND);
    }
  }
}

/*
// https://wokwi.com/arduino/projects/287197957875302920
#define FIRE_WIDTH 60
#define FIRE_HEIGHT NUM_RINGS
void runFire() {
  static uint8_t heat[FIRE_WIDTH][FIRE_HEIGHT];
  
  uint8_t activity = random8() / 2 + 128;

  for (uint8_t h = 0; h < FIRE_WIDTH; h++) {
    // Step 1.  Cool down every cell a little
    for( uint8_t i = 0; i < FIRE_HEIGHT; i++) {
      heat[h][i] = qsub8( heat[h][i],  random8(33));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( uint8_t k = FIRE_HEIGHT - 1; k >= 1; k--) {
      uint8_t hleft = (h + FIRE_WIDTH - 1) % FIRE_WIDTH;
      uint8_t hright = (h+1) % FIRE_WIDTH;
      heat[h][k] = (heat[h][k]
                  + heat[hleft][k - 1]
                  + heat[h][k - 1]
                  + heat[hright][k - 1] ) / 4;
    }

    if( random8() < activity ) {
      heat[h][0] = qadd8( heat[h][0], random8(42));
    }
  }

  CRGB *led = leds;
  uint8_t ring = 0;
  // map the modified fire onto the rings
  do {
    uint8_t count = led_count[ring];
    uint16_t td = FIRE_WIDTH * 255 / count;
    uint16_t t = 0;
    for (uint8_t i = 0; i < count ; i++) {
      uint8_t h = heat[t >> 8][FIRE_HEIGHT - 1 - ring];
      if (ring >= NUM_RINGS - 2)
        h = qadd8(h, 128);
      *led++ = HeatColor(h);
      t += td;
    }
  } while (++ring < NUM_RINGS);
}
*/

void singlePress() {
  on = !on;

  if (!on) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}

void doublePress() {
  if (!on) {
    return;
  }
}

void longPress() {
  if (!on) {
    on = true;
    return;
  }
  
  mode++;
  if (mode > MODE_MAX_) {
    mode = 0;
  }
}

void sustain(bool isLast) {
  if (!on) {
    return;
  }

  if (isLast) {
    configDirection = !configDirection;
    return;
  }
  
  if (configDirection) {
    brightness = qadd8(brightness, 5);
  } else {
    brightness = qsub8(brightness, 5);
  }
}

void manageButton() {
  if (analogRead(BUTTON_PIN) > BUTTON_THRES) {
    switch (btnState) {
      case BTN_RELEASE:
        btnDouble = true;
      case BTN_IDLE:
        btnTime = millis();
        btnState = BTN_SINGLE;
        break;
        
      case BTN_SINGLE:
        if (millis() - btnTime > BUTTON_SUSTAIN_DELAY) {
          sustain(false);
          btnTime = millis();
          btnState = BTN_SUSTAIN;
        }
        break;
        
      case BTN_SUSTAIN:
        if (millis() - btnTime > BUTTON_SUSTAIN_INTERVAL) {
          sustain(false);
          btnTime = millis();
        }
        break;
    }
    
  } else {
    switch (btnState) {
      case BTN_SINGLE:
        if (millis() - btnTime < BUTTON_DELAY) {
          btnTime = millis();
          btnState = BTN_RELEASE;
        } else {
          longPress();
          btnState = BTN_IDLE;
          btnDouble = false;
        }
        break;
        
      case BTN_SUSTAIN:
        sustain(true);
        btnState = BTN_IDLE;
        btnDouble = false;
        break;

      case BTN_RELEASE:
        if (btnDouble) {
          doublePress();
          btnState = BTN_IDLE;
          btnDouble = false;
        } else if (millis() - btnTime >= BUTTON_DELAY) {
          singlePress();
          btnState = BTN_IDLE;
          btnDouble = false;
        }
        break;
    }
  }
}

// from: https://github.com/FastLED/FastLED/pull/202
CRGB ColorFromPaletteExtended(const CRGBPalette16& pal, uint16_t index, uint8_t brightness, TBlendType blendType) {
  // Extract the four most significant bits of the index as a palette index.
  uint8_t index_4bit = (index >> 12);
  // Calculate the 8-bit offset from the palette index.
  uint8_t offset = (uint8_t)(index >> 4);
  // Get the palette entry from the 4-bit index
  const CRGB* entry = &(pal[0]) + index_4bit;
  uint8_t red1   = entry->red;
  uint8_t green1 = entry->green;
  uint8_t blue1  = entry->blue;

  uint8_t blend = offset && (blendType != NOBLEND);
  if (blend) {
    if (index_4bit == 15) {
      entry = &(pal[0]);
    } else {
      entry++;
    }

    // Calculate the scaling factor and scaled values for the lower palette value.
    uint8_t f1 = 255 - offset;
    red1   = scale8_LEAVING_R1_DIRTY(red1,   f1);
    green1 = scale8_LEAVING_R1_DIRTY(green1, f1);
    blue1  = scale8_LEAVING_R1_DIRTY(blue1,  f1);

    // Calculate the scaled values for the neighbouring palette value.
    uint8_t red2   = entry->red;
    uint8_t green2 = entry->green;
    uint8_t blue2  = entry->blue;
    red2   = scale8_LEAVING_R1_DIRTY(red2,   offset);
    green2 = scale8_LEAVING_R1_DIRTY(green2, offset);
    blue2  = scale8_LEAVING_R1_DIRTY(blue2,  offset);
    cleanup_R1();

    // These sums can't overflow, so no qadd8 needed.
    red1   += red2;
    green1 += green2;
    blue1  += blue2;
  }
  if (brightness != 255) {
    // nscale8x3_video(red1, green1, blue1, brightness);
    nscale8x3(red1, green1, blue1, brightness);
  }
  return CRGB(red1, green1, blue1);
}
