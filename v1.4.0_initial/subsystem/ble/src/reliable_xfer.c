
#include "reliable_xfer.h"
#include "ry_type.h"

#include "pt.h"

#include "ryos.h"
#include "ry_utils.h"
#include "ryos_timer.h"

#include "ble_task.h"

/*********************************************************************
 * MACROS
 */

#define R_XFER_TIMEOUT             10000  // 10s

/*********************************************************************
 * CONSTANTS
 */

#define R_XFER_THREAD_PRIORITY     4
#define R_XFER_THREAD_STACK_SIZE   256 // 1K Bytes


/*********************************************************************
 * TYPEDEFS
 */

/**
 * @brief Definitaion for frame control format of the R_XFER.
 */
typedef struct {
    uint8_t mode;
    uint8_t type;
    uint8_t arg[4];
} r_xfer_fctrl_t;

/**
 * @brief Definitaion for frame format of R_XFER.
 */
typedef struct {
    uint16_t sn;
    union {
        uint8_t           data[18];
        r_xfer_fctrl_t    ctrl;
    } f;
} r_xfer_frame_t;

 
/**
 * @brief Definitaion for control block of r_xfer
 */
typedef struct {
    uint16_t        amount;
    uint16_t       curr_sn;
    uint16_t  curr_session;
    uint8_t           mode;
    uint8_t           type;
    uint8_t         *pdata;
    uint8_t     last_bytes;
    uint8_t   timeout_flag;
    uint16_t  lost_sn_list[8];
    uint8_t   n_lost_sn_list;
    uint32_t     total_len;
    r_xfer_stat_t   status;
    uint8_t          isSec;
} r_xfer_t;



struct xfer_schedule_cmd
{
    pt_t *pt;
    uint32_t fStart; /*!< Buttom GPIO pin */
};



typedef struct {
    uint16_t        amount;
    uint16_t       curr_sn;
    uint16_t  curr_session;
    uint8_t           mode;
    uint8_t           type;
    uint8_t         *pdata;
    uint8_t     last_bytes;
    uint8_t   timeout_flag;
    uint16_t  lost_sn_list[8];
    uint8_t   n_lost_sn_list;
    uint32_t     total_len;
    r_xfer_stat_t   status;
    uint8_t          isSec;
} r_xfer_session_t;


typedef struct {
    r_xfer_send_t       send;
    r_xfer_recv_cb_t    recv_cb;
    r_xfer_tx_b_t       send_cb;
} r_xfer_inst_t;




/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
static r_xfer_t         r_xfer_v;
static pt_t             pt_r_xfer, pt_r_xfer_txd;
static ryos_thread_t    *r_xfer_rxThread_handle;
static ryos_thread_t    *r_xfer_txThread_handle;
static ryos_semaphore_t *tx_sem;
static ryos_semaphore_t *rx_sem;
static ryos_semaphore_t *txd_sem;

static int              r_xfer_timer_id;
r_xfer_send_t    p_fn_xfer_send;
r_xfer_recv_cb_t p_fn_xfer_recv_cb;
r_xfer_tx_b_t    p_fn_xfer_send_cb;
void*            p_send_cb_data;




uint8_t      r_test_buf[2000];
uint8_t      r_test_tx_buf[200] = {0};



/*********************************************************************
 * LOCAL FUNCTIONS
 */
void r_xfer_schedule_start(void);
void r_xfer_schedule_stop(void);
int r_xfer_rxd_thread(pt_t *pt, r_xfer_t *p_ctx, uint8_t data_type);
void r_xfer_schedule_txd_start(void);
void r_xfer_receive_start(int pkt_num);



void r_xfer_timeout_handler(void* arg)
{
    r_xfer_v.timeout_flag = 1;
    r_xfer_v.status = R_XFER_DONE;
    LOG_DEBUG("[r_xfer]: receive timeout.\n");
}


void r_xfer_timer_start(uint32_t timeOut)
{
    ry_timer_start(r_xfer_timer_id, timeOut, r_xfer_timeout_handler, NULL);
}

