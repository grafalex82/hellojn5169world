#include "AppHardwareApi.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "ZTimer.h"
#include "portmacro.h"


#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)
#define BOARD_LED_CTRL_MASK         (BOARD_LED_PIN)

ZTIMER_tsTimer timers[1];
uint8 blinkTimerHandle;

PUBLIC void blinkFunc(void *pvParam)
{
	static int iteration = 0;
	DBG_vPrintf(TRUE, "Blink iteration %d\n", iteration++);

	uint32 currentState = u32AHI_DioReadInput();
	vAHI_DioSetOutput(currentState^BOARD_LED_PIN, currentState&BOARD_LED_PIN);

	ZTIMER_eStart(blinkTimerHandle, ZTIMER_TIME_MSEC(1000));
}

PUBLIC void vAppMain(void)
{
	// Initialize the hardware
        TARGET_INITIALISE();
        SET_IPL(0);
        portENABLE_INTERRUPTS();

	// Initialize UART
	DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

	// Initialize hardware
	vAHI_DioSetDirection(0, BOARD_LED_CTRL_MASK);

	// Init and start timers
	ZTIMER_eInit(timers, sizeof(timers) / sizeof(ZTIMER_tsTimer));
	ZTIMER_eOpen(&blinkTimerHandle, blinkFunc, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
	ZTIMER_eStart(blinkTimerHandle, ZTIMER_TIME_MSEC(1000));

	while(1)
	{
		ZTIMER_vTask();

		vAHI_WatchdogRestart();
	}
}

void vAppRegisterPWRMCallbacks(void)
{
}
