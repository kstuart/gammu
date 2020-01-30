#include <malloc.h>
#include <string.h>
#include <limits.h>
#include "streambuffer.h"
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include <gammu-unicode.h>

#include "mms-data.h"
#include "mms-decoders.h"
#include "mms-encoders.h"
#include "mms-tables.h"
#include "mms-service.h"

typedef enum _IntToStringFmt {
	FMT_DECIMAL,
	FMT_HEX
} IntToStringFmt;

bool IsASCII(CSTR str)
{
	assert(str);
	const size_t len = strlen(str);
	for(size_t i = 0; i < len; i++)
		if(str[i] < 32)
			return false;

	return true;
}

const char *IntToStr(unsigned long i, IntToStringFmt fmt)
{
	static char buf[(CHAR_BIT * sizeof(unsigned long) - 1) / 3 + 2];
	switch(fmt) {
		default:
		case FMT_DECIMAL:
			snprintf(buf, sizeof(buf), "%zu", i);
			break;
		case FMT_HEX:
			snprintf(buf, sizeof(buf), "%#lx", i);
	}
	return buf;
}

bool IsEmptyString(CSTR str)
{
	return str && strlen(str) > 0 ? false : true;
}

MMSCHARSET MMS_GetCharset(CSTR str)
{
	assert(str);
	int ch = IsASCII(str) ? CHARSET_ASCII : CHARSET_UTF8;
	return MMS_Charset_FindByID(ch);
}


// MMSValue

MMSError MMSValue_AsString(SBUFFER stream, MMSVALUE v)
{
	assert(stream);
	static SBUFFER b = NULL;

	if(!b)
		b = SB_InitWithCapacity(4096);
	else
		SB_Clear(b);

	switch(v->type) {
		default:
			SB_PutString(stream, "<NOT-CONVERTED-PLEASE-REPORT>");
			return MMS_ERR_UNIMPLEMENTED;
		case VT_SHORT_INT:
			SB_PutString(stream, IntToStr(v->v.short_int, FMT_DECIMAL));
			break;
		case VT_VERSION:
			SB_PutString(stream, IntToStr(v->v.short_int, FMT_HEX));
			break;
		case VT_LONG_INT:
			SB_PutString(stream, IntToStr(v->v.long_int, FMT_DECIMAL));
			break;
		case VT_DATE:
			SB_PutString(stream, IntToStr(v->v.long_int, FMT_DECIMAL));
			char *date = asctime(gmtime((time_t *) &v->v.long_int));
			date[strlen(date) - 1] = 0;
			SB_PutString(stream, date);
			break;
		case VT_CONSTRAINED:
		case VT_TEXT:
			SB_PutString(stream, v->v.str);
			break;
		case VT_ENCODED_STRING:
		case VT_FROM:
		case VT_ADDRESS:
				switch(v->v.encoded_string.charset->code) {
					default:
						return MMS_ERR_UNSUPPORTED;
					case CHARSET_ASCII:
						SB_PutString(stream, v->v.encoded_string.text);
						break;
					case CHARSET_UTF8: {
						int len = MMSEncodedText_Length(&v->v.encoded_string);
						SB_MinCapacity(b, len / 2 + 1);
						DecodeUTF8(SBBase(b), v->v.encoded_string.text, len);
						SB_PutString(stream, SBBase(b));
					}
					break;
			}
			break;
		case VT_ENUM:
		case VT_YESNO:
		case VT_MESSAGE_TYPE:
		case VT_MESSAGE_CLASS:
		case VT_HIDESHOW:
		case VT_RESPONSE_STATUS:
		case VT_PRIORITY:
		case VT_WK_CHARSET:
			SB_PutString(stream, v->v.enum_v->name);
			break;
		case VT_QVALUE:
			SB_PutFormattedString(stream, "%f", v->v.float_v);
			break;
		case VT_CONTENT_TYPE:
			MMS_ContentTypeAsString(stream, &v->v.content_type);
			break;
	}

	return MMS_ERR_NONE;
}

MMSError MMSValue_SetEnum(MMSVALUE out, MMSValueType vt, MMSVALUEENUM entry)
{
	assert(out);
	assert(entry);

	MMSValue_Clear(out);
	out->type = vt;
	out->v.enum_v = entry;
	return MMS_ERR_NONE;
}