void r_xfer_timer_stop(void)
{
    ry_timer_stop(r_xfer_timer_id);
}



void r_xfer_set_rxbuf(uint8_t* rxbuf)
{
    r_xfer_v.pdata = rxbuf;
}


r_xfer_sts_t r_xfer_ack(fctrl_ack_t ack, uint8_t* user_data, uint8_t len)
{
    r_xfer_frame_t  frame;
    uint16_t        data_len;

    LOG_DEBUG("[r_xfer] Send ACK. ack = %d \n", ack);
    memset((void*)&frame, 0, sizeof(frame));

    frame.f.ctrl.mode = R_XFER_MODE_ACK;
    frame.f.ctrl.type = ack;
    
    //ASSERT(len <= 4);
    if (len > 0) {
        memcpy(frame.f.ctrl.arg, user_data, len);
    }
    data_len = 4 + len; // sizeof(frame.sn) + sizeof(frame.f.ctrl.type) + sizeof(frame.f.ctrl.mode) + 4;

    p_fn_xfer_send((uint8_t*)&frame, data_len);

    return R_XFER_SUCC;         
}

#if 1
r_xfer_sts_t r_xfer_cmd(fctrl_cmd_t cmd, uint16_t amount, uint16_t session_id)
{
	r_xfer_frame_t       frame = {0};
	uint16_t             data_len;

    LOG_DEBUG("[r_xfer] Send Command. cmd = %d \n", cmd);
	frame.f.ctrl.mode   = R_XFER_MODE_CMD;
	frame.f.ctrl.type   = cmd;
    frame.f.ctrl.arg[0] = LO_UINT16(amount);
    frame.f.ctrl.arg[1] = HI_UINT16(amount);
    frame.f.ctrl.arg[2] = LO_UINT16(session_id);
    frame.f.ctrl.arg[3] = HI_UINT16(session_id);

	data_len = 8;//sizeof(frame.sn) + sizeof(frame.f.ctrl) + session_id;

    if (RY_SUCC == p_fn_xfer_send((uint8_t*)&frame, data_len)) {
        return R_XFER_SUCC;
    } else {
        return R_XFER_BUSY;
    }

	
}

