//LCD.c
//Max Sigrist

#include "lm3s811.h"
#include "LCD.h"
#include "Time.h"

void Nibble_Out(unsigned char nibble){
	LCD_Port = (LCD_Port & 0x0F) + nibble;
	LCD_Port |= LCD_En;
	Clock_UsWait(6);
	LCD_Port &= ~LCD_En;
}
//LCD_Init
//Initializes the LCD Display
//Requires Time_Init() to have been called
void LCD_Init(void){
	int i;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD; // activate port D
  i = SYSCTL_RCGC2_R;      // allow time for clock to stabilize
  GPIO_PORTD_DIR_R |= (RS + LCD_En + DB7 + DB6 + DB5 + DB4);    // make PD7-2 out
  GPIO_PORTD_AFSEL_R &= ~(RS + LCD_En + DB7 + DB6 + DB5 + DB4); // regular port function 
  GPIO_PORTD_DEN_R |= (RS + LCD_En + DB7 + DB6 + DB5 + DB4);    // enable digital I/O on PD3-0
	GPIO_PORTD_DR8R_R = (RS + LCD_En + DB7 + DB6 + DB5 + DB4);      // 5) enable 8 mA drive
	LCD_Port = 0x00;
	Clock_MsWait(15);		//wait 15ms
	Nibble_Out(0x30);	//write 0x30 three times
	Clock_MsWait(5); //wait 10ms
	Nibble_Out(0x30);
	Clock_UsWait(160); //wait 160 us
	Nibble_Out(0x30);
	Clock_UsWait(160);
	Nibble_Out(0x20);
	Clock_MsWait(5);
	LCD_Cmd(0x28); //set in 4 bit mode, 2line
	LCD_Cmd(0x10);	//display off, cursor off, blink off
	LCD_Cmd(0x0c);  //clear screen and return cursor home
	LCD_Cmd(0x06);	//Increment cursor to the right when writing and dont shift screen
	return;
}

void LCD_Cmd(unsigned char cmd){
	unsigned char nibble;
	nibble = cmd & 0xF0;
	LCD_Port &= ~RS;
	Nibble_Out(nibble);
	nibble = cmd & 0x0F;
	nibble = nibble << 4;
	Nibble_Out(nibble);
	Clock_UsWait(40);
	return;
}
void LCD_Data(unsigned char data){
	unsigned char nibble;
	nibble = data & 0xF0;
	LCD_Port |= RS;
	Nibble_Out(nibble);
	nibble = data & 0x0F;
	nibble = nibble << 4;
	Nibble_Out(nibble);
	Clock_UsWait(40);
	return;
}

void LCD_OutString(char *string){
	while(*string){
		LCD_Data(*string);
		string++;
	}
	return;
}

void LCD_ClearScreen(void){
	LCD_Cmd(0x01);
	Clock_UsWait(1600);
	LCD_Cmd(0x02);
	Clock_UsWait(1600);
	return;
}

//void LCD_SetCursor(
//-----------------------LCD_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void LCD_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    LCD_OutUDec(n/10);
    n = n%10;
  }
  LCD_Data(n+'0'); /* n is between 0 and 9 */
}