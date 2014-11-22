/*
* Neri Lemus		(860977196)
* 
* Final Project
* Lab Section 021
* 
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/


/*
A  = x01
AB = x03
B  = x02
BC = x06
C  = x04
CD = x0C
D  = x08
DA = x09
*/
//ATmega1284 include files
#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 

//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h"

//CS 122A include files
#include "keypad.h"
#include "bit.h"
#include "lcd.h"

// Menu Strings
unsigned char * s = "Door Unlocker   Prototype";
unsigned char * page_one = "Door Lock: A    Change Code: B";
unsigned char * page_two = "Set AC temp: D  Next Page: C";
unsigned char * unlock_string = "Unlocking door";
unsigned char * lock_string = "Locking Door";
unsigned char * change_code_string = "Changing code";
unsigned char * set_thermo_string = "Setting AC Temp";

// Global Variables
unsigned char choice;
bool locked;
char page;

enum keyState {INITK, set_a, set_b, set_c, set_d} key_state;

void Key_Tick()
{
// Transitions
	switch(key_state)
	{
		case INITK:
			if(GetKeypadKey() == 'A')
				key_state = set_a;
			else if(GetKeypadKey() == 'B')
				key_state = set_b;
			else if(GetKeypadKey() == 'C')
				key_state = set_c;
			else if(GetKeypadKey() == 'D')
				key_state = set_d;
			else
				key_state = INITK;
			break;
		case set_a:
			key_state = INITK;
			break;
		case set_b:
			key_state = INITK;
			break;
		case set_c:
			key_state = INITK;
			break;
		case set_d:
			key_state = INITK;
			break;
	}

// Actions
	switch(key_state)
	{
		case INITK:
			break;
		case set_a:
			choice = 'A';
			break;
		case set_b:
			choice = 'B';
			break;
		case set_c:
			choice = 'C';
			break;
		case set_d:
			choice = 'D';
			break;
	}
}

enum LCD_states {LCDinit, door, ch_code, n_page, set_thermo} lcd_state;

void LCD_tick()
{
	// Actions
	switch(lcd_state)
	{
		case LCDinit:
			(page == 1) ? LCD_DisplayString(1, page_one) : LCD_DisplayString(1, page_two);
			break;
		case door:
			(locked) ? LCD_DisplayString(1, unlock_string) : LCD_DisplayString(1, lock_string);
			delay_ms(1500);
			choice = NULL;
			break;
		case ch_code:
			LCD_DisplayString(1, change_code_string);
			delay_ms(1500);
			choice = NULL;
			break;
		case n_page:
			page = (page == 1) ? 2 : 1;
			choice = NULL;
			break;
		case set_thermo:
			LCD_DisplayString(1, set_thermo_string);
			delay_ms(1500);
			choice = NULL;
			break;
	}
	
	// Transitions
	switch(lcd_state)
	{
		case LCDinit:
			if(choice == '\0' || choice == NULL)
				break;
			else if(choice == 'A')
				lcd_state = door;
			else if(choice == 'B')
				lcd_state = ch_code;
			else if(choice == 'C')
				lcd_state = n_page;
			else if(choice == 'D')
				lcd_state = set_thermo;
			else
				break;
			break;
		case door:
			lcd_state = LCDinit;
			break;
		case ch_code:
			lcd_state = LCDinit;
			break;
		case n_page:
			lcd_state = LCDinit;
			break;
		case set_thermo:
			lcd_state = LCDinit;
			break;
	}
}

void key_Init()
{
	key_state = INITK;
	choice = NULL;
}

void KeyTask()
{
	key_Init();
	for(;;)
	{
		Key_Tick();
		vTaskDelay(100);
	}
}

void LCD_task_init()
{
	locked = true;
	page = 1;
	lcd_state = LCDinit;
}

void LCDTask()
{
	LCD_task_init();
	LCD_init();
	LCD_DisplayString(1, s);
	delay_ms(3000);
	for(;;) 
	{
		LCD_tick();
		vTaskDelay(500);
	}
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(KeyTask, (signed portCHAR *)"KeyTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(LCDTask, (signed portCHAR *)"LCDTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	

int main(void) 
{
DDRA = 0xFF; PORTA = 0x00;
DDRB = 0xFF; PORTB = 0x00;
DDRC = 0xF0; PORTC = 0x0F;
DDRD = 0xFF; PORTD = 0x00;

//Start Tasks  
StartSecPulse(1);
//RunSchedular 
vTaskStartScheduler(); 

return 0; 
}