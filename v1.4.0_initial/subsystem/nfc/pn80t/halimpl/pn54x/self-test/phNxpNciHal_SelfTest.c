/*
 * Copyright (C) 2015 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <phNxpNciHal_SelfTest.h>
#include <phNxpLog.h>
#include <phOsalNfc_Thread.h>
#include <phOsalNfc_Timer.h>
#include <phOsalNfc.h>
#include <phOsalNfc_Log.h>

#ifdef NXP_HW_SELF_TEST
#if(NFC_NXP_CHIP_TYPE == PN553)
// Define the platform specific macros
#define NXP_NFCC_RESET_RSP_LEN       (0x10U)
#endif
#define HAL_READ_TIMEOUT   (2000)   /* Timeout value to wait for response from PN54X */
#define HAL_WRITE_MAX_RETRY     (10)
#define CORE_INIT_NCI2_0  FALSE

/******************* Structures and definitions *******************************/

typedef uint8_t (*st_validator_t)(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);

phAntenna_St_Resp_t phAntenna_resp;

typedef struct nci_test_data
{
    nci_data_t cmd;
    nci_data_t exp_rsp;
    nci_data_t exp_ntf;
    st_validator_t rsp_validator;
    st_validator_t ntf_validator;

}nci_test_data_t;

/******************* Global variables *****************************************/


static uint32_t timeoutTimerId = 0;
static int hal_read_timer_fired = 0;
static phNxpNciSelfTest_Control_t nxpnciselftest_ctrl;

/* TML Context */
extern phTmlNfc_Context_t *gpphTmlNfc_Context;

/* Global HAL Ref */
extern phNxpNciHal_Control_t nxpncihal_ctrl;

/* Driver parameters */
phLibNfc_sConfig_t   gDrvCfg;

NFCSTATUS gtxldo_status = NFCSTATUS_FAILED;
NFCSTATUS gagc_value_status = NFCSTATUS_FAILED;
NFCSTATUS gagc_nfcld_status = NFCSTATUS_FAILED;
NFCSTATUS gagc_differential_status = NFCSTATUS_FAILED;


