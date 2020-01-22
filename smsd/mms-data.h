#ifndef GAMMU_MMS_DATA_H
#define GAMMU_MMS_DATA_H

#include <stddef.h>
#include <stdint.h>

#define MMS_MAX_PARAMS 10

typedef struct _StreamBuffer *SBUFFER;

typedef uint8_t BYTE;
typedef uint8_t *PBYTE;
typedef const uint8_t *CPTR;

typedef char *STR;
typedef const char *CSTR;

typedef uint8_t MMSShortInt;
typedef uint32_t MMSLongInt;

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
	MMS_ERR_UNIMPLEMENTED,
} MMSError;

enum {
	NO_VALUE = 0,
	END_OF_STRING = 0,
	Q_TOKEN = 128,
	TOK_ADDRESS_PRESENT = 128,
	TOK_ADDRESS_INSERT = 129,
	ANY_CHARSET = 128,
	TEXT_QUOTE = 127,
};

typedef enum _MMSFieldKind {
	WSP_HEADER = 0x01,
	MMS_HEADER = 0x02
} MMSFieldKind;

typedef enum _MMSValueType {
	VT_NONE,
	VT_LONG_INT,
	VT_SHORT_INT,
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
	VT_FROM,
	VT_YESNO,
	VT_HIDESHOW,
	VT_COMPACT,
	VT_WK_CHARSET,
	VT_WK_MEDIA,
	VT_CONSTRAINED,
	VT_QVALUE,
	VT_UNSUPPORTED
} MMSValueType;

enum WSPFieldID {
	WSP_ACCEPT = 0x00,
	WSP_ACCEPT_CHARSET = 0x01,
	WSP_ACCEPT_ENCODING = 0x02,
	WSP_ACCEPT_LANGUAGE = 0x03,
	WSP_ACCEPT_RANGES = 0x04,
	WSP_AGE = 0x05,
	WSP_ALLOW = 0x06,
	WSP_AUTHORIZATION = 0x07,
	WSP_CACHE_CONTROL = 0x08,
	WSP_CONNECTION = 0x09,
	WSP_CONTENT_BASE = 0x0A,
	WSP_CONTENT_ENCODING = 0x0B,
	WSP_CONTENT_LANGUAGE = 0x0C,
	WSP_CONTENT_LENGTH = 0x0D,
	WSP_CONTENT_LOCATION = 0x0E,
	WSP_CONTENT_MD5 = 0x0F,
	WSP_CONTENT_RANGE = 0x10,
	WSP_CONTENT_TYPE = 0x11,
	WSP_DATE = 0x12,
	WSP_ETAG = 0x13,
	WSP_EXPIRES = 0x14,
	WSP_FROM = 0x15,
	WSP_HOST = 0x16,
	WSP_IF_MODIFIED_SINCE = 0x17,
	WSP_IF_MATCH = 0x18,
	WSP_IF_NONE_MATCH = 0x19,
	WSP_IF_RANGE = 0x1A,
	WSP_IF_UNMODIFIED_SINCE = 0x1B,
	WSP_LOCATION = 0x1C,
	WSP_LAST_MODIFIED = 0x1D,
	WSP_MAX_FORWARDS = 0x1E,
	WSP_PRAGMA = 0x1F,
	WSP_PROXY_AUTHENTICATE = 0x20,
	WSP_PROXY_AUTHORIZATION = 0x21,
	WSP_PUBLIC = 0x22,
	WSP_RANGE = 0x23,
	WSP_REFERER = 0x24,
	WSP_RETRY_AFTER = 0x25,
	WSP_SERVER = 0x26,
	WSP_TRANSFER_ENCODING = 0x27,
	WSP_UPGRADE = 0x28,
	WSP_USER_AGENT = 0x29,
	WSP_VARY = 0x2A,
	WSP_VIA = 0x2B,
	WSP_WARNING = 0x2C,
	WSP_WWW_AUTHENTICATE = 0x2D,
	WSP_CONTENT_DISPOSITION = 0x2E,
	WSP_X_WAP_APPLICATION_ID = 0x2F,
	WSP_X_WAP_CONTENT_URI = 0x30,
	WSP_X_WAP_INITIATOR_URI = 0x31,
	WSP_ACCEPT_APPLICATION = 0x32,
	WSP_BEARER_INDICATION = 0x33,
	WSP_PUSH_FLAG = 0x34,
	WSP_PROFILE = 0x35,
	WSP_PROFILE_DIFF = 0x36,
	WSP_PROFILE_WARNING = 0x37,
};

