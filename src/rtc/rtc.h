/* Copyright (C) 2011, 2012, 2013, 2014, 2015 The uOFW team
   See the file COPYING for copying permission.\n
*/

#ifndef __RTC_H__
#define __RTC_H__

typedef struct {
	u16 year;
	u16 month;
	u16 day;
	u16 hour;
	u16 minutes;
	u16 seconds;
	u32 microseconds;
} pspTime;

enum pspRtcCheckValidErrors {
	PSP_TIME_INVALID_YEAR = -1,
	PSP_TIME_INVALID_MONTH = -2,
	PSP_TIME_INVALID_DAY = -3,
	PSP_TIME_INVALID_HOUR = -4,
	PSP_TIME_INVALID_MINUTES = -5,
	PSP_TIME_INVALID_SECONDS = -6,
	PSP_TIME_INVALID_MICROSECONDS = -7
};

/**
 * Get the resolution of the tick counter
 *
 * @return # of ticks per second
 */
u32 sceRtcGetTickResolution(void);

s32 sceRtcSetAlarmTick(u64 *tick);
s32 sceRtcGetAlarmTick(u64 *tick);
s32 sceRtcIsAlarmed(void);
s32 sceRtcRegisterCallback(SceUID id);

/**
 * Get current tick count
 *
 * @param tick - pointer to u64 to receive tick count
 * @return 0 on success, < 0 on error
 */
s32 sceRtcGetCurrentTick(u64 *tick);

/**
 * Get current tick count, adjusted for local time zone
 *
 * @param time - pointer to pspTime struct to receive time
 * @param tz - time zone to adjust to (minutes from UTC)
 * @return 0 on success, < 0 on error
 */
s32 sceRtcGetCurrentClock(pspTime *time, int tz);

/**
 * Get current local time into a pspTime struct
 *
 * @param time - pointer to pspTime struct to receive time
 * @return 0 on success, < 0 on error
 */
s32 sceRtcGetCurrentClockLocalTime(pspTime *time);

/**
 * Convert a UTC-based tickcount into a local time tick count
 *
 * @param tickUTC - pointer to u64 tick in UTC time
 * @param tickLocal - pointer to u64 to receive tick in local time
 * @return 0 on success, < 0 on error
 */
void sceRtcConvertUtcToLocalTime(u64* tickUTC, const u64* tickLocal);

/**
 * Convert a local time based tickcount into a UTC-based tick count
 *
 * @param tickLocal - pointer to u64 tick in local time
 * @param tickUTC - pointer to u64 to receive tick in UTC based time
 * @return 0 on success, < 0 on error
 */
void sceRtcConvertLocalTimeToUTC(const u64* tickLocal, u64* tickUTC);

/**
 * Check if a year is a leap year
 *
 * @param year - year to check if is a leap year
 * @return 1 on leapyear, 0 if not
 */
s32 sceRtcIsLeapYear(int year);

/**
 * Get number of days in a specific month
 *
 * @param year - year in which to check (accounts for leap year)
 * @param month - month to get number of days for
 * @return # of days in month, <0 on error (?)
 */
s32 sceRtcGetDaysInMonth(int year, int month);

/**
 * Get day of the week for a date
 *
 * @param year - year in which to check (accounts for leap year)
 * @param month - month that day is in
 * @param day - day to get day of week for
 * @return day of week with 0 representing Monday
 */
s32 sceRtcGetDayOfWeek(int year, int month, int day);

/**
 * Validate pspDate component ranges
 *
 * @param date - pointer to pspDate struct to be checked
 * @return 0 on success, one of ::CheckValidErrors on error
 */
s32 sceRtcCheckValid(const pspTime* date);

/**
 * Set a pspTime struct based on ticks
 *
 * @param date - pointer to pspTime struct to set
 * @param tick - pointer to ticks to convert
 * @return 0 on success, < 0 on error
 */
s32 sceRtcSetTick(pspTime* date, const u64* tick);

/**
 * Set ticks based on a pspTime struct
 *
 * @param date - pointer to pspTime to convert
 * @param tick - pointer to tick to set
 * @return 0 on success, < 0 on error
 */
s32 sceRtcGetTick(const pspTime* date, u64 *tick);

/**
 * Compare two ticks
 *
 * @param tick1 - pointer to first tick
 * @param tick2 - poiinter to second tick
 * @return 0 on equal, <0 when tick1 < tick2, >0 when tick1 > tick2
 */
s32 sceRtcCompareTick(const u64* tick1, const u64* tick2);

/**
 * Add two ticks
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numTicks - number of ticks to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddTicks(u64* destTick, const u64* srcTick, u64 numTicks);

/**
 * Add an amount of ms to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numMS - number of ms to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddMicroseconds(u64* destTick, const u64* srcTick, u64 numMS);

/**
 * Add an amount of seconds to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numSecs - number of seconds to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddSeconds(u64* destTick, const u64* srcTick, u64 numSecs);

/**
 * Add an amount of minutes to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numMins - number of minutes to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddMinutes(u64* destTick, const u64* srcTick, u64 numMins);

/**
 * Add an amount of hours to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numHours - number of hours to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddHours(u64* destTick, const u64* srcTick, int numHours);

/**
 * Add an amount of days to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numDays - number of days to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddDays(u64* destTick, const u64* srcTick, int numDays);

/**
 * Add an amount of weeks to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numWeeks - number of weeks to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddWeeks(u64* destTick, const u64* srcTick, int numWeeks);


/**
 * Add an amount of months to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numMonths - number of months to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddMonths(u64* destTick, const u64* srcTick, int numMonths);

/**
 * Add an amount of years to a tick
 *
 * @param destTick - pointer to tick to hold result
 * @param srcTick - pointer to source tick
 * @param numYears - number of years to add
 * @return 0 on success, <0 on error
 */
s32 sceRtcTickAddYears(u64* destTick, const u64* srcTick, int numYears);

//s32 sceRtcSetTime_t(pspTime* date, const time_t time);
//s32 sceRtcGetTime_t(const pspTime* date, time_t *time);
s32 sceRtcSetDosTime(pspTime* date, u32 dosTime);
s32 sceRtcGetDosTime(pspTime* date, u32 dosTime);
s32 sceRtcSetWin32FileTime(pspTime* date, u64* win32Time);
s32 sceRtcGetWin32FileTime(pspTime* date, u64* win32Time);

s32 sceRtcParseDateTime(u64 *destTick, const char *dateString);

/**
 * Format Tick-representation UTC time in RFC2822 format
 */
s32 sceRtcFormatRFC2822(char *pszDateTime, const u64 *pUtc, int iTimeZoneMinutes);

/**
 * Format Tick-representation UTC time in RFC2822 format
 */
s32 sceRtcFormatRFC2822LocalTime(char *pszDateTime, const u64 *pUtc);

/**
 * Format Tick-representation UTC time in RFC3339(ISO8601) format
 */
s32 sceRtcFormatRFC3339(char *pszDateTime, const u64 *pUtc, int iTimeZoneMinutes);

/**
 * Format Tick-representation UTC time in RFC3339(ISO8601) format
 */
s32 sceRtcFormatRFC3339LocalTime(char *pszDateTime, const u64 *pUtc);

/**
 * Parse time information represented in RFC3339 format
 */
s32 sceRtcParseRFC3339(u64 *pUtc, const char *pszDateTime);

void registerFunctions(void);

#endif
