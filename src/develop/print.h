#pragma once
#include <stdarg.h>


void vprint(char const * formatString, va_list args);

void print(char const * formatString, ...);
void printc(bool shouldPrint, char const * formatString, ...);

void print16f(float const * f, char const * indent = "");
void print16fc(bool shouldPrint, float const * f, char const * indent = "");
