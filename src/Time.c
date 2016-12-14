// Clock.c

#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value
//EDITING FOR 50MHz Clock
// Initialize SysTick with busy wait running at bus clock (assumes 6 MHz).
unsigned long CyclesPerUs;
unsigned long CyclesPerMs;              // private global
void Time_Init(void){                  // public function
  NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
  NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // maximum reload value
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
                                        // enable SysTick with core clock
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC;
	CyclesPerUs = 50;												//50 counts per us
  CyclesPerMs = 50000;                   // 6000 counts per ms
}
// Time delay using busy wait.
// The cycles parameter is in units of the 6 MHz core clock. (167 nsec)
void wait(unsigned long cycles){        // private function
  volatile unsigned long elapsedTime;
  unsigned long startTime = NVIC_ST_CURRENT_R;
  do{
    elapsedTime = (startTime-NVIC_ST_CURRENT_R)&0x00FFFFFF;
  }
  while(elapsedTime <= cycles);
}
// Time delay using busy wait.
// The time parameter is in units of 1 ms.
void Clock_MsWait(unsigned long time){  // public function
  for(;time>0;time--){
    wait(CyclesPerMs);                  // 1.00ms wait
  }
}
void Clock_UsWait(unsigned long time){
	for(;time>0;time--){
		wait(CyclesPerUs);
	}
}
