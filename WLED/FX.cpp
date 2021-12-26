typedef struct Psylobe {
  uint16_t k;
} psylobe;

// TODO use speed and intensity settings
uint16_t WS2812FX::mode_psylobe(void) {
  if (!SEGENV.allocateData(sizeof(Psylobe))) return mode_static(); //allocation failed

  Psylobe* data = reinterpret_cast<Psylobe*>(SEGENV.data);
  
  if (SEGENV.call == 0) {
    data->k = random16();
  }
  
  uint8_t H[SEGLEN];
  memset(H, 0, SEGLEN);
  fill_raw_noise8(H, SEGLEN, 4, 0, 10, data->k);

  CRGB color;

  for (int i = 0; i < SEGLEN; ++i) {
    uint8_t index = 255.0 * i / SEGLEN;
    uint8_t one = sin8(index * 4 - data->k * 2);
    uint8_t two = sin8(index * 16 + data->k);
    color.setHSV(H[i] + data->k, 255, one * two / 255.0);
    setPixelColor(i, color.r, color.g, color.b);
  } 

  data->k = data->k+1;

  return 10;
};