r_xfer_sts_t r_xfer_data(r_xfer_t *p_ctx, uint16_t sn)
{
	r_xfer_frame_t   frame = {0};
	uint16_t         data_len;
    ry_sts_t         status;

    //LOG_DEBUG("[r_xfer] Send Data. sn = %d \n", sn);
	uint8_t (*pdata)[18] = (void*)p_ctx->pdata;
	frame.sn = sn;
	pdata   += sn - 1;

	if (sn == p_ctx->amount) {
		data_len = p_ctx->last_bytes;
	} else {
		data_len = sizeof(frame.f.data);
	}
	
	memcpy(frame.f.data, pdata, data_len);
	
	data_len += sizeof(frame.sn);

#if 0
    if (RY_SUCC == p_fn_xfer_send((uint8_t*)&frame, data_len)) {
        return R_XFER_SUCC;
    } else {
        
        LOG_DEBUG("[r_xfer_data]: block, waiting TX Ready...\r\n");
        status = ryos_semaphore_wait(txd_sem);
        if (RY_SUCC != status) {
            LOG_ERROR("[r_xfer_data]: Semaphore Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        } else {
            LOG_DEBUG("[r_xfer_data]: Continue TX.\r\n");
        }
        return R_XFER_SUCC;
    }
#endif

    if (RY_SUCC == p_fn_xfer_send((uint8_t*)&frame, data_len)) {

        LOG_DEBUG("[r_xfer_data]: block, waiting TX Ready...\r\n");
        status = ryos_semaphore_wait(txd_sem);
        if (RY_SUCC != status) {
            LOG_ERROR("[r_xfer_data]: Semaphore Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        } else {
            LOG_DEBUG("[r_xfer_data]: Continue TX.\r\n");
        }
        
        return R_XFER_SUCC;
        
    } 

}
#endif

void r_xfer_tx_recover(void)
{
    ryos_semaphore_post(txd_sem);
}


/*
 * r_xfer_on_receive - Handle recevied data by reliable xfer.
 *
 * @param   pdata     - The received data from peer device.
 * @param   len       - The length of received data.
 *
 * @return  None
 */
void r_xfer_on_receive(uint8_t* pdata, uint16_t len, uint8_t isSecure)
{
#if 1
    ry_data_dump(pdata, len, ' ');
#endif

    r_xfer_frame_t frame;
    r_xfer_frame_t *pframe = &frame;
    ry_memcpy((uint8_t*)&frame, pdata, len);
    //r_xfer_frame_t *pframe = (r_xfer_frame_t*)pdata;
    uint16_t  cur_sn = pframe->sn;
    LOG_DEBUG("[r_xfer_on_receive]: cur_sn: 0x%04x\r\n", cur_sn);
    
    if (isSecure) {
        p_fn_xfer_send = ble_reliableSend;
    } else {
        p_fn_xfer_send = ble_reliableUnsecureSend;
    }

    if (0 == cur_sn) {
        /* frame control block */
        if (pframe->f.ctrl.mode == R_XFER_MODE_CMD) {
			fctrl_cmd_t cmd = (fctrl_cmd_t)pframe->f.ctrl.type;
            r_xfer_v.isSec  = isSecure;
			r_xfer_v.mode   = R_XFER_MODE_CMD;
			r_xfer_v.type   = cmd;
            r_xfer_v.amount = *(uint16_t*)pframe->f.ctrl.arg;

            if (r_xfer_v.status == R_XFER_READY || 
                r_xfer_v.status == R_XFER_DONE) {
                //r_xfer_start();
                LOG_DEBUG("================Start RX=====================\n");
                r_xfer_receive_start(r_xfer_v.amount);
            }
            
			switch (cmd) {
				case DEV_RAW:
                    //r_xfer_v.curr_session = *((uint16_t*)(pframe->f.ctrl.arg+2));
                    LOG_DEBUG("[r_xfer]: amount: %d\n", r_xfer_v.amount);
					break;
				default:
					LOG_DEBUG("[r_xfer]: Unknown reliable CMD.\n");
			}
		}
		else {
			fctrl_ack_t ack = (fctrl_ack_t)pframe->f.ctrl.type;
            LOG_DEBUG("[r_xfer]: ack received. type = %d\n", ack);
			r_xfer_v.mode = R_XFER_MODE_ACK;
			r_xfer_v.type = ack;
			switch (ack) {
				case A_SUCCESS:
				case A_READY:
					r_xfer_v.curr_sn = 0;
					break;						
				case A_LOST:
					r_xfer_v.curr_sn = *(uint16_t*)pframe->f.ctrl.arg;
                    memcpy(r_xfer_v.lost_sn_list, pframe->f.ctrl.arg, len - 4);
                    r_xfer_v.n_lost_sn_list = (len-4) >> 1;
                    LOG_DEBUG("[r_xfer]: lost sn num:%d.\n", r_xfer_v.n_lost_sn_list);
					break;
				default:
					LOG_DEBUG("[r_xfer]: Unknown reliable ACK.\n");
			}
		}
        
    } else {
        /* data block */
        //LOG_DEBUG("[r_xfer]: data block, sn:%d\n", cur_sn);
        //LOG_DEBUG("[r_xfer]: sn:%d\n", cur_sn);

#if 0
        // Test packet loss
        static int once = 0;
        if (cur_sn == 2 && once == 0) {
            LOG_DEBUG("[r_xfer]: drop, simulate packet loss.\n");
            once = 0;
            return;
        }

        if (cur_sn == 3 && once == 0) {
            LOG_DEBUG("[r_xfer]: drop, simulate packet loss.\n");
            once = 0;
            return;
        }

        if (cur_sn == 4 && once == 0) {
            LOG_DEBUG("[r_xfer]: drop, simulate packet loss.\n");
            once = 1;
            return;
        }
#endif
        
        if (r_xfer_v.pdata != NULL) {
            memcpy(r_xfer_v.pdata + (pframe->sn - 1) * 18, pframe->f.data, len-2);
            r_xfer_v.total_len += len-2;
		}
		r_xfer_v.curr_sn = cur_sn;
    }
}


uint16_t find_lost_sn(r_xfer_t *p_ctx)
{
	static uint16_t checked_sn = 1;
	uint8_t (*p_pkg)[18] = (void*)p_ctx->pdata;

	p_pkg += checked_sn - 1;
	while (((uint16_t*)p_pkg)[0] != 0 ) {
		checked_sn++;
		p_pkg++;
	}

	if (checked_sn > p_ctx->amount) {
		checked_sn = 1;
		return 0;
	}
	else
		return checked_sn;

}

void find_lost_sn_list(r_xfer_t *p_ctx, uint16_t* list, uint8_t* p_num)
{
    int i;
    uint8_t *p = p_ctx->pdata;
    uint8_t count = 0;
    
    for (i = 0; i < p_ctx->amount; i++) {
        if ((*(p + (18*i)) == 0) && (*(p + (18*i)+1) == 0)) {
            list[count] = i+1;//*(uint16_t*)(p+18*i);
            count++;
            if (count >= 8) {
                break;
            }
        }
    }

    *p_num = count;
}


uint8_t is_lost_sn(uint16_t cur_sn, uint16_t* lost_sn_list, uint8_t n_list)
{
    int i;
    for (i = 0; i < n_list; i++) {
        if (lost_sn_list[i] == cur_sn) {
            return 1;
        }
    }

    return 0;
}



pt_t pt_resend;
uint16_t temp_last_sn;
static int pthd_resend(pt_t *pt, r_xfer_t *p_ctx)
{
	PT_BEGIN(pt);

	static  uint16_t sn;
    uint16_t sn_list[8] = {0};
    uint8_t  n_list = 0;
    int i;

	while(1) {
        #if 0
		sn = find_lost_sn(p_ctx);
        
		if (sn == 0) {
			PT_WAIT_UNTIL(pt, r_xfer_ack(A_SUCCESS, NULL, 0) == R_XFER_SUCC);
			PT_EXIT(pt);
		}
		else {
            LOG_DEBUG("lost sn: %d\n", sn);
			PT_WAIT_UNTIL(pt, r_xfer_ack(A_LOST, (uint8_t*)&sn, 2) == R_XFER_SUCC);
			PT_WAIT_UNTIL(pt, p_ctx->curr_sn == sn);
		}
        #endif

        find_lost_sn_list(p_ctx, sn_list, &n_list);

        if (n_list == 0) {
            PT_WAIT_UNTIL(pt, r_xfer_ack(A_SUCCESS, NULL, 0) == R_XFER_SUCC);
			PT_EXIT(pt);
        } else {
            LOG_DEBUG("lost sn: ");
            for (i = 0; i < n_list; i++) {
                LOG_DEBUG("%d ", sn_list[i]);
            }

            LOG_DEBUG("\n");
            
			PT_WAIT_UNTIL(pt, r_xfer_ack(A_LOST, (uint8_t*)sn_list, n_list*2) == R_XFER_SUCC);
            LOG_DEBUG("PT wait sn: %d\n", sn_list[n_list-1]);
            temp_last_sn = sn_list[n_list-1];
			PT_WAIT_UNTIL(pt, p_ctx->curr_sn == temp_last_sn);
            LOG_DEBUG("PT wait sn_list\r\n");
        }
	}

	PT_END(pt);
}



pt_t pt_send;
static int pthd_send(pt_t *pt, r_xfer_t *p_ctx)
{
	PT_BEGIN(pt);

    int i;
	static uint16_t sn;
	sn = 1;

	while(sn <= p_ctx->amount) {
		PT_WAIT_UNTIL(pt, r_xfer_data(p_ctx, sn) == R_XFER_SUCC);
		sn++;
	}

	while(p_ctx->mode == R_XFER_MODE_ACK && p_ctx->type != A_SUCCESS) {
		PT_WAIT_UNTIL(pt, p_ctx->curr_sn != 0 || p_ctx->type == A_SUCCESS);
		if (p_ctx->type == A_SUCCESS) {
			break;
		}
		else if(p_ctx->curr_sn <= p_ctx->amount) {
            for (i = 0; i < p_ctx->n_lost_sn_list; i++) {
			    PT_WAIT_UNTIL(pt, r_xfer_data(p_ctx, p_ctx->lost_sn_list[i]) == R_XFER_SUCC);
            }
		}
		p_ctx->curr_sn = 0;
	}

	PT_END(pt);
}




/*
 * r_xfer_thread      - Reliable transfer thread.
 *
 * @param   pdata     - The received data from peer device.
 * @param   len       - The length of received data.
 *
 * @return  None
 */
int r_xfer_rxd_thread(pt_t *pt, r_xfer_t *p_ctx, uint8_t data_type)
{
    
    PT_BEGIN(pt);

    /* Check if another XFER runing */
    if (p_ctx->status != R_XFER_READY &&
        p_ctx->status != R_XFER_DONE) {

        /* Send ACK  */
        //PT_WAIT_UNTIL(pt, r_xfer_ack(A_BUSY, NULL, 0) == R_XFER_SUCC);
        
        //PT_END(pt);
        LOG_DEBUG("[r_xfer: BUSY]\r\n");
    }
    
    /* Receive frame control packet */
    PT_WAIT_UNTIL(pt, p_ctx->amount != 0 && p_ctx->type == data_type);

    /* Send ACK  */
    PT_WAIT_UNTIL(pt, r_xfer_ack(A_READY, NULL, 0) == R_XFER_SUCC);

    /* Start a timer to wait XFER done */
    r_xfer_timer_start(p_ctx->amount * PACK_TIMEOUT_MS);

    /* Receive data packet */
    PT_WAIT_UNTIL(pt, (p_ctx->amount == p_ctx->curr_sn) || (p_ctx->timeout_flag == 1));

    /* Check and resend */
    PT_SPAWN(pt, &pt_resend, pthd_resend(&pt_resend, p_ctx));

    p_ctx->status = R_XFER_DONE;

    LOG_DEBUG("[rxd_thread]: PT End.\n");

    PT_END(pt);
}


#if 1
pt_t pt_r_txd_thd;

int r_xfer_txd_thread(pt_t *pt, r_xfer_t *p_ctx, uint8_t data_type)
{
	PT_BEGIN(pt);

	PT_WAIT_UNTIL(pt, r_xfer_cmd(data_type, p_ctx->amount, 0) == R_XFER_SUCC);
	PT_WAIT_UNTIL(pt, p_ctx->mode == R_XFER_MODE_ACK && p_ctx->type == A_READY);
	PT_SPAWN(pt, &pt_send, pthd_send(&pt_send, p_ctx));

	p_ctx->amount = 0;
    LOG_DEBUG("[txd_thread]: PT End.\n");
	PT_END(pt);
}
#endif


void r_xfer_test(void)
{
    int i = 0;
    for (i = 0; i < 100; i++) {
        r_test_tx_buf[i] = i;
    }
    
    r_xfer_v.pdata = r_test_tx_buf;
    r_xfer_v.amount = 6;
    r_xfer_v.last_bytes = 10;
    //r_xfer_txd_start();
    //r_xfer_txd_thread(&pt_r_txd_thd, &r_xfer_v, DEV_RAW);
}



/**
 * @brief   API to start receive procedure. 
 *
 * @param   pkt_num, the packet number to receive.
 *
 * @return  None
 */
void r_xfer_receive_start(int pkt_num)
{
    uint8_t *pRecvBuf = NULL;

    /* Assign the receive buffer */
    if (pkt_num < 220) {
        /* Normal Packet */
        pRecvBuf = ry_malloc(pkt_num*18);
        if (!pRecvBuf) {
            LOG_ERROR("[r_xfer_receive_start]: No Mem.\r\n");
            RY_ASSERT(pRecvBuf==NULL);
        }

        r_xfer_set_rxbuf(pRecvBuf);
        r_xfer_v.total_len = 0;
        r_xfer_v.curr_sn = 0;
        r_xfer_v.timeout_flag = 0;
    } else {
        /* For large data, let's save it into flash every 4K. */
        
    }
  
    /* Start to receive */
    PT_INIT(&pt_r_xfer);
    ryos_semaphore_post(rx_sem);
}


/**
 * @brief   API to start transmit procedure. 
 *
 * @param   data, the packet to be sent, it should be a global data or from ry_malloc.
 * @param   len, the length of the data.
 *
 * @return  None
 */
void r_xfer_send_start(u8_t* data, int len)
{
    int cnt = len/18;
    int last_cnt = len%18;
    r_xfer_v.pdata = data;
    if (last_cnt) {
        cnt++;
    }

    r_xfer_v.amount = cnt;
    r_xfer_v.last_bytes = last_cnt;

    PT_INIT(&pt_r_xfer_txd);
    ryos_semaphore_post(tx_sem);
    LOG_DEBUG("[r_xfer_send_start]\r\n");
}


/**
 * @brief   API to send data through reliable protocol.
 *
 * @param   data, the data to be sent
 * @param   len,  the length of data
 * @param   txCb, callback function of transmit status.
 *
 * @return  Status.
 */
void r_xfer_send(u8_t* data, int len, r_xfer_tx_b_t txCb, void* usrdata)
{
    int pkt_num  = len/18;
    int last_cnt = len%18;
    
    pkt_num = (last_cnt == 0) ? (len/18) : (len/18)+1;

    /* Setting Data */
    r_xfer_v.pdata      = data;
    r_xfer_v.amount     = pkt_num;
    r_xfer_v.last_bytes = (last_cnt == 0) ? 18:last_cnt;
    p_fn_xfer_send_cb   = txCb;
    p_send_cb_data      = usrdata;

    /* Start Transfer */
    PT_INIT(&pt_r_xfer_txd);
    ryos_semaphore_post(tx_sem);
    LOG_DEBUG("[r_xfer_send_start]\r\n");
}




/**
 * @brief   RX task of Reliable Transfer 
 *
 * @param   None
 *
 * @return  None
 */
int r_xfer_rxThread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    int rx_state = 0;
    int pt_sts;
    
    LOG_INFO("[r_xfer_rxThread]: Enter.\r\n");

    /* Create the Semaphore to Control RX start */
    status = ryos_semaphore_create(&rx_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_rxThread]: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    while (1) {

        if (rx_state == 0) {
            LOG_DEBUG("[r_xfer_rxThread]: Waiting RX Sem...\r\n");
            status = ryos_semaphore_wait(rx_sem);
            if (RY_SUCC != status) {
                LOG_ERROR("[r_xfer_rxThread]: Semaphore Wait Error.\r\n");
                RY_ASSERT(RY_SUCC == status);
            } else {
                LOG_DEBUG("[r_xfer_rxThread]: Start RX.\r\n");
            }
        }

        
        rx_state = 1;
        pt_sts = r_xfer_rxd_thread(&pt_r_xfer, &r_xfer_v, DEV_RAW);
        if (PT_ENDED == pt_sts) {
            rx_state = 0;

            /* Send receive data to the upper layer */
            p_fn_xfer_recv_cb(r_xfer_v.pdata, r_xfer_v.total_len, r_xfer_v.isSec);
        }
       
        ryos_delay_ms(10);
    }
}

/**
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int r_xfer_txThread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    int tx_state = 0;
    int pt_sts;
    
    LOG_DEBUG("[r_xfer_txThread]: Enter.\r\n");

    /* Create the Semaphore to Control RX start */
    status = ryos_semaphore_create(&tx_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_txThread]: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    /* Create the Semaphore to Control TXD process */
    status = ryos_semaphore_create(&txd_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_txThread]: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    while (1) {

       if (tx_state == 0) {
            LOG_DEBUG("[r_xfer_txThread]: Waiting TX Sem...\r\n");
            status = ryos_semaphore_wait(tx_sem);
            if (RY_SUCC != status) {
                LOG_ERROR("[r_xfer_txThread]: Semaphore Wait Error.\r\n");
                RY_ASSERT(RY_SUCC == status);
            } else {
                LOG_DEBUG("[r_xfer_txThread]: Start TX.\r\n");
            }
        }

        
        tx_state = 1;
        pt_sts = r_xfer_txd_thread(&pt_r_xfer_txd, &r_xfer_v, DEV_RAW);
        if (PT_ENDED == pt_sts) {
            tx_state = 0;
            p_fn_xfer_send_cb(RY_SUCC, p_send_cb_data);
        }
       
       
       ryos_delay_ms(10);
    }
}


