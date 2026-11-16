/*
2026-08-05
Library of useful functions

Ver 1.0 2026-08-05
 - KB_dtoa() added (Created with GitHub Copilot) [1]

Refs:
[1] https://github.com/copilot/c/3d8a1cd4-e2aa-4937-9b74-60801e7b243c
 */


#ifndef KB_AUX_H
#define KB_AUX_H
#include <Arduino.h>

#define KB_DTOA_MAX_PREC 18 // max precision for double (KB_dtoa())

bool KB_dtoa(double value, char *buf, size_t buf_size, int precision);  // dtoa for arduino


#endif
