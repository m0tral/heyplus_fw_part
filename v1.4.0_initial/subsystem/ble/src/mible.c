/*
 *  Copyright (C) Xiaomi Ltd.
 *  All Rights Reserved.
 */



/*********************************************************************
 * INCLUDES
 */

#include "mible_arch.h"
#include "mible.h"
#include "mible_crypto.h"

#include "ry_type.h"
#include "ry_utils.h"
#include "scheduler.h"


/*********************************************************************
 * CONSTANTS
 */ 
    
#define MIBLE_EMPTY                       0xFF



#define MIBLE_WIFICFG_UID_LEN             8
#define MIBLE_WIFICFG_UTC_LEN             4
    
/*
 * @brief Definition for MAX Authenticated Devicee Number
 */
#define MAX_AUTH_CENTRAL_DEVICES          1
#define MAX_AUTH_PERIPHERAL_DEVICES       1
    
    
#define MIBLE_STATE_IDLE                  0
#define MIBLE_STATE_AUTHENGICATING        1
    
     
/*
 * @brief Definition for Mi Service state
 */
#define AUTH_STATE_IDLE                   0
#define AUTH_STATE_WAIT_AUTH_START        1
#define AUTH_STATE_WAIT_T1                2
#define AUTH_STATE_WAIT_BINDING_ACK       3 
#define AUTH_STATE_WAIT_LOGIN_ACK         4
#define AUTH_STATE_BOND                   5
#define AUTH_STATE_WAIT_SN                6
#define AUTH_STATE_WAIT_CLOUD_BINDING     7

     
/*
 * @brief Definition for Mi Service Constant
 */
//#define AUTH_START_VAL                    0xDE85CA90
//#define AUTH_ACK_VAL                      0xFA54AB92
//#define AUTH_LOGIN                        0xCD43BC00
//#define AUTH_LOGIN_CFM                    0x93BFAC09
//#define AUTH_LOGIN_CFM_ACK                0x369A58C9
//#define AUTH_BIND_SUCC                    0xAC61B130
//#define AUTH_BIND_FAIL                    0xE941BF30

#define MI_AUTH_START_VAL                   (0xDE85CA90)
#define MI_AUTH_ACK_VAL                     (0xFA54AB92)
#define MI_AUTH_LOGIN                       (0xCD43BC00)
#define MI_AUTH_LOGIN_CFM                   (0x93BFAC09)
#define MI_AUTH_LOGIN_CFM_ACK               (0x369A58C9)
#define MI_AUTH_BIND_SUCC                   (0xAC61B130)
#define MI_AUTH_BIND_FAIL                   (0xE941BF30)

#define RY_AUTH_START_VAL                   (0xDE85CA90+1)
#define RY_AUTH_ACK_VAL                     (0xFA54AB92+1)
#define RY_AUTH_LOGIN                       (0xCD43BC00+1)
#define RY_AUTH_LOGIN_CFM                   (0x93BFAC09+1)
#define RY_AUTH_LOGIN_CFM_ACK               (0x369A58C9+1)
#define RY_AUTH_BIND_SUCC                   (0xAC61B130+1)
#define RY_AUTH_BIND_FAIL                   (0xE941BF30+1)


#define RY_AUTH_TOKEN_VERIFY                (RY_AUTH_LOGIN+1)
#define RY_AUTH_TOKEN_VERIFY_CFM            (RY_AUTH_LOGIN_CFM+1)
#define RY_AUTH_TOKEN_VERIFY_CFM_ACK        (RY_AUTH_LOGIN_CFM_ACK+1)



/*
 * @brief Definition for Mi Service Events
 */
#define EVENT_AUTH_CHANGED                0x0002
#define EVENT_TOKEN_CHANGED               0x0004
#define EVENT_TERMINATE_CONNECTION        0x0008
#define EVENT_AUTH_TIMEOUT                0x0010
#define EVENT_SN_CHANGED                  0x0020
#define EVENT_WIFI_CONFIG                 0x0040
     
/*
 * @brief Definition for Authentication timerout
 */
#define MIBLE_FIRST_AUTH_TIMEOUT          10000 // 10s
#define MIBLE_AUTH_TIMEOUT                3000  // 3s
#define MIBLE_CLOUD_BINDING_TIMEOUT       7000



/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    u16 connHandle;
    u16 eventID;
    u8  data[20];
    u8  len;
} smArg_t; 

typedef void ( *mible_stateHandler_t )(u16 connHandle, u8 len, u8* data ); 
 
/** @brief  Mi Service state machine */
typedef struct
{
    u8   curState;                /*! The current state of mi service */
    u16  event;                   /*! The event ID */
    mible_stateHandler_t handler; /*! The corresponding event handler */
} mible_stateMachine_t;


typedef struct {
    u16 productID;
    u8  relation;
    u8  state;
    u8  version[MIBLE_VER_LEN];
    u8  sn[MIBLE_SN_LEN];
    u8  beaconKey[MIBLE_BEACONKEY_LEN];
    u8  token[MIBLE_TOKEN_LEN];
    u32 authTimeout;
    u32 authTick;
    u16 startHandle;
    u16 endHandle;
    u8  addr[6];
} mible_devInfo_t;

typedef struct {
    u8  addr[6];
    u8  addrType;
    u16 connHandle;
} mible_centralDev_t;


typedef struct {
    u16 connHandle;
    u16 productID;
    u8  addr[6];
    u8  addrType;
    u8  token[MIBLE_TOKEN_LEN];
    u8  beaconKey[MIBLE_BEACONKEY_LEN];
} mible_periDev_t;


typedef struct {
    mible_periDev_t    periDevs[MAX_CONNECTED_PERIPHERAL_DEVS];
    mible_centralDev_t centralDevs;
    mible_devInfo_t    info;
    mible_appCbs_t     *cbs;
    arch_timer_id_t    authTimer;
} mible_ctrl_t;

#if MIBLE_SUPPORT_CFG_WIFI

#define WIFICFG_STATE_WAIT_UUID    1
#define WIFICFG_STATE_WAIT_PW      2

