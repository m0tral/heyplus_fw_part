#ifndef _SLEEPALGORITHM_H
#define _SLEEPALGORITHM_H
#include <string.h>
#include <stdint.h>

/*
typedef	unsigned char 		    uint8_t;
typedef signed char			    int8_t;
typedef unsigned short		    uint16_t;
typedef	signed short		    int16_t;
typedef unsigned long int 	    uint32_t;
typedef	signed long int		    int32_t;
*/
#define S_3AXIS 				25              	//一分钟的采样长度！


#define SAMPLE_FREQUENCE   		25          
#define SLEEP_LIST_LEN  		100

#define SLEEP_SAMP_FIFO_SIZE	100     

#ifndef ABS
#define ABS(x) 					((x)<0 ? -(x) : x)
#endif

#define ROUND(x) 				(int16_t)((x)+0.5)	//四舍五入取整宏定义

#ifndef TRUE
#define TRUE 				    1
#endif

#ifndef FALSE
#define FALSE                   0
#endif

typedef struct{
	int16_t x;
	int16_t y;
	int16_t z;
}G3Axis;

typedef struct{
	uint16_t Dur;
	int8_t   State;  		//0-Activity   1-Light sleep    2-Deep sleep    3-unwear
	int8_t   VSleep; 		// whether the sleep is valid or not! The parameter is not in use yet. 
}_SleepList;

typedef struct{
	_SleepList _sleepList[SLEEP_LIST_LEN];
	uint8_t  LNo;  			// the current list No.
	uint8_t  SFlag;			// 0->1 start sleep   1->0  end sleep
	uint16_t SDur;
	uint16_t TurnNum;
	uint16_t LSInvalid;		// last sleep is invalid!   1 -> invalid
}SleepList;


 //_SleepList _sleepList;
 //SleepList sleepList;

//int32_t	glbRNo; //test

//G3Axis SleepSampleFifo[SLEEP_SAMP_FIFO_SIZE];// <--- @_@

void SLEEP_SampWrite(int16_t x, int16_t y, int16_t z);  // <--- @_@

void InitSleepAlgorithm(void);
int16_t getSleepList(G3Axis Rac[],int16_t acLength,SleepList *sleepList,int8_t synch);
int16_t get_SleepList(SleepList *sleepList,int8_t synch);

#endif
