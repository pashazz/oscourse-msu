/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/time.h>
#include <inc/vsyscall.h>
#include <kern/vsyscall.h>

int gettime(void)
{
	nmi_disable();
	// LAB 12: your code here
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int year;
    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;

    // Note: This uses the "read registers until you get the same values twice in a row" technique
    //       to avoid getting dodgy/inconsistent values due to RTC updates

    while (mc146818_read(RTC_AREG) & (1 << 7));                // Make sure an update isn't in progress
    second = mc146818_read(0x00);
    minute = mc146818_read(0x02);
    hour = mc146818_read(0x04);
    day = mc146818_read(0x07);
    month = mc146818_read(0x08);
    year = mc146818_read(0x09);

    do {
          last_second = second;
          last_minute = minute;
          last_hour = hour;
          last_day = day;
          last_month = month;
          last_year = year;

          while (mc146818_read(RTC_AREG) & (1 << 7));           // Make sure an update isn't in progress
          second = mc146818_read(0x00);
          minute = mc146818_read(0x02);
          hour = mc146818_read(0x04);
          day = mc146818_read(0x07);
          month = mc146818_read(0x08);
          year = mc146818_read(0x09);
    } while( (last_second != second) || (last_minute != minute) || (last_hour != hour) ||
             (last_day != day) || (last_month != month) || (last_year != year));


    // Convert BCD to binary values if necessary

    second = (second & 0x0F) + ((second / 16) * 10);
    minute = (minute & 0x0F) + ((minute / 16) * 10);
    hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
    day = (day & 0x0F) + ((day / 16) * 10);
    month = (month & 0x0F) + ((month / 16) * 10);
    year = (year & 0x0F) + ((year / 16) * 10);

    struct tm t;
    t.tm_sec = second;
    t.tm_min = minute;
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_mon = month - 1;
    t.tm_year = year;
    int ret = timestamp(&t);

	nmi_enable();
	return ret;
}

void
rtc_init(void)
{
	nmi_disable();

    outb(IO_RTC_CMND, RTC_BREG);
    uint8_t b = inb(IO_RTC_DATA);
    b |= RTC_PIE;
    outb(IO_RTC_CMND, RTC_BREG);
    outb(IO_RTC_DATA, b);

    outb(IO_RTC_CMND, RTC_AREG);
    uint8_t a = inb(IO_RTC_DATA);
    outb(IO_RTC_CMND, RTC_AREG);
    outb(IO_RTC_DATA, a | 0x0F);

    vsys[VSYS_gettime] = gettime();

	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status = 0;
	outb(IO_RTC_CMND, RTC_CREG);
    status = inb(IO_RTC_DATA);

	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}

