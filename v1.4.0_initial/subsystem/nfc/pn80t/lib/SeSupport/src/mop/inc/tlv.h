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
 * tlv.h
 *
 *  Created on: Oct 5, 2015
 *      Author: robert
 */

#ifndef TLV_H_
#define TLV_H_

#include "p61_data_types.h"

// Forward declarations:
typedef struct tlvList t_tlvList;
typedef struct tlvListItem t_tlvListItem;

// TLV object
typedef struct TLV
{
	UINT16	tag;
	UINT16	length;
	UINT8*	value;
	t_tlvList *nodes;
}t_TLV;

typedef struct tlvListItem
{
	t_TLV*			data;
	t_tlvListItem*	next;
}t_tlvListItem;

typedef struct tlvList
{
	t_tlvListItem*	head;
	t_tlvListItem*	tail;
}t_tlvList;

t_TLV* tlv_newTLV(UINT16 tag, UINT16 length, const UINT8* value);
t_TLV* tlv_newTLVWithList(UINT16 tag, UINT16 length, const UINT8* value, t_tlvList* tlvList);
void tlv_delete(t_TLV* tlv);
UINT8 tlv_getTlvAsBytes(t_TLV* tlv, UINT8* outBytes, UINT16* outBytesLen);
UINT16 tlv_getTag(t_TLV* tlv);
void tlv_getValue(t_TLV* tlv, UINT8** value, UINT16* length);
t_tlvList* tlv_getNodes(t_TLV* tlv);

void tlvListItem_delete(t_tlvListItem* item);

t_tlvList* tlvList_newList(void);
void tlvList_delete(t_tlvList* list);
void tlvList_add(t_tlvList *tlvList, t_TLV *tlv);
t_TLV* tlvList_find(t_tlvList* list, UINT16 tag);
void tlvList_make(t_tlvList* list, UINT8* outData, UINT16* outDataLen);
t_TLV* tlvList_get(t_tlvList* list, UINT8 idx);

t_tlvList* tlv_parse(const UINT8* rapdu, UINT16 rapduLen);
t_tlvList* tlv_parseWithPrimitiveTags(const UINT8* rapdu, UINT16 rapduLen, const UINT16* primitiveTags, UINT16 primitiveTagLen);

void tlvList_doDump(t_tlvList* list);

#endif /* TLV_H_ */