typedef struct {
    u8 ssid[32];
    u8 ssidLen;
    u8 pw[64];
    u8 pwLen;
    u8 uid[8];
    u8 state;
    u8 offset;
} mible_wifiInfo_t;



#endif  /* MIBLE_SUPPORT_CFG_WIFI */


/*********************************************************************
 * LOCAL VARIABLES
 */
mible_ctrl_t mible_vs;
mible_ctrl_t ry_mible_vs;
mible_ctrl_t *mible_v = &mible_vs;
static mible_wifiSts_t wifiStatus;
static mible_wifiInfo_t *mible_wifiInfo_v = NULL;





/*
 * @brief API for get/set state
 */
#define MIBLE_STATE_SET(s)                 (mible_v->info.state = s)
#define MIBLE_STATE_GET()                  (mible_v->info.state)

#define MIBLE_GET_RELATION()               (mible_v->info.relation)



/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void mible_authHandler(u16 connHandle, u8 len, u8* data);
static void mible_t1Handler(u16 connHandle, u8 len, u8* data);
static void mible_bindingCfmHandler(u16 connHandle, u8 len, u8* data);
static void mible_loginCfmHandler(u16 connHandle, u8 len, u8* data);
static void mible_snWrittenHandler(u16 connHandle, u8 len, u8* data);
static void mible_state_machine(smArg_t* arg);
static void mible_authFailHandler(void);
#if MIBLE_SUPPORT_CFG_WIFI
static void mible_wifiCfgHandler(u16 connHandle, u8 len, u8* data);
#endif
static void mible_smtcfg_construct(u8 type, u8 leftLen, u8* data, u8 dataLen);



static const mible_stateMachine_t mible_stateMachine[] = 
{
     /*  current state,                       event,                       handler    */
    { AUTH_STATE_IDLE,                 EVENT_AUTH_CHANGED,             mible_authHandler },
    { AUTH_STATE_BOND,                 EVENT_AUTH_CHANGED,             mible_authHandler },
    { AUTH_STATE_WAIT_T1,              EVENT_TOKEN_CHANGED,            mible_t1Handler },
    { AUTH_STATE_WAIT_BINDING_ACK,     EVENT_TOKEN_CHANGED,            mible_bindingCfmHandler },
    { AUTH_STATE_WAIT_LOGIN_ACK,       EVENT_TOKEN_CHANGED,            mible_loginCfmHandler },
    { AUTH_STATE_WAIT_SN,              EVENT_SN_CHANGED,               mible_snWrittenHandler},
    { AUTH_STATE_WAIT_CLOUD_BINDING,   EVENT_AUTH_CHANGED,             mible_authHandler },
    
#if MIBLE_SUPPORT_CFG_WIFI
    { AUTH_STATE_BOND,                 EVENT_WIFI_CONFIG,              mible_wifiCfgHandler},
#endif

// To compatible with Mijia.
    { AUTH_STATE_WAIT_SN,              EVENT_AUTH_CHANGED,             mible_authHandler},
    
};



static bool isEmpty(u8* data, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (data[i] != MIBLE_EMPTY) {
            return FALSE;
        }
    }
    return TRUE;
}

static bool isValidSn(u8* sn)
{
    int i;
    for (i = 0; i < MIBLE_SN_LEN; i++) {
        if (sn[i] == 'b') {
            if (sn[i+1] == 'l' && sn[i+2] == 't') {
                return TRUE;
            } else {

                return FALSE;
            }
        }
    }
    return FALSE;
}


/**
 *@brief API to save all necessary data.
 *
 *@param None.
 *
 *@return None.
 */
static void mible_psm_save(u8_t type)
{
    u8 temp[MIBLE_PSM_LEN];

    if (type == MIBLE_TYPE_MIJIA) {

        mible_v = &mible_vs;
    } else if (type == MIBLE_TYPE_RYEEX) {
        mible_v = &ry_mible_vs;
    }

    arch_memcpy(temp, mible_v->info.token, MIBLE_TOKEN_LEN);
    arch_memcpy(temp + MIBLE_TOKEN_LEN, mible_v->info.sn, MIBLE_SN_LEN);
    arch_memcpy(temp + MIBLE_TOKEN_LEN + MIBLE_SN_LEN, mible_v->info.beaconKey, MIBLE_BEACONKEY_LEN);
    if (0 == arch_flash_write(temp, MIBLE_PSM_LEN, type)) {
        LOG_DEBUG("flash write succ\r\n");
    } else {
        LOG_WARN("flash write fail\r\n");
    }
}

void mible_psm_reset(u8_t type)
{
    if (type == MIBLE_TYPE_MIJIA) {

        mible_v = &mible_vs;
    } else if (type == MIBLE_TYPE_RYEEX) {
        mible_v = &ry_mible_vs;
    }
    
    arch_memset(mible_v->info.token, 0xFF, MIBLE_TOKEN_LEN);
    arch_memset(mible_v->info.beaconKey, 0xFF, MIBLE_BEACONKEY_LEN);
    mible_psm_save(type);
}


/**
 *@brief API to retrive all necessary data from FLASH. If retrive fail, set to EMPTY.
 *
 *@param None.
 *
 *@return None.
 */
static void mible_psm_restore(u8_t type)
{
    u8 temp[MIBLE_PSM_LEN];

    if (type == MIBLE_TYPE_MIJIA) {

        mible_v = &mible_vs;
    } else if (type == MIBLE_TYPE_RYEEX) {
        mible_v = &ry_mible_vs;
    }
    
    if (0 == arch_flash_read(temp, MIBLE_PSM_LEN, type)) {
        LOG_DEBUG("retrive succ, \n");
        ry_data_dump(temp, MIBLE_PSM_LEN, ' ');
        arch_memcpy(mible_v->info.token, temp, MIBLE_TOKEN_LEN);
        arch_memcpy(mible_v->info.sn, temp + MIBLE_TOKEN_LEN, MIBLE_SN_LEN);
        arch_memcpy(mible_v->info.beaconKey, temp + MIBLE_TOKEN_LEN + MIBLE_SN_LEN, MIBLE_BEACONKEY_LEN);
        LOG_DEBUG("retrive token, %02x %02x\n", mible_v->info.token[10], mible_v->info.token[11]);
        
        if (0 == isValidSn(mible_v->info.sn)) {
            // Not valid
            arch_memset(mible_v->info.sn, 0xFF, MIBLE_SN_LEN);
        }
        
    } else {
        arch_memset(mible_v->info.sn, 0xFF, MIBLE_SN_LEN);
        arch_memset(mible_v->info.token, 0xFF, MIBLE_TOKEN_LEN);
        arch_memset(mible_v->info.beaconKey, 0xFF, MIBLE_BEACONKEY_LEN);
        LOG_WARN("flash read error\n");
    }
}

