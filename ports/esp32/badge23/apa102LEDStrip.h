#ifndef DEF_apa102LEDStrip
#define DEF_apa102LEDStrip

struct apa102LEDStrip
{
    unsigned char *LEDs;
    short int _frameLength;
    short int _endFrameLength;
    short int _numLEDs;
    unsigned char _bytesPerLED;
    short int _counter;
    unsigned char _globalBrightness;
};
void initLEDs(struct apa102LEDStrip *ledObject, short int numLEDs, unsigned char bytesPerLED, unsigned char globalBrightness);
void setPixel(struct apa102LEDStrip *ledObject, short int pixelIndex, unsigned char *pixelColour);
void getPixel(struct apa102LEDStrip *ledObject, short int pixelIndex, unsigned char *pixelColour);
#endif
