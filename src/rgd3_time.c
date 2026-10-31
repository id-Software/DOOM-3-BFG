/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_time.c - integer milliseconds since startup, as the original
   Sys_Milliseconds (neo/sys/win32/win_shared.cpp) so game timing math stays int. */

static u64 rgd3_time_base;

void rgd3_time_init(void)
{
	rg_time_init();
	rgd3_time_base = rg_time_ticks();
}

int rgd3_time_ms(void)
{
	return (int)rg_time_ticks_to_ms(rg_time_ticks() - rgd3_time_base);
}

u64 rgd3_time_us(void)
{
	return (u64)rg_time_ticks_to_us(rg_time_ticks() - rgd3_time_base);
}

void rgd3_time_sleep_ms(int ms)
{
	if (ms <= 0)
		rg_time_yield();
	else
		rg_time_sleep_ms((u32)ms);
}
