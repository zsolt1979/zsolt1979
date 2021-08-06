#include <stdio.h>
#include "Log_data.h"

void test_method(unsigned int i, unsigned int Sel)
{
	switch (Sel)
	{
	case 0:
		printf("TIM0 LSB: %i\n", i);
		break;

	case 1:
		printf("TIM0 MSB: %i\n", i);
		break;

	case 2:
		printf("Timer value: %i\n", i);
		break;

	case 3:
		printf("Int status int: %x\n", i);
		break;


	default:
		printf("ADC CH0 value: %i\n", i);
		break;

	}
}
