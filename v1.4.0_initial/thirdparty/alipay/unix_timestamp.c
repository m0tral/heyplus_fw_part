//*****************************************************************************
//
//! @file unix_timestamp.c
//!
//! @brief unix time stamp functions.
//!
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2018, Ambiq Micro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 1.2.11 of the AmbiqSuite Development Package.
//
//*****************************************************************************
#include "unix_timestamp.h"

extern am_hal_rtc_time_t hal_time;

#define UTC_BASE_YEAR 1970
#define MONTH_PER_YEAR 12
#define DAY_PER_YEAR 365
#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

/* 每个月的天数 */
const uint32_t g_day_per_mon[MONTH_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
 * 功能：
 *     判断是否是闰年
 * 参数：
 *     year：需要判断的年份数
 * 返回值：
 *     闰年返回1，否则返回0
 */
uint32_t is_leap_year(uint32_t year)
{
    if ((year % 400) == 0) {
        return 1;
    } else if ((year % 100) == 0) {
        return 0;
    } else if ((year % 4) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * 功能：
 *     得到每个月有多少天
 * 参数：
 *     month：需要得到天数的月份数
 *     year：该月所对应的年份数
 *
 * 返回值：
 *     该月有多少天
 *
 */
uint32_t day_of_mon(uint32_t month, uint32_t year)
{
    if (month != 1) {
        return g_day_per_mon[month];
    } else {
        return g_day_per_mon[1] + is_leap_year(year);
    }
}

/*
 * 功能：
 *     根据时间计算出UTC时间戳
 *
 * 返回值：
 *     UTC时间戳
 */
uint32_t time_2_utc_sec(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    uint32_t i;
    uint32_t no_of_days = 0;
    uint32_t utc_time;
    uint32_t curr_year;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/

    /* year */
    curr_year = hal_time.ui32Year + 2000;

    for (i = UTC_BASE_YEAR; i < curr_year; i++) {
        no_of_days += (DAY_PER_YEAR + is_leap_year(i));
    }

    /* month */
    for (i = 0; i < hal_time.ui32Month; i++) {
        no_of_days += day_of_mon(i, curr_year);
    }

    /* day */
    no_of_days += (hal_time.ui32DayOfMonth - 1);

    /* sec */
    utc_time = no_of_days * SEC_PER_DAY + (hal_time.ui32Hour * SEC_PER_HOUR + \
                                            hal_time.ui32Minute * SEC_PER_MIN + \
                                             hal_time.ui32Second);

    /* UTC+8的北京时间 */
    utc_time -= SEC_PER_HOUR;

    return utc_time;
}

