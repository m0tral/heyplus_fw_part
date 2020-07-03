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


#include "mijia_service.h"
#include "MiScene.pb.h"
#include "rbp.pb.h"
#include "rbp.h"
#include "nanopb_common.h"
#include "ryos.h"
#include "ry_ble.h"
#include "ryos_timer.h"
#include "ry_hal_flash.h"
#include "scheduler.h"
#include "ry_hal_spi_flash.h"



/*********************************************************************
 * CONSTANTS
 */ 

#define FLASH_ADDR_MIJIA_SCENE_TABLE     0xFD200

#define IS_SCENE_EMPTY(p)            (p->scene_id == 0xFFFFFFFF)



/*********************************************************************
 * TYPEDEFS
 */



/*
 * @brief Time service control block
 */
typedef struct {
    mijia_scene_tbl_t *sceneTbl;
} mijia_ctx_t;

 
/*********************************************************************
 * LOCAL VARIABLES
 */

mijia_ctx_t mijia_ctx_v;


/*********************************************************************
 * LOCAL FUNCTIONS
 */


ry_sts_t mijia_scene_tbl_alloc(void)
{
    if (!mijia_ctx_v.sceneTbl) {

        mijia_ctx_v.sceneTbl = (mijia_scene_tbl_t*)ry_malloc(sizeof(mijia_scene_tbl_t));
        if (mijia_ctx_v.sceneTbl == NULL) {

            return RY_SET_STS_VAL(RY_CID_MIJIA, RY_ERR_NO_MEM);
        }
    }

    return RY_SUCC;
}


ry_sts_t scene_tbl_reset(void)
{
    ry_sts_t status;

    if(mijia_ctx_v.sceneTbl == NULL){
        
        return RY_SUCC;
    }

    ry_memset((u8_t*)mijia_ctx_v.sceneTbl, 0xFF, sizeof(mijia_scene_tbl_t));
    mijia_ctx_v.sceneTbl->curNum = 0;
    
    for (int i =0; i < MAX_MIJIA_SCENE_NUM; i++) {
        mijia_ctx_v.sceneTbl->scenes[i].launchs_count = 0;
        mijia_ctx_v.sceneTbl->scenes[i].actions_count = 0;              
    }

    //status = ry_hal_flash_write(FLASH_ADDR_MIJIA_SCENE_TABLE, (u8_t*)&mijia_ctx_v.sceneTbl, sizeof(mijia_scene_tbl_t));

    ry_hal_spi_flash_sector_erase(MIJIA_SCENE_SECTOR * EXFLASH_SECTOR_SIZE);
    ry_hal_spi_flash_write((u8_t*)&mijia_ctx_v.sceneTbl, MIJIA_SCENE_SECTOR * EXFLASH_SECTOR_SIZE, sizeof(mijia_scene_tbl_t));
    
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_MIJIA, RY_ERR_PARA_SAVE);
    }
    return RY_SUCC;
}


ry_sts_t scene_tbl_save(void)
{
    ry_sts_t status;

    if(mijia_ctx_v.sceneTbl == NULL){
        
        return RY_SUCC;
    }

    //status = ry_hal_flash_write(FLASH_ADDR_MIJIA_SCENE_TABLE, (u8_t*)&mijia_ctx_v.sceneTbl, sizeof(mijia_scene_tbl_t));
    ry_hal_spi_flash_sector_erase(MIJIA_SCENE_SECTOR * EXFLASH_SECTOR_SIZE);
        
    ry_hal_spi_flash_write((u8_t*)mijia_ctx_v.sceneTbl, MIJIA_SCENE_SECTOR * EXFLASH_SECTOR_SIZE, sizeof(mijia_scene_tbl_t));
    
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_MIJIA, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}


