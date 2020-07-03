/// NOTICE:
/// 
/// 
/// (1) The function prefix with _platform_ is platform dependency.
///     You should implement platform related function
/// (2) The function suffix with _optional_ is optional function.
///     You may need not do this. Remove it form the source.
///

///----------------------------------------------------------------------------
/// Headers
/// ---------------------------------------------------------------------------
#include "mcube_sample.h"
#include "mcube_custom_config.h" 
#include "mcube_drv.h"

#include <ry_hal_inc.h>
#include "sensor_alg.h"
#include "algorithm.h"
#include "hrm.h"
#include "ry_utils.h"
#include "board.h"

#define UNUSED_PARAMETER(x) ((void)(x)) 


//#include "app_timer.h"
//#include "nrf_delay.h"


//#define TEST_TIME
#ifdef TEST_TIME 
#include "nrf_gpio.h"
#endif 
///----------------------------------------------------------------------------
/// Local variables
/// ---------------------------------------------------------------------------



static int g_bIsMC3610 = 0;
static uint8_t  sample_interval = 40; //ms
const S_M_DRV_MC_UTIL_OrientationReMap *_ptOrienMap = &g_MDrvUtilOrientationReMap[E_M_DRV_UTIL_ORIENTATION_TOP_RIGHT_UP];



#ifdef MC36XX_ACC_FIFO_POLLING
APP_TIMER_DEF(m_gsensor_timer_id);
#define GSENSOR_INTERVAL             APP_TIMER_TICKS(1000, 0)  // for 25Hz,27Hz,30Hz sample
#endif 
void mcube_fifo_timer_handle(void * p_context);


#if SUPPORT_PEDOMETER
static uint32_t pedo_time=0; //ms
static uint32_t working_step_count = 0;
static uint32_t running_step_count = 0;
static uint32_t pre_total_step = 0;
mCubeLibPedState_t cur_step_state = 0; 
uint32_t cur_total_step = 0;
static bool is_pedo_enable = false;
#endif 

#if SUPPORT_SLEEPMETER
static uint32_t sleep_time = 0;
static bool is_sleep_enable = false;
#endif 

#if SUPPORT_LIFTARM
static bool is_liftarm_enable = false;
#endif

#if SUPPORT_LONGSIT 
static bool is_longsit_enable = false;
static uint32_t longsit_tsec=0; 

APP_TIMER_DEF(m_longsit_timer_id);
#define LONGSIT_INTERVAL             APP_TIMER_TICKS(1000, 0)
static void mcube_longsit_timer_handle(void * p_context);

#endif


#if SUPPORT_PEDOMETER
void enable_pedometer(void)
{
  unsigned long version; 
  if (!is_pedo_enable)
  {
   // init pedometer data;
	pedo_time=0;
	working_step_count = 0;
	running_step_count = 0;
	cur_step_state = 0; 
	cur_total_step = 0;
	pre_total_step = 0;
	version = Ped_GetVersion(); 
	mcube_printf("Ped:ver: %x\r\n", version);
    if(!Ped_Open())
    {
       mcube_printf("Ped_Open:fail.\r\n");
    }else
    {		
       mcube_printf("Ped_Open:success!\r\n");
	}	
	is_pedo_enable = true;
	 
  }	

}

void disable_pedometer(void)
{
  if(is_pedo_enable)
  {
    Ped_Close();
	is_pedo_enable = false;

	//clear step data and stop timer while closing pedo
	pedo_time=0;
	working_step_count = 0;
	running_step_count = 0;
	cur_step_state = 0; 
	cur_total_step = 0;
	pre_total_step = 0;
  }	
}
#endif 


#if SUPPORT_SLEEPMETER

