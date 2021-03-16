#include "AppHardwareApi.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "ZTimer.h"
#include "ZQueue.h"
#include "portmacro.h"
#include "pwrm.h"

#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)

#define BOARD_BTN_BIT               (1)
#define BOARD_BTN_PIN               (1UL << BOARD_BTN_BIT)

PRIVATE uint8 keepAliveTime = 10;
PRIVATE pwrm_tsWakeTimerEvent wakeStruct;

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

uint8 enabled = TRUE;

PUBLIC void blinkFunc(void *pvParam)
{
	static uint8 fastBlink = TRUE;

	ButtonPressType value;	
	if(ZQ_bQueueReceive(&queueHandle, (uint8*)&value))
	{
		DBG_vPrintf(TRUE, "Processing message in blink task\n");

		if(value == BUTTON_SHORT_PRESS)
			fastBlink = fastBlink ? FALSE : TRUE;

		if(value == BUTTON_LONG_PRESS)
		{
			DBG_vPrintf(TRUE, "Stop Blinking\n");
			vAHI_DioSetOutput(BOARD_LED_PIN, 0);
			enabled = FALSE;
		}
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
		else if(duration > 5)
		{
			DBG_vPrintf(TRUE, "Button released. Short press detected\n");
			ButtonPressType value = BUTTON_SHORT_PRESS;
			ZQ_bQueueSend(&queueHandle, &value);
		}

		duration = 0;
	}

	ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));
}

PUBLIC void vISR_SystemController(void)
{
    // clear pending DIO changed bits by reading register
    uint8 u8WakeInt = u8AHI_WakeTimerFiredStatus();
    uint32 u32IOStatus = u32AHI_DioInterruptStatus();

    DBG_vPrintf(TRUE, "In vISR_SystemController\n");

    if(u32IOStatus & BOARD_BTN_PIN)
    {
        DBG_vPrintf(TRUE, "Button interrupt\n");
	enabled = TRUE;
	PWRM_vWakeInterruptCallback();
    }

    if(u8WakeInt & E_AHI_WAKE_TIMER_MASK_1)
    {
        /* wake timer interrupt got us here */
        DBG_vPrintf(TRUE, "APP: Wake Timer 1 Interrupt\n");

        PWRM_vWakeInterruptCallback();
    }
}

PUBLIC void wakeCallBack(void)
{
    DBG_vPrintf(TRUE, "wakeCallBack()\n");
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
	ZTIMER_eOpen(&blinkTimerHandle, blinkFunc, NULL, ZTIMER_FLAG_ALLOW_SLEEP);
	ZTIMER_eStart(blinkTimerHandle, ZTIMER_TIME_MSEC(1000));
	ZTIMER_eOpen(&buttonScanTimerHandle, buttonScanFunc, NULL, ZTIMER_FLAG_ALLOW_SLEEP);
	ZTIMER_eStart(buttonScanTimerHandle, ZTIMER_TIME_MSEC(10));

	// Initialize queue
	ZQ_vQueueCreate(&queueHandle, 3, sizeof(ButtonPressType), (uint8*)queue);

	// Let the device go to sleep if there is nothing to do
	PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);

	while(1)
	{
		ZTIMER_vTask();

		vAHI_WatchdogRestart();

		if(enabled == FALSE)
		{
			DBG_vPrintf(TRUE, "Scheduling wake task\n");
			PWRM_eScheduleActivity(&wakeStruct, keepAliveTime * 32000, wakeCallBack);
		}

		PWRM_vManagePower();
	}
}

static PWRM_DECLARE_CALLBACK_DESCRIPTOR(PreSleep);
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(Wakeup);

PWRM_CALLBACK(PreSleep)
{
	DBG_vPrintf(TRUE, "Going to sleep..\n\n");
        DBG_vUartFlush();

	ZTIMER_vSleep();

        // Disable UART (if enabled)
        vAHI_UartDisable(E_AHI_UART_0);

	// clear interrupts
        u32AHI_DioWakeStatus();                         

	// Set the wake condition on falling edge of the button pin
        vAHI_DioWakeEdge(0, BOARD_BTN_PIN);
        vAHI_DioWakeEnable(BOARD_BTN_PIN, 0);
}

PWRM_CALLBACK(Wakeup)
{
    	// Stabilise the oscillator
        while (bAHI_GetClkSource() == TRUE);

        // Now we are running on the XTAL, optimise the flash memory wait states
        vAHI_OptimiseWaitStates();

	// Re-initialize Debug UART
	DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

        DBG_vPrintf(TRUE, "\nWaking..\n");
	DBG_vUartFlush();

	// Re-initialize hardware and interrupts
        TARGET_INITIALISE();
        SET_IPL(0);
        portENABLE_INTERRUPTS();

	// Wake the timers
        ZTIMER_vWake();
}

void vAppRegisterPWRMCallbacks(void)
{
    PWRM_vRegisterPreSleepCallback(PreSleep);
    PWRM_vRegisterWakeupCallback(Wakeup);	
}