/**
 * @brief   API to register Reliable Transfer function
 *
 * @param   send_func - The specified TX function
 *
 * @return  None
 */
void r_xfer_register_tx(r_xfer_send_t send_func)
{
    p_fn_xfer_send = send_func;
}


/**
 * @brief   API to init Reliable Transfer module
 *
 * @param   None
 *
 * @return  None
 */
void r_xfer_init(r_xfer_send_t send_func, r_xfer_recv_cb_t recv_cb)
{
    /* Init the Reliable_Xfer Task */
    ryos_threadPara_t para;
    ry_timer_parm timer_para;

    memset((void*)&r_xfer_v, 0, sizeof(r_xfer_t));

    p_fn_xfer_send    = send_func;
    p_fn_xfer_recv_cb = recv_cb;
    r_xfer_v.status   = R_XFER_READY;

    // For test
    //extern uint8_t r_test_buf[];
    //r_xfer_set_rxbuf(r_test_buf);
    //memset(r_test_buf, 0, 100);

    /* Create the Transfer Timer */
    timer_para.timeout_CB = r_xfer_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "r_xfer_timer";
    r_xfer_timer_id = ry_timer_create(&timer_para);

    /* Start RX Thread */
    strcpy((char*)para.name, "r_xfer_rxThread");
    para.stackDepth = 5 * 1024;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = R_XFER_THREAD_PRIORITY;
    ryos_thread_create(&r_xfer_rxThread_handle, r_xfer_rxThread, &para);

    /* Start TX Thread */
    strcpy((char*)para.name, "r_xfer_txThread");
    para.stackDepth = R_XFER_THREAD_STACK_SIZE;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = R_XFER_THREAD_PRIORITY;
    ryos_thread_create(&r_xfer_txThread_handle, r_xfer_txThread, &para);
}