void enable_sleepmeter(void)
{
	unsigned long version; 
	if (!is_sleep_enable)
	{
	    version = mCubeSleep_get_version(); 
		mcube_printf("Sleep:ver: %x\r\n", version);
		mCubeSleep_open_with_sensitivity(SLPMTR_SENS_MEDIUM);
		if(mCubeSleep_is_opened() == 1)
		{
			is_sleep_enable = true;
			sleep_time = 0;
			mcube_printf("mCubeSleep_Open: ok! \r\n");
		}	
	}
}

void disable_sleepmeter(void)
{
    if (is_sleep_enable) {
        mCubeSleep_close(); // Close sleepmeter
        is_sleep_enable = false;
		sleep_time = 0;
    }
}
#endif 


#if SUPPORT_LIFTARM
void enable_liftarm(void)
{
	 if (!is_liftarm_enable)
	 { 
		liftarm_open();
		if(liftarm_is_open() == 1)
		{
		   is_liftarm_enable = true;
		   mcube_printf("liftarm_open: ok! \r\n");
		} 
	 }
}

void disable_liftarm(void)
{
    if (is_liftarm_enable)
	{
		liftarm_close();
		is_liftarm_enable = false;
	}
}
#endif

#if SUPPORT_LONGSIT 
static void mcube_longsit_timer_handle(void * p_context);

void enable_longsit(void)
{
	 if (!is_longsit_enable)
	 { 
		LongSit_open();
		if(LongSit_is_open() == 1)
		{
		   is_longsit_enable = true;
		   longsit_tsec = 0;
		   LongSit_set_time_threshold(30);//set timer to notify, 30minutes
		   mcube_printf("longsit_open: ok! \r\n");
		} 
	 }
}

void disable_longsit(void)
{
    if (is_longsit_enable)
	{
		LongSit_close();
		is_longsit_enable = false;
		longsit_tsec = 0;
	}
}
#endif




#if SUPPORT_SOFTTAP

void enable_softtap(int32_t type)
{
	softtap_config_t config = {0};
	if (type == SOFTTAP_TYPE_UNDIRECTIONAL) {
		config.type = SOFTTAP_TYPE_UNDIRECTIONAL;
		config.max = 3;				// the max tap count is 3. It means we support series tapping to the triple tap.
		config.sensitivity = 4;		// LSB for 0.125G, for the sensor 1G = 32 LSB.
		config.pulse_threshold.undirectional.u = 9;	// threshold is sensitivity * 9 = 4 * 9 = 36 (1.125G)
		config.pulse_limit = 3;		// time unit is the sampling time (For example, 20ms for 50 Hz. the value 3 is 60 ms) 
		config.pulse_latency = 10;
		config.pulse_window = 15;
	} 
	else if (type == SOFTTAP_TYPE_DIRECTIONAL) 
	{
	    config.type = SOFTTAP_TYPE_DIRECTIONAL;
		config.max = 3;				// the max tap count is 3. It means we support series tapping to the triple tap.
		config.sensitivity = 4;		
		config.pulse_threshold.directional.x = SOFTTAP_DISABLE_THRESHOLD;	// disable tap detection of x direction.
		config.pulse_threshold.directional.y = SOFTTAP_DISABLE_THRESHOLD;	// disable tap detection of y direction.
		config.pulse_threshold.directional.z = 10;	// threshold is sensitivity * 10 = 4 * 10 = 40 (1.25G)
		config.pulse_limit = 3;		// time unit is the sampling time (For example, 20ms for 50 Hz. the value 3 is 60 ms) 
		config.pulse_latency = 10;
		config.pulse_window = 15;
	}

	if ((type == SOFTTAP_TYPE_UNDIRECTIONAL) || (type == SOFTTAP_TYPE_DIRECTIONAL)) {
		softtap_init(&config);
	}
}

void disable_softtap(void)
{
	softtap_deinit();
}

