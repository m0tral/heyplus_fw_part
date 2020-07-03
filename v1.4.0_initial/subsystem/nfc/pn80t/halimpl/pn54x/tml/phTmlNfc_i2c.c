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

/*
 * DAL I2C port implementation for linux
 *
 * Project: Trusted NFC Linux
 *
 */
#ifdef ANDROID
#include <hardware/nfc.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#else
#include "board.h"
//#include "port.h"
//#include "driver_config.h"
#endif

#include <phNxpLog.h>
#include <phTmlNfc_i2c.h>
#include <phNfcStatus.h>
#include <string.h>
#include "phOsalNfc.h"

#ifdef GDI_HAL_ENABLE
#include "gdi_type.h"
#include "gdi_hal_i2c.h"
#endif


#ifdef RY_HAL_ENABLE
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_hal_inc.h"
#include "nfc_pn80t.h"
#endif


#define CRC_LEN                     2
#define NORMAL_MODE_HEADER_LEN      3
#define FW_DNLD_HEADER_LEN          2
#define FW_DNLD_LEN_OFFSET          1
#define NORMAL_MODE_LEN_OFFSET      2
#define FRAGMENTSIZE_MAX            PHNFC_I2C_FRAGMENT_SIZE
#define I2C_READ_TIMEOUT            ((uint16_t)TIMEOUT_2S)
static bool_t bFwDnldFlag = FALSE;
phTmlNfc_i2cfragmentation_t fragmentation_enabled = I2C_FRAGMENATATION_DISABLED;

extern volatile bool RxIrqReceived;
extern volatile bool isirqEnabled;
extern void * RxIrqSem;

/*******************************************************************************
**
** Function         phTmlNfc_i2c_close
**
** Description      Closes PN54X device
**
** Parameters       pDevHandle - device handle
**
** Returns          None
**
*******************************************************************************/
void phTmlNfc_i2c_close(void *pDevHandle)
{
#ifdef ANDROID
	if (NULL != pDevHandle)
    {
        close((intptr_t)pDevHandle);
    }
#else

#if (GDI_HAL_ENABLE == TRUE)
    gdi_hal_i2c_se_close();
#elif (RY_HAL_ENABLE == TRUE)
    ry_hal_i2cm_powerdown((ry_i2c_t)nfc_i2c_instance);
    phOsalNfc_DeleteSemaphore(&RxIrqSem);
#else
	i2cmhandler_close();
#endif
#endif
    return;
}