/**
 *@brief Authentication timeout handler.
 *
 *@param[in] arg. Not used in this function.
 *
 *@return None.
 */
static void mible_authTimeoutHandler(void* arg)
{
    u8_t type;
    LOG_DEBUG("mible_authTimeoutHandler\n");
	
    if (mible_v == &mible_vs) {
		    type = MIBLE_TYPE_MIJIA;
		} else {
		    type = MIBLE_TYPE_RYEEX;
		}
    
    if (MIBLE_RELATIONSHIP_STRONG == MIBLE_GET_RELATION()){
        if (AUTH_STATE_WAIT_SN == MIBLE_STATE_GET()) {
            /* Reset TOKEN */
            arch_memset(mible_v->info.token, 0xFF, MIBLE_TOKEN_LEN);
            LOG_ERROR("SN timeout, clear token\r\n");
        }
    } else if(MIBLE_RELATIONSHIP_WEAK == MIBLE_GET_RELATION()) {
        if (AUTH_STATE_WAIT_SN == MIBLE_STATE_GET()) {
            /* For weak device, Weak device, SN  */
            LOG_WARN("Weak SN timeout.\n");
            MIBLE_STATE_SET(AUTH_STATE_BOND);
            mible_psm_save(type);
            return;
        }
    }
    
    mible_authFailHandler();
}

/**
 *@brief Authentication Failure. Notify APP layer and set the state machine back.
 *
 *@param None. 
 *
 *@return None.
 */
static void mible_authFailHandler(void)
{
    /* Set State back to IDLE */
    MIBLE_STATE_SET(AUTH_STATE_IDLE);

    //Send response to application
    if (mible_v->cbs->authCbFunc) {
        mible_v->cbs->authCbFunc((u8)MI_AUTH_BIND_FAIL);
    }
}



/**
 *@brief Initialize mi service.
 *
 *@param[in] initPara					Init parameter of mi service.
 *
 *@return Status
 */
u8 mible_init(mible_init_t *initPara, u8_t type)
{
    int i;

    if (type == MIBLE_TYPE_MIJIA) {

        mible_v = &mible_vs;
    } else if (type == MIBLE_TYPE_RYEEX) {
        mible_v = &ry_mible_vs;
    }
	
    // Retrive Token/SN/BeaconKey from flash 
    mible_psm_restore(type);

    mible_v->info.state         = AUTH_STATE_IDLE;
    mible_v->info.productID     = initPara->productID;
    mible_v->info.relation      = initPara->relationship;
    mible_v->info.authTimeout   = initPara->authTimeout;
    mible_v->cbs                = initPara->cbs;
    arch_memcpy(mible_v->info.version, initPara->version, MIBLE_VER_LEN);
    
    mible_v->centralDevs.connHandle = 0xFFFF;

    arch_ble_getOwnPublicAddr(mible_v->info.addr);
    
    MIBLE_STATE_SET(AUTH_STATE_IDLE);

    // Check if we need to generate a Beaconkey
    if (isEmpty(mible_v->info.beaconKey, MIBLE_BEACONKEY_LEN)) {
        for (i = 0; i < MIBLE_BEACONKEY_LEN; i++) {
            mible_v->info.beaconKey[i] = i;
        }
    }

    u8 temp[MIBLE_PSM_LEN];
    u8* p;
   

    // Set SN, BeaconKey to GATT
    if (isEmpty(mible_v->info.sn, MIBLE_SN_LEN)) {
        p = mible_v->info.sn;
    } else {
        mible_encrypt(mible_v->info.sn, MIBLE_SN_LEN, temp);
        p = temp;
    }
    arch_ble_setCharVal(MISERVICE_CHAR_SN_UUID, MIBLE_SN_LEN, p);

    mible_encrypt(mible_v->info.beaconKey, MIBLE_BEACONKEY_LEN, temp);
    arch_ble_setCharVal(MISERVICE_CHAR_BEACONKEY_UUID, MIBLE_BEACONKEY_LEN, temp);

    mible_encrypt(mible_v->info.version, MIBLE_VER_LEN, temp);
    arch_ble_setCharVal(MISERVICE_CHAR_VER_UUID, MIBLE_VER_LEN, temp);
	
    return MIBLE_SUCC;
}


/**
 *@brief Service registered handler.
 *
 *@param[in] startHandle		Start handle of Mi serivce
 *@param[in] endHandle                  End handle of Mi Service
 *
 *@return None
 */
void mible_onServiceRegistered(u16 startHandle, u16 endHandle)
{
    u8 temp[MIBLE_PSM_LEN];
    u8* p;
    
    mible_v->info.startHandle = startHandle;
    mible_v->info.endHandle = endHandle;

    // Set SN, BeaconKey to GATT
    if (isEmpty(mible_v->info.sn, MIBLE_SN_LEN)) {
        p = mible_v->info.sn;
    } else {
        mible_encrypt(mible_v->info.sn, MIBLE_SN_LEN, temp);
        p = temp;
    }
    arch_ble_setCharVal(MISERVICE_CHAR_SN_UUID, MIBLE_SN_LEN, p);

    mible_encrypt(mible_v->info.beaconKey, MIBLE_BEACONKEY_LEN, temp);
    arch_ble_setCharVal(MISERVICE_CHAR_BEACONKEY_UUID, MIBLE_BEACONKEY_LEN, temp);

    mible_encrypt(mible_v->info.version, MIBLE_VER_LEN, temp);
    arch_ble_setCharVal(MISERVICE_CHAR_VER_UUID, MIBLE_VER_LEN, temp);
}

