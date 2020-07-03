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
 * tlv.c
 *
 *  Created on: Sep 28, 2015
 *      Author: robert
 */

//----------------------------------------------------------------
#include "tlv.h"
#include "p61_data_types.h"
#if(MIFARE_OP_ENABLE == TRUE)
#ifndef COMPANION_DEVICE
#include "gki.h"
#include "OverrideLog.h"
#endif

t_TLV* tlv_newTLV(UINT16 tag, UINT16 length, const UINT8* value)
{
	t_TLV* tmp;

	tmp = (t_TLV*) GKI_os_malloc(sizeof(t_TLV));
	if(tmp != NULL)
	{
		tmp->tag = tag;
		tmp->length = length;

		tmp->value = GKI_os_malloc(length);
		memcpy(tmp->value, value, length);

		tmp->nodes = NULL;
	}

	return tmp;
}

t_TLV* tlv_newTLVWithList(UINT16 tag, UINT16 length, const UINT8* value, t_tlvList* tlvList)
{
	t_TLV* tmp;

	tmp = (t_TLV*) GKI_os_malloc(sizeof(t_TLV));
	if(tmp != NULL)
	{
		tmp->tag = tag;
		tmp->length = length;

		tmp->value = GKI_os_malloc(length);
		memcpy(tmp->value, value, length);

		tmp->nodes = tlvList;
	}

	return tmp;
}

void tlv_delete(t_TLV* tlv)
{
	if(tlv != NULL)
	{
		tlv->tag = 0;
		tlv->length = 0;
		GKI_os_free(tlv->value);
		tlvList_delete(tlv->nodes);

		GKI_os_free(tlv);
	}
}

UINT8 tlv_getTlvAsBytes(t_TLV* tlv, UINT8* outBytes, UINT16* outBytesLen)
{
	UINT8 result = FALSE;
	UINT8 *pOut;

	if(tlv != NULL)
	{
		if( *outBytesLen >= (tlv->length + 5) )
		{
			pOut = outBytes;

			if( (tlv->tag & 0xFF00) == 0x0000)
			{
				*pOut++ = tlv->tag & 0x00FF;
			}
			else
			{
				*pOut++ = (UINT8) (tlv->tag >> 8) & 0x00FF;
				*pOut++ = (UINT8) (tlv->tag & 0x00FF);
			}

			if(tlv->length < 128)
			{
				*pOut++ = (UINT8) tlv->length;
			}
			else if(tlv->length < 256)
			{
				*pOut++ = 0x81;
				*pOut++ = (UINT8) tlv->length;
			}
			else
			{
				*pOut++ = 0x82;
				*pOut++ = (UINT8) (tlv->length >> 8) & 0x00FF;
				*pOut++ = (UINT8) (tlv->length & 0x00FF);
			}

			memcpy(pOut, tlv->value, tlv->length);

			*outBytesLen = pOut - outBytes + tlv->length;

			result = TRUE;
		}
	}

	return result;
}

//----------------------------------------------------------------
// TLV List object

t_tlvList* tlvList_newList(void)
{
	t_tlvList*	tmp;

	tmp = GKI_os_malloc(sizeof(t_tlvListItem));
	tmp->head = NULL;
	tmp->tail = NULL;

	return tmp;
}

void tlvList_delete(t_tlvList* list)
{
	t_tlvListItem *tmp;
	t_tlvListItem *tmpDel;

	if(list != NULL)
	{
		// delete all the list items
		tmp = list->head;
		while(tmp != NULL)
		{
			tlv_delete(tmp->data);

			tmpDel = tmp;
			tmp = tmp->next;
			tmpDel->data = NULL;
			tmpDel->next = NULL;
			GKI_os_free(tmpDel);
		}

		// now delete the list object
		list->head = NULL;
		list->tail = NULL;
		GKI_os_free(list);
	}
}