int16_t softtap_process(int16_t x, int16_t y, int16_t z)
{
	int16_t tap_count;
	
	tap_count = softtap_detect(x, y, z);
	
	switch (tap_count) {
	case 1:
		mcube_printf("single tap\r\n");
		break;
		
	case 2:
		mcube_printf("double tap\r\n");
		break;
		
	case 3:
		mcube_printf("triple tap\r\n");
		break;
		
	default:
		//mcube_printf("error\r\n");
		break;
	}
	
	return(tap_count);
}
#endif

unsigned char reg[64];

extern void _M_DRV_MC36XX_CfgSniff(void);


int mcube_accel_init(int interrupt_mode)
{
    int ret; 
#if 0	
#ifdef MC36XX_ACC_FIFO_POLLING	
	  app_timer_create(&m_gsensor_timer_id,APP_TIMER_MODE_REPEATED,mcube_fifo_timer_handle);
#endif 	
#ifdef M_DRV_MC36XX_CFG_BUS_SPI
    nordic_spi_init();
#endif

	ret = M_DRV_MC3610_Init();

	if (ret == M_DRV_MC3610_RETCODE_SUCCESS) 
	{
		g_bIsMC3610 = 1;
        mcube_printf("Accel is MC3610.\r\n");
	} 
	else 
#endif		
	{
		ret = M_DRV_MC36XX_Init();	// Initialize MC36XX
		if (M_DRV_MC36XX_RETCODE_SUCCESS == ret)
		{
			g_bIsMC3610 = 0;
            mcube_printf("Accel is MC36xx.\r\n");
		}else
		{
			mcube_printf("Unknow accel, init fail (%d). \r\n", ret);
			return 0; 		
		}
	}
#if BUILD_MCUBE_3610
	if(	g_bIsMC3610 == 1) 
	{
		M_DRV_MC3610_SetMode(E_M_DRV_MC3610_MODE_STANDBY);  // Set MC3610 to stanby mode
		M_DRV_MC3610_ConfigRegRngResCtrl(E_M_DRV_MC3610_RANGE_4G, E_M_DRV_MC3610_RESOLUTION_12BIT);
		M_DRV_MC3610_SetSampleRate(E_M_DRV_MC3610_CWAKE_SR_LP_25Hz, E_M_DRV_MC3610_SNIFF_SR_DEFAULT_6Hz);
		sample_interval=40; //3  
		// Enable FIFO in the watermaek mode
		M_DRV_MC3610_EnableFIFO(E_M_DRV_MC3610_FIFO_CONTROL_ENABLE, E_M_DRV_MC3610_FIFO_MODE_WATERMARK, FIFO_THRESHOLD);
#ifdef MC36XX_ACC_FIFO_POLLING
		//Disable fifo interrupt		
		M_DRV_MC3610_ConfigINT(0,0,0,0,0);
#else
        // Enable the interrupt if threshold reached
        M_DRV_MC3610_ConfigINT(1,0,0,0,0);
#endif 
		M_DRV_MC3610_SetMode(E_M_DRV_MC3610_MODE_CWAKE);
	}
	else
#endif		
	{
		M_DRV_MC36XX_SetMode(E_M_DRV_MC36XX_MODE_STANDBY);  // Set MC36XX to stanby mode	
		M_DRV_MC36XX_ConfigRegRngResCtrl(E_M_DRV_MC36XX_RANGE_4G, E_M_DRV_MC36XX_RESOLUTION_12BIT);
		//Again4x 
       //M_DRV_MC36XX_ReadDoff_Dgain();
	   //M_DRV_MC36XX_ReadAofsp();
	   //M_DRV_MC36XX_SetD0ff4x_DGain4x();
	   //_M_DRV_MC36XX_AnalogGain();
	   //Again4x 
		sample_interval= 35;  
		// Enable FIFO in the watermaek mode
		M_DRV_MC36XX_SetSampleRate(E_M_DRV_MC36XX_CWAKE_SR_LOW_PR_28Hz, E_M_DRV_MC36XX_SNIFF_SR_DEFAULT_6Hz);
		M_DRV_MC36XX_EnableFIFO(E_M_DRV_MC36XX_FIFO_CONTROL_ENABLE, E_M_DRV_MC36XX_FIFO_MODE_WATERMARK, FIFO_THRESHOLD);

		if (interrupt_mode)
			M_DRV_MC36XX_ConfigINT(1,1,0,0,0,0);	//Disable fifo interrupt	
		else
			M_DRV_MC36XX_ConfigINT(0,0,0,0,0,0);	// Enable the interrupt if threshold reached

        M_DRV_MC36XX_SetMode(E_M_DRV_MC36XX_MODE_CWAKE);
	//	M_DRV_MC36XX_SetMode(E_M_DRV_MC36XX_MODE_SWAKE);
	//	M_DRV_MC36XX_ReadRegMap(reg);
	}

 #if SUPPORT_PEDOMETER    
     enable_pedometer(); 
 #endif 

 #if SUPPORT_SLEEPMETER
     enable_sleepmeter();
 #endif 
 
 #if SUPPORT_LIFTARM 
     enable_liftarm();
 #endif

 #if SUPPORT_LONGSIT 
     enable_longsit();
     app_timer_create(&m_longsit_timer_id,APP_TIMER_MODE_REPEATED,mcube_longsit_timer_handle);
	 app_timer_start(m_longsit_timer_id, LONGSIT_INTERVAL, 0);
 #endif

 #if SUPPORT_SOFTTAP
     enable_softtap(SOFTTAP_TYPE_UNDIRECTIONAL);
 #endif
 
 #ifdef TEST_TIME
	nrf_gpio_cfg_output(14);
	nrf_gpio_pin_clear(14);
 #endif 
 
 #ifdef MC36XX_ACC_FIFO_POLLING
	//start to read acc data; 
	app_timer_start(m_gsensor_timer_id, GSENSOR_INTERVAL, 0);
 #endif
  return 0;
}




