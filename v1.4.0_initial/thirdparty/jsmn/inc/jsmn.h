#ifndef __JSMN_H_
#define __JSMN_H_
#include <stddef.h>
#include "ry_type.h"


#define JSMN_STRICT

#define JSMN_OPTIMIZE

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_INVALID = 0,
	JSMN_PRIMITIVE = 1,
	JSMN_OBJECT = 2,
	JSMN_ARRAY = 3,
	JSMN_STRING = 4
} jsmntype_t;

typedef enum {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3,
	/* Everything was fine */
	JSMN_SUCCESS = 0
} jsmnerr_t;

/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */

 #ifdef JSMN_OPTIMIZE

typedef struct {
	jsmntype_t type;
	signed char size;
	signed short start;
	signed short end;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;

#else

typedef struct {
        jsmntype_t type;
        int start;
        int end;
        int size;
#ifdef JSMN_PARENT_LINKS
        int parent;
#endif
} jsmntok_t;


#endif

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
#ifdef JSMN_OPTIMIZE
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, int js_len, jsmntok_t *tokens, unsigned int num_tokens);
#else
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens);
#endif

#endif /* __JSMN_H_ */