//
// Created by thari on 14/09/2021.
//

#ifndef STM32_DSHOT_DSHOT_DMA_H
#define STM32_DSHOT_DSHOT_DMA_H

#include "stm32f4xx.h"

void GPIOinit();

volatile uint_fast8_t transferDone;
//volatile uint16_t esc_value;			//only needed in continous mode

uint_fast8_t counter;		//todo- use TIM3_RCR value
uint16_t safe_packet_data[19];		//with last value zero to send pin low

uint16_t Dshot_build_packet(uint16_t throttle);
void Dshot_preparePacketData(uint16_t packet);

void Dshot_enableConfigureTimer();
void DshotWrite(uint16_t value);

//void TIM8_UP_TIM13_IRQHandler (void);
void DMA2_Stream1_IRQHandler (void);

#endif //STM32_DSHOT_DSHOT_DMA_H
