/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_UTILS_H__
#define __RY_UTILS_H__

//#include "ry_mcu_config.h"
//#include "ry_bsp.h"
#include "app_config.h"
#include "board.h"
#include "ry_type.h"
//#include "data_management_service.h"

#if (RY_BUILD == RY_DEBUG)
    void _ry_assert(const char *filename, unsigned long line);
    #define RY_ASSERT(x)   if((x) == 0) _ry_assert( __FILE__, __LINE__ )
#else
    void _ry_assert(const char *filename, unsigned long line);    
    #define RY_ASSERT(x)   if((x) == 0) _ry_assert( __FILE__, __LINE__ ) //while(1) // TODO: Should change to send report to diagnostic task
#endif  /* (RY_BUILD == RY_DEBUG) */


typedef enum {
    PRINT_LEVEL_DEBUG = 0,
    PRINT_LEVEL_INFO,
    PRINT_LEVEL_WARNING,
    PRINT_LEVEL_ERROR,
    PRINT_LEVEL_FATAL,
} print_level_t;

extern print_level_t print_level;

char* print_time(void);

#ifndef LOG_LEVEL_RATE
#define LOG_LEVEL_RATE	        1
#endif


#ifndef APPCONFIG_DEBUG_ENABLE
#define APPCONFIG_DEBUG_ENABLE  0
#endif

extern void ry_rawLog_info_save(const char* fmt, ...);

#if APPCONFIG_DEBUG_ENABLE
#if (LOG_LEVEL_RATE  > 0)
    #define LOG_DEBUG(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_DEBUG){ry_board_printf("%s[D] "_fmt_, print_time(), ##__VA_ARGS__);}}while(0)
#else
	#define LOG_DEBUG(_fmt_, ...)
#endif
    #define LOG_INFO(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_INFO){ry_board_printf("%s[I] "_fmt_, print_time(), ##__VA_ARGS__);}}while(0)
    #define LOG_WARN(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_WARNING){ry_board_printf("%s[W] "_fmt_, print_time(), ##__VA_ARGS__);}}while(0)
    #define LOG_ERROR(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_ERROR){ry_board_printf("%s[E] "_fmt_, print_time(), ##__VA_ARGS__);}}while(0)
    #define LOG_FATAL(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_FATAL){ry_board_printf("%s[F] "_fmt_, print_time(), ##__VA_ARGS__);}}while(0)	
	#define LOG_RAW(_fmt_, ...)     do{if(RAWLOG_SAMPLE_ENABLE == TRUE)ry_rawLog_info_save(_fmt_, ##__VA_ARGS__);}while(0) 		
#else
    /*#define dbg(...)
    #define LOG_DEBUG(_fmt_, ...) ry_board_printf("[D] "_fmt_, ##__VA_ARGS__)
    #define LOG_INFO(_fmt_, ...)  ry_board_printf("[D] "_fmt_, ##__VA_ARGS__)
    #define LOG_WARN(_fmt_, ...)  ry_board_printf("[D] "_fmt_, ##__VA_ARGS__)
    #define LOG_ERROR(_fmt_, ...) ry_board_printf("[D] "_fmt_, ##__VA_ARGS__)
    #define LOG_FATAL(_fmt_, ...) ry_board_printf("[D] "_fmt_, ##__VA_ARGS__)*/
    #define dbg(...)
    #define LOG_DEBUG(_fmt_, ...) 
    extern void ry_error_info_save(const char* fmt, ...);
    #define LOG_INFO(_fmt_, ...)        do{if(RAWLOG_SAMPLE_ENABLE != TRUE)ry_error_info_save("%s[I] "_fmt_"\n", print_time(), ##__VA_ARGS__);}while(0)
    #define LOG_WARN(_fmt_, ...)        //do{ry_error_info_save("%s[W] "_fmt_"\n", print_time(), ##__VA_ARGS__);}while(0)
    #define LOG_ERROR(_fmt_, ...)       do{if(RAWLOG_SAMPLE_ENABLE != TRUE)ry_error_info_save("%s[E] "_fmt_"\n", print_time(), ##__VA_ARGS__);}while(0)
    #define LOG_FATAL(_fmt_, ...) 
    #define LOG_RAW(_fmt_, ...)         do{if(RAWLOG_SAMPLE_ENABLE == TRUE)ry_rawLog_info_save(_fmt_, ##__VA_ARGS__);}while(0)    
#endif /* APPCONFIG_DEBUG_ENABLE */


#ifndef MIN
#define MIN(n,m)   (((n) < (m)) ? (n) : (m))
#endif

#ifndef MAX
#define MAX(n,m)   (((n) < (m)) ? (m) : (n))
#endif

#ifndef ABS
#define ABS(n)     (((n) < 0) ? -(n) : (n))
#endif

// error info str

#define ERR_STR_MALLOC_FAIL         "malloc fail"
#define ERR_STR_ENCODE_FAIL         "encode fail"
#define ERR_STR_DECODE_FAIL         "decode fail"

#define ERR_STR_QUEUE_SEND_FAIL     "Queue send error"
#define ERR_STR_FILE_OPEN           "file open fail"
#define ERR_STR_FILE_WRITE          "file write fail"
#define ERR_STR_FILE_READ           "file read fail"
#define ERR_STR_FILE_SEEK           "file seek fail"
#define ERR_STR_FILE_CLOSE          "file close fail"


/**
 * @brief   Data dump utility API
 *
 * @param   None
 *
 * @return  None
 */
void ry_data_dump(uint8_t* data, int len, char split);

/**
 * @brief   Utility API to check if all data is empty: 0xFF
 *
 * @param   data - Data to check
 * @param   len - Len of data
 *
 * @return  None
 */
bool is_empty(u8* data, int len);


ry_sts_t ry_tbl_add(u8_t *table, u8_t maxEntryNum, u32_t entrySize, u8_t* newEntry, u8_t keySize, u8_t keyOffset);
ry_sts_t ry_tbl_del(u8_t *table, u8_t maxEntryNum, u32_t entrySize, u8_t* key, u8_t keySize, u8_t entryOffset);
void*    ry_tbl_search(u8_t maxEntryNum, u32_t entrySize, u8_t* table, u8_t offset, u8_t cmpSize, u8_t* cmpKey);
void     ry_tbl_reset(u8_t* table, u8_t maxEntryNum, u32_t entrySize);
void     reverse_mac(u8_t* mac);

int      str2hex(char *s, int len, u8_t* h);
int      ry_utc_parse(uint32_t ts, uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* weekday, uint8_t* hour, uint8_t* min, uint8_t* sec);

ry_sts_t ry_aes_cbc_encrypt(u8_t* key, u8_t* iv, int len, u8_t* input, u8_t* output);
ry_sts_t ry_aes_cbc_decrypt(u8_t* key, u8_t* iv, int len, u8_t* input, u8_t* output);

u64_t get_utc_tick(void);
u32_t ry_rand(void);
void ry_delay_ms(uint32_t time);



#endif  /* __RY_UTILS_H__ */
