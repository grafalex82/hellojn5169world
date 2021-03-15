#include "AppHardwareApi.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "ZTimer.h"
#include "ZQueue.h"
#include "portmacro.h"

#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)

#define BOARD_BTN_BIT               (1)
#define BOARD_BTN_PIN               (1UL << BOARD_BTN_BIT)

ZTIMER_tsTimer timers[2];
uint8 blinkTimerHandle;
uint8 buttonScanTimerHandle;

typedef enum
{
	BUTTON_SHORT_PRESS,
	BUTTON_LONG_PRESS
} ButtonPressType;

ButtonPressType queue[3];
tszQueue queueHandle;

PUBLIC void blinkFunc(void *pvParam)
{
	static uint8 fastBlink = TRUE;
	static uint8 enabled = TRUE;

	ButtonPressType value;	
	if(ZQ_bQueueReceive(&queueHandle, (uint8*)&value))
	{
		if(value == BUTTON_SHORT_PRESS)
			fastBlink = fastBlink ? FALSE : TRUE;

		if(value == BUTTON_LONG_PRESS)
			enabled = enabled ? FALSE : TRUE;
	}

	if(enabled)
	{
		uint32 currentState = u32AHI_DioReadInput();
		vAHI_DioSetOutput(currentState^BOARD_LED_PIN, currentState&BOARD_LED_PIN);
	}

	ZTIMER_eStart(blinkTimerHandle, fastBlink? ZTIMER_TIME_MSEC(200) : ZTIMER_TIME_MSEC(1000));
}


PUBLIC void buttonScanFunc(void *pvParam)
{
	static int duration = 0;

	uint32 input = u32AHI_DioReadInput();
	bool btnState = (input & BOARD_BTN_PIN) == 0;

	if(btnState)
	{
		duration++;
		DBG_vPrintf(TRUE, "Button still pressed for %d ticks\n", duration);
		ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));
	}
	else
	{
		// detect long press
		if(duration > 200)
		{
			DBG_vPrintf(TRUE, "Button released. Long press detected\n");
			ButtonPressType value = BUTTON_LONG_PRESS;
			ZQ_bQueueSend(&queueHandle, (uint8*)&value);
		}

		// detect short press
		else if(duration > 10)
		{
			DBG_vPrintf(TRUE, "Button released. Short press detected\n");
			ButtonPressType value = BUTTON_SHORT_PRESS;
			ZQ_bQueueSend(&queueHandle, &value);
		}

		duration = 0;
	}
}

PUBLIC void vISR_SystemController(void)
{
    /* clear pending DIO changed bits by reading register */
    //uint8 u8WakeInt = u8AHI_WakeTimerFiredStatus();
    uint32 u32IOStatus = u32AHI_DioInterruptStatus();

    DBG_vPrintf(TRUE, "In vISR_SystemController\n");

    if(u32IOStatus & BOARD_BTN_PIN)
    {
        DBG_vPrintf(TRUE, "Button interrupt\n");

        ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));
    }
#if 0
    if(u8WakeInt & E_AHI_WAKE_TIMER_MASK_1)
    {
        /* wake timer interrupt got us here */
        DBG_vPrintf(TRUE, "APP: Wake Timer 1 Interrupt\n");

#ifdef SLEEP_ENABLE
        PWRM_vWakeInterruptCallback();
#endif

    }
#ifdef SLEEP_ENABLE
    vManageWakeUponSysControlISR(eInterruptType);
#endif
#endif //0
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
	vAHI_DioSetDirection(BOARD_BTN_PIN, BOARD_LED_PIN);
	vAHI_DioSetPullup(BOARD_BTN_PIN, 0);
	vAHI_DioInterruptEdge(0, BOARD_BTN_PIN);
	vAHI_DioInterruptEnable(BOARD_BTN_PIN, 0);

	// Init and start timers
	ZTIMER_eInit(timers, sizeof(timers) / sizeof(ZTIMER_tsTimer));
	ZTIMER_eOpen(&blinkTimerHandle, blinkFunc, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
	ZTIMER_eStart(blinkTimerHandle, ZTIMER_TIME_MSEC(1000));
	ZTIMER_eOpen(&buttonScanTimerHandle, buttonScanFunc, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
	//ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));

	// Initialize queue
	ZQ_vQueueCreate(&queueHandle, 3, sizeof(ButtonPressType), (uint8*)queue);

	while(1)
	{
		ZTIMER_vTask();

		vAHI_WatchdogRestart();
	}
}

void vAppRegisterPWRMCallbacks(void)
{
}
