#ifndef GAMMU_MMS_DATA_H
#define GAMMU_MMS_DATA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Interpretation of First Octet
 * 0 - 30     This octet is followed by the indicated number (0 –30) of data octets
 * 31         This octet is followed by a uintvar, which indicates the number of data octets after it
 * 32 - 127   The value is a text string, terminated by a zero octet (NUL character)
 * 128 - 255  It is an encoded 7-bit value; this header has no more data
 ********/

#define MMS_MAX_PARAMS 10

typedef struct _StreamBuffer *SBUFFER;

typedef uint8_t BYTE;
typedef uint8_t *PBYTE;
typedef const uint8_t *CPTR;

typedef char *STR;
typedef const char *CSTR;

typedef uint8_t MMSShortInt;
typedef uint32_t MMSLongInt;
typedef uint32_t MMSUint;
typedef float MMSQValue;

typedef int64_t LocalTXID;

typedef enum _MMS_Version {
	MMS_VERSION_10 = 0x10,
	MMS_VERSION_12 = 0x12,
	MMS_VERSION_1X = 0x1F,
} MMS_Version;

typedef enum _MMSMessageTypeID {
	M_SEND_REQ = 128,
	M_SEND_CONF = 129,
	M_NOTIFICATION_IND = 130,
	M_NOTIFYRESP_IND = 131,
	M_RETRIEVE_CONF = 132,
	M_ACKNOWLEDGE_IND = 133,
	M_DELIVERY_IND = 134,
} MMSMessageTypeID;

typedef enum _MMSError {
	MMS_ERR_NONE,
	MMS_ERR_INVALID_DATA,
	MMS_ERR_INVALID_VALUE,
	MMS_ERR_INVALID_PARAM,
	MMS_ERR_OUTOFRANGE,
	MMS_ERR_BADLENGTH,
	MMS_ERR_NOCODEC,
	MMS_ERR_LOOKUP_FAILED,
	MMS_ERR_MEMORY,
	MMS_ERR_UNSUPPORTED,
	MMS_ERR_READ,
	MMS_ERR_REQUIRED_FIELD,
	MMS_ERR_ENC_NO_ENCODER,
	MMS_ERR_UNKNOWN_MIME,
	MMS_ERR_NOTFOUND,
	MMS_ERR_UNIMPLEMENTED,
} MMSError;
//{"ASCII",           3},
//{"UTF-8",           106},
//{"*",               128},

enum {
	NO_VALUE = 0,
	END_OF_STRING = 0,
	Q_TOKEN = 128,
	TEXT_QUOTE = 127,
	LENGTH_QUOTE = 31,
	TOK_ADDRESS_PRESENT = 128,
	TOK_ADDRESS_INSERT = 129,
	CHARSET_ANY = 128,
	CHARSET_ASCII = 3,
	CHARSET_UTF8 = 106,
	CHARSET_DEFAULT = CHARSET_UTF8,
};

typedef enum _MMSFieldKind {
	WSP_HEADER = 0x01,
	MMS_HEADER = 0x02
} MMSFieldKind;

typedef enum _MMSValueType {
	VT_NONE,
	VT_SHORT_INT,
	VT_VERSION,
	VT_LONG_INT,
	VT_LOCAL_TXID,
	VT_TEXT,
	VT_ENCODED_STRING,
	VT_CONTENT_TYPE,
	VT_ENUM,
	VT_MESSAGE_CLASS,
	VT_MESSAGE_TYPE,
	VT_PRIORITY,
	VT_RESPONSE_STATUS,
	VT_STATUS,
	VT_DTIME,
	VT_DATE,
	VT_FROM,
	VT_ADDRESS,
	VT_YESNO,
	VT_HIDESHOW,
	VT_COMPACT,
	VT_WK_CHARSET,
	VT_WK_MEDIA,
	VT_CONSTRAINED,
	VT_QVALUE,
	VT_UNSUPPORTED
} MMSValueType;


typedef struct _MMSValueEnum {
	const char *name;
	int code;
} MMSValueEnum;

typedef MMSValueEnum *MMSVALUEENUM;
typedef MMSVALUEENUM MMSCHARSET;

typedef struct _Charset {
	const char *Name;
	const int id;
} Charset;

typedef Charset *CHARSET;

typedef struct _EncodedString {
	MMSVALUEENUM charset;
	STR text;
} EncodedString;
typedef EncodedString *ENCODEDSTRING;

typedef struct _MMSFieldInfo {
	const int code;
	const char *name;
	MMSValueType vt;
} MMSFieldInfo;
typedef MMSFieldInfo *MMSFIELDINFO;

typedef struct _MMSParameter *MMSPARAMETER;

typedef struct _MMSParameters {
	int count;
	MMSPARAMETER entries;
} MMSParameters;
typedef MMSParameters *MMSPARAMETERS;

typedef struct _MMSContentType {
	MMSValueType vt;
	union {
		MMSVALUEENUM wk_media;
		STR ext_media;
	} v;
	MMSParameters params;
} MMSContentType;
typedef MMSContentType *MMSCONTENTTYPE;

typedef struct _MMSValue {
	MMSValueType type;
	bool allocated;
	union {
		PBYTE ptr;
		MMSShortInt short_int;
		MMSLongInt long_int;
		STR str;
		EncodedString encoded_string;
		MMSVALUEENUM enum_v;
		MMSContentType content_type;
		LocalTXID local_txid;
		float float_v;
	} v;
} MMSValue;
typedef MMSValue *MMSVALUE;