void tlvList_add(t_tlvList *tlvList, t_TLV *tlv)
{
	t_tlvListItem* tmp;

	tmp = GKI_os_malloc(sizeof(t_tlvListItem));
	tmp->next = NULL;
	tmp->data = tlv;

	// no items in the list yet
	if(tlvList->tail == NULL)
	{
		tlvList->head = tmp;
		tlvList->tail = tmp;
	}
	// one or more items in the list already
	else
	{
		tlvList->tail->next = tmp;
		tlvList->tail = tmp;
	}
}

t_TLV* tlvList_find(t_tlvList* list, UINT16 tag)
{
	if(list != NULL)
	{
		t_tlvListItem* item = list->head;
		while(item != NULL)
		{
			if(tlv_getTag(item->data) == tag)
				return item->data;

			item = item->next;
		}
	}

	return NULL;
}

void tlvList_make(t_tlvList* list, UINT8* outData, UINT16* outDataLen)
{
	UINT8* pOut;
	UINT16 outSize;
	t_tlvListItem* item;
	UINT8 res;

	if(list != NULL)
	{
		item = list->head;

		pOut = outData;
		outSize = *outDataLen;
		while(item != NULL)
		{
			res = tlv_getTlvAsBytes(item->data, pOut, &outSize);
			if( res == TRUE )
			{
				pOut += outSize;
				outSize = *outDataLen - (pOut - outData);
			}
			else
			{
				break;
			}

			item = item->next;
		}

		*outDataLen = pOut - outData;
	}
}

t_TLV* tlvList_get(t_tlvList* list, UINT8 idx)
{
	UINT8 cnt;

	if(list != NULL)
	{
		t_tlvListItem* item = list->head;
		cnt = 0;
		for(;item != NULL; item = item->next)
		{
			if(idx == cnt)
				return item->data;
			cnt++;
		}
	}

	return NULL;
}

UINT16 tlvList_getCount(t_tlvList* list)
{
	UINT16 cnt = 0;

	if(list != NULL)
	{
		t_tlvListItem* item = list->head;
		for(;item != NULL; item = item->next)
		{
			cnt++;
		}
	}

	return cnt;
}

void tlvListItem_delete(t_tlvListItem* item)
{
    if(item != NULL)//CID10882
    {
        if(item->data)
        {
            GKI_os_free(item->data->value);
            tlvList_delete(item->data->nodes);
        }
        if(item != NULL)
        {
            GKI_os_free(item->data);
        }
        GKI_os_free(item);
    }
}

UINT16 tlv_getTag(t_TLV* tlv)
{
	if(tlv != NULL)
		return tlv->tag;

	return 0x0000;
}

void tlv_getValue(t_TLV* tlv, UINT8** value, UINT16* length)
{
	if(tlv != NULL)
	{
		*value = tlv->value;
		*length = tlv->length;
	}
	else
	{
		*value = NULL;
		*length = 0;
	}
}

t_tlvList* tlv_getNodes(t_TLV* tlv)
{
	if(tlv != NULL)
	{
		return tlv->nodes;
	}

	return NULL;
}

//----------------------------------------------------------------
// TLV Parsing
t_tlvList* tlv_parse(const UINT8* rapdu, UINT16 rapduLen)
{
	return tlv_parseWithPrimitiveTags(rapdu, rapduLen, NULL, 0);
}