/*******************************************************************************
**
** Function         phTmlNfc_i2c_open_and_configure
**
** Description      Open and configure PN54X device
**
** Parameters       pConfig     - hardware information
**                  pLinkHandle - device handle
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS            - open_and_configure operation success
**                  NFCSTATUS_INVALID_DEVICE     - device open operation failure
**
*******************************************************************************/
NFCSTATUS phTmlNfc_i2c_open_and_configure(void ** pLinkHandle)
{

    int nHandle;
#if (GDI_HAL_ENABLE == TRUE)
    nfc_hw_init();
    nHandle = 1;
#elif (RY_HAL_ENABLE == TRUE)
    nfc_hw_init();
    nHandle = 1;
#else
    nHandle = i2cmhandler_init();
#endif

    if (nHandle < 0)
    {
        NXPLOG_TML_E("_i2c_open() Failed: retval %x",nHandle);
        *pLinkHandle = NULL;
        return NFCSTATUS_INVALID_DEVICE;
    }

    *pLinkHandle = (void*) ((intptr_t)nHandle);

    /*Reset PN54X*/
    phTmlNfc_i2c_reset(1);
    phOsalNfc_Delay(100);
    phTmlNfc_i2c_reset(0);
    phOsalNfc_Delay(100);
    phTmlNfc_i2c_reset(1);

    return NFCSTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         phTmlNfc_i2c_read
**
** Description      Reads requested number of bytes from PN54X device into given buffer
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - buffer for read data
**                  nNbBytesToRead   - number of bytes requested to be read
**
** Returns          numRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*******************************************************************************/
#if 1
int phTmlNfc_i2c_read(uint8_t * pBuffer, int nNbBytesToRead)
{
#if (GDI_HAL_ENABLE == TRUE)
    gdi_sts_t gdi_status;
#elif (RY_HAL_ENABLE == TRUE)
    ry_sts_t  ry_status;
    u32_t time;
    u32_t irq_time = nfc_irq_time;
#endif

    int ret;
    int numread = 0;
    uint16_t totalBtyesToRead = 0;
    NFCSTATUS status;

    UNUSED(nNbBytesToRead);

    /* Enable and start waiting for IRQ, if it has not enabled and triggered by now */
    if(!isirqEnabled && RxIrqReceived == FALSE)
    {
        Enable_GPIO_IRQ();
    }

    status = phOsalNfc_ConsumeSemaphore(RxIrqSem);
    if (NFCSTATUS_SUCCESS == status)
    {
        RxIrqReceived = FALSE;

        /* Read Header */
        if (FALSE == bFwDnldFlag)
        {
            totalBtyesToRead = NORMAL_MODE_HEADER_LEN;
        }
        else
        {
            totalBtyesToRead = FW_DNLD_HEADER_LEN;
        }

        /* Read with 2 second timeout, so that the read thread can be aborted
           when the PN54X does not respond and we need to switch to FW download
           mode. This should be done via a control socket instead. */
#if (GDI_HAL_ENABLE == TRUE)
        gdi_status = gdi_hal_i2c_se_txrx(NULL, pBuffer, totalBtyesToRead);
        if (gdi_status == GDI_SUCC) {
            ret = 1;
        } else {
            ret = 0;
            totalBtyesToRead = 0;
        }
#elif (RY_HAL_ENABLE == TRUE)
        ry_status = ry_hal_i2cm_rx((ry_i2c_t)nfc_i2c_instance, pBuffer, totalBtyesToRead);
        if (ry_status == RY_SUCC) {
            ret = 1;
            
        } else {
            LOG_ERROR("nfc i2c rx fail. %x\r\n", ry_status);
            ryos_delay_ms(10);
            ret = 0;
            totalBtyesToRead = 0;
        }
#else
        ret = i2cmhandler_receive(pBuffer,&(totalBtyesToRead), I2C_READ_TIMEOUT);
#endif
            /*can't get the number of bytes read*/
        if (ret == 0)
        {
            NXPLOG_TML_E("i2c receive() Timeout: Header Read1");
            numread = -1;
            goto exit_phTmlNfc_i2c_read;
        }
        else
        {
            numread += totalBtyesToRead;
            if (FALSE == bFwDnldFlag)
            {
                totalBtyesToRead = NORMAL_MODE_HEADER_LEN;
            }
            else
            {
                totalBtyesToRead = FW_DNLD_HEADER_LEN;
            }
            /* While waiting for data,
             * If mode switch DNLD->NCI has occurred, issue another read to get the last byte of header */
            /* If mode switch NCI->DNLD has occurred, we have also read first byte of payload,
               so, read one byte less while reading the payload */
            if(totalBtyesToRead > numread)
            {
                totalBtyesToRead -= numread;
#if (GDI_HAL_ENABLE == TRUE)
                gdi_status = gdi_hal_i2c_se_txrx(NULL, (pBuffer+numread), totalBtyesToRead);
                if (gdi_status == GDI_SUCC) {
                    ret = 1;
                } else {
                    ret = 0;
                    totalBtyesToRead = 0;
                }
#elif (RY_HAL_ENABLE == TRUE)
                ry_status = ry_hal_i2cm_rx((ry_i2c_t)nfc_i2c_instance, (pBuffer+numread), totalBtyesToRead);
                if (ry_status == RY_SUCC) {
                    ret = 1;
                } else {
                    LOG_ERROR("nfc i2c rx fail. %x\r\n", ry_status);
                    ryos_delay_ms(10);
                    ret = 0;
                    totalBtyesToRead = 0;
                }
#else

                ret = i2cmhandler_receive((pBuffer+numread),&(totalBtyesToRead), I2C_READ_TIMEOUT);
#endif
                if (ret == 0)
                {
                    NXPLOG_TML_E("i2c receive() Timeout: Header Read2");
                    numread = -1;
                    goto exit_phTmlNfc_i2c_read;
                }
            }
            /* Read Payload */
            if(FALSE == bFwDnldFlag)
            {
                totalBtyesToRead = NORMAL_MODE_HEADER_LEN + pBuffer[NORMAL_MODE_LEN_OFFSET];
            }
            else
            {
                totalBtyesToRead = FW_DNLD_HEADER_LEN + pBuffer[FW_DNLD_LEN_OFFSET] + CRC_LEN;
            }
            totalBtyesToRead -=numread;
            if(totalBtyesToRead !=0)
            {
#if (GDI_HAL_ENABLE == TRUE)
                gdi_status = gdi_hal_i2c_se_txrx(NULL, (pBuffer+numread), totalBtyesToRead);
                if (gdi_status == GDI_SUCC) {
                    ret = 1;
                } else {
                    ret = 0;
                    totalBtyesToRead = 0;
                }
#elif (RY_HAL_ENABLE == TRUE)
                ry_status = ry_hal_i2cm_rx((ry_i2c_t)nfc_i2c_instance, (pBuffer+numread), totalBtyesToRead);
                if (ry_status == RY_SUCC) {
                    ret = 1;
                } else {
                    LOG_ERROR("nfc i2c rx fail. %x\r\n", ry_status);
                    ryos_delay_ms(10);
                    ret = 0;
                    totalBtyesToRead = 0;
                }
                
#else
                ret = i2cmhandler_receive((pBuffer+numread), &(totalBtyesToRead), I2C_READ_TIMEOUT);
#endif
                if (ret == 0)
                {
                    NXPLOG_TML_E("i2c receive() Timeout: Payload");
                    numread = -1;
                    goto exit_phTmlNfc_i2c_read;
                }
                else
                {
                    numread += totalBtyesToRead;
                }
            }
            else
            {
                NXPLOG_TML_E("_>>>>> Empty packet received !!");
            }
        }

exit_phTmlNfc_i2c_read:
        //Clear_GPIO0_IRQ();

        time = ry_hal_clock_time();
        
        
        if(!isirqEnabled)
        {
            Enable_GPIO_IRQ();
        }

        //LOG_DEBUG("NFC RX duration: %d ms.\r\n", ry_hal_calc_ms(time - irq_time));

        
        
    }
    return (numread);
}


#else
int phTmlNfc_i2c_read(uint8_t * pBuffer, int nNbBytesToRead)
{
#if (GDI_HAL_ENABLE == TRUE)
    gdi_sts_t gdi_status;
#elif (RY_HAL_ENABLE == TRUE)
    ry_sts_t  ry_status;
    u32_t time;
    u32_t irq_time = nfc_irq_time;
#endif

    int ret;
    int numread = 0;
    uint16_t totalBtyesToRead = 0;
    NFCSTATUS status;

    UNUSED(nNbBytesToRead);


    status = phOsalNfc_ConsumeSemaphore(RxIrqSem);
    if (NFCSTATUS_SUCCESS == status)
    {
        RxIrqReceived = FALSE;

        if (nfc_bufs[nfc_buf_rd].inuse) {
            ry_memcpy(pBuffer, nfc_bufs[nfc_buf_rd].buf, nfc_bufs[nfc_buf_rd].len);
            numread = nfc_bufs[nfc_buf_rd].len;
            nfc_buf_rd++;

            if (nfc_buf_rd == 8) {
                nfc_buf_rd = 0;
            }
        }

        time = ry_hal_clock_time();
        ry_board_debug_printf("NFC RX duration: %d ms.\r\n", ry_hal_calc_ms(time - irq_time));

        
        
    }
    return (numread);
}

#endif


/*******************************************************************************
**
** Function         phTmlNfc_i2c_write
**
** Description      Writes requested number of bytes from given buffer into PN54X device
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - buffer for read data
**                  nNbBytesToWrite  - number of bytes requested to be written
**
** Returns          numWrote   - number of successfully written bytes
**                  -1         - write operation failure
**
*******************************************************************************/
int phTmlNfc_i2c_write(uint8_t * pBuffer, int nNbBytesToWrite)
{

#if (GDI_HAL_ENABLE == TRUE)
    gdi_sts_t gdi_status;
#elif (RY_HAL_ENABLE == TRUE)
    ry_sts_t  ry_status;
#endif


    int ret;
    int numWrote = 0;

    int numBytes = nNbBytesToWrite;
    int delay;

    if(fragmentation_enabled == I2C_FRAGMENATATION_DISABLED && nNbBytesToWrite > FRAGMENTSIZE_MAX)
    {
        NXPLOG_TML_E("i2c_write() data larger than maximum I2C  size,enable I2C fragmentation");
        return -1;
    }



    while (numWrote < nNbBytesToWrite)
    {
        if(fragmentation_enabled == I2C_FRAGMENTATION_ENABLED && nNbBytesToWrite > FRAGMENTSIZE_MAX)
        {
            if(nNbBytesToWrite - numWrote > FRAGMENTSIZE_MAX)
            {
                numBytes = numWrote+ FRAGMENTSIZE_MAX;
            }
            else
            {
                numBytes = nNbBytesToWrite;
            }
        }


#ifdef GDI_HAL_ENABLE
        status = gdi_hal_i2c_se_txrx((void*) (pBuffer+numWrote), NULL, (numBytes-numWrote));
        if (status == GDI_SUCC) {
            ret = 1;
        } else {
            ret = 0;
        }
#elif (RY_HAL_ENABLE == TRUE)
        ry_status = ry_hal_i2cm_tx((ry_i2c_t)nfc_i2c_instance, (void*) (pBuffer+numWrote), (numBytes-numWrote));
        if (ry_status == RY_SUCC) {
            ret = numBytes-numWrote;
        } else {
            LOG_WARN("nfc i2c tx fail. %x\r\n", ry_status);
            ryos_delay_ms(10);
            ret = 0;
        }
#else
        ret = i2cmhandler_send((void*) (pBuffer+numWrote), (numBytes-numWrote));
#endif


        //ret = write((intptr_t)pDevHandle, pBuffer + numWrote, numBytes - numWrote);
        if (ret > 0)
        {
            numWrote += ret;
            if(fragmentation_enabled == I2C_FRAGMENTATION_ENABLED && numWrote < nNbBytesToWrite)
            {
                //usleep(500);
                //delay = 48*400;
                //while (delay--);
            }
        }
        else if (ret == 0)
        {
            NXPLOG_TML_E("_i2c_write() EOF");
            return -1;
        }
        else
        {
            NXPLOG_TML_E("_i2c_write() errno : %x",errno);
            return -1;
        }
    }


    return numWrote;



#if 0

	if(fragmentation_enabled == I2C_FRAGMENTATION_ENABLED && nNbBytesToWrite > FRAGMENTSIZE_MAX)
	{
		if(nNbBytesToWrite - numWrote > FRAGMENTSIZE_MAX)
		{
			numBytes = numWrote+ FRAGMENTSIZE_MAX;
		}
		else
		{
			numBytes = nNbBytesToWrite;
		}
	}
#ifdef GDI_HAL_ENABLE
    status = gdi_hal_i2c_se_txrx((void*) (pBuffer+numWrote), NULL, (numBytes-numWrote));
    if (status == GDI_SUCC) {
        ret = 1;
    } else {
        ret = 0;
    }
#elif (RY_HAL_ENABLE == TRUE)
    ry_status = ry_hal_i2cm_tx(I2C_IDX_NFC, (void*) (pBuffer+numWrote), (numBytes-numWrote));
    if (ry_status == RY_SUCC) {
        ret = 1;
    } else {
        ret = 0;
    }
#else
	ret = i2cmhandler_send((void*) (pBuffer+numWrote), (numBytes-numWrote));
#endif
	if(ret == 0)
	{
		NXPLOG_TML_E("_i2c_write() Error");
		return -1;
	}
    return (numBytes);

#endif
}

/*******************************************************************************
**
** Function         phTmlNfc_i2c_reset
**
** Description      Reset PN54X device, using VEN pin
**
** Parameters       pDevHandle     - valid device handle
**                  level          - reset level
**
** Returns           0   - reset operation success
**                  -1   - reset operation failure
**
*******************************************************************************/
#define PN544_SET_PWR _IOW(0xe9, 0x01, unsigned int)
int phTmlNfc_i2c_reset(long level)
{
    int ret = 0;
    NXPLOG_TML_D("phTmlNfc_i2c_reset(), VEN level %ld", level);

    bFwDnldFlag = FALSE;

#if (GDI_HAL_ENABLE == TRUE)
    switch(level)
	{
        case 0:
            gdi_hal_gpio_set(PORT_PIN_DNLD, PIN_DNLD, 0);
            gdi_hal_gpio_set(PORT_PIN_VEN, PIN_VEN, 0);
        break;

        case 1:
            gdi_hal_gpio_set(PORT_PIN_DNLD, PIN_DNLD, 0);
            gdi_hal_gpio_set(PORT_PIN_VEN, PIN_VEN, 1);
        break;

        case 2:
            gdi_hal_gpio_set(PORT_PIN_VEN,PIN_VEN,1);
            phOsalNfc_Delay(10);
            gdi_hal_gpio_set(PORT_PIN_DNLD, PIN_DNLD, 1);
            phOsalNfc_Delay(10);
            gdi_hal_gpio_set(PORT_PIN_VEN,PIN_VEN,0);
            phOsalNfc_Delay(10);
            gdi_hal_gpio_set(PORT_PIN_VEN,PIN_VEN,1);
            phOsalNfc_Delay(10);

            bFwDnldFlag = TRUE;
        break;

        default:
            NXPLOG_TML_E("i2c_reset() invalid param");
        break;
    }

#elif (RY_HAL_ENABLE == TRUE)
    switch(level)
	{
        case 0:
            ry_hal_gpio_set(GPIO_IDX_NFC_DNLD, 0);
            ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 0);
        break;

        case 1:
            ry_hal_gpio_set(GPIO_IDX_NFC_DNLD, 0);
            ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 1);
        break;

        case 2:
            ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 1);
            phOsalNfc_Delay(10);
            ry_hal_gpio_set(GPIO_IDX_NFC_DNLD, 1);
            phOsalNfc_Delay(10);
            ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 0);
            phOsalNfc_Delay(10);
            ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 1);
            phOsalNfc_Delay(10);

            bFwDnldFlag = TRUE;
        break;

        default:
            NXPLOG_TML_E("i2c_reset() invalid param");
        break;
    }
#else

    switch(level)
	{
        case 0:
            Chip_GPIO_WritePortBit(LPC_GPIO, PORT_PIN_DNLD, PIN_DNLD, 0);
            Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,0);
        break;

        case 1:
            Chip_GPIO_WritePortBit(LPC_GPIO, PORT_PIN_DNLD, PIN_DNLD, 0);
            Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,1);
        break;

        case 2:
            Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,1);
            phOsalNfc_Delay(10);
            Chip_GPIO_WritePortBit(LPC_GPIO, PORT_PIN_DNLD, PIN_DNLD, 1);
            phOsalNfc_Delay(10);
            Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,0);
            phOsalNfc_Delay(10);
            Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,1);
            phOsalNfc_Delay(10);

            bFwDnldFlag = TRUE;
        break;

        default:
            NXPLOG_TML_E("i2c_reset() invalid param");
        break;
    }
#endif

    return ret;
}

/*******************************************************************************
**
** Function         getDownloadFlag
**
** Description      Returns the current mode
**
** Parameters       none
**
** Returns           Current mode download/NCI
*******************************************************************************/
bool_t getDownloadFlag(void)
{

    return bFwDnldFlag;
}