/// 
/// IRQ handler for FIFO
/// 
void mcube_fifo_irq_handler(void /* or handler data */)
{
	unsigned char Wake = 0; 
	unsigned char Swake_sniff =0;
	unsigned char Fifo_threshold = 0;
#if BUILD_MCUBE_3610	
    S_M_DRV_MC3610_InterruptEvent evt_mc3610 = {0};
#endif
    S_M_DRV_MC36XX_InterruptEvent evt_mc36xx = {0};
	unsigned char buf[6];
	short x,y,z;
#if BUILD_MCUBE_3610	
	if(	g_bIsMC3610 == 1) 
	{
		M_DRV_MC3610_HandleINT(&evt_mc3610);
		Wake = evt_mc3610.bWAKE;
		Fifo_threshold = evt_mc3610.bFIFO_THRESHOLD;	  
	}
	else
#endif		
	{ 
		M_DRV_MC36XX_HandleINT(&evt_mc36xx);
		Wake = evt_mc36xx.bWAKE;
		Swake_sniff = evt_mc36xx.bSWAKE_SNIFF;
		Fifo_threshold = evt_mc36xx.bFIFO_THRESHOLD;
	} 

    if (Wake)
    {
       //interrupt for sniff;
	}
	if(Swake_sniff)
    {
     // mcube_read_regs(0x02, buf, 6);
	  //x = (short)(((unsigned short)buf[1] << 8) + (unsigned short)buf[0]);
	  //y = (short)(((unsigned short)buf[3] << 8) + (unsigned short)buf[2]);
	  //z = (short)(((unsigned short)buf[5] << 8) + (unsigned short)buf[4]);
	  //mcube_printf("Accel:  %d, %d, %d\r\n",  x, y, z);
       //interrupt for swake_sniff;
	}
	if(Fifo_threshold)
	{
		mcube_fifo_timer_handle(0);
	}
}


static AccData GensorData;
AccData* p_acc_data = &GensorData;

