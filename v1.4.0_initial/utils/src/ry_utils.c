/*
* Copyright (c), ry Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_hal_rtc.h"
#include <stdio.h>

#include "ryos.h"
#include "ry_hal_mcu.h" 
#include "am_hal_reset.h"
#include "timer_management_service.h"
#include "data_management_service.h"

/*********************************************************************
 * CONSTANTS
 */ 
#define EMPTY  0xFF

 /*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    u32_t curNum;
    u8_t* tblEntrys;
} ry_tbl_t;


/*********************************************************************
 * LOCAL VARIABLES
 */

print_level_t print_level = PRINT_LEVEL_DEBUG;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

#if (RY_BUILD == RY_DEBUG)

/**
 * @brief   Assert function
 *
 * @param   None
 *
 * @return  None
 */
void _ry_assert(const char *filename, unsigned long line)
{
    volatile u32_t continueDebug = 0;
	u32_t level;
    LOG_DEBUG("[ry_assert]: Assert Called from file %s Line %d!\r\n", filename, line);
	
    level = ryos_enter_critical();
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set continueDebug to a non-zero
        value. */
        while( continueDebug == 0 );
    }
    ryos_exit_critical(level);
}

#else 

/**
 * @brief   Assert function
 *
 * @param   None
 *
 * @return  None
 */
void _ry_assert(const char *filename, unsigned long line)
{
    volatile u32_t continueDebug = 0;
	u32_t level;
    //LOG_ERROR("[ry_assert]: Assert Called from file %s Line %d!\r\n", filename, line);
    ry_assert_log();
    am_hal_reset_poi();
	
    level = ryos_enter_critical();
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set continueDebug to a non-zero
        value. */
        while( continueDebug == 0 );
    }
    ryos_exit_critical(level);
}


#endif  /* (RY_BUILD == RY_DEBUG) */


/**
 * @brief   Data dump utility API
 *
 * @param   None
 *
 * @return  None
 */
void ry_data_dump(uint8_t* data, int len, char split)
{
#if (RY_BUILD == RY_DEBUG)
    int i;
    for (i = 0; i < len; i++) {
        ry_board_printf("%02x ", data[i]);
    }
    ry_board_printf("\r\n");
#endif
}

void ry_hci_data_dump(uint8_t ui8Type, uint32_t ui32Len, uint8_t *pui8Buf)
{
    LOG_DEBUG("type: %02X \n", ui8Type);

    ry_data_dump(pui8Buf, ui32Len, ' ');    
}


/**
 * @brief   Utility API to check if all data is empty: 0xFF
 *
 * @param   data - Data to check
 * @param   len - Len of data
 *
 * @return  None
 */
bool is_empty(u8_t* data, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (data[i] != EMPTY) {
            return FALSE;
        }
    }
    return TRUE;
}