/**
 *@brief Connect event handler when device is connected by some Central device.
 *
 *@param[in] connHandle					Connection Handle
 *@param[in] peerAddrType               Address type of the connected peer device
 *@param[in] peerAddr                   Address of the connected peer address
 * 
 *@return None
 */
void mible_onConnect(u16 connHandle, u8* peerAddr, u8 peerAddrType, u8 peerRole)
{
    // Add device to table
    LOG_DEBUG("[mible_onConnected]\n");

    if (peerRole == MIBLE_ROLE_CENTRAL) {
        if (mible_v->centralDevs.connHandle == 0xFFFF) {
            mible_v->centralDevs.connHandle = connHandle;
            mible_v->centralDevs.addrType = peerAddrType;
            arch_memcpy(mible_v->centralDevs.addr, peerAddr, 6);
        }
    } else {
        //TODO: Add to peripheral device table
    }

    // Start a timer to wait Authentication
    arch_timer_set(&mible_v->authTimer, mible_v->info.authTimeout, 
        TRUE, mible_authTimeoutHandler, NULL);
}


/**
 *@brief Disconnect event handler when device is loss connection.
 *
 *@param[in] connHandle					Connection Handle
 *
 *@return None
 */
void mible_onDisconnect(u16 connHandle)
{
    // Del device from table

    if (connHandle == mible_v->centralDevs.connHandle) {
        mible_v->centralDevs.connHandle = 0xFFFF;
        mible_v->centralDevs.addrType = 0xFF;
        arch_memset(mible_v->centralDevs.addr, 0xFF, 6);
    }

    MIBLE_STATE_SET(AUTH_STATE_IDLE);
}


/**
 *@brief API to start the state machine
 *
 *@param[in] connHandle                  Connection Handle
 *@param[in] event                       Event id
 *@param[in] len                         The length of data.
 *@param[in] pData                       Data to be sent to state machine
 *
 *@return None.
 */
static void mible_schedule_task(u16 connHandle, u16 event, u8 len, u8* pData)
{
    smArg_t *arg;
    
    arg = (smArg_t*)arch_buf_alloc(sizeof(smArg_t));
    if (NULL == arg) {
        LOG_ERROR("mible_schedule_task: No Mem.");
        while(1);
        //return;
    }

    arg->connHandle = connHandle;
    arg->eventID = event;
    arg->len = len;
    arch_memcpy(arg->data, pData, len); 
    
    arch_task_post((taskFunc_t)mible_state_machine, arg);
}

/**
 *@brief GATT Write handler.
 *
 *@param[in] index                      Index of characteristic
 *@param[in] len                        Length of written data.
 *@param[in] pData                      Pointer of the written data.
 *
 *@return Status, which should be used in write reponse.
 */
u8 mible_onCharWritten(u16 connHandle, mible_charIdx_t index, u8 len, u8* pData)
{
    u8 status = MIBLE_SUCC;

    LOG_DEBUG("[mible_onCharWritten] index:%d, len:%d, cur state:%d\r\n", index, len, MIBLE_STATE_GET());
    
    
    
    switch (index) {
        case MIBLE_CHAR_IDX_TOKEN:
            if (len != MIBLE_TOKEN_LEN && len != 4) {
                status = MIBLE_ERR_GATT_UNEXPECTED_LEN;
            }
            
            mible_schedule_task(connHandle, EVENT_TOKEN_CHANGED, len, pData);
            break;

        case MIBLE_CHAR_IDX_TOKEN_CCC:
        case MIBLE_CHAR_IDX_PRODUCTID:
        case MIBLE_CHAR_IDX_VER:
        case MIBLE_CHAR_MAX:
            break;

        case MIBLE_CHAR_IDX_AUTH:
            if (len != MIBLE_AUTH_LEN) {
                status = MIBLE_ERR_GATT_UNEXPECTED_LEN;
            }
            
            mible_schedule_task(connHandle, EVENT_AUTH_CHANGED, len, pData);
            break;

        case MIBLE_CHAR_IDX_SN:
            if (len != MIBLE_SN_LEN) {
                status = MIBLE_ERR_GATT_UNEXPECTED_LEN;
            }
            mible_schedule_task(connHandle, EVENT_SN_CHANGED, len, pData);
            break;

        case MIBLE_CHAR_IDX_BEACONKEY:
            break;

        case MIBLE_CHAR_IDX_WIFICFG:
#if MIBLE_SUPPORT_CFG_WIFI
            mible_schedule_task(connHandle, EVENT_WIFI_CONFIG, len, pData);
#endif
            break;

        case MIBLE_CHAR_IDX_WIFICFG_CCC:
            break;

        default:
            status = MIBLE_ERR_GATT_NOT_WRITABLE;
            break;
            
    }

    return status;
}


/**
 *@brief GATT Read handler.
 *
 *@param[in] handle                     Characteristic Handle.
 *@param[in] index                      Index of characteristic
 *@param[out] pLen                      Length of written data.
 *@param[out] pData                     Pointer of the written data.
 *
 *@return Status, which should be used in write reponse.
 */
u8 mible_onCharRead(u16 connHandle, mible_charIdx_t index, u8 *pLen, u8 *pData)
{
    return 0;
}



/**
 *@brief State machine of Mi Serivce. 
 *
 *@param[in] arg                        Argument of the state machine
 *
 *@return None.
 */
static void mible_state_machine(smArg_t* arg)
{
    int i, size;
    u16 evt = arg->eventID;

    LOG_DEBUG("[mible_state_machine] \r\n");

    size = sizeof(mible_stateMachine)/sizeof(mible_stateMachine_t);

    /* Search an appropriate event handler */
    for (i = 0; i < size; i++) {
        if (mible_stateMachine[i].curState == MIBLE_STATE_GET() &&
            mible_stateMachine[i].event == evt) {
            /* call event handler */
            mible_stateMachine[i].handler(arg->connHandle, arg->len, arg->data);
            arch_buf_free((u8*)arg);
            return;
         }
    }

    arch_buf_free((u8*)arg);
    return;
}