MMSError MMSValue_SetShort(MMSVALUE out, MMSShortInt value)
{
	assert(out);

	MMSValue_Clear(out);
	out->type = VT_SHORT_INT;
	out->v.short_int = value;
	return MMS_ERR_NONE;
}

MMSError MMSValue_SetLocalTransactionID(MMSVALUE out, LocalTXID value)
{
	assert(out);

	MMSValue_Clear(out);
	out->type = VT_LOCAL_TXID;
	out->v.local_txid = value;
	return MMS_ERR_NONE;
}

void MMSValue_Init(MMSVALUE v)
{
	assert(v);
	memset(v, 0, sizeof(MMSValue));
}

void MMSValue_Clear(MMSVALUE value)
{
	assert(value);
	if(value->type == VT_CONTENT_TYPE) {
		if(value->v.content_type.params.count)
			free(value->v.content_type.params.entries);
	}

	if(value->allocated) {
		switch(value->type) {
			default:
				free(value->v.ptr);
				break;
			case VT_ENCODED_STRING:
			case VT_FROM:
			case VT_ADDRESS:
				free(value->v.encoded_string.text);
				break;
		}
	}

	memset(value, 0, sizeof(MMSValue));
}

MMSError MMSValue_CopyStr(MMSVALUE v, MMSValueType t, CSTR str)
{
	assert(v);
	assert(str);
	MMSValue_Clear(v);

	v->v.str = malloc(strlen(str));
	if(!v->v.str)
		return MMS_ERR_MEMORY;

	v->allocated = 1;
	v->type = t;
	strcpy(v->v.str, str);
	return MMS_ERR_NONE;
}

MMSError MMSValue_CopyEncodedStr(MMSVALUE v, MMSValueType t, ENCODEDSTRING str)
{
	assert(v);
	assert(str);
	MMSValue_Clear(v);

	size_t len = MMSEncodedText_Length(str);

	v->v.encoded_string.text = malloc(len + 1);
	if (!v->v.encoded_string.text)
		return MMS_ERR_MEMORY;

	v->allocated = 1;
	v->type = t;
	v->v.encoded_string.charset = str->charset;
	memcpy(v->v.encoded_string.text, str->text, len);
	v->v.encoded_string.text[len] = 0;

	return MMS_ERR_NONE;
}

MMSError MMSValue_SetEncodedString(MMSVALUE v, MMSCHARSET ch, CSTR str)
{
	assert(v);
	assert(str);

	if(!ch)
		ch = MMS_GetCharset(str);

	const size_t len = strlen(str);
	STR text = malloc(len + 1);
	if(!text)
		return MMS_ERR_MEMORY;

	memcpy(text, str, len);
	text[len] = 0;

	MMSValue_Clear(v);
	v->allocated = 1;
	v->type = VT_ENCODED_STRING;
	v->v.encoded_string.charset = ch;
	v->v.encoded_string.text = text;

	return MMS_ERR_NONE;
}

MMSError MMSValue_SetFromString(MMSVALUE v, MMSFIELDINFO fi, CSTR str)
{
	assert(v);
	assert(str);
	MMSError error;

	switch(fi->vt) {
		default:
			printf("No setter for value of %s", fi->name);
			return MMS_ERR_UNIMPLEMENTED;
		case VT_ENCODED_STRING:
			error = MMSValue_SetEncodedString(v, NULL, str);
	}

	if(error != MMS_ERR_NONE)
		return error;

	switch(fi->code) {
		case MMS_BCC:
		case MMS_CC:
		case MMS_TO:
			v->type = VT_ADDRESS;
			break;
	}

	return error;
}