ry_sts_t scene_tbl_restore(void)
{
    //ry_hal_flash_read(FLASH_ADDR_MIJIA_SCENE_TABLE, (u8_t*)&mijia_ctx_v.sceneTbl, sizeof(mijia_scene_tbl_t));
    ry_sts_t status;
    status = mijia_scene_tbl_alloc();
    if (status != RY_SUCC) {
        RY_ASSERT(0);
    }

    if (mijia_ctx_v.sceneTbl == NULL) {
        return RY_SUCC;
    }

    ry_hal_spi_flash_read( (u8_t*)mijia_ctx_v.sceneTbl, MIJIA_SCENE_SECTOR * EXFLASH_SECTOR_SIZE, sizeof(mijia_scene_tbl_t));

    if (mijia_ctx_v.sceneTbl->curNum <= 0 || mijia_ctx_v.sceneTbl->curNum > MAX_MIJIA_SCENE_NUM) {
        // Using default
        ry_memset((u8_t*)mijia_ctx_v.sceneTbl, 0xFF, sizeof(mijia_scene_tbl_t));
        mijia_ctx_v.sceneTbl->curNum = 0;

        for (int i =0; i < MAX_MIJIA_SCENE_NUM; i++) {
            mijia_ctx_v.sceneTbl->scenes[i].launchs_count = 0;
            mijia_ctx_v.sceneTbl->scenes[i].actions_count = 0;              
        }
    }

    return RY_SUCC;
}

MiSceneInfo* scene_search(int id)
{
    u8_t key[4] = {0};
    ry_memcpy(key, (u8_t*)&id, 4);
    return (MiSceneInfo*)ry_tbl_search(MAX_MIJIA_SCENE_NUM, 
                            sizeof(MiSceneInfo),
                            (u8_t*)&mijia_ctx_v.sceneTbl->scenes,
                            0,
                            4,
                            key);
}


ry_sts_t scene_add(int id, char* mid, char* name, u16_t launchCnt, MiSceneLaunchInfo* launch, u16_t actionCnt, MiSceneActionInfo* action)
{
    MiSceneInfo *newScene = (MiSceneInfo*)ry_malloc(sizeof(MiSceneInfo));
    if (newScene == NULL)
        return RY_SET_STS_VAL(RY_CID_MIJIA, RY_ERR_NO_MEM);
    
    ry_sts_t status;
    int midLen = strlen(mid);
    int nameLen = strlen(name);

    if (midLen > MAX_MIJIA_MID_LEN) {
        ry_free((u8_t*)newScene);
        return RY_SET_STS_VAL(RY_CID_MIJIA, RY_ERR_INVALIC_PARA);
    }

    if (nameLen > MAX_MIJIA_SCENE_NAME_LEN) {
        ry_free((u8_t*)newScene);
        return RY_SET_STS_VAL(RY_CID_MIJIA, RY_ERR_INVALIC_PARA);
    }

    newScene->scene_id = id;
    ry_memset(newScene->name, 0, MAX_MIJIA_SCENE_NAME_LEN);
    ry_memset(newScene->mid, 0, MAX_MIJIA_MID_LEN);
    strcpy(newScene->name, name);
    strcpy(newScene->mid, mid);
    newScene->launchs_count = launchCnt;
    ry_memcpy(newScene->launchs, launch, sizeof(MiSceneLaunchInfo)*launchCnt);

    if (actionCnt <= 1) {
        newScene->actions_count = actionCnt;
        ry_memcpy(newScene->actions, action, sizeof(MiSceneActionInfo)*actionCnt);
    }
   
    status = ry_tbl_add((u8_t*)mijia_ctx_v.sceneTbl, 
                 MAX_MIJIA_SCENE_NUM,
                 sizeof(MiSceneInfo),
                 (u8_t*)newScene,
                 4, 
                 0);
								 
	ry_free((u8_t*)newScene);


    if (status == RY_SUCC) {
        LOG_DEBUG("Mijia Scene add success.\r\n");
    } else {
        LOG_WARN("Mijia Scene add fail, error code = %x\r\n", status);
    }

    return status;
}

ry_sts_t scene_del(int id)
{
    ry_sts_t ret;
    u8_t key[4]= {0};
    
    ry_memcpy(key, (u8_t*)&id, 4);
    ret = ry_tbl_del((u8*)mijia_ctx_v.sceneTbl,
                MAX_MIJIA_SCENE_NUM,
                sizeof(MiSceneInfo),
                key,
                4,
                0);


    if (ret != RY_SUCC) {
        LOG_WARN("Mijia Scene del fail, error code = 0x02%x\r\n", ret);
    }

    return ret;
}



void mijia_service_init(void)
{
    scene_tbl_restore();
}