#if 1
	#define acc_tail cbuff_acc.tail
	#define acc_head cbuff_acc.head
	#define buff_cnt cbuff_acc.cnt
	#define acc_rd_ptr   cbuff_acc.rd_ptr
#else
	uint8_t acc_head, acc_tail;
#endif


void mcube_fifo_timer_handle(void * p_context)
{
    UNUSED_PARAMETER(p_context);
	unsigned char i, fifolength = 0;
	unsigned char buf[6];
	short x,y,z;
	static char j = 0;
	
	static int32_t  acc_sample_sync = 0;
	static uint32_t sample_sync_cnt;
	static uint32_t sample_start;
	uint16_t cbuff_head = 0;
	uint8_t tmp_head = 0;
	int32_t buffer_size = 0;
	int32_t buffer_size_hr = 0;

	for (i = 0; i < FIFO_DEEP; i ++) {
		if ((alg_get_work_mode() == ALGO_IMU) || (hrm_st_is_prepare() == 0)){
			cbuff_head = acc_head + 1;
			if (cbuff_head >= (FIFO_BUFF_DEEP << 1)) {
				cbuff_head = 0;
			}
			if (((cbuff_head - acc_tail) & ((FIFO_BUFF_DEEP << 1) - 1)) >= FIFO_BUFF_DEEP){
				acc_tail = (cbuff_head - FIFO_BUFF_DEEP) & ((FIFO_BUFF_DEEP << 1) - 1);		//max buffer size is FIFO_BUFF_DEEP
			}	
			if (((cbuff_head - acc_rd_ptr) & ((FIFO_BUFF_DEEP << 1) - 1)) >= FIFO_BUFF_DEEP){
				acc_rd_ptr = (cbuff_head - FIFO_BUFF_DEEP) & ((FIFO_BUFF_DEEP << 1) - 1);		//max buffer size is FIFO_BUFF_DEEP
			}
			tmp_head = (cbuff_head >= FIFO_BUFF_DEEP) ? (cbuff_head - FIFO_BUFF_DEEP) : cbuff_head; 
		}
		mcube_read_regs(0x08, buf, 1);
		if (buf[0] & 0x10) {
			break;
		}
		else{	 // Read out raw data
			mcube_read_regs(0x02, buf, 6);
			fifolength ++;
			sample_sync_cnt ++;
#if 0
			static float sample_rate_filt;
			static float sample_rate;
			uint32_t sample_end;
			if (sample_sync_cnt >= 25){
				sample_end = ry_hal_clock_time();		
				sample_sync_cnt = 0;
				sample_rate = 25*1000/ry_hal_calc_ms(sample_end - sample_start);
				sample_rate_filt  = (sample_rate_filt < 20) ? sample_rate : (sample_rate + sample_rate_filt * 7) / 8;
				LOG_DEBUG("sample_25 time is: %ds. sample_rate is %.1f, sample_rate_ave: %.1f \r\n\r\n", \
					ry_hal_calc_ms(sample_end -sample_start), sample_rate, sample_rate_filt);	
				sample_start = ry_hal_clock_time();
			}
#else
			//sync gsensor sample rate to 25Hz from 26 - 31Hz
			for (int sample_rate = 26; sample_rate < 31; sample_rate ++){
				int skip_point = 26 / (sample_rate - 25);
				if (sample_sync_cnt >= skip_point){
					uint32_t time = ry_hal_calc_ms(ry_hal_clock_time() - sample_start);
					if (time <= (skip_point * 1000 / sample_rate)){
						//LOG_DEBUG("data_sync_at: %dHz, skip per %d sample, deta_time %d\r\n",sample_rate, skip_point, time);	
						sample_start = ry_hal_clock_time();
						acc_sample_sync = 1;
						sample_sync_cnt = 0;
						break;
					}
				}
			}
#endif
			if ((alg_get_work_mode() == ALGO_IMU) || (hrm_st_is_prepare() == 0)){
				//calc buffer size and check sync 25Hz
				buffer_size = (acc_head - acc_rd_ptr >= 0) ? \
					(acc_head - acc_rd_ptr) : (acc_head - acc_rd_ptr + (FIFO_BUFF_DEEP << 1));

				buffer_size_hr = (cbuff_hr.head - cbuff_hr.rd_ptr >= 0) ? \
					(cbuff_hr.head - cbuff_hr.rd_ptr) : (cbuff_hr.head - cbuff_hr.rd_ptr + (FIFO_BUFF_DEEP << 1));

				if (s_alg.mode == ALGO_HRS){
					if (buffer_size - buffer_size_hr <= 0){
						acc_sample_sync = 0;				//make sure the hrm buffer is not larger than acc buffer
					}
				}

				if (acc_sample_sync > 0){
					;// mcube_printf("sample_gsensor_skip, buffer_size: %d, buffer_size_hr:%d\n\r", buffer_size, buffer_size_hr);
				}
				else{
					GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_X] = (short)(((unsigned short)buf[1] << 8) + (unsigned short)buf[0]);
					GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Y] = (short)(((unsigned short)buf[3] << 8) + (unsigned short)buf[2]);
					GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Z] = (short)(((unsigned short)buf[5] << 8) + (unsigned short)buf[4]);
				}
			}
			//mcube_printf("RAWData:  %d, %d, %d\r\n", GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_X], GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Y], GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Z]);
