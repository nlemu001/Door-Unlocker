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
unsigned char * enter_cur_code_string = "Enter current code:";
unsigned char * enter_new_code_string = "Enter new 4-digit code:";
unsigned char * invalid_code_string = "Invalid code!";
unsigned char * new_temp_received = "New temp accepted";

// Global Variables
unsigned char choice;
bool locked;
char page;
bool motor_engage;
int one_eighty;
unsigned char motor_phase;
unsigned char direction;
int motor_cnt;
char code[5] = {'1','2','3','A','#'};
int new_temp = 72;
char buffer[33]  = "  ";
static unsigned char adc_value = 0;

// Start Analog to digital
void A2D_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

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

bool verify_code()
{
	bool first, sec, third, fourth, fifth = false;
	char temp = '\0';
	char temp_code[6] = {' ',' ',' ',' ',' ','\0'};
	LCD_DisplayString(1, enter_cur_code_string);
	// Verifying current passcode
	for(int i = 0; i < 5; i++)
	{
		while(true)
		{
			temp = GetKeypadKey();
			delay_ms(300);
			if(temp != '\0') break;
		}
		temp_code[i] = temp;

		LCD_DisplayString(1, temp_code);
		temp = '\0';
	}
	if(!(temp_code[0] == code[0] && temp_code[1] == code[1] && temp_code[2] == code[2] && 
		temp_code[3] == code[3] && temp_code[4] == code[4]))
		{
			LCD_DisplayString(1, invalid_code_string);
			return false;
		}
	
	return true;
}

void change_code()
{
	char temp = '\0';
	char n_code[6] = {' ',' ',' ',' ',' ','\0'};	
	LCD_DisplayString(1, enter_new_code_string);
	for(int i = 0; i < 5; i++)
	{
		while(true)
		{
			temp = GetKeypadKey();
			delay_ms(300);
			if(temp != '\0') break;
		}
		n_code[i] = temp;
		code[i] = temp;
		if(temp != '#')
			LCD_DisplayString(1, n_code);
		temp = '\0';
	}
}

enum JSStates {js_init, js_wait, js_check, js_l, js_r} js_state;
void change_temp()
{
	//js_state = js_wait;
	while(true)
	{
		adc_value = (unsigned char) (ADC>>2);
		snprintf(buffer, sizeof(buffer), "%d", new_temp);
		LCD_DisplayString(1, buffer);
		
		if(adc_value >= 130)
		{
			new_temp = new_temp + 1;
		}
		else if(adc_value <= 125)
			new_temp = new_temp - 1;
		else
			continue;
		delay_ms(500);
		if((~PINA & 0x80))
			break;
	
	}
	LCD_DisplayString(1, new_temp_received);
	delay_ms(1500);
}

void LCD_tick()
{
	// Actions
	switch(lcd_state)
	{
		case LCDinit:
			(page == 1) ? LCD_DisplayString(1, page_one) : LCD_DisplayString(1, page_two);
			break;
		case door:
			if(verify_code())
			{
				motor_engage = true;
				(locked) ? LCD_DisplayString(1, lock_string) : LCD_DisplayString(1, unlock_string);
				delay_ms(1500);
			}
			choice = NULL;
			break;
		case ch_code:
			LCD_DisplayString(1, change_code_string);
			delay_ms(1500);
			if(verify_code())
				change_code();
			choice = NULL;
			break;
		case n_page:
			page = (page == 1) ? 2 : 1;
			choice = NULL;
			break;
		case set_thermo:
			change_temp();
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

enum motorState {motor_init,L0,L1,L2,L3,L4,L5,L6,L7} motor_state;

void Motor_Tick()
{
	// Transitions
	switch(motor_state)
	{
		case motor_init:
			if(motor_engage && locked)
			{
				motor_state = L0;
				direction = 1;
			}
			else if(motor_engage && !locked)
			{
				motor_state = L7;
				direction = 2;
			}
			else
				break;
			break;
		case L0:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L1;
			else
				motor_state = L7;
			break;
		case L1:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L2;
			else
				motor_state = L0;
			break;
		case L2:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L3;
			else
				motor_state = L1;
			break;
		case L3:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L4;
			else
				motor_state = L2;
			break;
		case L4:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L5;
			else
				motor_state = L3;
			break;
		case L5:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L6;
			else
				motor_state = L4;
			break;
		case L6:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L7;
			else
				motor_state = L5;
			break;
		case L7:
			if(motor_cnt == one_eighty)
			{
				motor_state = motor_init;
				motor_engage = false;
				locked = (direction == 1) ? false : true;
			}
			else if(direction == 1)
				motor_state = L0;
			else
				motor_state = L6;
			break;
	}
	// Actions (Phases are left-shifted two bits to 
	//          accomodate LCD output of A0, A1)
	switch(motor_state)
	{
		case motor_init:
			motor_cnt = 0;
			break;
		case L0:
			motor_phase = 0x04;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L1:
			motor_phase = 0x0C;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L2:
			motor_phase = 0x08;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L3:
			motor_phase = 0x18;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L4:
			motor_phase = 0x10;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L5:
			motor_phase = 0x30;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L6:
			motor_phase = 0x20;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
		case L7:
			motor_phase = 0x24;
			//PORTA = PORTA | motor_phase;
			PORTA = motor_phase;
			motor_cnt += 3;
			break;
	}
}

/*
void Joystick_SM () 
{
	switch (js_state) 
	{
		case js_l:
			new_temp = new_temp - 1;
			break;
		case js_r:
			new_temp = new_temp + 1;
			break;
		default:
			break;
	}
	
	switch (js_state) 
	{
		case js_init:
			break;
		case js_wait:
			js_state = (adc_value >=125 && adc_value <= 135) ? js_wait : js_check;
			break;
		case js_check:
			js_state = (adc_value >= 132) ? js_r : js_l;
			break;
		default:
			js_state = js_wait;
			break;
	}
}*/

/*void JoystickTask() 
{
	js_state = js_init;
	for ( ; ; ) {
		Joystick_SM ();
		vTaskDelay(100);
	}
}*/

void motor_Init()
{
	motor_state = motor_init;
	motor_phase = 0x00;
	one_eighty = 3072;
	direction = 0;
	motor_cnt = 0;
}

void MotorTask()
{
	motor_Init();
	for(;;)
	{
		Motor_Tick();
		vTaskDelay(3);
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
	xTaskCreate(KeyTask, 		(signed portCHAR *)"KeyTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(LCDTask, 		(signed portCHAR *)"LCDTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(MotorTask, 		(signed portCHAR *)"MotorTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	//xTaskCreate(JoystickTask, 	(signed portCHAR *)"JoystickTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	

int main(void) 
{
	DDRA = 0x7E; PORTA = 0x81; // Enable button on A7
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xF0; PORTC = 0x0F;
	DDRD = 0xFF; PORTD = 0x00;

	//Start Tasks
	A2D_init ();
	StartSecPulse(1);
	//RunSchedular 
	vTaskStartScheduler(); 

	return 0; 
}