MMSError MMSValue_Encode(SBUFFER stream, MMSVALUE v)
{
	assert(stream);
	assert(v);

	switch(v->type) {
		default:
			printf("missing value type encoder (%d)\n", v->type);
			return MMS_ERR_NOCODEC;
		case VT_SHORT_INT:
			return MMS_EncodeShortInteger(stream, v->v.short_int);
		case VT_LONG_INT:
			return MMS_EncodeLongInteger(stream, v->v.long_int);
		case VT_ENCODED_STRING:
			return MMS_EncodeEncodedText(stream, v->v.encoded_string.charset, v->v.encoded_string.text);
		case VT_FROM:
			return MMS_EncodeFromAddress(stream, &v->v.encoded_string);
		case VT_ADDRESS:
			return MMS_EncodeAddress(stream, &v->v.encoded_string);
		case VT_ENUM:
		case VT_YESNO:
		case VT_MESSAGE_TYPE:
		case VT_MESSAGE_CLASS:
		case VT_HIDESHOW:
		case VT_RESPONSE_STATUS:
		case VT_PRIORITY:
			return MMS_EncodeShortInteger(stream, v->v.enum_v->code);
		case VT_WK_CHARSET:
			return MMS_EncodeInteger(stream, v->v.long_int);
		case VT_CONTENT_TYPE:
			return MMS_EncodeContentType(stream, v);
		case VT_LOCAL_TXID:
			SB_PutAsBinHex(stream, &v->v.local_txid, sizeof(LocalTXID));
			SB_PutByte(stream, 0);
			return MMS_ERR_NONE;
	}
}

// MMSFieldInfo

MMSFIELDINFO MMSFieldInfo_FindByID(MMSFieldKind kind, int id)
{
	switch(kind) {
		default:
			return NULL;
		case(MMS_HEADER):
			return MMSFields_FindByID(id);
		case(WSP_HEADER):
			return WSPFields_FindByID(id);
	}
}

// MMSHeader

void MMSHeader_Clear(MMSHEADER header)
{
	MMSValue_Clear(&header->value);
}

MMSHEADER MMSHeader_FindByID(MMSHEADERS headers, MMSFieldKind type, int fieldId)
{
	MMSHEADER header;

	for(size_t i = 0; i < headers->end; i++) {
		header = &headers->entries[i];
		if (header->id.kind == type && header->id.info->code == fieldId)
			return header;
	}

	return NULL;
}

bool MMSHeader_SameHeader(MMSHEADER l, MMSHEADER r)
{
	if(!l && !r)
		return true;

	if(!(l && r))
		return false;

	return (l->id.kind == r->id.kind) && (l->id.info == r->id.info);
}

// MMSHeaders

void MMSHeader_ShallowClone(MMSHEADER dest, MMSHEADER src)
{
	dest->id = src->id;
	dest->value = src->value;
	dest->value.allocated = false;
}

MMSError MMSHeader_AsString(SBUFFER stream, MMSHEADER header)
{
	assert(stream);
	assert(header);

	SB_PutFormattedString(stream, "%s=", header->id.info->name);
	MMSValue_AsString(stream, &header->value);
	SB_PutString(stream, "\n");
	return MMS_ERR_NONE;
}

int MMSHeaders_GrowNumEntries(MMSHEADERS headers, int delta)
{
	MMSHEADER entries = realloc(headers->entries, (headers->capacity + delta) * sizeof(MMSHeader));
	if(entries) {
		memset(entries + headers->capacity, 0, delta * sizeof(MMSHeader));
		headers->capacity += delta;
		headers->entries = entries;
	}

	return headers->capacity;
}

MMSHEADERS MMSHeaders_InitWithCapacity(int num_headers)
{
	MMSHEADERS headers = malloc(sizeof(MMSHeaders));
	if(!headers)
		return NULL;

	memset(headers, 0, sizeof(MMSHeaders));
	if(MMSHeaders_GrowNumEntries(headers, num_headers) != num_headers) {
		free(headers);
		return NULL;
	}

	return headers;
}

void MMSHeaders_Destroy(MMSHEADERS *headers)
{
	for(size_t i =0; i < (*headers)->end; i++)
		MMSHeader_Clear(&(*headers)->entries[i]);

	free((*headers)->entries);
	free(*headers);
	*headers = NULL;
}

MMSHEADER MMSHeaders_NewHeader(MMSHEADERS headers)
{
	if(headers->end == headers->capacity)
		MMSHeaders_GrowNumEntries(headers, (int)(headers->capacity * 0.5));

	if(headers->end >= headers->capacity)
		return NULL;

	return &headers->entries[headers->end++];
}

MMSHEADER MMSHeaders_FindByID(MMSHEADERS h, MMSFieldKind kind, int field_id)
{
	assert(h);
	for(size_t i = 0; i < h->end; i++)
		if(h->entries[i].id.kind == kind && h->entries[i].id.info->code == field_id)
			return &h->entries[i];

	return NULL;
}

