//LCD.h

//Pin assignments
#define RS (0x04)
#define LCD_En (0x08)
#define DB4 (0x10)
#define DB5 (0x20)
#define DB6 (0x40)
#define DB7 (0x80)
#define LCD_Port (GPIO_PORTD_DATA_R) //lcd data port

//function definitions
void LCD_Init(void);
void LCD_Cmd(unsigned char cmd);
void LCD_NibbleOut(unsigned char nibble);
void LCD_OutString(char *string);
void LCD_ClearScreen(void);
void LCD_OutUDec(unsigned long n);