t_tlvList* tlv_parseWithPrimitiveTags(const UINT8* rapdu, UINT16 rapduLen, const UINT16* primitiveTags, UINT16 primitiveTagLen)
{
	int i;
	UINT16 idx = 0;
	UINT8 isPrimitive;
	UINT16 tag;

	t_tlvList* list = tlvList_newList();

	while(idx < rapduLen)
	{

		tag = (UINT16) rapdu[idx++];

		isPrimitive = ((tag & 0x20) == 0x00);

		if((tag & 0x1F) == 0x1F)
		{
			// 2-byte tag
			tag = (tag << 8) + (rapdu[idx++]);
		}

		// check the provided list of primitive tags
		if(!isPrimitive && (primitiveTags != NULL))
		{
			for(i=0; i<primitiveTagLen; i++)
			{
				if(tag == primitiveTags[i])
				{
					isPrimitive = TRUE;
					break;
				}
			}
		}

		UINT16 length = (UINT16) rapdu[idx++];
		if(length <= 0x7F)
		{
			// 1-byte length
		}
		else if(length == 0x81)
		{
			length = rapdu[idx++];
		}
		else if(length == 0x82)
		{
			length =  (((UINT16) rapdu[idx]) << 8) + (UINT16) rapdu[idx+1];
			idx += 2;
		}
		else
		{
			tlvList_delete(list);
			return NULL;
		}

		// rapdu + idx points to the location of the beginning of the value data in the TLV

		 // length bytes don't match actual data length
		if(idx + length > rapduLen)
		{
			tlvList_delete(list);
			return NULL;
		}

		if(isPrimitive)
		{
			tlvList_add(list, tlv_newTLV(tag, length, (rapdu + idx)));
		}
		else
		{
			tlvList_add(list, tlv_newTLVWithList(tag, length, (rapdu + idx),
					tlv_parseWithPrimitiveTags((rapdu + idx), length, primitiveTags, primitiveTagLen)));
		}

		idx += length;
	}

	if(0 == tlvList_getCount(list))
	{
		tlvList_delete(list);
		list = NULL;
	}

	return list;
}

//----- DEBUGGING and TESTING -----
#ifdef TLV_DEBUG
#define MAXTABS	32
void tlv_dump(char* tabs, t_TLV* data);
void tlvList_dump(char* tabs, t_tlvList* list);

void tlvList_doDump(t_tlvList* list)
{
	char tabs[MAXTABS];

	memset(tabs, 0, sizeof(tabs));
	ALOGD("%s------------ BEGIN DUMP ------------",tabs);
	tlvList_dump(tabs, list);
	ALOGD("%s------------ END DUMP ------------",tabs);
}

void tlv_addTab(char* tabs)
{
	if(strlen(tabs) < MAXTABS)
	{
		tabs[strlen(tabs)] = '\t';
	}
}

void tlv_removeTab(char* tabs)
{
	if(strlen(tabs) < MAXTABS && strlen(tabs) > 0)
	{
		tabs[strlen(tabs)-1] = 0;
	}
}

void tlvList_dump(char* tabs, t_tlvList* list)
{
	t_tlvListItem *tmp;
	t_tlvListItem *tmpDel;
	t_TLV *data;

	if(list != NULL)
	{
		// delete all the list items
		tmp = list->head;
		while(tmp != NULL)
		{
			tlv_dump(tabs, tmp->data);

			tmp = tmp->next;
		}
	}

}

void tlv_dump(char* tabs, t_TLV* data)
{
	char label[64];

	if(data != NULL)
	{
		memset(label, 0, 64);
		if(data->nodes == NULL)
		{
			ALOGD("%sPRIMITIVE: Tag = %04X, Length = %d", tabs, data->tag, data->length);
			memcpy(label, tabs, strlen(tabs));
			memcpy(label + strlen(tabs), "Value", 5);
			if(data->length != 0)
				utils_LogByteArrayToHex(label, data->value, data->length);
			else
				ALOGD("%s --- no value ---", label);
		}
		else
		{
			ALOGD("%sTag = %04X, Length = %d", tabs, data->tag, data->length);
			memcpy(label, tabs, strlen(tabs));
			memcpy(label + strlen(tabs), "Value", 5);
			if(data->length != 0)
				utils_LogByteArrayToHex(label, data->value, data->length);
			else
				ALOGD("%s --- no value ---", label);
		}

	    if(data->nodes != NULL)
	    {
	    	tlv_addTab(tabs);
	    	tlv_addTab(tabs);
	    	tlvList_dump(tabs, data->nodes);
	    	tlv_removeTab(tabs);
	    	tlv_removeTab(tabs);
	    }
	}
}
#endif
#endif/*MIFARE_OP_ENABLE*/
