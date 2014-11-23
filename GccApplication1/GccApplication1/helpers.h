/*
 * helpers.c
 *
 * Created: 11/22/2014 6:31:50 PM
 *  Author: Neri Lemus
 */ 
#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 

bool verify_code()
{
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

#endif //HELPERS_H