MMSError MMSHeaders_Remove(MMSHEADERS headers, MMSHEADER header)
{
	assert(headers);
	if(!header)
		return MMS_ERR_NONE;

	size_t pos = 0;
	while(pos < headers->end)
		if(MMSHeader_SameHeader(&headers->entries[pos], header))
			break;

	if(pos == headers->end)
		return MMS_ERR_NOTFOUND;

	MMSHEADER found = &headers->entries[pos];
	MMSHeader_Clear(found);

	memmove(found,
	        &headers->entries[pos + 1],
	        sizeof(MMSHeader) * (headers->end - pos));

	headers->end--;

	return MMS_ERR_NONE;
}

// MMSPart

void MMSPart_Destroy(MMSPART *part)
{
	MMSHeaders_Destroy(&(*part)->headers);
	if((*part)->allocated)
		free((*part)->data);

	*part = NULL;
}

int MMSParts_GrowNumEntries(MMSPARTS parts, int delta)
{
	MMSPART entries = realloc(parts->entries, (parts->capacity + delta) * sizeof(MMSPart));
	if(entries) {
		memset(entries + parts->capacity, 0, delta * sizeof(MMSPart));
		parts->capacity += delta;
		parts->entries = entries;
	}

	return parts->capacity;
}


MMSPARTS MMSParts_InitWithCapacity(int num_parts)
{
	MMSPARTS parts = malloc(sizeof(MMSParts));
	if(!parts)
		return NULL;

	memset(parts, 0, sizeof(MMSParts));
	if(MMSParts_GrowNumEntries(parts, num_parts) != num_parts) {
		free(parts);
		return NULL;
	}

	return parts;
}

void MMSParts_Destroy(MMSPARTS *parts)
{
	MMSPART part;
	for(size_t i =0; i < (*parts)->end; i++) {
		part = &(*parts)->entries[i];
		MMSPart_Destroy(&part);
	}

	free((*parts)->entries);
	free(*parts);
	*parts = NULL;
}

MMSPART MMSParts_NewPart(MMSPARTS parts)
{
	MMSPART part;
	if(parts->end == parts->capacity)
		MMSParts_GrowNumEntries(parts, (int)(parts->capacity * 0.5));

	if(parts->end >= parts->capacity)
		return NULL;

	part = &parts->entries[parts->end++];
	part->allocated = false;
	part->headers = MMSHeaders_InitWithCapacity(2);
	return part;
}

MMSMESSAGE MMSMessage_Init()
{
	MMSMESSAGE msg = malloc(sizeof(MMSMessage));
	if(!msg)
		return NULL;

	memset(msg, 0, sizeof(MMSMessage));
	msg->Headers = MMSHeaders_InitWithCapacity(10);
	if(!msg->Headers) {
		free(msg);
		return NULL;
	}

	msg->Parts = MMSParts_InitWithCapacity(3);
	if(!msg->Parts) {
		MMSHeaders_Destroy(&msg->Headers);
		free(msg);
		return NULL;
	}

	return msg;
}

void MMSMessage_Destroy(MMSMESSAGE *message)
{
	MMSHeaders_Destroy(&(*message)->Headers);
	MMSParts_Destroy(&(*message)->Parts);
	if((*message)->id)
		free((*message)->id);

	free(*message);
	*message = NULL;
}

MMSError MMSMessage_SetID(MMSMESSAGE message, CSTR id)
{
	assert(message);
	if(message->id)
		free(message->id);

	message->id = malloc(strlen(id));
	if(!message->id)
		return MMS_ERR_MEMORY;

	strcpy(message->id, id);

	return MMS_ERR_NONE;
}

MMSHEADER MMSMessage_FindHeader(MMSMESSAGE m, MMSFieldKind kind, int id)
{
	assert(m);
	return MMSHeaders_FindByID(m->Headers, kind, id);
}

MMSHEADER MMSMessage_FindOrNewHeader(MMSMESSAGE m, MMSFieldKind kind, int id)
{
	assert(m);
	MMSHEADER h = MMSMessage_FindHeader(m, kind, id);
	if(h)
		return h;

	h = MMSHeaders_NewHeader(m->Headers);
	if (!h)
		return NULL;

	MMSFIELDINFO fi = MMSFieldInfo_FindByID(kind, id);
	assert(fi);

	h->id.kind = kind;
	h->id.info = fi;

	return h;
}

