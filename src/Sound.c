//Sound.c
//Works on TLV5618 and lm3s811

#include "lm3s811.h"
#include "Tone.h"
#include "Time.h"
#include "LCD.h"
#include "songs.h"
#include "Potentiometers.h"

//SSI 0 is used for Sound Output.
//SSI Pins are located on Porta as follows:
#define SSITx (0x20)
#define SSIRx (0x10)
#define SSIClk (0x04)
//Chip select on PB4
#define DAC_CS (0x10)

int music;
static unsigned long time;

static unsigned long Channel_Music_Data[16];
static unsigned short music_data;
static unsigned short previous_music_data;
static unsigned long temp;
signed int pitch_bend;
int bass_boost;
int playing = 0;
int SongActive = 0;

struct ChannelOptions {
	int Tone;											//Sound Wave to use from Tone.h
	int Note;											//Current Note to play
	int Time_elapsed;							//Time elapsed for other channel options
	unsigned long End_Time;									//Time to turn off note
	int Next_Note;								//Next Note to play
	unsigned long Start_Time;								//Time to start playing next note
	unsigned long Next_End_Time;						//End time for next note
	int Place_inMusicSheet;				//keep track of place in music_sheet
	Music_Sheet *Pointer;					//Pointer to follow notes in the music sheet according to channel 
  int Attack;
	int Decay;
	int Sustain;
	int Release;
};

typedef struct ChannelOptions Sound_ChannelOptions;

Sound_ChannelOptions SoundChannel[16];

//DAC_Init
//Initializes the required systems to interface to the DAC
void DAC_Init(void){
	unsigned int delay;
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_SSI0;  // activate SSI0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
	delay = SYSCTL_RCGC2_R; //dummy instruction to allow time for activation
	GPIO_PORTA_AFSEL_R |= 0xFF; //Alternate function on PE 0,1,3
	GPIO_PORTA_DEN_R |= 0xFF;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B for DAC chip select pin
	delay = SYSCTL_RCGC2_R; //dummy instruction to allow time for activation
	GPIO_PORTB_DIR_R |= DAC_CS;    // make DAC_CS out
	GPIO_PORTB_AFSEL_R &= ~DAC_CS; //No Alternate function on DAC_CS
	GPIO_PORTB_DEN_R |= DAC_CS;
	GPIO_PORTB_DATA_R |= DAC_CS;
	SSI0_CR1_R &= ~SSI_CR1_SSE; //disable SSI
	SSI0_CR1_R &= ~SSI_CR1_MS; //master mode
	SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M) + 4; //12.5MHz
	SSI0_CR0_R = ~(SSI_CR0_SCR_M | SSI_CR0_SPH | SSI_CR0_SPO); //SCR = 0, SPH = 0, SPO = 0
	SSI0_CR0_R |= SSI_CR0_SPH;
	SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO; //freescale mode
	SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_DSS_M) + SSI_CR0_DSS_16; //16-bit data
	SSI0_DR_R = 0xC000;
	SSI0_CR1_R |= SSI_CR1_SSE; //enable SSI
}

//Sound_Out
//Sends 12 bit value through SSI
void DAC_Out(unsigned short data){
	GPIO_PORTB_DATA_R &= ~DAC_CS;
	data |= 0xC000; //add configuration data for DAC IC
	while((SSI0_SR_R & SSI_SR_TNF) == 0){}; //wait until room in FIFO
	wait(20);	//wait for setup time
	SSI0_DR_R = data; //send data
	while((SSI0_SR_R & SSI_SR_TNF) == 0){};
	wait(20); //wait for hold time
	GPIO_PORTB_DATA_R |= DAC_CS;
}

