/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_time.h - Sys_Milliseconds / Sys_Microseconds equivalents on rg_time. */
#ifndef RGD3_TIME_H
#define RGD3_TIME_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

void rgd3_time_init(void);
int rgd3_time_ms(void);
u64 rgd3_time_us(void);
void rgd3_time_sleep_ms(int ms);

#endif