enum MMSFieldID {
	MMS_BCC = 0x01,
	MMS_CC = 0x02,
	MMS_CONTENT_LOCATION = 0x03,
	MMS_CONTENT_TYPE = 0x04,
	MMS_DATE = 0x05,
	MMS_DELIVERY_REPORT = 0x06,
	MMS_DELIVERY_TIME = 0x07,
	MMS_EXPIRY = 0x08,
	MMS_FROM = 0x09,
	MMS_MESSAGE_CLASS = 0x0A,
	MMS_MESSAGE_ID = 0x0B,
	MMS_MESSAGE_TYPE = 0x0C,
	MMS_MMS_VERSION = 0x0D,
	MMS_MESSAGE_SIZE = 0x0E,
	MMS_PRIORITY = 0x0F,
	MMS_READ_REPLY = 0x10,
	MMS_REPORT_ALLOWED = 0x11,
	MMS_RESPONSE_STATUS = 0x12,
	MMS_RESPONSE_TEXT = 0x13,
	MMS_SENDER_VISIBILITY = 0x14,
	MMS_STATUS = 0x15,
	MMS_SUBJECT = 0x16,
	MMS_TO = 0x17,
	MMS_TRANSACTION_ID = 0x18
};

typedef struct _MMSValueEnum {
	const char *name;
	int code;
} MMSValueEnum;

typedef MMSValueEnum *MMSVALUEENUM;

typedef struct _Charset {
	const char *Name;
	const int id;
} Charset;

typedef Charset *CHARSET;

typedef struct _EncodedString {
	MMSVALUEENUM charset;
	PBYTE string;
} EncodedString;

typedef struct _MMSFieldInfo {
	const int id;
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

typedef struct _MMSValue {
	MMSValueType type;
	int allocated;
	union {
		PBYTE ptr;
		MMSShortInt short_int;
		MMSLongInt long_int;
		STR str;
		EncodedString encoded_string;
		MMSVALUEENUM enum_v;
		MMSContentType content_type;
		float float_v;
	} v;
} MMSValue;
typedef MMSValue *MMSVALUE;

typedef struct _TypedParam {
	MMSFIELDINFO type;
	MMSValue value;
} TypedParam;

typedef struct _UntypedParam {
	MMSValueType value_type;
	STR token_text;
	union {
		MMSLongInt integer;
		STR text;
	} v;
} UntypedParam;

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
	MMSHEADERS Headers;
	MMSPARTS Parts;
} MMSMessage;
typedef MMSMessage *MMSMESSAGE;

CSTR MMSValue_AsString(int field_id, MMSValue *v);
MMSError MMSValue_SetEnum(MMSVALUE out, MMSValueType vt, MMSVALUEENUM entry);

void MMSHeader_Clear(MMSHEADER header);

MMSHEADERS MMSHeaders_InitWithCapacity(int num_headers);
void MMSHeaders_Destroy(MMSHEADERS *headers);
MMSHEADER MMSHeaders_NewHeader(MMSHEADERS headers);
int MMSHeaders_GrowNumEntries(MMSHEADERS headers, int delta);
MMSHEADER MMSHeader_FindByID(MMSHEADERS headers, MMSFieldKind type, int fieldId);

int MMSParts_GrowNumEntries(MMSPARTS parts, int delta);
MMSPARTS MMSParts_InitWithCapacity(int num_parts);
void MMSParts_Destroy(MMSPARTS *parts);
MMSPART MMSParts_NewPart(MMSPARTS parts);
MMSPART MMS_CreatePart(SBUFFER stream, MMSPARTS parts);

void MMSPart_Destroy(MMSPART *part);

MMSMESSAGE MMSMessage_Init(void);
void MMSMessage_Destroy(MMSMESSAGE *message);

#endif //GAMMU_MMS_DATA_H
