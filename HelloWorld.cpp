#include <stdio.h>
#include <stdlib.h>

#include "AppHardwareApi.h"


#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)
#define BOARD_LED_CTRL_MASK         (BOARD_LED_PIN)


int main(void)
{
	// Initialize hardware
	vAHI_DioSetDirection(0, BOARD_LED_CTRL_MASK);
	//vAHI_DioSetOutput(BOARD_LED_PIN, 0);
	vAHI_DioSetOutput(0, BOARD_LED_PIN);

	puts("Hello World!!!");
	return EXIT_SUCCESS;
}