void mijia_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    } else {
        LOG_DEBUG("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    }
}




void mijia_scene_get_list(u8_t* data, int len)
{
    int i, j;
    int status;
    ry_sts_t ret;
    MiSceneList * result = (MiSceneList *)ry_malloc(sizeof(MiSceneList));
    if (!result) {
        LOG_ERROR("[MiSceneList] No mem.\r\n");
        RY_ASSERT(0);
    }

    ry_memset((u8_t*)result, 0, sizeof(MiSceneList));
    
    /* Encode result */
    int num = 0;
    MiSceneInfo* p;
    result->mi_scene_infos_count = mijia_ctx_v.sceneTbl->curNum;
    for (i = 0; num < result->mi_scene_infos_count; i++) {
        p = &mijia_ctx_v.sceneTbl->scenes[i];
        if (IS_SCENE_EMPTY(p)) {
            continue;
        }
        
        result->mi_scene_infos[num].scene_id = p->scene_id;
        strcpy(result->mi_scene_infos[num].mid, p->mid);
        strcpy(result->mi_scene_infos[num].name, p->name);

        num++;
    }
    

    u8_t* buf = ry_malloc(sizeof(MiSceneList) + 20);

    size_t message_length;
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(MiSceneList) + 20);

    status = pb_encode(&stream, MiSceneList_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[mijia_scene_get_list]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        ret = ble_send_response(CMD_DEV_MI_SCENE_GET_LIST, RBP_RESP_CODE_ENCODE_FAIL, buf, message_length, 1,
                mijia_ble_send_callback, (void*)__FUNCTION__);
        goto exit;
    }

    ret = ble_send_response(CMD_DEV_MI_SCENE_GET_LIST, RBP_RESP_CODE_SUCCESS, buf, message_length, 1,
                mijia_ble_send_callback, (void*)__FUNCTION__);

    if (ret != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ret);
    }
    

exit:
    ry_free(buf);
    ry_free((u8_t*)result);
}



void mijia_scene_add(u8_t* data, int len)
{
    int status;
    MiSceneInfo *p;
    MiSceneInfo * item = (MiSceneInfo *)ry_malloc(sizeof(MiSceneInfo));
    ry_sts_t ry_sts = RY_SUCC;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    
    status = pb_decode(&stream_i, MiSceneInfo_fields, item);
    if(!status){
        LOG_ERROR("[%s] decoding failed.\r\n", __FUNCTION__);
    }

    /* Check if the scene is already exist */
    p = scene_search(item->scene_id);
    if (p) {
        /* Exist, update value */
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_ALREADY_EXIST, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
    } else {
        /* Not exist, Add a new */
        ry_sts = scene_add(item->scene_id, item->mid, item->name, item->launchs_count, item->launchs, item->actions_count, item->actions);
    }

    ry_free((u8_t*)item);

    if (ry_sts == RY_SUCC) {
        scene_tbl_save();
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
    } else {
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, (u32_t)ry_sts, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
    }


    if (ry_sts != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }
}


void mijia_scene_add_batch(u8_t* data, int len)
{
    int status;
    MiSceneInfo *p;
    MiSceneList * list = (MiSceneList *)ry_malloc(sizeof(MiSceneList));
    ry_sts_t ry_sts = RY_SUCC;
    int i;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    
    status = pb_decode(&stream_i, MiSceneList_fields, list);
    if(!status){
        ry_free((u8_t*)list);
        LOG_ERROR("[%s] decoding failed.\r\n", __FUNCTION__);
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
        return;
    }

    for (i = 0; i < list->mi_scene_infos_count; i++) {
        p = scene_search(list->mi_scene_infos[i].scene_id);
        if (p == NULL) {
            /* Not exist, Add a new */
            ry_sts = scene_add(list->mi_scene_infos[i].scene_id, list->mi_scene_infos[i].mid, list->mi_scene_infos[i].name,
                        list->mi_scene_infos[i].launchs_count, list->mi_scene_infos[i].launchs,
                        list->mi_scene_infos[i].actions_count, list->mi_scene_infos[i].actions);
        }
        
    }

    ry_free((u8_t*)list);

    scene_tbl_save();
    ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);


    if (ry_sts != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }
}


