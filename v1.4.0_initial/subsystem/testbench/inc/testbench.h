/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __TESTBENCH_H__
#define __TESTBENCH_H__

typedef enum {
    TB_ST_IDLE      =   0x00,
    TB_ST_READY     =   0x01,
    TB_ST_BUSY      =   0x02,
    TB_ST_DONE      =   0x04,
    TB_ST_ERROR     =   0x80,
} tb_status_t;

typedef enum {
    TB_CMD_IDLE     =   0x00,
    TB_CMD_RUN      =   0x01,
    TB_CMD_LP       =   0x02,
    TB_CMD_SLEEP    =   0x04,
    TB_CMD_WAKEUP   =   0x08,
    TB_CMD_POFF     =   0x10,
} tb_cmd_t;

typedef struct
{
    uint8_t description[16];   
    uint8_t id;
    uint8_t cmd;     
    uint8_t status;
    uint8_t len;
    uint8_t para[256];
    uint8_t result[16];
}tb_script_t;

typedef struct
{
    uint32_t height;   
    uint32_t width;
    uint32_t brightness;     
    uint32_t resv;
}tb_oled_t;

typedef struct
{
    uint8_t     id;
    tb_script_t* pscript;
    void   (*setup)(tb_script_t* sc);        //测试预处理
    void   (*run)(tb_script_t* sc);          //测试程序
    void   (*teardown)(tb_script_t* sc);     //后处理
}tb_app_t;

extern tb_script_t tb_sc;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   testbench_machine
 *          decription: get test command and action
 *
 * @param   None
 *
 * @return  None
 */
void testbench_machine(void);

#endif //__TESTBENCH_H__