#ifdef M_DRV_MC36XX_SUPPORT_LPF
			_M_DRV_MC36XX_LowPassFilter(&(GensorData.RawData[tmp_head][0]));
#endif
#if SIGN_AND_REMAP			
			//sign and map
			GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_X] = _ptOrienMap->bSign[M_DRV_MC_UTL_AXIS_X] * GensorData.RawData[tmp_head][_ptOrienMap->bMap[M_DRV_MC_UTL_AXIS_X]];
			GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Y] = _ptOrienMap->bSign[M_DRV_MC_UTL_AXIS_Y] * GensorData.RawData[tmp_head][_ptOrienMap->bMap[M_DRV_MC_UTL_AXIS_Y]];
			GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Z] = _ptOrienMap->bSign[M_DRV_MC_UTL_AXIS_Z] * GensorData.RawData[tmp_head][_ptOrienMap->bMap[M_DRV_MC_UTL_AXIS_Z]];

			GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_X] = -GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_X];
			GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Y] = -GensorData.RawData[tmp_head][M_DRV_MC_UTL_AXIS_Y];
#endif
			// mcube_printf("[gsensor]Accel buffer head tail write done: acc_head: %d, acc_tail: %d\r\n", tmp_head, acc_tail);			
		}
		if (acc_sample_sync > 0){
			acc_sample_sync = 0;
		}
		else{
			acc_head = cbuff_head;
		}	
	}
	
	//static uint32_t sample_point;
	// LOG_DEBUG("sample_time_is: %d ms. sample_cnts: %d, acc_head: %d, acc_rd_ptr, acc_tail: %d.\r\n", \
		ry_hal_calc_ms(ry_hal_clock_time() - sample_point), fifolength, acc_head, acc_rd_ptr, acc_tail);	
	//sample_point = ry_hal_clock_time();	

	if (sample_sync_cnt >= 32){
		sample_sync_cnt = 0;
		sample_start = ry_hal_clock_time();
	}

	GensorData.DataLen = fifolength;
	for(i = 0; i< FIFO_BUFF_DEEP; i++){
		if (acc_head != acc_tail)
		{
			if ((alg_get_work_mode() == ALGO_IMU) || (hrm_st_is_prepare() == 0)){
				
				uint16_t cbuff_tail = acc_tail + 1;
				
				if (cbuff_tail >= (FIFO_BUFF_DEEP << 1)) {
					cbuff_tail = 0;
					// mcube_printf("\n");
				}			
				uint8_t tmptail = (cbuff_tail >= FIFO_BUFF_DEEP) ? (cbuff_tail - FIFO_BUFF_DEEP) : cbuff_tail;            
				//re-range to real accelarate
				// +/-4g, 12bit, so [((2 << 13) >> 4) = 512] 1x count is 1g; Make 1024 = 1g.
				x = GensorData.RawData[tmptail][M_DRV_MC_UTL_AXIS_X] << 1;
				y = GensorData.RawData[tmptail][M_DRV_MC_UTL_AXIS_Y] << 1;
				z = GensorData.RawData[tmptail][M_DRV_MC_UTL_AXIS_Z] << 1;

				GensorData.RawData[tmptail][M_DRV_MC_UTL_AXIS_X] = x << 2;
				GensorData.RawData[tmptail][M_DRV_MC_UTL_AXIS_Y] = y << 2;
				GensorData.RawData[tmptail][M_DRV_MC_UTL_AXIS_Z] = z << 2;

				// mcube_printf("[gsensor] ataToAlg buffer is doing, x: %d, y: %d, z: %d, acc_head: %d, acc_tail: %d, buff_cnt: %d, acc_rd_ptr: %d\n", \
						x, y, z, acc_head, tmptail, buff_cnt, acc_rd_ptr);
				acc_tail = cbuff_tail;
			}
#if SUPPORT_PEDOMETER 			
			 // Push data into pedometer library
			if (is_pedo_enable){ 
			 	// save last total step count before process data
				 pre_total_step = cur_total_step;
				 Ped_ProcessData(x, y, z);  		//25hz 

				 pedo_time += sample_interval;
				 GensorData.TimeStamp[i] = pedo_time;
				 if(cur_step_state == MCUBE_LIB_WALKING) 
				 	working_step_count += (cur_total_step - pre_total_step);

				 if(cur_step_state == MCUBE_LIB_RUNNING)
				 	running_step_count += (cur_total_step - pre_total_step);
            }
#endif 
		  
#if SUPPORT_SLEEPMETER
			if (is_sleep_enable){
				slpmtr_input_t  sleepinput;
				sleepinput.mpss[0] = (int32_t) x*9807/1000;
				sleepinput.mpss[1] = (int32_t) y*9807/1000;
				sleepinput.mpss[2] = (int32_t) z*9807/1000;
				if (j % 6 != 0) {
					++j;
				} 
				else {
					j = 1;
				    mCubeSleep_detect(&sleepinput, sleep_time);
				    sleep_time += 250;
				}
			}
#endif
		     
#if SUPPORT_LIFTARM
			 if(is_liftarm_enable) {
		     	int32_t input[3] = {0};
				 //x,y,z 1g = 1024, 1024*9.8cnt/g
				input[0] = (int32_t) x*9807/1000;
				input[1] = (int32_t) y*9807/1000;
				input[2] = (int32_t) z*9807/1000; 
				liftarm_process(input);
			}	
#endif
#if SUPPORT_SOFTTAP
		 //4g, 8bit@50hz for softtap 32cnt/g
		 //softtap_process(x*32/1024, y*32/1024,z*32/1024);
#endif  
		}
				 
    }
}

#if SUPPORT_LONGSIT 
static void mcube_longsit_timer_handle(void * p_context)
{
  UNUSED_PARAMETER(p_context);

   if(is_longsit_enable && is_pedo_enable)
   {
       uint32_t step = Ped_GetStepCount();
	  // mcube_printf("LongSit_Process:step= %d,time=%d\r\n", step, longsit_tsec);
	   LongSit_Process(step, longsit_tsec);
	   longsit_tsec ++;
   }
}	
#endif


void gsensor_handle_int(void)
{
	 mcube_fifo_irq_handler();
}

#if 0
int gsensor_init(void)
{
	ry_hal_spim_init(SPI_IDX_GSENSOR);	
	int ret = mcube_accel_init(0);
	gsensor_handle_int();
	return ret;
}
#endif