/**
 *@brief Handler when received characteristic AUTH data
 *
 *@param[in] connHandle                Connection Handle
 *@param[in] len                       Len of recevied data
 *@param[in] data                      Data received in Char UUID 0x0010
 *
 *@return None.
 */
static void mible_authHandler(u16 connHandle, u8 len, u8* data)
{
    u32 authData;
    u8  secTick[4];
    u8_t type;

    /* Get Authentication data */
    arch_memcpy((u8*)&authData, data, MIBLE_AUTH_LEN);

    if (MI_AUTH_START_VAL == authData || MI_AUTH_LOGIN == authData) {
        mible_v = &mible_vs;
        type = MIBLE_TYPE_MIJIA;
    } else if (RY_AUTH_START_VAL == authData || RY_AUTH_LOGIN == authData || RY_AUTH_TOKEN_VERIFY == authData) {
        mible_v = &ry_mible_vs;
        type = MIBLE_TYPE_RYEEX;
    }

    LOG_DEBUG("[mible_authHandler]: Auth handler, %x, %d\n", authData, type);
    
    /* Verify the AUTH START data */
    if (MI_AUTH_START_VAL == authData || RY_AUTH_START_VAL == authData) {        
        /* Set state to wait T1 */
        MIBLE_STATE_SET(AUTH_STATE_WAIT_T1);
    
        /* Restart timer to wait T1 */
        //arch_timer_restart(mible_v->authTimer);
        arch_timer_clear(mible_v->authTimer);
        arch_timer_set(&mible_v->authTimer, mible_v->info.authTimeout, 
            TRUE, mible_authTimeoutHandler, NULL);

        ry_sched_msg_send(IPC_MSG_TYPE_BIND_START, 0, NULL);
        
    } else if (MI_AUTH_LOGIN == authData || RY_AUTH_LOGIN == authData || RY_AUTH_TOKEN_VERIFY == authData) {
        /* Begin to wait login ACK */  
        MIBLE_STATE_SET(AUTH_STATE_WAIT_LOGIN_ACK);
        
        /* Restart timer to wait login ack */
        //arch_timer_restart(mible_v->authTimer);
        arch_timer_clear(mible_v->authTimer);
        arch_timer_set(&mible_v->authTimer, mible_v->info.authTimeout, 
            TRUE, mible_authTimeoutHandler, NULL);

        /* Generate current tick and send to Peer */
        mible_v->info.authTick = arch_random();
        LOG_DEBUG("[mible_authHandler]: Auth tick, 0x%x\n", mible_v->info.authTick);
        mible_encrypt((u8*)&mible_v->info.authTick, 4, secTick);

        /* Send notification to Peer */
        arch_ble_sendNotify(connHandle, MISERVICE_CHAR_TOKEN_UUID, 4, secTick);

    } else {
        /* The left CONST will be encrypted, we should decrypted first. */
        mible_decrypt((u8*)&authData, 4, (u8*)&authData);

        LOG_DEBUG("decrypted auth, %x\n", authData);

        
        if (MI_AUTH_BIND_SUCC == authData || RY_AUTH_BIND_SUCC == authData) {
            LOG_DEBUG("cloud bind succ\n");
            if (MIBLE_RELATIONSHIP_STRONG == MIBLE_GET_RELATION()) {
                /* For Strong relationship device, we should think that means */
                arch_timer_clear(mible_v->authTimer);
                MIBLE_STATE_SET(AUTH_STATE_BOND);
                mible_psm_save(type);
                if (mible_v->cbs->authCbFunc) {
                    mible_v->cbs->authCbFunc(MIBLE_AUTH_SUCC);
                }
                
                LOG_DEBUG("mi_service, bond succ\n");
            } else {
                /* Do nothing */
                arch_timer_clear(mible_v->authTimer);
                MIBLE_STATE_SET(AUTH_STATE_BOND);
            }
            
            
        } else if (MI_AUTH_BIND_FAIL == authData || RY_AUTH_BIND_FAIL == authData) {
            LOG_DEBUG("cloud bind fail\n");
            if (MIBLE_RELATIONSHIP_STRONG == MIBLE_GET_RELATION()) {
                /* For Strong relationship device, we should think that means authenticaton
                fail in the cloud. */
                arch_timer_clear(mible_v->authTimer);
                mible_authFailHandler();
            } else {
                /* Do nothing */
            }
        } else {
            /* Not expect data, auth fail */
            arch_timer_clear(mible_v->authTimer);
            mible_authFailHandler();
        }
    }
}


/**
 *@brief Handler when received characteristic TOKEN data
 *
 *@param[in] connHandle                Connection Handle
 *@param[in] len                       Len of recevied data
 *@param[in] data                      Data received in Char UUID 0x0001
 *
 *@return None.
 */
static void mible_t1Handler(u16 connHandle, u8 len, u8* data)
{
    u8 temp[MIBLE_TOKEN_LEN];
    u16 productId;
    int i;

    LOG_DEBUG("[mible_t1Handler]\n");

    arch_memcpy(temp, data, MIBLE_TOKEN_LEN);
    productId = mible_v->info.productID;

    LOG_DEBUG("temp: ");
    ry_data_dump(temp, MIBLE_TOKEN_LEN, ' ');

    LOG_DEBUG("addr: ");
    ry_data_dump(mible_v->info.addr, 6, ' ');
    
    /* parse the token from t1 */
    mible_mix_1(temp, NULL, mible_v->info.addr, (u8*)&productId, mible_v->info.token);
    LOG_DEBUG("token: ");
    ry_data_dump(mible_v->info.token, MIBLE_TOKEN_LEN, ' ');
    
    /* Set state to wait binding ack */
    MIBLE_STATE_SET(AUTH_STATE_WAIT_BINDING_ACK);

    /* Restart timer to wait ack */
    //arch_timer_restart(mible_v->authTimer);
    arch_timer_clear(mible_v->authTimer);
    arch_timer_set(&mible_v->authTimer, mible_v->info.authTimeout, 
            TRUE, mible_authTimeoutHandler, NULL);

    /* Generate t2 from t1 and send to peer */
    mible_mix_2(temp, NULL, mible_v->info.addr, (u8*)&productId, temp);

    LOG_DEBUG("mix2: ");
    ry_data_dump(temp, MIBLE_TOKEN_LEN, ' ');
    
    arch_ble_sendNotify(connHandle, MISERVICE_CHAR_TOKEN_UUID, MIBLE_TOKEN_LEN, temp);
}