u64_t get_utc_tick(void)
{
    u8_t i = 0;
    u8_t month_day[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    
    u8_t month_day_leap[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    ry_time_t time = {0};
    tms_get_time(&time);
    
    bool is_leap_year = ((time.year % 4) == 0);
    
    u32_t days = (time.year  + 30)*365 + (time.year + 30)/4;        

    if (is_leap_year)
        for (i = 0; i < time.month - 1; i++) days += month_day_leap[i];
    else
        for (i = 0; i < time.month - 1; i++) days += month_day[i];
    
    days += time.day - 1;
    
    u64_t tick = days * 86400 + time.hour * 3600 + time.minute *60 + time.second;
    
    return tick;
}


/**
 * @brief   Utility API print the time in LOG.
 *
 * @param   None
 *
 * @return  None
 */

#if RY_BUILD == RY_DEBUG
char* print_time(void)
{
    static char print_time_buffer[32];
    uint32_t utc = ry_hal_rtc_get();

    uint32_t y = 2000+hal_time.ui32Year;
    uint32_t mon = hal_time.ui32Month;
    uint32_t d = hal_time.ui32DayOfMonth;
    uint32_t h = hal_time.ui32Hour;
    uint32_t min = hal_time.ui32Minute;
    uint32_t s = hal_time.ui32Second;
    uint32_t ms = hal_time.ui32Hundredths;
//  uint32_t weekday = hal_time.ui32Weekday;

    uint32_t len = sprintf(print_time_buffer, "%04d.%02d.%02d,%02d:%02d:%02d.%02d: ", y, mon, d, h, min, s, ms);
    if(len < 30) {
        print_time_buffer[len] = '\0';
    }
    return print_time_buffer;
//	ry_board_printf();
}
#else
char* print_time(void)
{
    static char release_print_time_buffer[32];
    uint32_t utc = ry_hal_rtc_get();

    uint32_t mon = hal_time.ui32Month;
    uint32_t d = hal_time.ui32DayOfMonth;
    uint32_t h = hal_time.ui32Hour;
    uint32_t min = hal_time.ui32Minute;
    uint32_t s = hal_time.ui32Second;
    uint32_t len = sprintf(release_print_time_buffer,"[%d-%d %d:%d:%d] ", mon, d, h, min, s);
    if(len < 30) {
        release_print_time_buffer[len] = '\0';
    }

    return release_print_time_buffer;
}
#endif



ry_sts_t ry_tbl_add(u8_t *table, u8_t maxEntryNum, u32_t entrySize, u8_t* newEntry, u8_t keySize, u8_t keyOffset)
{
    ry_sts_t ret = RY_SUCC;
    u8_t *entry = NULL;
    u8_t *empty;
    ry_tbl_t *p = (ry_tbl_t*)table;
    
    if (p->curNum == maxEntryNum) {
        ret = RY_SET_STS_VAL(RY_CID_TBL, RY_ERR_TABLE_FULL);
        goto tbl_add_err;
    }

    entry = ry_tbl_search(maxEntryNum, entrySize, (u8_t*)&p->tblEntrys, keyOffset, keySize, newEntry+keyOffset);
    if (entry) {
        ret = RY_SET_STS_VAL(RY_CID_TBL, RY_ERR_TABLE_EXISTED);
        goto tbl_add_err;
    }

    empty = ry_malloc(entrySize);
    ry_memset(empty, 0xFF, entrySize);
    if (!empty) {
        ret = RY_SET_STS_VAL(RY_CID_TBL, RY_ERR_NO_MEM);
        goto tbl_add_err;
    }

    //LOG_INFO("tbl add. table:0x%x, entry:0x%x, keysize:%d, keyoffset:%d\r\n",
    //    table, &p->tblEntrys, keySize, keyOffset);

    entry = ry_tbl_search(maxEntryNum, entrySize, (u8_t*)&p->tblEntrys, keyOffset, keySize, empty);
    if (!entry) {
        
        ret = RY_SET_STS_VAL(RY_CID_TBL, RY_ERR_TABLE_FULL);
        ry_free(empty);
        goto tbl_add_err;
    }

    ry_memcpy(entry, newEntry, entrySize);
    p->curNum++;

    ry_free(empty);

tbl_add_err:
    return ret;
}


ry_sts_t ry_tbl_del(u8_t *table, u8_t maxEntryNum, u32_t entrySize, u8_t* key, u8_t keySize, u8_t entryOffset)
{
    ry_sts_t ret = RY_SUCC;
    u8_t *entry = NULL;
    ry_tbl_t *p = (ry_tbl_t*)table;
    
    entry = ry_tbl_search(maxEntryNum, entrySize, (u8_t*)&p->tblEntrys, entryOffset, keySize, key);
    if (!entry) {
        ret = RY_SET_STS_VAL(RY_CID_TBL, RY_ERR_TABLE_NOT_FOUND);
        goto tbl_del_err;
    }

    if (p->curNum >= 1) {
        p->curNum --;
    } else {
        LOG_ERROR("[ry_tbl_del]: table count invalid\n");
    }

    ry_memset(entry, 0xFF, entrySize);

tbl_del_err:
    return ret;
}


void* ry_tbl_search(u8_t maxEntryNum, u32_t entrySize, u8_t* table, u8_t offset, u8_t cmpSize, u8_t* cmpKey)
{
    int i;
    for (i = 0; i < maxEntryNum; i++) {
        if (0 == ry_memcmp(table+i*entrySize+offset, cmpKey, cmpSize)) {
            /* Found */
            return table+i*entrySize;
        }
    }

    return NULL;
}


void ry_tbl_reset(u8_t* table, u8_t maxEntryNum, u32_t entrySize)
{
    ry_tbl_t *t = (ry_tbl_t*)table;
    ry_memset((u8_t*)t, 0xFF, entrySize * maxEntryNum + 4);
	t->curNum = 0;
}


#define SECONDS_IN_365_DAY_YEAR  (31536000)
#define SECONDS_IN_A_DAY         (86400)
#define SECONDS_IN_A_HOUR        (3600)
#define SECONDS_IN_A_MINUTE      (60)

static const uint32_t secondsPerMonth[12] =
{
    31*SECONDS_IN_A_DAY,
    28*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
};

unsigned char applib_dt_dayindex(unsigned short year, unsigned char month, unsigned char day)
{
    char century_code, year_code, month_code, day_code;
    int week = 0;

    century_code = year_code = month_code = day_code = 0;

    if (month == 1 || month == 2) {
        century_code = (year - 1) / 100;
        year_code = (year - 1) % 100;
        month_code = month + 12;
        day_code = day;
    } else {
        century_code = year / 100;
        year_code = year % 100;
        month_code = month;
        day_code = day;
    }

    week = year_code + year_code / 4 + century_code / 4 - 2 * century_code + 26 * (month_code + 1) / 10 + day_code - 1;
    week = week > 0 ? (week % 7) : ((week % 7) + 7);
    
// fix for 8 day of week    
    if (week == 7) week = 0;

    return week;
}


int ry_utc_parse(uint32_t ts, uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* weekday, uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	uint32_t time = ts; // + get_timezone();
    uint32_t days = ts/SECONDS_IN_A_DAY;

	/* Calculate year */
	uint32_t tmp = 1970 + time/SECONDS_IN_365_DAY_YEAR;
	uint32_t number_of_leap_years = (tmp - 1968 - 1)/4;
	time = time - ((tmp-1970)*SECONDS_IN_365_DAY_YEAR);
	if ( time > (number_of_leap_years*SECONDS_IN_A_DAY))
	{
		time = time - (number_of_leap_years*SECONDS_IN_A_DAY);
	}
	else
	{
		time = time - (number_of_leap_years*SECONDS_IN_A_DAY) + SECONDS_IN_365_DAY_YEAR;
		--tmp;
	}

	if(year)
		*year = tmp;

	/* Remember if the current year is a leap year */
	uint8_t  is_a_leap_year = ((tmp - 1968)%4 == 0);

	/* Calculate month */
	tmp = 1;
	int a;
	for (a = 0; a < 12; ++a){

		uint32_t seconds_per_month = secondsPerMonth[a];
		/* Compensate for leap year */
		if ((a == 1) && is_a_leap_year)
			seconds_per_month += SECONDS_IN_A_DAY;

		if (time > seconds_per_month){
			time -= seconds_per_month;
			tmp += 1;
		}else break;
	}

	if(month)	*month = tmp;

	/* Calculate day */
	tmp = time/SECONDS_IN_A_DAY;
	time = time - (tmp*SECONDS_IN_A_DAY);
	++tmp;
	if(day)*day = tmp;

    /* Calculate Weekday */
    //*weekday = (4 + days) % 7;
    *weekday = applib_dt_dayindex(*year, *month, *day);


	/* Calculate hour */
	tmp = time/SECONDS_IN_A_HOUR;
	time = time - (tmp*SECONDS_IN_A_HOUR);
	if(hour)*hour = tmp;

	/* Calculate minute */
	tmp = time/SECONDS_IN_A_MINUTE;
	time = time - (tmp*SECONDS_IN_A_MINUTE);
	if(min)*min = tmp;

	/* Calculate seconds */
	if(sec)*sec = time;

	return 0;
}


void reverse_mac(u8_t* mac)
{
    u8_t temp;
    int i;

    for (i = 0; i < 3; i++) {
        temp = mac[i];
        mac[i] = mac[5-i];
        mac[5-i] = temp;
    }
}

#if 0
int32_t g_time_zone = 8 * 60; // time zone, offset value in minutes
void appln_print_time(void)
{
    struct tm date_time;
    time_t curtime;
    curtime = arch_os_utc_now() + g_time_zone * 60;
    gmtime_r((const time_t *) &curtime, &date_time);
    ry_board_printf("%02d:%02d:%02d.%03d ", date_time.tm_hour, date_time.tm_min, date_time.tm_sec, arch_os_tick_now() % 1000);
}
#endif


int str2hex(char *s, int len, u8_t* h)  
{  
    int i;
    int j = 0;
    for (i = 0; i < len; i=i+2) {
        sscanf(s+i, "%02x", &h[j++]); 
    }

    return j;
}  


#include "aes.h"
ry_sts_t ry_aes_cbc_encrypt(u8_t* key, u8_t* iv, int len, u8_t* input, u8_t* output)
{
    int cnt;
    int last_len;
    int ret;
    mbedtls_aes_context *ctx = (mbedtls_aes_context*)ry_malloc(sizeof(mbedtls_aes_context));
    if (ctx == NULL) {
        LOG_ERROR("[ry_aes_cbc_encrypt] No mem. %d \r\n", ret);
        return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_NO_MEM);
    }

    mbedtls_aes_init( ctx );
    ret = mbedtls_aes_setkey_enc( ctx, key, 128 );
    if (ret < 0) {
        ry_free(ctx);
        LOG_ERROR("[ry_aes_cbc_encrypt] AES Set key Error. %d \r\n", ret);
        return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_ENCRYPTION);
    }

    last_len = len%16;
    cnt = last_len ? (len>>4)+1 : (len>>4);


    for (int i = 0; i < cnt; i++) {
        
        ret = mbedtls_aes_crypt_cbc( ctx, MBEDTLS_AES_ENCRYPT, 16, iv, input + i*16, output + i*16 );
        if (ret < 0) {
            ry_free(ctx);
            LOG_ERROR("AES Crypt Error. %d \r\n", ret);
            return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_ENCRYPTION);;
        }

        ry_data_dump(output + i*16, 16, ' ');
    }

    ry_free(ctx);
    return RY_SUCC;
}