void Sound_Init(void){
	int delay;
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1; // 0) activate timer1
	DAC_Init();
  TIMER1_CTL_R &= ~0x00000001;     // 1) disable timer1A during setup
  TIMER1_CFG_R = 0x00000004;       // 2) configure for 16-bit timer mode
  TIMER1_TAMR_R = 0x00000002;      // 3) configure for periodic mode
  TIMER1_TAILR_R = 61;						// 4) interrupt every 61us for 16384 dac updates per sec
  TIMER1_TAPR_R = 49;              // 5) 1us timer0A
  TIMER1_ICR_R = 0x00000001;       // 6) clear timer0A timeout flag
  TIMER1_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x000002000; // 8) priority 1
  NVIC_EN0_R |= NVIC_EN0_INT21;    // 9) enable interrupt 21 in NVIC
	//TIMER1_CTL_R |= 0x00000001;      // 10) enable timer1A
	
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0; // 0) activate timer0
	delay = SYSCTL_RCGC1_R;
  TIMER0_CTL_R &= ~0x00000001;     // 1) disable timer0A during setup
  TIMER0_CFG_R = 0x00000000;       // 2) configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;      // 3) configure for periodic mode
  TIMER0_TAILR_R = 500000;					// 4) interrupt every 10ms
  TIMER0_ICR_R = 0x00000001;       // 6) clear timer0A timeout flag
  TIMER0_IMR_R |= 0x00000001;      // 7) arm timeout interrupt
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // 8) priority 2
  NVIC_EN0_R |= NVIC_EN0_INT19;    // 9) enable interrupt 19 in NVIC
  //TIMER0_CTL_R |= 0x00000001;      // 10) enable timer0A
	return;
}

//Play
//Performs any initialization required to begin playing a song such as loading song data, initializing variables, and starting timers
Music_Sheet *Music_Playing;
static int NumOf_Active_Channels;
static int ActiveChannels[16];

void Play(Song *PlaySong){
	int i = 0;
	int x = 0;
	time = 0;				//variable that keeps track of time
	Music_Playing = PlaySong->Song;
	//Load info of all Channels
	for(; i<16; i++){
		if(PlaySong->Channel[i].Active==1){
			SoundChannel[i].Tone = PlaySong->Channel[i].Tone;
			SoundChannel[i].Place_inMusicSheet = 0;
			ActiveChannels[x] = i;
			x++;
		}
	}
	NumOf_Active_Channels = x;
	for(i = 0; i < NumOf_Active_Channels; i++){
			//Initialize first note
			//Search for first note
			while(Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].channel != ActiveChannels[i]){
				SoundChannel[ActiveChannels[i]].Place_inMusicSheet++;
			}
			//once found, update the next_note info of the channel
			SoundChannel[ActiveChannels[i]].Next_Note = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].midi_note;
			SoundChannel[ActiveChannels[i]].Start_Time = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].start_time;
			SoundChannel[ActiveChannels[i]].Next_End_Time = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].end_time;
			SoundChannel[ActiveChannels[i]].End_Time = 100;
			//update Place_inMusicSheet so that it doesnt repeat on the next search
			SoundChannel[ActiveChannels[i]].Place_inMusicSheet++;
	}
	SongActive = 1;
	playing = 1;
	TIMER0_CTL_R |= 0x00000001;      // 10) enable timer0A
}


