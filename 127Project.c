

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "127Project.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "inc/tm4c123gh6pm.h"


//*****************************************************************************
uint32_t ui32ADC0Value[8];
volatile uint32_t ui32Left = 10;
volatile uint32_t ui32Straight = 10;
volatile uint32_t	ui32Right = 10;
volatile bool turning_left = false;
volatile bool turning_right = false;

void pwmInit(void)
{
// PWM settings

SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

// PWM PD0, motor A
GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
GPIOPinConfigure(GPIO_PD0_M1PWM0);

// PWM PD1, motor B
GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_1);
GPIOPinConfigure(GPIO_PD1_M1PWM1);

// PWM clk and frequency
PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, 1000);

PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);
PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, 1000);

// PWM 0 and 1
PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
PWMOutputState(PWM1_BASE, PWM_OUT_1_BIT, true);


PWMGenEnable(PWM1_BASE, PWM_GEN_0);
PWMGenEnable(PWM1_BASE, PWM_GEN_1);
}

void ADC0_Init(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); // configure the system clock to be 40MHz
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);	//activate the clock of ADC0
	SysCtlDelay(2);	//insert a few cycles after enabling the peripheral to allow the clock to be fully activated.

	ADCSequenceDisable(ADC0_BASE, 0); //disable ADC0 before the configuration is complete
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0); // will use ADC0, SS3, processor-trigger, priority 0'
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0); //Sequencer Step 0: Samples Channel PE3
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH1);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH1);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH1);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH2);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH2 | ADC_CTL_IE|ADC_CTL_END); //Sequencer Step 1: Samples Channel PE2
	
	ADCSequenceEnable(ADC0_BASE, 0); //enable ADC0
}

void PortFunctionInit(void)
{
	// Enable Peripheral Clocks 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

	// Enable pin PE3 for ADC AIN0
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1);
}

int main(void)
{
	int count = 0;
	PortFunctionInit();
	ADC0_Init();
	pwmInit();
	IntMasterEnable();
	ADCProcessorTrigger(ADC0_BASE, 0);
	
	while(1)
	{
		ADCIntClear(ADC0_BASE, 0);
		ADCProcessorTrigger(ADC0_BASE, 0);
		while(!ADCIntStatus(ADC0_BASE, 0, false))
		{
		}
		ADCSequenceDataGet(ADC0_BASE, 0, ui32ADC0Value); //Grab the Entire FIFO
		//looks like all we really need is one for each;
		ui32Right = ui32ADC0Value[0];
		ui32Straight = ui32ADC0Value[5];
		ui32Left = ui32ADC0Value[3];
		
		
		if(ui32Straight > 2000)
		{
			turning_left = false;
			turning_right = false;
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 930);
			SysCtlDelay(100);
		}
		//turn left
		else if(ui32Left > 2000)
		{
			turning_left = true;
			turning_right = false;
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 800);
		}
		//turn right
		else if(ui32Right > 2000)
		{
			turning_left = false;
			turning_right = true;
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 930);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 800);
		}
		
		
		
		if(turning_left)
		{
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 100);
		}
		
		else if(turning_right)
		{
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 1000); 
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 100);
		}
		
		else
		{
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 1000);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 930);
			SysCtlDelay(100); 
		}
		 
		
	}

}