/**
 *@brief Handler when received binding confirm data
 *
 *@param[in] connHandle                Connection Handle
 *@param[in] len                       Len of recevied data
 *@param[in] data                      Data received in Char UUID 0x0010
 *
 *@return None.
 */
static void mible_bindingCfmHandler(u16 connHandle, u8 len, u8* data)
{
    u32 value;
    u8 temp[4];
    u32 timeout;
    u8 tempGATT[MIBLE_SN_LEN];
    u8_t type;

    arch_memcpy(temp, data, 4);
    
    /* Decrypt binding ACK according to token */
    mible_decrypt(temp, 4, (u8*)&value);

    if (mible_v == &mible_vs) {
        type = MIBLE_TYPE_MIJIA;
    } else {
        type = MIBLE_TYPE_RYEEX;
    }

    LOG_DEBUG("[mible_bindingCfmHandler]\n");

    if (MI_AUTH_ACK_VAL == value || RY_AUTH_ACK_VAL == value) {

        LOG_DEBUG("[mible_bindingCfmHandler]: Auth ack value\n");
        
        /* Verify OK, enter normal */
        arch_timer_clear(mible_v->authTimer);
        MIBLE_STATE_SET(AUTH_STATE_BOND);

        // Encrypt SN with new token and set to GATT              
        mible_encrypt(mible_v->info.sn, MIBLE_SN_LEN, tempGATT);
        arch_ble_setCharVal(MISERVICE_CHAR_SN_UUID, MIBLE_SN_LEN, tempGATT);
        LOG_DEBUG("SN.%s\n", mible_v->info.sn+2);
        ry_data_dump(mible_v->info.sn, MIBLE_SN_LEN, ' ');
        
        // Encrypt BEACONKEY with new token and set to GATT              
        mible_encrypt(mible_v->info.beaconKey, MIBLE_BEACONKEY_LEN, tempGATT);
        arch_ble_setCharVal(MISERVICE_CHAR_BEACONKEY_UUID, MIBLE_BEACONKEY_LEN, tempGATT);
        LOG_DEBUG("Register succ, new token, encrypt sn, beaconkey.\n");


        mible_encrypt(mible_v->info.version, MIBLE_VER_LEN, tempGATT);
        arch_ble_setCharVal(MISERVICE_CHAR_VER_UUID, MIBLE_VER_LEN, tempGATT);

        /* According to relationship to see if it works */
        if (MIBLE_RELATIONSHIP_STRONG == MIBLE_GET_RELATION()) {
 
            /* Check if the SN is empty. If so, we should wait the SN be written
            first. If not, jump the SN waiting ,directly waiting Bonding Status */
            if (isEmpty(mible_v->info.sn, MIBLE_SN_LEN)) {
                timeout = mible_v->info.authTimeout;
                MIBLE_STATE_SET(AUTH_STATE_WAIT_SN);
                LOG_DEBUG("mi_service, waiting SN\n");
            } else {
                
                /* Already has the SN, now we should waiting for Cloud Binding. */                
                timeout = MIBLE_CLOUD_BINDING_TIMEOUT;
                MIBLE_STATE_SET(AUTH_STATE_WAIT_CLOUD_BINDING);
            }
            
            /* Restart timer to Read/Write SN */
            arch_timer_set(&mible_v->authTimer, timeout, 
                TRUE, mible_authTimeoutHandler, NULL);
        } else if(MIBLE_RELATIONSHIP_WEAK == MIBLE_GET_RELATION()) {

            /* For weak relationship device, now we think it should working now, Even
            if the SN not be written yet */
            if (isEmpty(mible_v->info.sn, MIBLE_SN_LEN)) {
                MIBLE_STATE_SET(AUTH_STATE_WAIT_SN);
                arch_timer_set(&mible_v->authTimer, mible_v->info.authTimeout, 
                    TRUE, mible_authTimeoutHandler, NULL);
                mible_psm_save(type); //  For Ryeex APP to compatible with Mijia.
            } else {
                /* Save TOKEN to flash */
                mible_psm_save(type);
            }

            LOG_DEBUG("WEAK dev bind succ\n");

            //Send response to application
            if (mible_v->cbs->authCbFunc) {
                mible_v->cbs->authCbFunc(MIBLE_AUTH_SUCC);
            }
            
        } else {
            /* Save TOKEN to flash */
			mible_psm_save(type);
			LOG_DEBUG("dev bind success\n");
			//Send response to application
            if (mible_v->cbs->authCbFunc) {
                mible_v->cbs->authCbFunc(MIBLE_AUTH_SUCC);
            }
		}
    } else {
        /* Failure */
        arch_timer_clear(mible_v->authTimer);
        mible_authFailHandler();
    }
}


/**
 *@brief Handler when received login confirm data
 *
 *@param[in] connHandle                Connection Handle
 *@param[in] len                       Len of recevied data
 *@param[in] data                      Data received in Char UUID 0x0010
 *
 *@return None.
 */
