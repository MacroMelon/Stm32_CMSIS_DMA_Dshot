//
// Created by thari on 14/09/2021.
//

#include "Dshot_DMA.h"

//TODO- maybe add double buffering?

uint16_t Dshot_build_packet(uint16_t throttle)
{
	uint16_t packet;

	//throttle += 48;		//ensure we dont go into sending commands

	packet = (throttle << 1);	//we dont need telemetry

	// compute checksum
	uint16_t csum = 0;
	uint16_t csum_data = packet;

	for(int i = 0; i < 3; i++)
	{
		csum ^=  csum_data; // xor data by nibbles
		csum_data >>= 4;
	}

	csum &= 0xf;
	packet = (packet << 4) | csum;

	return packet;
}

void Dshot_preparePacketData(uint16_t packet)
{
	//#define MOTOR_BIT_0 7
	//#define MOTOR_BIT_1 14

	for(int i = 0; i < 16; i++)
	{
		//safe_packet_data[i] = (packet & 0x8000) ? 11 : 5;
		safe_packet_data[i] = (packet & 0x8000) ? 14 : 7;
		packet <<= 1;
	}
}

void Dshot_enableConfigureTimer()
{
	RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;			//enable clock to TIM8
	TIM8->CR1 = 0;								//reset CR just in case

	//GPIO configure PC6 for output Push Pull AF2 (TIM8_CH1)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;		//enable clock to GPIOC

	GPIOC->AFR[0] &= ~(GPIO_AFRL_AFSEL6);
	GPIOC->AFR[0] |= (3<< GPIO_AFRL_AFSEL6_Pos);	//enable TIM8_CH1 to PC6
	GPIOC->MODER &= ~(GPIO_MODER_MODE6);
	GPIOC->MODER |= GPIO_MODER_MODE6_1;				//MODER5[1:0] = 10 bin, Alternate function
	GPIOC->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR6);	//clear
	GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6_1;		// 10b, 50Mhz speed, plenty

	TIM8->CCMR1 |= TIM_CCMR1_OC1PE;		//enable preload for CCR1
	TIM8->CCMR1 &= ~(TIM_CCMR1_OC1M);	//clear
	TIM8->CCMR1 |= TIM_CCMR1_OC1M_1;	//enable pwm mode 1 (OC1M to 110b)
	TIM8->CCMR1 |= TIM_CCMR1_OC1M_2;	//enable pwm mode 1 (OC1M to 110b)
	TIM8->CCER |= TIM_CCER_CC1E;		//Enable relevant channel output and polarity from CCER
	TIM8->BDTR |= TIM_BDTR_MOE;			//Enable Master output from BDTR register

	TIM8->CR1 |= TIM_CR1_ARPE;			//enable preload for ARR
	//set upcounting mode - by default

	//for interrupt Dshot - lol its a bit messy
	//TIM8->DIER |= TIM_DIER_UIE;			//enable Update event interrupt

	TIM8->DIER |= TIM_DIER_UDE;			//enable update event DMA request

	//Heres the plan: 20 counts, high for 7 for bit 0, high for 14 for bit 1
	//Timer clock is 2*ABP domain clock if ABP != 1
	//So, for 1.2Mbits / sec, we need a prescaler of (from 168Mhz):
	//168/(1.2*num clicks per bit)
	//so if we take num clicks as 20, (168/(1.2*20)) = 7
	TIM8->PSC = 6;					//set prescaler - 6 + 1 = 7
	TIM8->ARR = 20;					//set Auto reload reg to get frequency of 1.2Mhz


	//for interrupt Dshot - lol its a bit messy
	//NVIC_SetPriority (TIM8_UP_TIM13_IRQn, 0);            // Set Timer priority
	//NVIC_EnableIRQ (TIM8_UP_TIM13_IRQn);                 // Enable Timer Interrupt


	//DMA - TIM8_UP is Stream 1 channel 7 of DMA2
	//so, on every DMA transfer done interrupt, update packet array (so no conflicts)
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	DMA2_Stream1->CR = 0;		//reset just in case

	//select channel 7
	DMA2_Stream1->CR |= DMA_SxCR_CHSEL_0;
	DMA2_Stream1->CR |= DMA_SxCR_CHSEL_1;
	DMA2_Stream1->CR |= DMA_SxCR_CHSEL_2;
	//no burst mode stuff
	//TODO- maybe double buffer
	DMA2_Stream1->CR |= DMA_SxCR_PL_1;			//Priority level 10b, High
	DMA2_Stream1->CR |= DMA_SxCR_MSIZE_0;		//memory data size 01b half word (16bit)
	DMA2_Stream1->CR |= DMA_SxCR_PSIZE_0;		//peripheral data size 01b half word (16bit)
	DMA2_Stream1->CR |= DMA_SxCR_MINC;			//increment memory address after each transfer
	//DMA2_Stream1->CR |= DMA_SxCR_CIRC;			//Circular mode, reset address back to 0 after transfers done
	DMA2_Stream1->CR |= DMA_SxCR_DIR_0;			//direction 01b, memory to peripheral

	//TODO- tune num data to be transferred
	DMA2_Stream1->NDTR = 18;			//number of data items to be transferred

	DMA2_Stream1->PAR = &(TIM8->CCR1);				//Peripheral address of transfer
	DMA2_Stream1->M0AR = &(safe_packet_data);		//memory address of transfer

	//DMA2_Stream1->CR |= DMA_SxCR_TCIE;			//transfer complete interrupt enable
	//NVIC_SetPriority (DMA2_Stream1_IRQn, 0);            // Set Timer priority
	//NVIC_EnableIRQ (DMA2_Stream1_IRQn);                 // Enable Timer Interrupt

	//DMA2_Stream1->CR |= DMA_SxCR_EN;			//and finally enable it


	//DMA stream config - pg233 of ref manual

	TIM8->CCR1 = 0;							//send the pin low

	//blank frames to send signal low for a while in continous mode
	safe_packet_data[16] = 0;
	safe_packet_data[17] = 0;
	safe_packet_data[18] = 0;
	counter = 0;

	//TIM8->CR1 |= TIM_CR1_UDIS;				//halt update event interrupts
	//TIM8->CR1 &= ~(TIM_CR1_UDIS);		//begin update events

	//TIM8->EGR |= TIM_EGR_UG;			//re-init counter etc before starting
	TIM8->CR1 |= TIM_CR1_CEN;			//enable clock
}