ry_sts_t ry_aes_cbc_decrypt(u8_t* key, u8_t* iv, int len, u8_t* input, u8_t* output)
{
    int cnt;
    int last_len;
    int ret;
    mbedtls_aes_context *ctx = (mbedtls_aes_context*)ry_malloc(sizeof(mbedtls_aes_context));
    if (ctx == NULL) {
        LOG_ERROR("[ry_aes_cbc_decrypt] No mem. %d \r\n", ret);
        return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_NO_MEM);
    }

    mbedtls_aes_init( ctx );
    ret = mbedtls_aes_setkey_dec( ctx, key, 128 );
    if (ret < 0) {
        LOG_ERROR("[ry_aes_cbc_decrypt] AES Set key Error. %d \r\n", ret);
        ry_free(ctx);
        return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_ENCRYPTION);
    }

    last_len = len%16;
    cnt = last_len ? (len>>4)+1 : (len>>4);


    for (int i = 0; i < cnt; i++) {
        
        ret = mbedtls_aes_crypt_cbc( ctx, MBEDTLS_AES_DECRYPT, 16, iv, input + i*16, output + i*16 );
        if (ret < 0) {
            LOG_ERROR("AES Crypt Error. %d \r\n", ret);
            ry_free(ctx);
            return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_DECRYPTION);;
        }

        ry_data_dump(output + i*16, 16, ' ');
    }

    FREE_PTR(ctx);
    return RY_SUCC;
}

static size_t nextRand = 1;
u32_t ry_rand(void)
{
    size_t multiplier = ( size_t ) ry_hal_clock_time(), increment = ( size_t ) 1;
    
    /* Utility function to generate a pseudo random number. */
    nextRand = ( multiplier * nextRand ) + increment;
    return( ( nextRand >> 16 ) & ( ( size_t ) 0x7fff ) ) | ((ry_hal_clock_time()&0x00000FFF)<<16);

}


void ry_delay_ms(uint32_t time)
{
    volatile int32_t n;
    
    while (time > 0) {
        for (n = 4800; n > 0; n--)
            {__asm("nop");
        }
        time--;
    }
}


