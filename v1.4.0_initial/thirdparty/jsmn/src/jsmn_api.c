
#include <stddef.h>

#include "jsmn_api.h"
#include <stdio.h>
#include <string.h>

#define TOKEN_VALID(pt) (pt != NULL \
                && ((pt)->type == JSMN_PRIMITIVE \
                        || (pt)->type == JSMN_OBJECT \
                        || (pt)->type == JSMN_ARRAY \
                        || (pt)->type == JSMN_STRING))
#define TOKEN_ISDIGIT (tk,js) ((tk)->type == JSMN_PRIMITIVE && arch_isdigit(js[(tk)->start]))

#define dec_char_value(c) ( (c)-'0' )
#define hex_char_value(c) ((u8_t)(arch_isdigit(c)? dec_char_value(c) : (((c) >='A' && (c)<= 'F')? ((c)-'A' + 10) : ((c)-'a' + 10))))


#define arch_isdigit(ch)	((unsigned)((ch) - '0') < 10)
#define arch_isalpha(ch)	((unsigned int)((ch | 0x20) - 'a') < 26u)
#define arch_isxdigit(c) (((c) >='0' && (c)<= '9')||((c) >='A' && (c)<= 'F')||((c) >='a' && (c)<= 'f'))

//isprint(0x20~0x7E)
#define arch_isprint(c) ((c) >= ' ' && (c)<= '~')
#define arch_toupper(a) ((a) >= 'a' && ((a) <= 'z') ? ((a)-('a'-'A')):(a))

size_t  arch_axtobuf_detail(const char* in, size_t in_size, u8_t* out, size_t out_size, size_t *in_len)
{
	const char *org_in = in;
	u8_t *org_out = out;

	while( arch_isxdigit((int)*in) && arch_isxdigit((int)*(in+1)) &&	//输入不硌牙
			(in  - org_in <  in_size) && (out - org_out < out_size)){	// 不超限

		*out = (0x0F & hex_char_value((int)*(in+1))) | (0xF0 & (hex_char_value((int)*in) << 4));	//转换处理

		in += 2; out += 1;	//调整指针
 	}

	if(in_len)
		*in_len = in  - org_in;

	return out - org_out;
}

u32_t arch_atoun(const char* c, size_t n)
{
	u32_t dig = 0;
	const char *org = c;

#if 0
    if(!arch_isdigit(c)){
        return 0xffffffff;
    }
#endif	
	while(arch_isdigit((int)*c) && (c-org < n) ){
		dig = dig*10 + *c - '0';
		c++;
	}
	return dig;
}

uint64_t arch_atoun64(const char* c, size_t n)
{
	uint64_t dig = 0;
	const char *org = c;

#if 0
    if(!arch_isdigit(c)){
        return 0xffffffff;
    }
#endif	
	while(arch_isdigit((int)*c) && (c-org < n) ){
		dig = dig*10 + *c - '0';
		c++;
	}
	return dig;
}



//将带有转义表示的串转化为 内存中字符串
//输入指针、长度，输出新长度
//新串头部设置$(取代")，尾部设置'\0'，空余设置为空格
/*
\a	响铃(BEL)	007
\b	退格(BS) ，将当前位置移到前一列	008
\f	换页(FF)，将当前位置移到下页开头	012
\n	换行(LF) ，将当前位置移到下一行开头	010
\r	回车(CR) ，将当前位置移到本行开头	013
\t	水平制表(HT) （跳到下一个TAB位置）	009
\v	垂直制表(VT)	011
\\	代表一个反斜线字符''\'	092
\'	代表一个单引号（撇号）字符	039
\"	代表一个双引号字符	034
\0	空字符(NULL)	000
*/
int str_unescape_to_buf(const char* src, size_t src_len, char* dst, size_t dst_size)
{
	const char *src_org;
	char *dst_org;

	src_org = src;
	dst_org = dst;

	while(src - src_org < src_len && dst - dst_org < dst_size-1){	//dst尾部需要加0

		if(*src == '\\'){	//若是转义字符
			src++;
			switch(*src){
			case 'a':*dst = 7;break;
			case 'b':*dst = 8;break;
			case 'f':*dst = 12;break;
			case 'n':*dst = 10;break;
			case 'r':*dst = 13;break;
			case 't':*dst = 9;break;
			case 'v':*dst = 11;break;
			case '\\':*dst = 92;break;
			case '\'':*dst = 39;break;
			case '"':*dst = 34;break;
			case '0':	*dst = 0;
				//("invalid char(\'\\0\') in string.");
				break;
			}
			dst++;
			src++;
		}
		else		//若非转义字符
			*dst++ = *src++;
	};
	*dst = '\0';
	return dst - dst_org;
}


//用于跳到下一个同层次的节点(从key跳到下一个key 或者 array单元跳到下一个)
jsmntok_t* jsmn_next(jsmntok_t* t)
{
	int range = 1;
	while(range > 0 && TOKEN_VALID(t)){
		range += t->size;
		t++; range--;
	}
	if(TOKEN_VALID(t))
		return t;
	else
		return NULL;
}

jsmntok_t* jsmn_key_value_force(const char* js, jsmntok_t* tokens, const char* key)
{
	jsmntok_t* t = tokens;
	const char * test = NULL;
	int i = 1;
	
	if(t->type != JSMN_OBJECT)
		return NULL;

	t += 1;
	while(TOKEN_VALID(t)){
		test = js + t->start;
		if(0 == strncmp(key, js + t->start, t->end - t->start))
			return t+1;
		//t = jsmn_next(t+1);

		while(TOKEN_VALID(t)){
			if(t[i].type == JSMN_INVALID){
				return NULL;
			}else if(t[i].type == JSMN_STRING){
				t = &t[i];
				break;
			}

			i++;
		}

	}
	return NULL;
}

