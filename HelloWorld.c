#include <stdio.h>
#include <stdlib.h>

#include "AppHardwareApi.h"
#include "dbg.h"
#include "dbg_uart.h"

#define BOARD_LED_BIT               (17)
#define BOARD_LED_PIN               (1UL << BOARD_LED_BIT)
#define BOARD_LED_CTRL_MASK         (BOARD_LED_PIN)


#ifndef DEBUG_START_UP
    #define TRACE_START FALSE
#else
    #define TRACE_START TRUE
#endif


PUBLIC void vAppMain(void)
{
	int i;


    #if JENNIC_CHIP_FAMILY == JN516x || JENNIC_CHIP_FAMILY == JN514x
        extern void *_stack_low_water_mark;
    #endif

    #if JENNIC_CHIP_FAMILY == JN516x
        /* Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed */
         while (bAHI_GetClkSource() == TRUE);
         /* Now we are running on the XTAL, optimise the flash memory wait states */
         vAHI_OptimiseWaitStates();
    #endif

    /*
     * Don't use RTS/CTS pins on UART0 as they are used for buttons
     * */
    vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);

    /*
     * Initialise the debug diagnostics module to use UART0 at 115K Baud;
     * Do not use UART 1 if LEDs are used, as it shares DIO with the LEDS
     * */
    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);
    #ifdef DEBUG_921600
    {
        /* Bump baud rate up to 921600 */
        vAHI_UartSetBaudDivisor(DBG_E_UART_0, 2);
        vAHI_UartSetClocksPerBit(DBG_E_UART_0, 8);
    }
    #endif
    DBG_vPrintf(TRACE_START, "APP: Switch Power Up\n");

    /*
     * Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size.
     */
    #if JENNIC_CHIP_FAMILY == JN516x || JENNIC_CHIP_FAMILY == JN514x
        vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);
    #endif
    /*
     * Catch resets due to watchdog timer expiry. Comment out to harden code.
     */
    if (bAHI_WatchdogResetEvent())
    {
        DBG_vPrintf(TRUE, "APP: Watchdog timer has reset device!\n");
        DBG_vDumpStack();
        #if HALT_ON_EXCEPTION
            vAHI_WatchdogStop();
            while (1);
        #endif
    }


	// Initialize hardware
	vAHI_DioSetDirection(0, BOARD_LED_CTRL_MASK);
	vAHI_DioSetOutput(BOARD_LED_PIN, 0);
	//vAHI_DioSetOutput(0, BOARD_LED_PIN);


	while(1)
	{
		vAHI_DioSetOutput(0, BOARD_LED_PIN);

		for(i=0; i<1000; i++)
		        DBG_vPrintf(TRUE, "On\n");

		vAHI_DioSetOutput(BOARD_LED_PIN, 0);

		for(i=0; i<1000; i++)
		        DBG_vPrintf(TRUE, "Off\n");
	}

}

void vAppRegisterPWRMCallbacks(void)
{
}