static void mible_loginCfmHandler(u16 connHandle, u8 len, u8* data)
{
    u8 newKey[MIBLE_TOKEN_LEN];
    u8 tick[4];
    u32 value;
    int i;
    u8 temp[4];
    u8_t tempSn[20];

    LOG_DEBUG("[mible_loginCfmHandler]: Enter\n");

    arch_memcpy(temp, data, 4);

    /* Generate new key */
    arch_memcpy(tick, (u8*)&mible_v->info.authTick, 4);
    arch_memcpy(newKey, mible_v->info.token, MIBLE_TOKEN_LEN);
    for (i = 0; i < 4; i++) {
        newKey[i] = mible_v->info.token[i] ^ tick[i];
    }

    /* Decrypt LOGIN ACK according to token */
    LOG_DEBUG("[mible_loginCfmHandler]: temp: ");
    ry_data_dump(temp, 4, ' ');
    LOG_DEBUG("[mible_loginCfmHandler]: newKey: ");
    ry_data_dump(newKey, 12, ' ');
    mible_internal_decrypt(temp, 4, newKey, MIBLE_TOKEN_LEN, (u8*)&value);
    LOG_DEBUG("[mible_loginCfmHandler]: decrypted value:0x%08x\n", value);

    if (MI_AUTH_LOGIN_CFM == value || RY_AUTH_LOGIN_CFM == value || RY_AUTH_TOKEN_VERIFY_CFM == value) {
        /* Verify OK, send ACK */
        if (mible_v == &mible_vs) {
            value = MI_AUTH_LOGIN_CFM_ACK;

        } else {
            if (RY_AUTH_LOGIN_CFM == value) {
                value = RY_AUTH_LOGIN_CFM_ACK;
            } else if (RY_AUTH_TOKEN_VERIFY_CFM == value) {
                value = RY_AUTH_TOKEN_VERIFY_CFM_ACK;
            }

        }
        mible_internal_encrypt((u8*)&value, 4, newKey, MIBLE_TOKEN_LEN, temp);
        arch_ble_sendNotify(connHandle, MISERVICE_CHAR_TOKEN_UUID, 4, temp);

        mible_encrypt(mible_v->info.sn, MIBLE_SN_LEN, tempSn);
        arch_ble_setCharVal(MISERVICE_CHAR_SN_UUID, MIBLE_SN_LEN, tempSn);

        mible_encrypt(mible_v->info.version, MIBLE_VER_LEN, temp);
        arch_ble_setCharVal(MISERVICE_CHAR_VER_UUID, MIBLE_VER_LEN, temp);
        

        /* Report to APP layer */       
        arch_timer_clear(mible_v->authTimer);
        MIBLE_STATE_SET(AUTH_STATE_BOND);
        if (mible_v->cbs->authCbFunc) {
            mible_v->cbs->authCbFunc(MIBLE_AUTH_SUCC);
        }
    
        LOG_DEBUG("mi_service, bond succ\n");
    } else {
        /* Failure */
        LOG_DEBUG("[mible_loginCfmHandler]: Login auth fail.\n");
        if (mible_v == &mible_vs) {
            value = MI_AUTH_LOGIN_CFM_ACK;

        } else {
            value = RY_AUTH_LOGIN_CFM_ACK;

        }
        mible_internal_encrypt((u8*)&value, 4, newKey, MIBLE_TOKEN_LEN, temp);
        arch_ble_sendNotify(connHandle, MISERVICE_CHAR_TOKEN_UUID, 4, temp);
       
        arch_timer_clear(mible_v->authTimer);
        mible_authFailHandler();
    }
}


/**
 *@brief Handler when received SN data
 *
 *@param[in] connHandle                Connection Handle
 *@param[in] len                       Len of recevied data
 *@param[in] data                      Data received in Char UUID 0x0010
 *
 *@return None.
 */
static void mible_snWrittenHandler(u16 connHandle, u8 len, u8* data)
{
    u8 i;
    u8_t type;
    
    if (mible_v == &mible_vs) {
        type = MIBLE_TYPE_MIJIA;
		} else {
        type = MIBLE_TYPE_RYEEX;
		}

    /* Get SN value from GATT database */
    arch_memcpy(mible_v->info.sn, data, MIBLE_SN_LEN);

    /* Decrypt LOGIN ACK according to token */
    mible_decrypt(mible_v->info.sn, MIBLE_SN_LEN, mible_v->info.sn);

    LOG_DEBUG("sn: ");
    ry_data_dump(mible_v->info.sn, 20, ' ');

    
    /* Save SN/TOKEN to flash */
    mible_psm_save(type);
    
    if (MIBLE_RELATIONSHIP_STRONG == MIBLE_GET_RELATION()) {
        /* Change state to wait Cloud Binding and restart timer */
        MIBLE_STATE_SET(AUTH_STATE_WAIT_CLOUD_BINDING);
        arch_timer_clear(mible_v->authTimer);
        arch_timer_set(&mible_v->authTimer, MIBLE_CLOUD_BINDING_TIMEOUT, TRUE, 
            mible_authTimeoutHandler, NULL);
    } else {
        /* We think alreay bond succ and could going on next step  */      
        MIBLE_STATE_SET(AUTH_STATE_BOND);
    }
}



/*
 * mible_encrypt - Encryption API in mi service
 *
 * @param[in]   in     - The input plain text
 * @param[in]   inLen  - The length of input plain text
 * @param[out]  out    - The generated cipher text
 * 
 * @return      None
 */
void mible_encrypt(u8* in, int inLen, u8* out)
{
    mible_internal_encrypt(in, inLen, mible_v->info.token, MIBLE_TOKEN_LEN, out);
}

/*
 * mis_decrypt - Decryption API in mi service
 *
 * @param[in]   in     - The input cipher text
 * @param[in]   inLen  - The length of input cipher text
 * @param[out]  out    - The generated plain text
 * 
 * @return      None
 */
void mible_decrypt(u8* in, int inLen, u8* out)
{
    mible_internal_decrypt(in, inLen, mible_v->info.token, MIBLE_TOKEN_LEN, out);
}


void mible_setToken(u8_t* token)
{
    arch_memcpy(ry_mible_vs.info.token, token, MIBLE_TOKEN_LEN);
}

void mible_getToken(u8_t* token)
{
    arch_memcpy(token, mible_v->info.token, MIBLE_TOKEN_LEN);
}

