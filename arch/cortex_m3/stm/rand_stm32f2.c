/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "rand.h"
#include "arch.h"
#include "dbg.h"

void srand()
{
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
	RNG->SR = 0;
	RNG->CR = RNG_CR_RNGEN;
	//generate first seed and discard it
	rand();
}

unsigned int rand()
{
	int retry = 3;
	while (--retry)
	{
		while (RNG->SR == 0) {}
		if (RNG->SR == RNG_SR_DRDY)
			return RNG->DR;
		//restart RNG
		RNG->CR &= ~RNG_CR_RNGEN;
		RNG->SR = 0;
		RNG->CR |= RNG_CR_RNGEN;
	}
#if (RANDOM_DEBUG)
	if (RNG->SR & RNG_SR_CECS)
		printf("RNG: clock failure\n\r");
	else if (RNG->SR & RNG_SR_SECS)
		printf("RNG: seed error\n\r");
#endif //RANDOM_DEBUG
	return 0;
}
