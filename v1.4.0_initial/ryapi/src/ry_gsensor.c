/*
* Copyright (c), RY Inc.
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
#include "ry_mcu_config.h"
#include "app_config.h"

#include "ry_gsensor.h"
#include "ry_hal_inc.h"

#include "gsensor.h"

#include "mcube_sample.h"
#include "mcube_drv.h"

#define GSENSOR_MCUBE_TEST_EN		1
#define GSENSOR_BMA_TEST_EN			0
#define GSENSOR_BMA_TEST_STEP		0

#if (RY_GSENSOR_ENABLE == TRUE)

/* Test Acquisition of sensor samples */
int ry_gsensor_processing(void)
{
	return 0;
}

#if 0
ry_sts_t ry_gsensor_init(void)
{
  	ry_sts_t ret = RY_ERR_INIT_DEFAULT;

#if GSENSOR_MCUBE_TEST_EN	
  	ret = gsensor_init();
  	while(1){
		mcube_fifo_timer_handle(0);		
		am_util_delay_ms(1000); 
		static uint32_t loop_cnt;
		am_util_debug_printf("\r\nloop time: %d\r\n", loop_cnt++);
	}
#endif

#if GSENSOR_BMA_TEST_EN
	ret = bma_accel_init();
	while(1){
		ret = bma_accel_polling();
	}
#endif


  	return ret;  
}
#endif


ry_sts_t ry_gsensor_init(void)
{
	ry_sts_t ret = RY_SUCC;

	ret = gsensor_init();

	/* Control block init */

	return ret;

}

ry_sts_t ry_gensor_sleep(void)
{
	/* Enter low power mode */
	return RY_SUCC;
}

ry_sts_t ry_gsensor_wakeup(void)
{
    return RY_SUCC;
}

ry_sts_t ry_gsensor_config()
{
    return RY_SUCC;
}

ry_sts_t ry_gsensor_data_reg()
{ 
    return RY_SUCC;
}

ry_sts_t ry_gsensor_data_unreg()
{
    return RY_SUCC;
}

ry_sts_t ry_gsensor_on_data()
{
    return RY_SUCC;
}


#endif  /* RY_GSENSOR_ENABLE == TRUE */

/*****END OF FILE****/

