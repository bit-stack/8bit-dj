#include "PLL.h"
#include "Time.h"
#include "LCD.h"
#include "Sound.h"
#include "lm3s811.h"
#include "Songs.h"
#include "Potentiometers.h"

#define SONGTOPLAY Megaman

unsigned char string[30];

int main(void){
	int delay;
	Song *PlaySong;
	PLL_Init();
	Time_Init();
	LCD_Init();
	Sound_Init();
	LCD_ClearScreen();
	Potentiometers_Init();
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B for DAC chip select pin
	delay = SYSCTL_RCGC2_R; //dummy instruction to allow time for activation
	GPIO_PORTB_DIR_R &= ~0x40;    // make DAC_CS out
	GPIO_PORTB_AFSEL_R &= ~0x40; //No Alternate function on DAC_CS
	GPIO_PORTB_DEN_R |= 0x40;
	
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port B for DAC chip select pin
	delay = SYSCTL_RCGC2_R; //dummy instruction to allow time for activation
	GPIO_PORTD_DIR_R &= ~0x03;    // make DAC_CS out
	GPIO_PORTD_AFSEL_R &= ~0x03; //No Alternate function on DAC_CS
	GPIO_PORTD_DEN_R |= 0x03;
	
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE; // activate port B for DAC chip select pin
	delay = SYSCTL_RCGC2_R; //dummy instruction to allow time for activation
	GPIO_PORTE_DIR_R &= ~0x03;    // make DAC_CS out
	GPIO_PORTE_AFSEL_R &= ~0x03; //No Alternate function on DAC_CS
	GPIO_PORTE_DEN_R |= 0x03;
	
	PlaySong = &overworld;
  while(1){
		if(((GPIO_PORTB_DATA_R&0x40) == 0)){
		}
		if((GPIO_PORTE_DATA_R&0x02) == 0){
			if(playing == 1){
				Pause();
			}
			else{
				if(SongActive == 0){
					Play(PlaySong);
				}
				else{
					Resume();
				}
			}
			while((GPIO_PORTE_DATA_R&0x02) == 0){
			}
		}
		if((GPIO_PORTE_DATA_R&0x01) == 0){
			Rewind();
			while((GPIO_PORTE_DATA_R&0x01) == 0){
			}
		}
		if((GPIO_PORTD_DATA_R&0x01) == 0){
			
			while((GPIO_PORTD_DATA_R&0x01) == 0){
			}
		}
	}
}