//Timer 0A handler updates note, channel property information, and any multipliers
//Actual sound wave calculations are done in Timer1A handler
//***NEEDS TO BE EDITTED STILL TO ACCOUNT FOR END OF SONG
void Timer0A_Handler(void){
	int i = 0;
	int end = 0;
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;
	if(Pots[5].flag == 1){
		pitch_bend = (((int)Pots[5].value) - 500)/70;
		Pots[5].flag = 0;
	}
	if(Pots[3].flag == 1){
	bass_boost = (((int)Pots[3].value))/80;
	Pots[3].flag = 0;
	}
	for(; i < NumOf_Active_Channels; i++){
		if(SoundChannel[ActiveChannels[i]].Start_Time <= time){
			SoundChannel[ActiveChannels[i]].Note = SoundChannel[ActiveChannels[i]].Next_Note;
			SoundChannel[ActiveChannels[i]].End_Time = SoundChannel[ActiveChannels[i]].Next_End_Time;
			//Search for the next note in the channel
			while((Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].channel != ActiveChannels[i]) & 
			(Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].channel != 17)){
				SoundChannel[ActiveChannels[i]].Place_inMusicSheet++;
			}
			if(Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].channel == 17){
				end++;
				SoundChannel[ActiveChannels[i]].Next_Note = OFF;
			}
			else{
				//once found, update the next_note info of the channel
				SoundChannel[ActiveChannels[i]].Next_Note = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].midi_note;
				SoundChannel[ActiveChannels[i]].Start_Time = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].start_time;
				SoundChannel[ActiveChannels[i]].Next_End_Time = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].end_time;
				//update Place_inMusicSheet so that it doesnt repeat on the next search
				SoundChannel[ActiveChannels[i]].Place_inMusicSheet++;
			}
		}
		else if(SoundChannel[i].End_Time <= time){
			SoundChannel[ActiveChannels[i]].Note = OFF;
		}
	}
	if(end == NumOf_Active_Channels){
		TIMER1_CTL_R &= ~0x00000001;      // 10) enable timer1A
		TIMER0_CTL_R &= ~0x00000001;      // 10) enable timer0A
		SongActive = 0;
		playing = 0;
		DAC_Out(0);
	}
	else{
		
		//Keep track of time
		time++;
		TIMER1_CTL_R |= 0x00000001;      // 10) enable timer1A
		TIMER0_ICR_R = TIMER_ICR_TATOCINT;
	}
}
void Rewind(void){
		int i = 0;
		TIMER1_CTL_R &= ~0x00000001;      // 10) enable timer1A
		TIMER0_CTL_R &= ~0x00000001;      // 10) enable timer0A
		for(i = 0; i < NumOf_Active_Channels; i++){
			//Initialize first note
			//Search for first note
			while(Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].channel != ActiveChannels[i]){
				SoundChannel[ActiveChannels[i]].Place_inMusicSheet++;
			}
			//once found, update the next_note info of the channel
			SoundChannel[ActiveChannels[i]].Next_Note = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].midi_note;
			SoundChannel[ActiveChannels[i]].Start_Time = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].start_time;
			SoundChannel[ActiveChannels[i]].Next_End_Time = Music_Playing[SoundChannel[ActiveChannels[i]].Place_inMusicSheet].end_time;
			SoundChannel[ActiveChannels[i]].End_Time = 100;
			//update Place_inMusicSheet so that it doesnt repeat on the next search
			SoundChannel[ActiveChannels[i]].Place_inMusicSheet++;
	}
	time = 0;
	playing = 1;
	SongActive = 1;
	TIMER0_CTL_R |= 0x00000001;      // 10) enable timer0A	
}

void Resume(void){
	TIMER1_CTL_R |= 0x00000001;      // 10) enable timer1A
	TIMER0_CTL_R |= 0x00000001;      // 10) enable timer0A
	playing = 1;
}

void Pause(void){
	TIMER1_CTL_R &= ~0x00000001;      // 10) enable timer1A
	TIMER0_CTL_R &= ~0x00000001;      // 10) enable timer0A
	playing = 0;
}


//Timer Handler
void Timer1A_Handler(void){
	int x = 0;
	unsigned short signal;
	int i = 0;
	int Note;
	music_data = 0;
	for(;i<NumOf_Active_Channels; i++){
		Note = SoundChannel[ActiveChannels[i]].Note;
		if(Note != OFF){
			Channel_Music_Data[ActiveChannels[i]] += ((FreqOfNotes[Note + pitch_bend]) * 32000)/16384;
			signal = SqaureWave[(Channel_Music_Data[ActiveChannels[i]]/1000)%32];
			if(Note < 50){
				if(signal > 2000){
				signal += ((50-Note) * bass_boost);
				}
				else{
					signal -=((50-Note) * bass_boost);
				}
			}
			music_data = music_data + signal;
			x++;
		}
	}
	if(x == 0){
		DAC_Out(previous_music_data);
		TIMER1_ICR_R = TIMER_ICR_TATOCINT;
		return;
	}
	previous_music_data = music_data;
	DAC_Out(music_data/x);
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;
	return;
}