void mible_getSn(u8_t* sn)
{
    if ((char)mible_vs.info.sn[2] == 'b')
        arch_memcpy(sn, mible_vs.info.sn+2, MIBLE_SN_LEN-2);
    else if ((char)mible_vs.info.sn[1] == 'b')
        arch_memcpy(sn, mible_vs.info.sn+1, MIBLE_SN_LEN-1);
}

u8_t mible_isSnEmpty(void)
{
    return isEmpty(mible_vs.info.sn, MIBLE_SN_LEN);
}

void mible_transfer(void)
{
    if (mible_v == &mible_vs) {
        mible_v = &ry_mible_vs;
    } else {
        mible_v = &mible_vs;
    }
}



#if MIBLE_SUPPORT_CFG_WIFI


/*********************************************************************
 * @fn      miio_ble_smtcfg_construct
 *
 * @brief   API to construct WIFI AP infomation
 *
 * @param   id      - Which item being set
 * @param   data    - New data from Mi Service
 * @param   dataLen - The length of new data
 *
 * @return  None
 */
void mible_smtcfg_construct(u8 type, u8 leftLen, u8* data, u8 dataLen)
{
    wifiInfoCb_t wifiCfg = NULL;

    switch(type) {
        case MIBLE_WIFICFG_TYPE_UID:
            if(mible_wifiInfo_v == NULL) {
                mible_wifiInfo_v = (mible_wifiInfo_t*)arch_buf_alloc(sizeof(mible_wifiInfo_t));
                if (mible_wifiInfo_v == NULL) {
                    LOG_ERROR("mible_smtcfg_construct -- No Mem\n");
                    break;
                }
                arch_memset(mible_wifiInfo_v, 0, sizeof(mible_wifiInfo_t));
            } else {
                LOG_ERROR("mible_smtcfg_construct -- last configuration is ongoing\n");
                break;
            }
            if(leftLen == dataLen && MIBLE_WIFICFG_UID_LEN == leftLen) {
                arch_memcpy(mible_wifiInfo_v->uid, data, 8);
                LOG_DEBUG("mible_smtcfg_construct -- wifi uid is set\n");
            } else {
                LOG_ERROR("mible_smtcfg_construct -- wifi uid length not match\n");
                arch_buf_free(mible_wifiInfo_v);
            }
            break;
        case MIBLE_WIFICFG_TYPE_SSID:
            if (NULL == mible_wifiInfo_v) {
                LOG_ERROR("mible_smtcfg_construct -- wifi info not exist\n");
                break;
            }
            arch_memcpy(mible_wifiInfo_v->ssid + mible_wifiInfo_v->offset, data, dataLen);
            mible_wifiInfo_v->offset += dataLen;
            if(leftLen == dataLen) {
                mible_wifiInfo_v->ssidLen = mible_wifiInfo_v->offset;
                mible_wifiInfo_v->offset = 0;
                LOG_DEBUG("mible_smtcfg_construct -- wifi ssid is set\n");
            }
            break;
        case MIBLE_WIFICFG_TYPE_PW:
            if (NULL == mible_wifiInfo_v) {
                LOG_ERROR("mible_smtcfg_construct -- wifi info not exist\n");
                break;
            }
            arch_memcpy(mible_wifiInfo_v->pw + mible_wifiInfo_v->offset, data, dataLen);
            mible_wifiInfo_v->offset += dataLen;
            if(leftLen == dataLen) {
                mible_wifiInfo_v->pwLen = mible_wifiInfo_v->offset;
                LOG_DEBUG("mible_smtcfg_construct -- wifi password is set\n");
                wifiCfg = mible_v->cbs->wifiInfoCbFunc;
                if (wifiCfg) {
                    wifiCfg(mible_wifiInfo_v->ssid,
                            mible_wifiInfo_v->ssidLen,
                            mible_wifiInfo_v->pw,
                            mible_wifiInfo_v->pwLen,
                            mible_wifiInfo_v->uid,
                            8);
                }
            }
            break;
        case MIBLE_WIFICFG_RETRY:
            wifiCfg = mible_v->cbs->wifiInfoCbFunc;
            if (wifiCfg && mible_wifiInfo_v) {
                wifiCfg(mible_wifiInfo_v->ssid,
                        mible_wifiInfo_v->ssidLen,
                        mible_wifiInfo_v->pw,
                        mible_wifiInfo_v->pwLen,
                        mible_wifiInfo_v->uid,
                        8);
            }
            break;
        case MIBLE_WIFICFG_RESTORE:
            if(mible_wifiInfo_v) {
                arch_buf_free((u8*)mible_wifiInfo_v);
                mible_wifiInfo_v = NULL;
            }
            break;
        case MIBLE_WIFICFG_UTC:
        default:
            LOG_DEBUG("mible_smtcfg_construct -- invalid parameter\n");
    }
}





/**
 *@brief Handler when received wifi config data
 *
 *@param[in] connHandle                Connection Handle
 *@param[in] len                       Len of recevied data
 *@param[in] data                      Data received in Char UUID 0x0005
 *
 *@return None.
 */
static void mible_wifiCfgHandler(u16 connHandle, u8 len, u8* data)

{
    int i;
    
    // Format of WIFI.
    mible_decrypt(data, len, data);

    LOG_DEBUG("wifi cfg: ");
    for (i = 0; i < len; i++) {
        LOG_DEBUG("%02x ", data[i]);
    }
    LOG_DEBUG("\n");

    
    //mible_smtcfg_construct(data[0]&0x03, data, len);
    mible_smtcfg_construct(data[1], data[0], &data[2], len-2);
}

void mible_wifiStatus_set(mible_wifiSts_t new_status)
{
	if(new_status > wifiStatus) {
		wifiStatus = new_status;
	}
	if((CONN_STATE_ONLINE == wifiStatus || CONN_STATE_PW_ERROR == wifiStatus)
		&& NULL != mible_wifiInfo_v) {
		arch_buf_free((u8 *)mible_wifiInfo_v);
		mible_wifiInfo_v = NULL;
	}
}

mible_wifiSts_t mible_wifiStatus_get(void)
{
	return wifiStatus;
}


#endif  /* MIBLE_SUPPORT_CFG_WIFI */






