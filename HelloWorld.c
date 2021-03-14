#include "AppHardwareApi.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "ZTimer.h"


#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)
#define BOARD_LED_CTRL_MASK         (BOARD_LED_PIN)

ZTIMER_tsTimer timers[1];
uint8 blinkTimerHandle;

PUBLIC void blinkFunc(void *pvParam)
{
//	static int iteration = 0;
//	DBG_vPrintf(TRUE, "Blink iteration %d\n", iteration++);
	DBG_vPrintf(TRUE, "Blink iteration\n");

	uint32 currentState = u32AHI_DioReadInput();
	vAHI_DioSetOutput(currentState^BOARD_LED_PIN, currentState&BOARD_LED_PIN);
}

PUBLIC void vAppMain(void)
{
	ZTIMER_teStatus status;

	// Initialize UART
	DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

	// Initialize hardware
	vAHI_DioSetDirection(0, BOARD_LED_CTRL_MASK);

	DBG_vPrintf(TRUE, "GPIO initialized\n");

	// Init and start timers
	status = ZTIMER_eInit(timers, sizeof(timers) / sizeof(ZTIMER_tsTimer));
	DBG_vPrintf(TRUE, "eInit status: %d\n", status);
	DBG_vPrintf(TRUE, "Array size: %d\n", sizeof(timers) / sizeof(ZTIMER_tsTimer));
	status = ZTIMER_eOpen(&blinkTimerHandle, blinkFunc, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
	DBG_vPrintf(TRUE, "eOpen status: %d\n", status);
	DBG_vPrintf(TRUE, "Handle: %d\n", blinkTimerHandle);
	status = ZTIMER_eStart(blinkTimerHandle, ZTIMER_TIME_MSEC(100));
	DBG_vPrintf(TRUE, "eStart status: %d\n", status);

	DBG_vPrintf(TRUE, "Timer initialized\n");

	while(1)
	{
		ZTIMER_vTask();

		vAHI_WatchdogRestart();
	}
}

void vAppRegisterPWRMCallbacks(void)
{
}