void DshotWrite(uint16_t value)
{
	if(!(DMA2_Stream1->CR & DMA_SxCR_EN)){	//check if prev transfer is complete
		//new packet time
		//GPIOC->ODR ^= (1<<1);
		Dshot_preparePacketData(Dshot_build_packet(value));
		DMA2->LIFCR |= DMA_LIFCR_CTCIF1;		//clear transfer complete flag
		DMA2_Stream1->CR |= DMA_SxCR_EN;
	}
}

//This is for continous transmission, TODO- Docs
/*
void DMA2_Stream1_IRQHandler (void){
	if (DMA2->LISR & DMA_LISR_TCIF1){
		DMA2->LIFCR |= DMA_LIFCR_CTCIF1;		//clear transfer complete flag
		//GPIOC->ODR ^= (1<<1);		//turn on ye olde debug LED
		//transfer complete, all 17 bits sent
		Dshot_preparePacketData(Dshot_build_packet(esc_value));
		DMA2_Stream1->CR |= DMA_SxCR_EN;
	}
}
 */

//non DMA dshot- a bit frisky if you ask me
/*
void TIM8_UP_TIM13_IRQHandler (void)
{
	//TODO- maybe get rid of CCR2 and update everything using CCR1... yeah with DMA no need extra timing
	if (TIM8->SR & TIM_SR_UIF){
		if (counter <= 17){				//data + 2 zero bits
			//load CCR1 value for next bit
			TIM8->CCR1 = safe_packet_data[counter];
			counter++;
		}
		else{
			//TIM8->CCR1 = 0;							//send the pin low
			//TIM8->CR1 |= TIM_CR1_UDIS;				//halt update event interrupts
			//transferDone = 1;
			TIM8->CR1 |= TIM_CR1_UDIS;				//halt update event interrupts
			counter = 0;
			//todo- this is a bit mad
			Dshot_preparePacketData(Dshot_build_packet(esc_value));
			TIM8->CR1 &= ~(TIM_CR1_UDIS);		//begin update events
		}
		TIM8->SR &= ~(TIM_SR_UIF);	//clear flag
	}
}
*/