MMSError MMSMessage_SetMessageType(MMSMESSAGE m,  MMSMessageTypeID type)
{
	assert(m);
	MMSHEADER h = MMSMessage_FindOrNewHeader(m, MMS_HEADER, MMS_MESSAGE_TYPE);
	if(!h)
		return MMS_ERR_MEMORY;

	MMSVALUEENUM typeinfo = MMS_MessageType_FindByID(type);
	assert(typeinfo);

	MMSValue_SetEnum(&h->value, VT_MESSAGE_TYPE, typeinfo);
	m->MessageType = typeinfo;

	return MMS_ERR_NONE;
}

MMSError MMSMessage_SetMessageVersion(MMSMESSAGE m, MMSVersion v)
{
	assert(m);
	MMSHEADER h = MMSMessage_FindOrNewHeader(m, MMS_HEADER, MMS_MMS_VERSION);
	if(!h)
		return MMS_ERR_MEMORY;

	m->Version = v;
	return MMSValue_SetShort(&h->value, v);
}

MMSError MMSMessage_SetTransactionID(MMSMESSAGE m, LocalTXID txid)
{
	assert(m);
	MMSHEADER h = MMSMessage_FindOrNewHeader(m, MMS_HEADER, MMS_TRANSACTION_ID);
	if(!h)
		return MMS_ERR_MEMORY;

	if(txid == -1)
		txid = CreateTransactionID();

	return MMSValue_SetLocalTransactionID(&h->value, txid);
}

MMSError MMSMessage_SetDeliveryReport(MMSMESSAGE m, MMSVALUEENUM v)
{
	assert(m);
	MMSHEADER h = MMSMessage_FindOrNewHeader(m, MMS_HEADER, MMS_DELIVERY_REPORT);
	if(!h)
		return MMS_ERR_MEMORY;

	return MMSValue_SetEnum(&h->value, VT_YESNO, v);
}

MMSError MMSMessage_CopyAddressFrom(MMSMESSAGE m, ENCODEDSTRING from)
{
	assert(m);
	assert(from);
	MMSHEADER h = MMSMessage_FindOrNewHeader(m, MMS_HEADER, MMS_FROM);
	if(!h)
		return MMS_ERR_MEMORY;

	return MMSValue_CopyEncodedStr(&h->value, VT_FROM, from);
}

MMSError MMSMessage_CopyAddressTo(MMSMESSAGE m, ENCODEDSTRING to)
{
	assert(m);
	assert(to);
	MMSHEADER h = MMSMessage_FindOrNewHeader(m, MMS_HEADER, MMS_TO);
	if(!h)
		return MMS_ERR_MEMORY;

	return MMSValue_CopyEncodedStr(&h->value, VT_ADDRESS, to);
}

// MMSFieldInfo

MMSError MMSMessage_AddPart(MMSMESSAGE m, CSTR media, CPTR data, size_t data_len)
{
	assert(m);
	assert(media);
	assert(data);

	MMSPART p = MMSParts_NewPart(m->Parts);
	if(!p)
		return MMS_ERR_MEMORY;

	MMSHEADER h = MMSHeaders_NewHeader(p->headers);
	if(!h)
		return MMS_ERR_MEMORY;

	MMSFIELDINFO fi = MMSFields_FindByID(MMS_CONTENT_TYPE);
	h->id.kind = MMS_HEADER;
	h->id.info = fi;

	MMSError error = MMS_ParseMediaType(media, &h->value.v.content_type);
	if(error != MMS_ERR_NONE)
		return error;

	if(*data == '\\' && *(data + 1) == 'x') {
		const size_t sz = (strlen(data) - 2) / 2;
		PBYTE buf = malloc(sz + 1);
		if(!buf)
			return MMS_ERR_MEMORY;

		if(DecodeHexBin(buf, data + 2, sz * 2) == FALSE) {
			free(buf);
			return MMS_ERR_INVALID_DATA;
		}

		p->data = buf;
		p->data_len = sz;
	}
	else {
		p->data = malloc(data_len);
		if(!p->data)
			return MMS_ERR_MEMORY;

		memcpy(p->data, data, data_len);
		p->data_len = data_len;
	}

	p->allocated = true;
	h->value.type = VT_CONTENT_TYPE;

	return MMS_ERR_NONE;
}
