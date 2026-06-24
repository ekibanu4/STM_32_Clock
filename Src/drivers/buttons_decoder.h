#ifndef BUTTONS_DECODER_H_
#define BUTTONS_DECODER_H_

#include <stdint.h>

typedef enum {
  BUTTON_NONE = 0,
  BUTTON_MODE,
  BUTTON_SET,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_OFF
} ButtonCode_t;

void ButtonsDecoder_Init(void);
uint16_t ButtonsDecoder_ReadRaw(void);
ButtonCode_t ButtonsDecoder_Decode(uint16_t adcValue);
ButtonCode_t ButtonsDecoder_Read(void);

#endif /* BUTTONS_DECODER_H_ */