#if 0
/**
 * @brief Definitaion for Reliable Transfer Module instance type
 */
typedef struct {
    ryos_thread_t*       txThread;
    ryos_semaphore_t*    txSem;
    ry_queue_t*          txQ;

    ryos_thread_t*       rxThread;
    ryos_semaphore_t*    rxSem;
    ry_queue_t*          rxQ;

    r_xfer_send_t        write;
    r_xfer_tx_b_t        write_cb;
    r_xfer_recv_cb_t     read_cb;

    int                  timer_id;
    void*                usrdata;

    uint16_t             amount;
    uint16_t             curr_sn;
    uint16_t             curr_session;
    uint8_t              mode;
    uint8_t              type;
    uint8_t              *pdata;
    uint8_t              last_bytes;
    uint8_t              timeout_flag;
    uint16_t             lost_sn_list[8];
    uint8_t              n_lost_sn_list;
    uint32_t             total_len;
    r_xfer_stat_t        status;
    
  
    uint8_t              txStatus;
    uint8_t              rxStatus;
    struct co_list       txQ;
    struct co_list       rxQ;
    gtm_sendFunc_t       send;
    gtm_transWriteFunc_t transWrite;
    gtm_transRegTxDone_t transRegTxDone;
    gtm_parser_t         parser;
    gtm_destroy_t        Destroy;
    void*                usrData;
} r_xfer_t;

#endif