typedef struct _TypedParam {
	MMSFIELDINFO type;
	MMSValue value;
} TypedParam;
typedef TypedParam *TYPEDPARAM;

typedef struct _UntypedParam {
	MMSValueType value_type;
	STR token_text;
	union {
		MMSLongInt integer;
		STR text;
	} v;
} UntypedParam;
typedef UntypedParam *UNTYPEDPARAM;

typedef enum _MMSParameterKind {
	MMS_PARAM_TYPED,
	MMS_PARAM_UNTYPED,
} MMSParameterKind;

typedef struct _MMSParameter {
	MMSParameterKind kind;
	union {
		TypedParam typed;
		UntypedParam untyped;
	} v;
} MMSParameter;

typedef struct _MMSFieldID {
	MMSFieldKind kind;
	MMSFIELDINFO info;
} MMSFieldID;

typedef struct _MMSHeader {
	MMSFieldID id;
	MMSValue value;
} MMSHeader;
typedef MMSHeader *MMSHEADER;

typedef struct _MMSHeaders {
	size_t capacity;
	size_t end;
	MMSHEADER entries;
} MMSHeaders;
typedef MMSHeaders *MMSHEADERS;

// WSP 8.5 Application/vnd.wap.multipart
typedef struct _MMSPart {
	MMSHEADERS headers;
	bool allocated;
	size_t data_len;
	PBYTE data;
} MMSPart;
typedef MMSPart *MMSPART;

typedef struct _MMSParts {
	size_t capacity;
	size_t end;
	MMSPART entries;
} MMSParts;
typedef MMSParts *MMSPARTS;

typedef struct _MMSMessage {
	MMSVALUEENUM MessageType;
	STR id;
	MMSVALUE Version;
	MMSHEADERS Headers;
	MMSPARTS Parts;
} MMSMessage;
typedef MMSMessage *MMSMESSAGE;

bool IsEmptyString(CSTR str);
size_t GetTextLength(MMSCHARSET charset, CSTR text);
int MMSEncodedText_Length(ENCODEDSTRING s);
LocalTXID CreateTransactionID(void);

void MMSValue_Init(MMSVALUE v);
void MMSValue_Clear(MMSVALUE value);
MMSError MMSValue_AsString(SBUFFER stream, MMSVALUE v);
MMSError MMSValue_SetEnum(MMSVALUE out, MMSValueType vt, MMSVALUEENUM entry);
MMSError MMSValue_SetShort(MMSVALUE out, MMSShortInt value);
MMSError MMSValue_SetLocalTransactionID(MMSVALUE out, LocalTXID value);
MMSError MMSValue_CopyEncodedStr(MMSVALUE v, MMSValueType t, ENCODEDSTRING str);
MMSError MMSValue_CopyStr(MMSVALUE v, MMSValueType t, CSTR str);
MMSError MMSValue_Encode(SBUFFER stream, MMSVALUE value);

void MMSHeader_Clear(MMSHEADER header);
void MMSHeader_ShallowClone(MMSHEADER dest, MMSHEADER src);
MMSHEADER MMSHeader_FindByID(MMSHEADERS headers, MMSFieldKind type, int fieldId);
bool MMSHeader_SameHeader(MMSHEADER l, MMSHEADER r);
MMSError MMSHeader_AsString(SBUFFER stream, MMSHEADER header);

MMSHEADERS MMSHeaders_InitWithCapacity(int num_headers);
void MMSHeaders_Destroy(MMSHEADERS *headers);
MMSHEADER MMSHeaders_NewHeader(MMSHEADERS headers);
int MMSHeaders_GrowNumEntries(MMSHEADERS headers, int delta);
MMSHEADER MMSHeaders_FindByID(MMSHEADERS h, MMSFieldKind kind, int field_id);
MMSError MMSHeaders_Remove(MMSHEADERS headers, MMSHEADER header);


int MMSParts_GrowNumEntries(MMSPARTS parts, int delta);
MMSPARTS MMSParts_InitWithCapacity(int num_parts);
void MMSParts_Destroy(MMSPARTS *parts);
MMSPART MMSParts_NewPart(MMSPARTS parts);
MMSPART MMS_CreatePart(SBUFFER stream, MMSPARTS parts);

void MMSPart_Destroy(MMSPART *part);

MMSMESSAGE MMSMessage_Init(void);
void MMSMessage_Destroy(MMSMESSAGE *message);
MMSError MMSMessage_SetID(MMSMESSAGE message, CSTR id);
MMSHEADER MMSMessage_FindHeader(MMSMESSAGE m, MMSFieldKind kind, int id);
MMSHEADER MMSMessage_FindOrNewHeader(MMSMESSAGE m, MMSFieldKind kind, int id);
MMSError MMSMessage_AddPart(MMSMESSAGE m, CSTR media, CPTR data, size_t data_len);

MMSError MMSMessage_SetMessageType(MMSMESSAGE m,  MMSMessageTypeID type);
MMSError MMSMessage_SetMessageVersion(MMSMESSAGE m, MMS_Version v);
MMSError MMSMessage_SetTransactionID(MMSMESSAGE m, LocalTXID txid);
MMSError MMSMessage_SetDeliveryReport(MMSMESSAGE m, MMSVALUEENUM v);

MMSError MMSMessage_CopyAddressFrom(MMSMESSAGE m, ENCODEDSTRING from);
MMSError MMSMessage_CopyAddressTo(MMSMESSAGE m, ENCODEDSTRING to);

MMSFIELDINFO MMSFieldInfo_FindByID(MMSFieldKind kind, int id);

#endif //GAMMU_MMS_DATA_H
