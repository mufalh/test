/*
 * Methods.h
 *
 *  Created on: Jul 28, 2016
 *      Author: mohammed
 */

#define METHODS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int digitValue(char);
long int RomToDec(char roman_Number[10]);
char * dec2romanstr(int num);
char * addRn(char Number1[10], char Number2[10]);
char * subRn(char Number1[10], char Number2[10]);
int TestRn(char Number[10]);