void mijia_scene_modify(u8_t* data, int len)
{
    int status;
    MiSceneInfo *p;
    MiSceneInfo * item = (MiSceneInfo *)ry_malloc(sizeof(MiSceneInfo));
    ry_sts_t ry_sts = RY_SUCC;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    
    status = pb_decode(&stream_i, MiSceneInfo_fields, item);
    if(!status){
        LOG_ERROR("[%s] decoding failed.\r\n", __FUNCTION__);
    }

    /* Check if the scene is already exist */
    p = scene_search(item->scene_id);
    if (p == NULL) {
        /* Exist, update value */
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_NOT_FOUND, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);

        ry_free((u8_t*)item);
        return;
    } 

    if (strlen(item->mid) > MAX_MIJIA_MID_LEN || strlen(item->name) > MAX_MIJIA_SCENE_NAME_LEN) {
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_NOT_FOUND, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
        ry_free((u8_t*)item);
        return;
    }

    ry_memset(p->mid, 0, MAX_MIJIA_MID_LEN);
    ry_memset(p->name, 0, MAX_MIJIA_SCENE_NAME_LEN);
    strcpy(p->mid, item->mid);
    strcpy(p->name, item->name);

    ry_free((u8_t*)item);

    ry_sts = ble_send_response(CMD_DEV_MI_SCENE_ADD, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
}


void mijia_scene_delete(u8_t* data, int len)
{
    int status,i ,j;
    ry_sts_t ry_sts = RY_SUCC;
    MiSceneDeleteParam delete_item = {0};

    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    
    status = pb_decode(&stream_i, MiSceneDeleteParam_fields, &delete_item);
    if(!status){
        LOG_ERROR("[%s] decoding failed.\r\n", __FUNCTION__);
    }

    ry_sts = scene_del(delete_item.scene_id);

    if (ry_sts == RY_SUCC) {
        scene_tbl_save();
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
    } else {
        ry_sts = ble_send_response(CMD_DEV_MI_SCENE_DELETE, (u32_t)ry_sts, NULL, 0, 1,
                    mijia_ble_send_callback, (void*)__FUNCTION__);
    }

    if (ry_sts != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }
}


void mijia_scene_delete_all(u8_t* data, int len)
{
    scene_tbl_reset();
    ble_send_response(CMD_DEV_MI_SCENE_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
            mijia_ble_send_callback, (void*)__FUNCTION__);
    
}


mijia_scene_tbl_t* mijia_scene_get_table(void)
{
#if DEV_MIJIA_DEBUG
    mijia_ctx_v.sceneTbl = (mijia_scene_tbl_t*)ry_malloc(sizeof(mijia_scene_tbl_t));
    mijia_ctx_v.sceneTbl->curNum = 1;
    mijia_ctx_v.sceneTbl->scenes[0].scene_id = 1;
    strcpy(mijia_ctx_v.sceneTbl->scenes[0].name, "Test scene number one");    
#endif    
    return mijia_ctx_v.sceneTbl;
}


bool is_mijia_sleep_scene_exist(u8_t type)
{
    MiSceneInfo* pScene;
    int hex_cnt;
    u8_t hex[10];

    if(mijia_ctx_v.sceneTbl == NULL){
        
        return FALSE;
    }

    for (int i =0; i < MAX_MIJIA_SCENE_NUM; i++) {
        pScene = &mijia_ctx_v.sceneTbl->scenes[i];

        for (int j = 0; j < pScene->launchs_count; j++) {
        
            /* Check if the lanch condition is for our product. */
            if (0 != strcmp(pScene->launchs[j].model, MIJIA_MODEL_SAKE)) {
                continue;
            }

            if (strcmp(pScene->launchs[j].event_str, MIJIA_EVT_STR_SLEEP) == 0) {
                hex_cnt = str2hex(pScene->launchs[j].event_value, strlen(pScene->launchs[j].event_value), hex);
                if (hex[0] == type) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
    
}


bool is_sleep_scene(MiSceneInfo* p)
{
    for (int i = 0; i < p->launchs_count; i++) {
        
        /* Check if the lanch condition is for our product. */
        if (0 != strcmp(p->launchs[i].model, MIJIA_MODEL_SAKE)) {
            continue;
        }

        if (strcmp(p->launchs[i].event_str, MIJIA_EVT_STR_SLEEP) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}



