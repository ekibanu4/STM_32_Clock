#ifndef SHIFT_REGISTER_H_
#define SHIFT_REGISTER_H_

#include <stdint.h>

void ShiftRegister_Init(void);
void ShiftRegister_OutputEnablePwm_Init(uint8_t brightness);
void ShiftRegister_SetBrightness(uint8_t brightness);
void ShiftRegister_WriteBoards(uint8_t board1Mask, uint8_t board2Mask);

#endif /* SHIFT_REGISTER_H_ */