//输入原始串 、解析数组 和 需要定位的key
//仅仅扫描当前层面的名值对
jsmntok_t* jsmn_key_value(const char* js, jsmntok_t* tokens, const char* key)
{
	jsmntok_t* t = tokens;
	int pairs = t->size /2;
	const char * test = NULL;
	//若不在object中 或者空obj则报错
	if(t->type != JSMN_OBJECT || pairs == 0)
		return NULL;

	t += 1;
	while(TOKEN_VALID(t) && pairs > 0){
		test = js + t->start;
		if(0 == strncmp(key, js + t->start, t->end - t->start))
			return t+1;
		t = jsmn_next(t+1);

		pairs --;
	}
	return NULL;
}

//输入array开头解析数组 和 idx
jsmntok_t* jsmn_array_value(const char* js, jsmntok_t* tokens, u32_t idx)
{
	jsmntok_t* t = tokens;
	u32_t num = t->size;

	//若不是array 或者空 则返回null
	if(t->type != JSMN_ARRAY || num == 0 || idx >= num)
		return NULL;

	t += 1;	//跳过外框

    while(idx--)
    {
        t = jsmn_next(t);
    }

    return t;
}



int jsmn_tkn2val_uint64(const char *js ,jsmntok_t * tk, uint64_t *val)
{
	if(NULL == tk || tk->type != JSMN_PRIMITIVE || !arch_isdigit(js[tk->start]))
		return STATE_NOTFOUND;
	*val = arch_atoun64( (const char *)&(js[tk->start]), tk->end - tk->start);
	return STATE_OK;
}


//通过tkn取得 u32_t
// tk 需要指向 整个对象
int jsmn_tkn2val_uint(const char* js, jsmntok_t* tk, u32_t* val)
{
	if(NULL == tk || tk->type != JSMN_PRIMITIVE || !arch_isdigit(js[tk->start]))
		return STATE_NOTFOUND;
	*val = arch_atoun( (const char *)&(js[tk->start]), tk->end - tk->start);
	return STATE_OK;
}


int jsmn_tkn2val_u16(const char* js, jsmntok_t* tk, u16_t* val)
{
	int ret;
	u32_t tmp;
	ret = jsmn_tkn2val_uint(js, tk, &tmp);
	if(STATE_OK == ret)*val = tmp;
	return ret;
}

int jsmn_tkn2val_u8(const char* js, jsmntok_t* tk, u8_t* val)
{
	int ret;
	u32_t tmp;
	ret = jsmn_tkn2val_uint(js, tk, &tmp);
	if(STATE_OK == ret)*val = tmp;
	return ret;
}



int jsmn_tkn2val_str(const char* js, jsmntok_t* tk, char* str, size_t str_size, size_t *str_len)
{
	if(NULL == tk || tk->type != JSMN_STRING)return STATE_NOTFOUND;
	size_t rlen = str_unescape_to_buf(&(js[tk->start]), tk->end - tk->start, str, str_size);
	if(str_len)*str_len = rlen;
	return STATE_OK;
}

int jsmn_tkn2val_xbuf(const char* js, jsmntok_t* tk, u8_t* buf, size_t buf_size, size_t *buf_len)
{
	if(NULL == tk || tk->type != JSMN_STRING)return STATE_NOTFOUND;
	size_t rlen =  arch_axtobuf_detail(&(js[tk->start]), tk->end - tk->start, buf, buf_size, NULL);
	if(buf_len)*buf_len = rlen;
	return STATE_OK;
}

int jsmn_tkn2val_bool(const char* js, jsmntok_t* tk, u8_t* val)
{
	if(NULL == tk || tk->type != JSMN_PRIMITIVE)return STATE_NOTFOUND;
	if(js[tk->start] == 't')*val = TRUE;
	else if(js[tk->start] == 'f')*val = FALSE;
	else return STATE_NOTFOUND;
	return STATE_OK;
}

int jsmn_key2val_uint64(const char *js ,jsmntok_t* tokens, const char* key, uint64_t* val)
{
    return jsmn_tkn2val_uint64(js, jsmn_key_value(js, tokens, key), val);
}

//通过key取得 u32_t
// tokens 需要指向 整个对象
int jsmn_key2val_uint(const char* js, jsmntok_t* tokens, const char* key, u32_t* val)
{
	return jsmn_tkn2val_uint(js, jsmn_key_value(js, tokens, key), val);
}

int jsmn_key2val_u16(const char* js, jsmntok_t* tokens, const char* key, u16_t* val)
{
	return jsmn_tkn2val_u16(js, jsmn_key_value(js, tokens, key), val);
}

int jsmn_key2val_u8(const char* js, jsmntok_t* tokens, const char* key, u8_t* val)
{
	return jsmn_tkn2val_u8(js, jsmn_key_value(js, tokens, key), val);
}


int jsmn_key2val_str(const char* js, jsmntok_t* tokens, const char* key, char* str, size_t str_size, size_t *str_len)
{
	return jsmn_tkn2val_str(js, jsmn_key_value(js, tokens, key), str, str_size, str_len);
}

int jsmn_key2val_xbuf(const char* js, jsmntok_t* tokens, const char* key, u8_t* buf, size_t buf_size, size_t *buf_len)
{
	return jsmn_tkn2val_xbuf(js, jsmn_key_value(js, tokens, key), buf, buf_size, buf_len);
}

int jsmn_key2val_bool(const char* js, jsmntok_t* tokens, const char* key, u8_t* val)
{
	return jsmn_tkn2val_bool(js, jsmn_key_value(js, tokens, key), val);
}