static uint8_t st_validator_testEquals(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_null(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_testSWP1_vltg(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_testAntenna_Txldo(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_testAntenna_AgcVal(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_testAntenna_AgcVal_FixedNfcLd(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_measureAntenna_Txldo(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_measureAntenna_AgcVal(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_measureAntenna_AgcVal_FixedNfcLd(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
#if(NXP_HW_ANTENNA_LOOP4_SELF_TEST==TRUE)
static uint8_t st_validator_testAntenna_AgcVal_Differential(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static uint8_t st_validator_measureAntenna_AgcVal_Differential(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
#endif
static NFCSTATUS phNxpNciHal_getPrbsCmd (phNxpNfc_PrbsType_t prbs_type, phNxpNfc_PrbsHwType_t hw_prbs_type,
        uint8_t tech, uint8_t bitrate, uint8_t *prbs_cmd, uint8_t prbs_cmd_len);

/* Test data to validate SWP line 2*/
static const nci_test_data_t swp2_test_data[] = {
    {
        {
            0x04, {0x20,0x00,0x01,0x00} /* cmd */
        },
        {
            0x06, {0x40,0x00,0x03,0x00,NXP_NFCC_RESET_RSP_LEN,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
#if((NFC_NXP_CHIP_TYPE != PN547C2) && (CORE_INIT_NCI2_0))
            0x05, {0x20,0x01,0x02,0x00,0x00} /* cmd */
#else
            0x03, {0x20,0x01,0x00}
#endif
        },
        {
#if((NFC_NXP_CHIP_TYPE==PN551)||(NFC_NXP_CHIP_TYPE==PN553))
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#elif(NFC_NXP_CHIP_TYPE == PN548AD)
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#else
            0x4, {0x40,0x01,0x17,0x00 } /* exp_rsp */
#endif
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x03, {0x2F,0x02,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x02,0x05,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x04, {0x2F,0x3E,0x01,0x01} /* cmd */
        },
        {
            0x04, {0x4F,0x3E,0x01,0x00} /* exp_rsp */
        },
        {
            0x04, {0x6F,0x3E,0x02,0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_testEquals
    },

};

/* Test data to validate SWP line 1*/
static const nci_test_data_t swp1_test_data[] = {

    {
        {
            0x04, {0x20,0x00,0x01,0x00} /* cmd */
        },
        {
            0x06, {0x40,0x00,0x03,0x00,NXP_NFCC_RESET_RSP_LEN,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
#if((NFC_NXP_CHIP_TYPE != PN547C2) && (CORE_INIT_NCI2_0))
            0x05, {0x20,0x01,0x02,0x00,0x00} /* cmd */
#else
            0x03, {0x20,0x01,0x00}
#endif
        },
        {
#if((NFC_NXP_CHIP_TYPE==PN551)||(NFC_NXP_CHIP_TYPE==PN553))
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#elif(NFC_NXP_CHIP_TYPE == PN548AD)
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#else
            0x4, {0x40,0x01,0x17,0x00 } /* exp_rsp */
#endif
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x03, {0x2F,0x02,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x02,0x05,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x04, {0x2F,0x3E,0x01,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x3E,0x01,0x00} /* exp_rsp */
        },
        {
            0x04, {0x6F,0x3E,0x02,0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_testSWP1_vltg
    },
};

static const nci_test_data_t prbs_test_data[] = {
    {
        {
            0x04, {0x20,0x00,0x01,0x00} /* cmd */
        },
        {
#if(NFC_NXP_CHIP_TYPE != PN547C2)
            0x06, {0x40,0x00,0x03,0x00,NXP_NFCC_RESET_RSP_LEN,0x00} /* exp_rsp */
#else
            0x06, {0x40,0x00,0x03,0x00,0x10,0x00} /* exp_rsp */
#endif
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
#if((NFC_NXP_CHIP_TYPE != PN547C2) && (CORE_INIT_NCI2_0))
            0x05, {0x20,0x01,0x02,0x00,0x00} /* cmd */
#else
            0x03, {0x20,0x01,0x00} /* cmd */
#endif
        },
        {
#if((NFC_NXP_CHIP_TYPE==PN551)||(NFC_NXP_CHIP_TYPE==PN553))
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#elif(NFC_NXP_CHIP_TYPE == PN548C2)
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#else
            0x4, {0x40,0x01,0x17,0x00 } /* exp_rsp */
#endif
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
#if(NFC_NXP_CHIP_TYPE != PN547C2)
    },
    {
        {
            0x04, {0x2F,0x00,0x01,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x00,0x01,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
#endif
    }
};
/* Download pin test data 1 */
static const nci_test_data_t pin_test_data1[] = {
    {
        {
            0x04, {0x20,0x00,0x01,0x00} /* cmd */
        },
        {
            0x06, {0x40,0x00,0x03,0x00,NXP_NFCC_RESET_RSP_LEN,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
};

/* Download pin test data 2 */
static const nci_test_data_t pin_test_data2[] = {
    {
        {
            0x08, {0x00, 0x04, 0xD0, 0x11, 0x00, 0x00, 0x5B, 0x46} /* cmd */
        },
        {
            0x08, {0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x87, 0x16} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
};
/* Antenna self test data*/
static const nci_test_data_t antenna_self_test_data[] = {
    {
        {
            0x04, {0x20,0x00,0x01,0x00} /* cmd */
        },
        {
            0x06, {0x40,0x00,0x03,0x00,NXP_NFCC_RESET_RSP_LEN,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
#if((NFC_NXP_CHIP_TYPE != PN547C2) && (CORE_INIT_NCI2_0))
            0x05, {0x20,0x01,0x02,0x00,0x00} /* cmd */
#else
            0x03, {0x20,0x01,0x00} /* cmd */
#endif
        },
        {
#if((NFC_NXP_CHIP_TYPE==PN551)||(NFC_NXP_CHIP_TYPE==PN553))
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#elif(NFC_NXP_CHIP_TYPE == PN548AD)
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#else
            0x4, {0x40,0x01,0x17,0x00 } /* exp_rsp */
#endif
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x03, {0x2F,0x02,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x02,0x05,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x04, {0x2F,0x00,0x01,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x00,0x01,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x05, {0x2F, 0x3D, 0x02, 0x01, 0x80} /* TxLDO cureent measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testAntenna_Txldo,
        st_validator_null
    },
    {
        {
            0x07, {0x2F, 0x3D, 0x04, 0x02, 0xC8, 0x60, 0x03} /* AGC measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testAntenna_AgcVal,
        st_validator_null
    },
    {
        {
            0x07, {0x2F, 0x3D, 0x04, 0x04, 0x20, 0x08, 0x20} /* AGC with NFCLD measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testAntenna_AgcVal_FixedNfcLd,
        st_validator_null
    },
#if(NXP_HW_ANTENNA_LOOP4_SELF_TEST==TRUE)
    {
        {
            0x07, {0x2F, 0x3D, 0x04, 0x08, 0x8C, 0x60, 0x03} /* AGC with NFCLD measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testAntenna_AgcVal_Differential,
        st_validator_null
    },
#endif
    {
        {
            0x04, {0x2F,0x00,0x01,0x01} /* cmd */
        },
        {
            0x04, {0x4F,0x00,0x01,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
};

/* Antenna self test data*/
static const nci_test_data_t antenna_measure_data[] = {
    {
        {
            0x04, {0x20,0x00,0x01,0x00} /* cmd */
        },
        {
            0x04, {0x40,0x00,0x03,0x00,NXP_NFCC_RESET_RSP_LEN,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x03, {0x20,0x01,0x00} /* cmd */
        },
        {
#if((NFC_NXP_CHIP_TYPE==PN551)||(NFC_NXP_CHIP_TYPE==PN553))
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#elif(NFC_NXP_CHIP_TYPE == PN548AD)
            0x4, {0x40,0x01,0x19,0x00 } /* exp_rsp */
#else
            0x4, {0x40,0x01,0x17,0x00 } /* exp_rsp */
#endif
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x03, {0x2F,0x02,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x02,0x05,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x04, {0x2F,0x00,0x01,0x00} /* cmd */
        },
        {
            0x04, {0x4F,0x00,0x01,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    },
    {
        {
            0x05, {0x2F, 0x3D, 0x02, 0x01, 0x80} /* TxLDO cureent measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_measureAntenna_Txldo,
        st_validator_null
    },
    {
        {
            0x07, {0x2F, 0x3D, 0x04, 0x02, 0xCD, 0x60, 0x03} /* AGC measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_measureAntenna_AgcVal,
        st_validator_null
    },
    {
        {
            0x07, {0x2F, 0x3D, 0x04, 0x04, 0x20, 0x08, 0x20} /* AGC with NFCLD measurement cmd */
        },
        {
            0x03, {0x4F, 0x3D, 05} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_measureAntenna_AgcVal_FixedNfcLd,
        st_validator_null
    },
    {
        {
            0x04, {0x2F,0x00,0x01,0x01} /* cmd */
        },
        {
            0x04, {0x4F,0x00,0x01,0x00} /* exp_rsp */
        },
        {
            0x00, {0x00} /* ext_ntf */
        },
        st_validator_testEquals, /* validator */
        st_validator_null
    }
};

/************** Self test functions ***************************************/

static uint8_t st_validator_testEquals(nci_data_t *exp, phTmlNfc_TransactInfo_t *act);
static void hal_write_cb(void *pContext, phTmlNfc_TransactInfo_t *pInfo);
static void hal_read_timeout_cb(uint32_t TimerId, void *pContext);
static void hal_read_cb(void *pContext, phTmlNfc_TransactInfo_t *pInfo);

/*******************************************************************************
**
** Function         st_validator_null
**
** Description      Null Validator
**
** Returns          One
**
*******************************************************************************/
static uint8_t st_validator_null(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    UNUSED(exp);
    UNUSED(act);
    return 1;
}

/*******************************************************************************
**
** Function         st_validator_testSWP1_vltg
**
** Description      Validator function to validate swp1 connection.
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_testSWP1_vltg(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if( (act->wLength == 0x05) &&
            (memcmp(exp->p_data,act->pBuff,exp->len) == 0))
    {
        if(act->pBuff[4] == 0x01 || act->pBuff[4] == 0x02)
        {
            result = 1;
        }
    }

    return result;
}

/*******************************************************************************
**
** Function         st_validator_testAntenna_Txldo
**
** Description      Validator function to validate Antenna TxLDO current measurement.
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_testAntenna_Txldo(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    long measured_val = 0;
    int tolerance = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    NXPLOG_NCIHAL_D("st_validator_testAntenna_Txldo = 0x%x", act->pBuff[3]);
    if (0x05 == act->pBuff[2])
    {
        if (NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            NXPLOG_NCIHAL_D("Antenna: TxLDO current measured raw value in mA : 0x%x", act->pBuff[4]);
            if(0x00 == act->pBuff[5])
            {
                NXPLOG_NCIHAL_D("Measured range : 0x00 = 50 - 100 mA");
                measured_val = ((0.40 * act->pBuff[4]) + 50);
                NXPLOG_NCIHAL_D("TxLDO current absolute value in mA = %ld", measured_val);
            }
            else
            {
                NXPLOG_NCIHAL_D("Measured range : 0x01 = 20 - 70 mA");
                measured_val = ((0.40 * act->pBuff[4]) + 20);
                NXPLOG_NCIHAL_D("TxLDO current absolute value in mA = %ld", measured_val);
            }

            tolerance = (phAntenna_resp.wTxdoMeasuredRangeMax  *
                         phAntenna_resp.wTxdoMeasuredTolerance)/100;
            if ((measured_val <= phAntenna_resp.wTxdoMeasuredRangeMax + tolerance))
            {
                tolerance = (phAntenna_resp.wTxdoMeasuredRangeMin *
                             phAntenna_resp.wTxdoMeasuredTolerance)/100;
                if((measured_val >= phAntenna_resp.wTxdoMeasuredRangeMin - tolerance))
                {
                    gtxldo_status = NFCSTATUS_SUCCESS;
                    NXPLOG_NCIHAL_E("Test Antenna Response for TxLDO measurement PASS");
                }
                else
                {
                    gtxldo_status = NFCSTATUS_FAILED;
                    NXPLOG_NCIHAL_E("Test Antenna Response for TxLDO measurement FAIL");
                }
            }
            else
            {
                gtxldo_status = NFCSTATUS_FAILED;
                NXPLOG_NCIHAL_E("Test Antenna Response for TxLDO measurement FAIL");
            }
        }
        else
        {
            gtxldo_status = NFCSTATUS_FAILED;
            NXPLOG_NCIHAL_E("Test Antenna Response for TxLDO measurement failed: Invalid status");
        }

    }
    else
    {
        gtxldo_status = NFCSTATUS_FAILED;
        NXPLOG_NCIHAL_E("Test Antenna Response for TxLDO measurement failed: Invalid payload length");
    }

    return result;
}

/*******************************************************************************
**
** Function         st_validator_testAntenna_AgcVal
**
** Description      Validator function reads AGC value of antenna and print the info
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_testAntenna_AgcVal(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    int agc_tolerance = 0;
    long agc_val = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if (0x05 == act->pBuff[2])
    {
        if (NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            agc_tolerance = (phAntenna_resp.wAgcValue * phAntenna_resp.wAgcValueTolerance)/100;
            agc_val =  ((act->pBuff[5] << 8) | (act->pBuff[4]));
            NXPLOG_NCIHAL_D("AGC value : %ld", agc_val);
            if(((phAntenna_resp.wAgcValue - agc_tolerance) <= agc_val) &&
               (agc_val <= (phAntenna_resp.wAgcValue + agc_tolerance)))
            {
                gagc_value_status = NFCSTATUS_SUCCESS;
                NXPLOG_NCIHAL_E("Test Antenna Response for AGC Values  PASS");
            }
            else
            {
                gagc_value_status = NFCSTATUS_FAILED;
                NXPLOG_NCIHAL_E("Test Antenna Response for AGC Values  FAIL");
            }
        }
        else
        {
            gagc_value_status = NFCSTATUS_FAILED;
            NXPLOG_NCIHAL_E("Test Antenna Response for AGC Values  FAIL");
        }

    }
    else
    {
        gagc_value_status = NFCSTATUS_FAILED;
        NXPLOG_NCIHAL_E("Test Antenna Response for AGC value failed: Invalid payload length");
    }

    return result;
}
/*******************************************************************************
**
** Function         st_validator_testAntenna_AgcVal_FixedNfcLd
**
** Description      Validator function reads and print AGC value of
**                  antenna with fixed NFCLD
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_testAntenna_AgcVal_FixedNfcLd(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    int agc_nfcld_tolerance = 0;
    long agc_nfcld = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if(0x05 == act->pBuff[2])
    {
        if(NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            agc_nfcld_tolerance = (phAntenna_resp.wAgcValuewithfixedNFCLD *
                                   phAntenna_resp.wAgcValuewithfixedNFCLDTolerance)/100;
            agc_nfcld =  ((act->pBuff[5] << 8) | (act->pBuff[4]));
            NXPLOG_NCIHAL_D("AGC value with Fixed Nfcld  : %ld", agc_nfcld);

            if(((phAntenna_resp.wAgcValuewithfixedNFCLD - agc_nfcld_tolerance) <= agc_nfcld) &&
              (agc_nfcld <= (phAntenna_resp.wAgcValuewithfixedNFCLD + agc_nfcld_tolerance)))
            {
                gagc_nfcld_status = NFCSTATUS_SUCCESS;
                NXPLOG_NCIHAL_E("Test Antenna Response for AGC value with fixed NFCLD PASS");
            }
            else
            {
                gagc_nfcld_status =  NFCSTATUS_FAILED;
                NXPLOG_NCIHAL_E("Test Antenna Response for AGC value with fixed NFCLD FAIL");
            }
        }
        else
        {
            gagc_nfcld_status =  NFCSTATUS_FAILED;
            NXPLOG_NCIHAL_E("Test Antenna Response for AGC value with fixed NFCLD failed: Invalid status");
        }

    }
    else
    {
        gagc_nfcld_status =  NFCSTATUS_FAILED;
        NXPLOG_NCIHAL_E("Test Antenna Response for AGC value with fixed NFCLD failed: Invalid payload length");
    }

    return result;
}
#if(NXP_HW_ANTENNA_LOOP4_SELF_TEST==TRUE)
/*******************************************************************************
**
** Function         st_validator_testAntenna_AgcVal_Differential
**
** Description      Reads the AGC value with open/short RM from buffer and print
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_testAntenna_AgcVal_Differential(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    int agc_toleranceopen1 = 0;
    int agc_toleranceopen2 = 0;
    long agc_differentialOpen1 = 0;
    long agc_differentialOpen2 = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if (0x05 == act->pBuff[2])
    {
        if (NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            agc_toleranceopen1=(phAntenna_resp.wAgcDifferentialWithOpen1 *
                                phAntenna_resp.wAgcDifferentialWithOpenTolerance1)/100;
            agc_toleranceopen2=(phAntenna_resp.wAgcDifferentialWithOpen2 *
                                phAntenna_resp.wAgcDifferentialWithOpenTolerance2)/100;
            agc_differentialOpen1 =  ((act->pBuff[5] << 8) | (act->pBuff[4]));
            agc_differentialOpen2 =  ((act->pBuff[7] << 8) | (act->pBuff[6]));
            NXPLOG_NCIHAL_D("AGC value differentialOpen 1 : %ld", agc_differentialOpen1);
            NXPLOG_NCIHAL_D("AGC value differentialOpen 2 : %ld", agc_differentialOpen2);

            if(((agc_differentialOpen1 >= phAntenna_resp.wAgcDifferentialWithOpen1 - agc_toleranceopen1) &&
               (agc_differentialOpen1 <= phAntenna_resp.wAgcDifferentialWithOpen1 + agc_toleranceopen1)) &&
               ((agc_differentialOpen2 >= phAntenna_resp.wAgcDifferentialWithOpen2 - agc_toleranceopen2) &&
               (agc_differentialOpen2 <= phAntenna_resp.wAgcDifferentialWithOpen2 + agc_toleranceopen2)))
            {
                gagc_differential_status = NFCSTATUS_SUCCESS;
                NXPLOG_NCIHAL_E("Test Antenna Response for AGC Differential Open PASS");
            }
            else
            {
                gagc_differential_status = NFCSTATUS_FAILED;
                NXPLOG_NCIHAL_E("Test Antenna Response for AGC Differential Open  FAIL");
            }
        }
        else
        {
            NXPLOG_NCIHAL_E("Test Antenna Response for AGC Differential failed: Invalid status");
            gagc_differential_status = NFCSTATUS_FAILED;
        }

    }
    else
    {
        NXPLOG_NCIHAL_E("Test Antenna Response for AGC Differential failed: Invalid payload length");
        gagc_differential_status = NFCSTATUS_FAILED;
    }

    return result;
}
#endif
/*******************************************************************************
**
** Function         st_validator_measureAntenna_Txldo
**
** Description      Validator function to validate Antenna TxLDO current measurement.
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_measureAntenna_Txldo(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    long measured_val = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    NXPLOG_NCIHAL_D("st_validator_measureAntenna_Txldo = 0x%x", act->pBuff[3]);
    if (0x05 == act->pBuff[2])
    {
        if (NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            NXPLOG_NCIHAL_D("Antenna: TxLDO current measured raw value in mA : 0x%x", act->pBuff[4]);
            if(0x00 == act->pBuff[5])
            {
                NXPLOG_NCIHAL_D("Measured range : 0x00 = 50 - 100 mA");
                measured_val = ((0.40 * act->pBuff[4]) + 50);
                NXPLOG_NCIHAL_D("TxLDO current absolute value in mA = %ld", measured_val);
            }
            else
            {
                NXPLOG_NCIHAL_D("Measured range : 0x01 = 20 - 70 mA");
                measured_val = ((0.40 * act->pBuff[4]) + 20);
                NXPLOG_NCIHAL_D("TxLDO current absolute value in mA = %ld", measured_val);
            }

            phAntenna_resp.wTxdoRawValue = measured_val;
            phAntenna_resp.wTxdoMeasuredRangeMin = 0;
            phAntenna_resp.wTxdoMeasuredRangeMax = 0;
            phAntenna_resp.wTxdoMeasuredTolerance = 0;

            gtxldo_status = NFCSTATUS_SUCCESS;
        }
        else
        {
            gtxldo_status = NFCSTATUS_FAILED;
            NXPLOG_NCIHAL_E("Measure Antenna Response for TxLDO measurement failed: Invalid status");
        }

    }
    else
    {
        gtxldo_status = NFCSTATUS_FAILED;
        NXPLOG_NCIHAL_E("Measure Antenna Response for TxLDO measurement failed: Invalid payload length");
    }

    return result;
}

/*******************************************************************************
**
** Function         st_validator_measureAntenna_AgcVal
**
** Description      Validator function reads AGC value of antenna and print the info
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_measureAntenna_AgcVal(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    long agc_val = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if (0x05 == act->pBuff[2])
    {
        if (NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            agc_val =  ((act->pBuff[5] << 8) | (act->pBuff[4]));
            NXPLOG_NCIHAL_D("AGC value : %ld", agc_val);
            phAntenna_resp.wAgcValue = agc_val;
            phAntenna_resp.wAgcValueTolerance = 0;

            gagc_value_status = NFCSTATUS_SUCCESS;
        }
        else
        {
            gagc_value_status = NFCSTATUS_FAILED;
            NXPLOG_NCIHAL_E("Measure Antenna Response for AGC Values  FAIL");
        }

    }
    else
    {
        gagc_value_status = NFCSTATUS_FAILED;
        NXPLOG_NCIHAL_E("Measure Antenna Response for AGC value failed: Invalid payload length");
    }

    return result;
}
/*******************************************************************************
**
** Function         st_validator_measureAntenna_AgcVal_FixedNfcLd
**
** Description      Validator function reads and print AGC value of
**                  antenna with fixed NFCLD
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_measureAntenna_AgcVal_FixedNfcLd(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    long agc_nfcld = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if(0x05 == act->pBuff[2])
    {
        if(NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            agc_nfcld =  ((act->pBuff[5] << 8) | (act->pBuff[4]));
            NXPLOG_NCIHAL_D("AGC value with Fixed Nfcld  : %ld", agc_nfcld);

            phAntenna_resp.wAgcValuewithfixedNFCLD = agc_nfcld;
            phAntenna_resp.wAgcValuewithfixedNFCLDTolerance = 0;

            gagc_nfcld_status =  NFCSTATUS_SUCCESS;
        }
        else
        {
            gagc_nfcld_status =  NFCSTATUS_FAILED;
            NXPLOG_NCIHAL_E("Measure Antenna Response for AGC value with fixed NFCLD failed: Invalid status");
        }

    }
    else
    {
        gagc_nfcld_status =  NFCSTATUS_FAILED;
        NXPLOG_NCIHAL_E("Measure Antenna Response for AGC value with fixed NFCLD failed: Invalid payload length");
    }

    return result;
}
#if(NXP_HW_ANTENNA_LOOP4_SELF_TEST==TRUE)
/*******************************************************************************
**
** Function         st_validator_measureAntenna_AgcVal_Differential
**
** Description      Reads the AGC value with open/short RM from buffer and print
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_measureAntenna_AgcVal_Differential(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;
    long agc_differentialOpen1 = 0;
    long agc_differentialOpen2 = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }

    if (0x05 == act->pBuff[2])
    {
        if (NFCSTATUS_SUCCESS == act->pBuff[3])
        {
            result = 1;
            agc_differentialOpen1 =  ((act->pBuff[5] << 8) | (act->pBuff[4]));
            agc_differentialOpen2 =  ((act->pBuff[7] << 8) | (act->pBuff[6]));
            NXPLOG_NCIHAL_D("AGC value differentialOpen 1 : %ld", agc_differentialOpen1);
            NXPLOG_NCIHAL_D("AGC value differentialOpen 2 : %ld", agc_differentialOpen2);

            phAntenna_resp.wAgcDifferentialWithOpen1 = agc_differentialOpen1;
            phAntenna_resp.wAgcDifferentialWithOpen2 = agc_differentialOpen2;

            gagc_differential_status = NFCSTATUS_SUCCESS;
        }
        else
        {
            NXPLOG_NCIHAL_E("Measure Antenna Response for AGC Differential failed: Invalid status");
            gagc_differential_status = NFCSTATUS_FAILED;
        }

    }
    else
    {
        NXPLOG_NCIHAL_E("Measure Antenna Response for AGC Differential failed: Invalid payload length");
        gagc_differential_status = NFCSTATUS_FAILED;
    }

    return result;
}
#endif
/*******************************************************************************
**
** Function         st_validator_testEquals
**
** Description      Validator function to validate for equality between actual
**                  and expected values.
**
** Returns          One if successful otherwise Zero.
**
*******************************************************************************/
static uint8_t st_validator_testEquals(nci_data_t *exp, phTmlNfc_TransactInfo_t *act)
{
    uint8_t result = 0;

    if(NULL == exp || NULL == act)
    {
        return result;
    }
    if(exp->len <= act->wLength &&
            (memcmp(exp->p_data,act->pBuff,exp->len) == 0))
    {
        result = 1;
    }

    return result;
}

/*******************************************************************************
**
** Function         hal_read_timeout_cb
**
** Description      Callback function for hal write response timer.
**
** Returns          None
**
*******************************************************************************/
static void hal_read_timeout_cb(uint32_t timerId, void *pContext)
{
    UNUSED(timerId);
    NXPLOG_NCIHAL_E("hal_read_timeout_cb - read timeout!!!");
    hal_read_timer_fired = 1;
    hal_read_cb(pContext,NULL);
}

/*******************************************************************************
**
** Function         hal_write_cb
**
** Description      Callback function for hal write.
**
** Returns          None
**
*******************************************************************************/
static void hal_write_cb(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    phNxpNciHal_Sem_t *p_cb_data = (phNxpNciHal_Sem_t*) pContext;

    if (pInfo->wStatus == NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("write successful status = 0x%x", pInfo->wStatus);
    }
    else
    {
        NXPLOG_NCIHAL_E("write error status = 0x%x", pInfo->wStatus);
    }

    p_cb_data->status = pInfo->wStatus;
    SEM_POST(p_cb_data);

    return;
}

/*******************************************************************************
**
** Function         hal_read_cb
**
** Description      Callback function for hal read.
**
** Returns          None
**
*******************************************************************************/
static void hal_read_cb(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    phNxpNciHal_Sem_t *p_cb_data = (phNxpNciHal_Sem_t*) pContext;
    NFCSTATUS status;
    nci_test_data_t *test_data;

    if(hal_read_timer_fired == 1)
    {
        NXPLOG_NCIHAL_E("hal_read_cb - read timeout occurred");

        hal_read_timer_fired = 0;
        p_cb_data->status = NFCSTATUS_RESPONSE_TIMEOUT;
        status = phTmlNfc_ReadAbort();
    }
    else
    {
        status = phOsalNfc_Timer_Stop(timeoutTimerId);

        if (NFCSTATUS_SUCCESS == status)
        {
            NXPLOG_NCIHAL_D("Response timer stopped");
        }
        else
        {
            NXPLOG_NCIHAL_E("Response timer stop ERROR!!!");
            p_cb_data->status  = NFCSTATUS_FAILED;
        }
        if(pInfo == NULL)
        {
            NXPLOG_NCIHAL_E("Empty TransactInfo");
            p_cb_data->status  = NFCSTATUS_FAILED;
        }
        else
        {
            if (pInfo->wStatus == NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_D("hal_read_cb successful status = 0x%x", pInfo->wStatus);
                p_cb_data->status = NFCSTATUS_SUCCESS;
            }
            else
            {
                NXPLOG_NCIHAL_E("hal_read_cb error status = 0x%x", pInfo->wStatus);
                p_cb_data->status = NFCSTATUS_FAILED;
            }


            p_cb_data->status = pInfo->wStatus;

            test_data = (nci_test_data_t*) p_cb_data->pContext;

            if(test_data->exp_rsp.len == 0)
            {
                /* Compare the actual notification with expected notification.*/
                if( test_data->ntf_validator(&(test_data->exp_ntf),pInfo) == 1 )
                {
                    p_cb_data->status = NFCSTATUS_SUCCESS;
                }
                else
                {
                    p_cb_data->status = NFCSTATUS_FAILED;

                }
            }

            /* Compare the actual response with expected response.*/
            else if( test_data->rsp_validator(&(test_data->exp_rsp),pInfo) == 1)
            {
                p_cb_data->status = NFCSTATUS_SUCCESS;
            }
            else
            {
                p_cb_data->status = NFCSTATUS_FAILED;
            }

            // This is to mark that the RSP has been received and we will only expect the NTF next
            test_data->exp_rsp.len = 0;
        }
    }

    SEM_POST(p_cb_data);
    return;
}

/*******************************************************************************
**
** Function         phNxpNciHal_test_rx_task
**
** Description      Task to fetch and process messages from message queue.
**
** Returns          None
**
*******************************************************************************/
static void phNxpNciHal_test_rx_task(void *arg)
{
    phNxpNciSelfTest_Control_t *p_nxpnciselftest_ctrl = (phNxpNciSelfTest_Control_t *) arg;
    phLibNfc_Message_t msg;
    NXPLOG_NCIHAL_D("Self test task started");

    p_nxpnciselftest_ctrl->thread_running = 1;

    for (;;)
    {
        while (p_nxpnciselftest_ctrl->thread_running == 1)
        {
            /* Fetch next message from the NFC stack message queue */
            if (phDal4Nfc_msgrcv(gDrvCfg.nClientId,
                    &msg, 0, 0) == -1)
            {
                NXPLOG_NCIHAL_E("Received bad message");
                continue;
            }

            if(p_nxpnciselftest_ctrl->thread_running == 0){
                       break;
            }

            switch (msg.eMsgType)
            {
                case PH_LIBNFC_DEFERREDCALL_MSG:
                {
                    phLibNfc_DeferredCall_t *deferCall =
                            (phLibNfc_DeferredCall_t *) (msg.pMsgData);

                    REENTRANCE_LOCK();
                    deferCall->pCallback(deferCall->pParameter);
                    REENTRANCE_UNLOCK();

                    break;
                }
                case NCI_HAL_CLOSE_CPLT_MSG:
                {
                    NXPLOG_NCIHAL_E("NFC client received NCI_HAL_CLOSE_CPLT_MSG");
                    REENTRANCE_LOCK();
                    phOsalNfc_ProduceSemaphore(p_nxpnciselftest_ctrl->halClientSemaphore);
                    REENTRANCE_UNLOCK();
                    break;
                }
            }
        }
    }



    NXPLOG_NCIHAL_D("Self test thread stopped");
   // phOsalNfc_Thread_Delete(NULL);

    return;
}

/*******************************************************************************
**
** Function         phNxpNciHal_readLocked
**
** Description      Reads response and notification from NFCC and waits for
**                  read completion, for a definitive timeout value.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED,
**                  NFCSTATUS_RESPONSE_TIMEOUT in case of timeout.
**
*******************************************************************************/
static NFCSTATUS phNxpNciHal_readLocked(nci_test_data_t *pData )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    phNxpNciHal_Sem_t cb_data;
    /* RX Buffer */
    uint8_t rx_data[NCI_MAX_DATA_LEN];

    /* Create the local semaphore */
    if (phNxpNciHal_init_cb_data(&cb_data, pData) != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Read Create cb data failed");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    /* call read pending */
    status = phTmlNfc_Read(
            rx_data,
            NCI_MAX_DATA_LEN,
            (pphTmlNfc_TransactCompletionCb_t) &hal_read_cb,
            &cb_data);

    if (status != NFCSTATUS_PENDING)
    {
        NXPLOG_NCIHAL_E("TML Read error status = %x", status);
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    status = phOsalNfc_Timer_Start(timeoutTimerId,
            HAL_READ_TIMEOUT,
            &hal_read_timeout_cb,
            &cb_data);

    if (NFCSTATUS_SUCCESS == status)
    {
        NXPLOG_NCIHAL_D("Response timer started");
    }
    else
    {
        NXPLOG_NCIHAL_E("Response timer not started");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    /* Wait for callback response */
    if (SEM_WAIT(cb_data))
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Read semaphore error");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    if(cb_data.status == NFCSTATUS_RESPONSE_TIMEOUT)
    {
        NXPLOG_NCIHAL_E("Response timeout!!!");
        status = NFCSTATUS_RESPONSE_TIMEOUT;
        goto clean_and_return;

    }

    if (cb_data.status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Read failed  ");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    clean_and_return:
    phNxpNciHal_cleanup_cb_data(&cb_data);

    return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_writeLocked
**
** Description      Send command to NFCC and waits for cmd write completion, for
**                  a definitive timeout value.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED,
**                  NFCSTATUS_RESPONSE_TIMEOUT in case of timeout.
**
*******************************************************************************/
static NFCSTATUS phNxpNciHal_writeLocked(nci_test_data_t *pData )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    phNxpNciHal_Sem_t cb_data;
    int retryCnt = 0;

    /* Create the local semaphore */
    if (phNxpNciHal_init_cb_data(&cb_data, NULL) != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Write Create cb data failed");
        goto clean_and_return;
    }

retry:
    status = phTmlNfc_Write(pData->cmd.p_data, pData->cmd.len,
            (pphTmlNfc_TransactCompletionCb_t) &hal_write_cb, &cb_data);

    if (status != NFCSTATUS_PENDING)
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Write error status = %x", status);
        goto clean_and_return;
    }

    /* Wait for callback response */
    if (SEM_WAIT(cb_data))
    {
        NXPLOG_NCIHAL_E("write_unlocked semaphore error");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    if (cb_data.status != NFCSTATUS_SUCCESS && retryCnt < HAL_WRITE_MAX_RETRY)
    {
        retryCnt++;
        NXPLOG_NCIHAL_E("write_unlocked failed - PN54X Maybe in Standby Mode - Retry %d",retryCnt);
        /* 1ms delay to give NFCC wake up delay */
        phOsalNfc_Delay(10);
        goto retry;
    }

    status = cb_data.status;

clean_and_return:
    phNxpNciHal_cleanup_cb_data(&cb_data);

    return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_performTest
**
** Description      Performs a single cycle of command,response and
**                  notification.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED,
**
*******************************************************************************/
NFCSTATUS phNxpNciHal_performTest(const nci_test_data_t *pData )
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    nci_test_data_t *pTestData;

    if(NULL == pData)
    {
        return NFCSTATUS_FAILED;
    }

    CONCURRENCY_LOCK();
    /* Copy the data to another Read-Write structure so that input can be declared as const to save RAM */
    pTestData = (nci_test_data_t *) phOsalNfc_GetMemory(sizeof(nci_test_data_t));
    if(NULL == pTestData)
    {
        return NFCSTATUS_INSUFFICIENT_RESOURCES;
    }
    phOsalNfc_MemCopy(pTestData, pData, sizeof(nci_test_data_t));

    status = phNxpNciHal_writeLocked(pTestData);

    if(status == NFCSTATUS_RESPONSE_TIMEOUT)
    {
        goto clean_and_return;
    }
    if(status != NFCSTATUS_SUCCESS)
    {
        goto clean_and_return;
    }

    status = phNxpNciHal_readLocked(pTestData);

    if(status != NFCSTATUS_SUCCESS)
    {
        goto clean_and_return;
    }

    if(0 != pTestData->exp_ntf.len)
    {
        status = phNxpNciHal_readLocked(pTestData);

        if(status != NFCSTATUS_SUCCESS)
        {
            goto clean_and_return;
        }
    }

clean_and_return:
    CONCURRENCY_UNLOCK();
    phOsalNfc_FreeMemory(pTestData);
    pTestData = NULL;
    return status;
}

/*******************************************************************************
 **
 ** Function         phNxpNciHal_TestMode_open
 **
 ** Description      It opens the physical connection with NFCC (PN54X) and
 **                  creates required client thread for operation.
 **
 ** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED.
 **
 *******************************************************************************/
NFCSTATUS phNxpNciHal_TestMode_open (void)
{
    /* Thread */
    phOsalNfc_ThreadCreationParams_t threadparams;

    phOsalNfc_Config_t tOsalConfig;
    phTmlNfc_Config_t tTmlConfig;
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    hal_read_timer_fired = 0;
    gtxldo_status = NFCSTATUS_FAILED;
    gagc_value_status = NFCSTATUS_FAILED;
    gagc_nfcld_status = NFCSTATUS_FAILED;
    gagc_differential_status = NFCSTATUS_FAILED;

#ifdef DEBUG
    /* initialize trace level */
    phNxpLog_InitializeLogLevel();
#endif

    if (phNxpNciHal_init_monitor() == NULL)
    {
        NXPLOG_NCIHAL_E("Init monitor failed");
        return NFCSTATUS_FAILED;
    }

    CONCURRENCY_LOCK();
    phOsalNfc_SetMemory(&nxpnciselftest_ctrl, 0x00, sizeof(nxpnciselftest_ctrl));
    phOsalNfc_SetMemory(&tOsalConfig, 0x00, sizeof(tOsalConfig));
    phOsalNfc_SetMemory(&tTmlConfig, 0x00, sizeof(tTmlConfig));

    status = phOsalNfc_CreateSemaphore(&nxpnciselftest_ctrl.halClientSemaphore, 0);
        if(status != NFCSTATUS_SUCCESS)
            return status;


#ifdef ANDRIOD
    gDrvCfg.nClientId = phDal4Nfc_msgget(0, 0600);
#else
    gDrvCfg.nClientId = phDal4Nfc_msgget();
#endif
    gDrvCfg.nLinkType = ENUM_LINK_TYPE_I2C;/* For PN54X */
    tTmlConfig.pDevName = (int8_t *) "/dev/pn544";
    tOsalConfig.dwCallbackTaskHandle = (uintptr_t) gDrvCfg.nClientId;
    tOsalConfig.pLogFile = NULL;
    tTmlConfig.dwGetMsgTaskHandle = (uintptr_t) gDrvCfg.nClientId;
    nxpncihal_ctrl.gDrvCfg.nClientId = gDrvCfg.nClientId;

    /* Initialize TML layer */
    status = phTmlNfc_Init(&tTmlConfig);
    if (status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Init Failed");
        goto clean_and_return;
    }

    threadparams.stackdepth = (1*1024);
    strcpy((char*)threadparams.taskname, "TestModeTask");
    threadparams.pContext = &nxpnciselftest_ctrl;
    threadparams.priority = 4;
    if(phOsalNfc_Thread_Create(&nxpnciselftest_ctrl.test_rx_task, phNxpNciHal_test_rx_task, (void*)&threadparams) != 0)
    {
        NXPLOG_NCIHAL_E("TestMode Task create failed");
        phTmlNfc_Shutdown();
        goto clean_and_return;
    }
    if (nxpnciselftest_ctrl.test_rx_task == NULL)
    {
        NXPLOG_NCIHAL_E("xTaskCreate failed");
        phTmlNfc_Shutdown();
        goto clean_and_return;
    }

    timeoutTimerId = phOsalNfc_Timer_Create(FALSE);

    if(timeoutTimerId == 0xFFFF)
    {
        NXPLOG_NCIHAL_E("phOsalNfc_Timer_Create failed");
    }
    else
    {
        NXPLOG_NCIHAL_D("phOsalNfc_Timer_Create SUCCESS");
    }
    CONCURRENCY_UNLOCK();

    return NFCSTATUS_SUCCESS;

clean_and_return:
    CONCURRENCY_UNLOCK();
    phNxpNciHal_cleanup_monitor();
    return NFCSTATUS_FAILED;
}
/******************************************************************************
 * Function         phNxpNciHal_close_complete
 *
 * Description      This function inform libnfc-nci about result of
 *                  phNxpNciHal_close.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciSelfTest_close_complete(NFCSTATUS status)
{
    static phLibNfc_Message_t msg;

    if (status == NFCSTATUS_SUCCESS)
    {
        msg.eMsgType = NCI_HAL_CLOSE_CPLT_MSG;
    }
    else
    {
        msg.eMsgType = NCI_HAL_ERROR_MSG;
    }
    msg.pMsgData = NULL;
    msg.Size = 0;

    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
            &msg);

    return;
}

/*******************************************************************************
 **
 ** Function         phNxpNciHal_TestMode_close
 **
 ** Description      This function close the NFCC interface and free all
 **                  resources.
 **
 ** Returns          None.
 **
 *******************************************************************************/

void phNxpNciHal_TestMode_close ()
{

    NFCSTATUS status = NFCSTATUS_SUCCESS;

    CONCURRENCY_LOCK();
    /*Changed part of warning fixes in wearable project*/
    UNUSED(status);

    if (NULL != gpphTmlNfc_Context->pDevHandle)
    {
        phNxpNciSelfTest_close_complete(NFCSTATUS_SUCCESS);
        //wait till message NCI_HAL_CLOSE_CPLT_MSG is posted to HAL Client Task
        phOsalNfc_ConsumeSemaphore(nxpncihal_ctrl.halClientSemaphore);
        /* Abort any pending read and write */
        status = phTmlNfc_ReadAbort();
        status = phTmlNfc_WriteAbort();

        phOsalNfc_Timer_Cleanup();
        status = phTmlNfc_Shutdown();

        NXPLOG_NCIHAL_D("phNxpNciHal_close return status = %d", status);

        nxpnciselftest_ctrl.thread_running = 0;
        phDal4Nfc_msgrelease(gDrvCfg.nClientId);
        phOsalNfc_Thread_Delete(nxpnciselftest_ctrl.test_rx_task);
        phOsalNfc_DeleteSemaphore(&nxpnciselftest_ctrl.halClientSemaphore);
        phOsalNfc_SetMemory (&nxpnciselftest_ctrl, 0x00, sizeof (nxpnciselftest_ctrl));
    }

    CONCURRENCY_UNLOCK();

    phNxpNciHal_cleanup_monitor();

    /* Return success always */
    return;
}

/*******************************************************************************
 **
 ** Function         phNxpNciHal_SwpTest
 **
 ** Description      Test function to validate the SWP line. SWP line number is
 **                  is sent as parameter to the API.
 **
 ** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED.
 **
 *******************************************************************************/

NFCSTATUS phNxpNciHal_SwpTest(uint8_t swp_line)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int len = 0;
    int cnt = 0;

    NXPLOG_NCIHAL_D("phNxpNciHal_SwpTest - start\n");

    if(swp_line == 0x01)
    {
        len = (sizeof(swp1_test_data)/sizeof(swp1_test_data[0]));

        for(cnt = 0; cnt < len; cnt++)
        {
            status = phNxpNciHal_performTest(&(swp1_test_data[cnt]));
            if(status == NFCSTATUS_RESPONSE_TIMEOUT ||
                    status == NFCSTATUS_FAILED
            )
            {
                break;
            }
        }
    }
    else if(swp_line == 0x02)
    {
        len = (sizeof(swp2_test_data)/sizeof(swp2_test_data[0]));

        for(cnt = 0; cnt < len; cnt++)
        {
            status = phNxpNciHal_performTest(&(swp2_test_data[cnt]));
            if(status == NFCSTATUS_RESPONSE_TIMEOUT ||
                    status == NFCSTATUS_FAILED
            )
            {
                break;
            }
        }
    }
    else
    {
        status = NFCSTATUS_FAILED;
    }

    if( status == NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_SwpTest - SUCCESSS\n");
    }
    else
    {
        NXPLOG_NCIHAL_E("phNxpNciHal_SwpTest - FAILED\n");
    }

    NXPLOG_NCIHAL_D("phNxpNciHal_SwpTest - end\n");

    return status;
}
/*******************************************************************************
 **
 ** Function         phNxpNciHal_PrbsTestStart
 **
 ** Description      Test function start RF generation for RF technology and bit
 **                  rate. RF technology and bit rate are sent as parameter to
 **                  the API.
 **
 ** Returns          NFCSTATUS_SUCCESS if RF generation successful,
 **                  otherwise NFCSTATUS_FAILED.
 **
 *******************************************************************************/

NFCSTATUS phNxpNciHal_PrbsTestStart (phNxpNfc_PrbsType_t prbs_type, phNxpNfc_PrbsHwType_t hw_prbs_type,
        phNxpNfc_Tech_t tech, phNxpNfc_Bitrate_t bitrate)
{
    NFCSTATUS status = NFCSTATUS_FAILED;

    nci_test_data_t prbs_cmd_data;

    uint8_t rsp_cmd_info[] = {0x4F, 0x30, 0x01, 0x00};
    prbs_cmd_data.cmd.len = 0x09;

    memcpy(prbs_cmd_data.exp_rsp.p_data, &rsp_cmd_info[0], sizeof(rsp_cmd_info));
    prbs_cmd_data.exp_rsp.len = sizeof(rsp_cmd_info);

    //prbs_cmd_data.exp_rsp.len = 0x00;
    prbs_cmd_data.exp_ntf.len = 0x00;
    prbs_cmd_data.rsp_validator = st_validator_testEquals;
    prbs_cmd_data.ntf_validator = st_validator_null;

    uint8_t len = 0;
    uint8_t cnt = 0;

//    [NCI] -> [0x2F 0x30 0x04 0x00 0x00 0x01 0xFF]

    status = phNxpNciHal_getPrbsCmd(prbs_type, hw_prbs_type, tech, bitrate,
            prbs_cmd_data.cmd.p_data,prbs_cmd_data.cmd.len);

    if( status == NFCSTATUS_FAILED)
    {
        //Invalid Param.
        NXPLOG_NCIHAL_E("phNxpNciHal_PrbsTestStart - INVALID_PARAM\n");

        goto clean_and_return;
    }

    len = (sizeof(prbs_test_data)/sizeof(prbs_test_data[0]));

    for(cnt = 0; cnt < len; cnt++)
    {
        status = phNxpNciHal_performTest(&(prbs_test_data[cnt]));
        if(status == NFCSTATUS_RESPONSE_TIMEOUT ||
                status == NFCSTATUS_FAILED
        )
        {
            break;
        }
    }

    /* Ignoring status, as there will be no response - Applicable till FW version 8.1.1*/
    status = phNxpNciHal_performTest(&prbs_cmd_data);
    clean_and_return:

    if( status == NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_PrbsTestStart - SUCCESSS\n");
    }
    else
    {
        NXPLOG_NCIHAL_E("phNxpNciHal_PrbsTestStart - FAILED\n");
    }

    NXPLOG_NCIHAL_D("phNxpNciHal_PrbsTestStart - end\n");

    return status;
}

/*******************************************************************************
 **
 ** Function         phNxpNciHal_PrbsTestStop
 **
 ** Description      Test function stop RF generation for RF technology started
 **                  by phNxpNciHal_PrbsTestStart.
 **
 ** Returns          NFCSTATUS_SUCCESS if operation successful,
 **                  otherwise NFCSTATUS_FAILED.
 **
 *******************************************************************************/

NFCSTATUS phNxpNciHal_PrbsTestStop ()
{
    NXPLOG_NCIHAL_D("phNxpNciHal_PrbsTestStop - Start\n");

    NFCSTATUS status = NFCSTATUS_SUCCESS;

    status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);

    if(NFCSTATUS_SUCCESS == status)
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_PrbsTestStop - SUCCESS\n");
    }
    else
    {
        NXPLOG_NCIHAL_E("phNxpNciHal_PrbsTestStop - FAILED\n");

    }
    NXPLOG_NCIHAL_D("phNxpNciHal_PrbsTestStop - end\n");

    return status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_getPrbsCmd
**
** Description      Test function frames the PRBS command.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED.
**
*******************************************************************************/

NFCSTATUS phNxpNciHal_getPrbsCmd (phNxpNfc_PrbsType_t prbs_type, phNxpNfc_PrbsHwType_t hw_prbs_type,
        uint8_t tech, uint8_t bitrate, uint8_t *prbs_cmd, uint8_t prbs_cmd_len)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int position_tech_param = 0;
    int position_bit_param = 0;

    NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - prbs_type 0x%x hw_prbs_type = 0x%x tech 0x%x bitrate = 0x%x",
                        prbs_type, hw_prbs_type, tech, bitrate);
    if(NULL == prbs_cmd ||
            prbs_cmd_len != 0x09)
    {
        return status;
    }

    prbs_cmd[0] = 0x2F;
    prbs_cmd[1] = 0x30;
    prbs_cmd[2] = 0x06;
    prbs_cmd[3] = (uint8_t)prbs_type;
    prbs_cmd[4] = (uint8_t)hw_prbs_type;
    //0xFF Error value used for validation.
    prbs_cmd[5] = 0xFF;//TECH
    prbs_cmd[6] = 0xFF;//BITRATE
    prbs_cmd[7] = 0x01;
    prbs_cmd[8] = 0xFF;
    position_tech_param = 5;
    position_bit_param = 6;

    switch (tech) {
        case NFC_RF_TECHNOLOGY_A:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_RF_TECHNOLOGY_A");
            prbs_cmd[position_tech_param] = 0x00;
            break;
        case NFC_RF_TECHNOLOGY_B:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_RF_TECHNOLOGY_B");
            prbs_cmd[position_tech_param] = 0x01;
            break;
        case NFC_RF_TECHNOLOGY_F:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_RF_TECHNOLOGY_F");
            prbs_cmd[position_tech_param] = 0x02;
            break;
        default:
            break;
    }

    switch (bitrate)
    {
        case NFC_BIT_RATE_106:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_BIT_RATE_106");
            if(prbs_cmd[position_tech_param] != 0x02)
            {
                prbs_cmd[position_bit_param] = 0x00;
            }
            break;
        case NFC_BIT_RATE_212:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_BIT_RATE_212");
            prbs_cmd[position_bit_param] = 0x01;
            break;
        case NFC_BIT_RATE_424:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_BIT_RATE_424");
            prbs_cmd[position_bit_param] = 0x02;
            break;
        case NFC_BIT_RATE_848:
            NXPLOG_NCIHAL_D("phNxpNciHal_getPrbsCmd - NFC_BIT_RATE_848");
            if(prbs_cmd[position_tech_param] != 0x02)
            {
                prbs_cmd[position_bit_param] = 0x03;
            }
            break;
        default:
            break;
    }

    if(prbs_cmd[position_tech_param] == 0xFF || prbs_cmd[position_bit_param] == 0xFF)
    {
        //Invalid Param.
        status = NFCSTATUS_FAILED;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         phNxpNciHal_AntennaTest
 **
 ** Description
 **
 ** Returns
 **
 *******************************************************************************/
#if PN547C2_DISABLE_DEAD_FUN
NFCSTATUS phNxpNciHal_AntennaTest ()
{
    NFCSTATUS status = NFCSTATUS_FAILED;

    return status;
}

#endif
/*******************************************************************************
**
** Function         phNxpNciHal_PinTest
**
** Description      Test function to validate the FW download pin connection.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED.
**
*******************************************************************************/
NFCSTATUS phNxpNciHal_PinTest(phPinTest_Type_t eTestType)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    NFCSTATUS retstatus = NFCSTATUS_SUCCESS;

    int len = 0;
    int cnt = 0;

    NXPLOG_NCIHAL_D("phNxpNciHal_PinTest - start\n");
    NXPLOG_NCIHAL_D("phNxpNciHal_PinTest - TestType %d\n", eTestType);

    switch(eTestType)
    {
        case DownloadPinTest:
        {
            NXPLOG_NCIHAL_D("Pulling High the Download_Req Pin");
            status = phTmlNfc_IoCtl(phTmlNfc_e_EnableDownloadMode);
            if (NFCSTATUS_SUCCESS != status)
            {
                NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - Download Pin Toggling FAILED\n");
                retstatus = status;
            }
            else
            {
                len = (sizeof(pin_test_data2)/sizeof(pin_test_data2[0]));

                for(cnt = 0; cnt < len; cnt++)
                {
                    status = phNxpNciHal_performTest(&(pin_test_data2[cnt]));
                    if(status == NFCSTATUS_RESPONSE_TIMEOUT || status == NFCSTATUS_FAILED)
                    {
                        break;
                    }
                }

                if( status != NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - Download Data Exchange in Download Mode FAILED\n");
                    retstatus = NFCSTATUS_FAILED;
                }

                len = (sizeof(pin_test_data1)/sizeof(pin_test_data1[0]));

                for(cnt = 0; cnt < len; cnt++)
                {
                    status = phNxpNciHal_performTest(&(pin_test_data1[cnt]));
                    if(status == NFCSTATUS_RESPONSE_TIMEOUT || status == NFCSTATUS_FAILED)
                    {
                        break;
                    }
                }

                if (status == NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - NCI Data Exchange in Download Mode succeeded\n");
                    retstatus = NFCSTATUS_FAILED;
                }
            }
            break;
        }

        case VenPinTest:
        {
            NXPLOG_NCIHAL_D("Performing a VEN Pin Test");
            NXPLOG_NCIHAL_D("Pulling Low the Ven Pin");
            status = phTmlNfc_IoCtl(phTmlNfc_e_PullVenLow);
            if (NFCSTATUS_SUCCESS != status)
            {
                NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - Ven Pin Toggling to Low FAILED\n");
                retstatus = status;
            }
            else
            {
                len = (sizeof(pin_test_data1)/sizeof(pin_test_data1[0]));

                for(cnt = 0; cnt < len; cnt++)
                {
                    status = phNxpNciHal_performTest(&(pin_test_data1[cnt]));
                    if(status == NFCSTATUS_RESPONSE_TIMEOUT || status == NFCSTATUS_FAILED)
                    {
                        break;
                    }
                }

                if (status == NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - Data Exchange in with VEN pin low succeeded\n");
                    retstatus = NFCSTATUS_FAILED;
                }
            }

            NXPLOG_NCIHAL_D("Pulling High the Ven Pin");
            status = phTmlNfc_IoCtl(phTmlNfc_e_PullVenHigh);
            if (NFCSTATUS_SUCCESS != status)
            {
                NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - Ven Pin Toggling to High FAILED\n");
                retstatus = status;
            }
            else
            {
                len = (sizeof(pin_test_data1)/sizeof(pin_test_data1[0]));

                for(cnt = 0; cnt < len; cnt++)
                {
                    status = phNxpNciHal_performTest(&(pin_test_data1[cnt]));
                    if(status == NFCSTATUS_RESPONSE_TIMEOUT || status == NFCSTATUS_FAILED)
                    {
                        break;
                    }
                }

                if (status != NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("phNxpNciHal_PinTest - Data Exchange with Ven Pin High didn't succeed\n");
                    retstatus = status;
                }
            }
            break;
        }

        default:
        {
            NXPLOG_NCIHAL_E("Invalid TestType");
        }
    }

    if(retstatus == NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_PinTest - SUCCESSS\n");
    }
    NXPLOG_NCIHAL_D("phNxpNciHal_PinTest - end\n");
    return retstatus;
}

/*******************************************************************************
**
** Function         phNxpNciHal_AntennaSelfTest
**
** Description      Test function to validate the Antenna's discrete
**                  components connection.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED.
**
*******************************************************************************/
NFCSTATUS phNxpNciHal_AntennaSelfTest(phAntenna_St_Resp_t * phAntenna_St_Resp )
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    NFCSTATUS antenna_st_status = NFCSTATUS_FAILED;
    int len = 0;
    int cnt = 0;

    NXPLOG_NCIHAL_D("phNxpNciHal_AntennaSelfTest - start\n");
    memcpy(&phAntenna_resp, phAntenna_St_Resp, sizeof(phAntenna_St_Resp_t));
    len = (sizeof(antenna_self_test_data)/sizeof(antenna_self_test_data[0]));

    for(cnt = 0; cnt < len; cnt++)
    {
        status = phNxpNciHal_performTest(&(antenna_self_test_data[cnt]));
        if(status == NFCSTATUS_RESPONSE_TIMEOUT || status == NFCSTATUS_FAILED)
        {
            NXPLOG_NCIHAL_E("phNxpNciHal_AntennaSelfTest: commnad execution - FAILED\n");
            break;
        }
    }

    if(status == NFCSTATUS_SUCCESS)
    {
        if((gtxldo_status == NFCSTATUS_SUCCESS) && (gagc_value_status == NFCSTATUS_SUCCESS) &&
           (gagc_nfcld_status == NFCSTATUS_SUCCESS)
#if(NXP_HW_ANTENNA_LOOP4_SELF_TEST==TRUE)
           && (gagc_differential_status == NFCSTATUS_SUCCESS)
#endif
        )
        {
            antenna_st_status = NFCSTATUS_SUCCESS;
            NXPLOG_NCIHAL_D("phNxpNciHal_AntennaSelfTest - SUCESS\n");
        }
        else
        {
            NXPLOG_NCIHAL_D("phNxpNciHal_AntennaSelfTest - FAILED\n");
        }
    }
    else
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_AntennaSelfTest - FAILED\n");
    }

    NXPLOG_NCIHAL_D("phNxpNciHal_AntennaSelfTest - end\n");

    return antenna_st_status;
}

/*******************************************************************************
**
** Function         phNxpNciHal_AntennaMeasure
**
** Description      Test function to measure the Antenna's discrete
**                  components.
**
** Returns          NFCSTATUS_SUCCESS if successful,otherwise NFCSTATUS_FAILED.
**
*******************************************************************************/
NFCSTATUS phNxpNciHal_AntennaMeasure(phAntenna_St_Resp_t * phAntenna_St_Resp )
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    NFCSTATUS antenna_st_status = NFCSTATUS_FAILED;
    int len = 0;
    int cnt = 0;

    NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - start\n");
    memcpy(&phAntenna_resp, phAntenna_St_Resp, sizeof(phAntenna_St_Resp_t));
    len = (sizeof(antenna_measure_data)/sizeof(antenna_measure_data[0]));

    for(cnt = 0; cnt < len; cnt++)
    {
        status = phNxpNciHal_performTest(&(antenna_measure_data[cnt]));
        if(status == NFCSTATUS_RESPONSE_TIMEOUT || status == NFCSTATUS_FAILED)
        {
            NXPLOG_NCIHAL_E("phNxpNciHal_AntennaMeasure: command execution - FAILED\n");
            break;
        }
    }

    memcpy(phAntenna_St_Resp, &phAntenna_resp, sizeof(phAntenna_St_Resp_t));

    NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - %d, %d, %d, %d\n", phAntenna_St_Resp->wTxdoRawValue, phAntenna_St_Resp->wAgcValue, phAntenna_St_Resp->wAgcValuewithfixedNFCLD, phAntenna_St_Resp->wAgcDifferentialWithOpen1);
    NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - %04x, %04x, %04x, %04x\n", phAntenna_St_Resp->wTxdoRawValue, phAntenna_St_Resp->wAgcValue, phAntenna_St_Resp->wAgcValuewithfixedNFCLD, phAntenna_St_Resp->wAgcDifferentialWithOpen1);

    if(status == NFCSTATUS_SUCCESS)
    {
        if((gtxldo_status == NFCSTATUS_SUCCESS) && (gagc_value_status == NFCSTATUS_SUCCESS) &&
           (gagc_nfcld_status == NFCSTATUS_SUCCESS)
#if(NXP_HW_ANTENNA_LOOP4_SELF_TEST==TRUE)
           && (gagc_differential_status == NFCSTATUS_SUCCESS)
#endif
          )
        {
            antenna_st_status = NFCSTATUS_SUCCESS;
            NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - SUCESS\n");
        }
        else
        {
            NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - FAILED\n");
        }
    }
    else
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - FAILED\n");
    }

    NXPLOG_NCIHAL_D("phNxpNciHal_AntennaMeasure - end\n");

    return antenna_st_status;
}

#endif /*#ifdef NXP_HW_SELF_TEST*/
