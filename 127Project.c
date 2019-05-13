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
#define constant 1000
#define LMax 3700
#define RMax 4095
#define CMax 3720
#define LMin 430 
#define RMin 350
#define CMin 290


//*****************************************************************************
uint32_t ui32ADC0Value[8];
uint32_t tempR = 0;
uint32_t tempL = 0;
uint32_t tempC = 0;
uint32_t sumR = 0;
uint32_t sumL = 0;
uint32_t sumC = 0;
volatile int ui32Left = 10;
volatile int ui32Center = 10;
volatile int	ui32Right = 10;
volatile bool turning_left = false;
volatile bool turning_right = false;
volatile bool test = false;
int right_speed = 700, left_speed = 700;
int right_wheel, left_wheel;
int	LRange, RRange, CRange;
int Left, Right, Center;
int count = 0;
int count2 = 0;

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
		
		//count++;
		tempR = ((ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[7])/3);
		tempC = ((ui32ADC0Value[5] + ui32ADC0Value[6])/2);
		tempL = ((ui32ADC0Value[2] + ui32ADC0Value[3] + ui32ADC0Value[4])/3);
		
		if(count < 100)
		{
			
			sumR = sumR + tempR;
			sumL = sumL + tempL;
			sumC = sumC + tempC;
			count ++;
		}
		else
		{
			
			ui32Right = sumR / count;
			ui32Left = sumL / count;
			ui32Center = sumC / count;
			count  = 0;
			
			sumR = 0;
			sumL = 0;
			sumC = 0;
			
			LRange = LMax - LMin;
			RRange = RMax - RMin;
			CRange = CMax - CMin;
			
			Left = (ui32Left *1000)/LRange;
			Right = (ui32Right * 1000)/RRange;
			Center = (ui32Center * 1000)/CRange;
			
			
			
			
			
			right_speed = constant + Left - Right;
			left_speed =  constant + Right - Left;
			
			if( ui32Right - ui32Left > 500)
			{
				turning_right = true;
				turning_left = false;
			}
			else if(ui32Left - ui32Right > 500)
			{
				turning_right = false;
				turning_left = true;
			}
//			else
//			{
//				turning_left  = false;
//				turning_right = false;
//			}
			
			
			if(right_speed < 100) right_speed = 200;
			if(left_speed < 100) left_speed = 200;
			
			right_wheel = right_speed/2;
			left_wheel = left_speed/2;
			
			if(right_wheel > 1000) right_wheel = 1000;
			if(left_wheel > 1000) left_wheel = 1000;
			
//			if((Center - ((Left + Right)/2)) < 200)
//			{
//				
//			}
			if((turning_right || turning_left)  & (( ui32Left < 2700) & (ui32Right < 2700) & (ui32Center < 2700)))
			{
				if(test)
				{
					count2++;
				}
				if(count2 > 120)
				{
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 100);
					PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 100);
				}
					
				else
				{
					test = true;
					if(turning_right)
					{
						if(ui32Right < 2600)
						{	
							PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 100);
							PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 800);
						}
					}
					else
					{
						if(ui32Left < 2350)
						{	
							PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 1000);
							PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 100);
						}
					}
					SysCtlDelay(100000);
				}
			}
			else
			{
				test = false;
				count2 = 0;
				PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, right_wheel);
				PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, left_wheel);
			}
		}
		
	}
}
