
// Runs on LM3S811
// Provide a function that initializes Timer2A to trigger ADC
// SS3 conversions and request an interrupt when the conversion
// is complete.

#include "lm3s811.h"
#include "Potentiometers.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Timer0A: enabled
// Mode: 16-bit, down counting
// One-shot or periodic: periodic
// Prescale value: programmable using variable 'prescale' [0:255]
// Interval value: programmable using variable 'period' [0:65535]
// Sample time is busPeriod*(prescale+1)*(period+1)
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: Timer0A
// SS3 1st sample source: programmable using variable 'channelNum' [0:3]
// SS3 interrupts: enabled and promoted to controller
void ADC_InitTimer2ATriggerSeq3(unsigned char channelNum, unsigned char prescale, unsigned short period){
  volatile unsigned long delay;
  // channelNum must be 0-3 (inclusive) corresponding to ADC0 through ADC3
  if(channelNum > 3){
    return;                                 // invalid input, do nothing
  }
  DisableInterrupts();
  // **** general initialization ****
  SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;       // activate ADC
  SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADCSPD_M; // clear ADC sample speed field
  SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADCSPD125K;// configure for 125K ADC max sample rate (default setting)
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER2;    // activate timer2
  delay = SYSCTL_RCGC1_R;                   // allow time to finish activating
  TIMER2_CTL_R &= ~TIMER_CTL_TAEN;          // disable timer2A during setup
  TIMER2_CTL_R |= TIMER_CTL_TAOTE;          // enable timer2A trigger to ADC
  TIMER2_CFG_R = TIMER_CFG_16_BIT;          // configure for 16-bit timer mode
  // **** timer2A initialization ****
  TIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;   // configure for periodic mode
  TIMER2_TAPR_R = prescale;                 // prescale value for trigger
  TIMER2_TAILR_R = period;                  // start value for trigger
  TIMER2_IMR_R &= ~TIMER_IMR_TATOIM;        // disable timeout (rollover) interrupt
  TIMER2_CTL_R |= TIMER_CTL_TAEN;           // enable timer2A 16-b, periodic, no interrupts
  // **** ADC initialization ****
                                            // sequencer 0 is highest priority (default setting)
                                            // sequencer 1 is second-highest priority (default setting)
                                            // sequencer 2 is third-highest priority (default setting)
                                            // sequencer 3 is lowest priority (default setting)
  ADC0_SSPRI_R = (ADC_SSPRI_SS0_1ST|ADC_SSPRI_SS1_2ND|ADC_SSPRI_SS2_3RD|ADC_SSPRI_SS3_4TH);
  ADC_ACTSS_R &= ~ADC_ACTSS_ASEN3;          // disable sample sequencer 3
  ADC0_EMUX_R &= ~ADC_EMUX_EM3_M;           // clear SS3 trigger select field
  ADC0_EMUX_R += ADC_EMUX_EM3_TIMER;        // configure for timer trigger event
  ADC0_SSMUX3_R &= ~ADC_SSMUX3_MUX0_M;      // clear SS3 1st sample input select field
                                            // configure for 'channelNum' as first sample input
  ADC0_SSMUX3_R += (channelNum<<ADC_SSMUX3_MUX0_S);
  ADC0_SSCTL3_R = (0                        // settings for 1st sample:
                   & ~ADC_SSCTL3_TS0        // read pin specified by ADC0_SSMUX3_R (default setting)
                   | ADC_SSCTL3_IE0         // raw interrupt asserted here
                   | ADC_SSCTL3_END0        // sample is end of sequence (default setting, hardwired)
                   & ~ADC_SSCTL3_D0);       // differential mode not used (default setting)
  ADC0_IM_R |= ADC_IM_MASK3;                // enable SS3 interrupts
  ADC_ACTSS_R |= ADC_ACTSS_ASEN3;           // enable sample sequencer 3
  // **** interrupt initialization ****
                                            // ADC3=priority 3
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00006000; // bits 13-15
  NVIC_EN0_R |= NVIC_EN0_INT17;             // enable interrupt 17 in NVIC
  EnableInterrupts();
}


void Multiplexer_Init(void)
{
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;    // activate port B
  GPIO_PORTB_DIR_R |= 0x0F;                // make PB0-PB3 Outputs
  GPIO_PORTB_DEN_R |= 0x0F;                // enable digital I/O on PB0-PB3 (default setting)
}

void Multiplexer_select( unsigned char id )
{
	GPIO_PORTB_DATA_R = id&0x0F;
}

struct Pot Pots[POT_COUNT];
unsigned char nextPotId;
void Potentiometers_Init(void)
{
	int i;
	ADC_InitTimer2ATriggerSeq3(0, 8, 119999/POT_COUNT); // ADC channel 0, 10 Hz sampling for each Potentiometer
	Multiplexer_Init();

	for( i=0; i< POT_COUNT; i++)
	{
		Pots[i].id = i;
		Pots[i].value = 0;
		Pots[i].flag = 0;
	}

	nextPotId = 0;
	Multiplexer_select( nextPotId );
}

void ADC3_Handler(void){
	unsigned long newValue = ADC0_SSFIFO3_R&ADC_SSFIFO3_DATA_M;
	unsigned long oldValue =  Pots[nextPotId].value;
  ADC0_ISC_R = ADC_ISC_IN3;             // acknowledge ADC sequence 3 completion

  if( oldValue - newValue > MIN_DELTA && newValue-oldValue > MIN_DELTA ) {
		Pots[nextPotId].value = newValue;
		Pots[nextPotId].flag = 1;
	}

	nextPotId = (nextPotId+1)%POT_COUNT;
	Multiplexer_select( nextPotId );
}
