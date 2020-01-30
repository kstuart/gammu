#include <assert.h>
#include <string.h>
#include <strings.h>
#include "mms-data.h"
#include "streambuffer.h"
#include "mms-tables.h"
#include "mms-encoders.h"
#include "../include/gammu-unicode.h"

MMSError MMS_EncodeFieldValue(SBUFFER stream, MMSFIELDINFO fi, MMSValue *out);


size_t GetTextLength(MMSCHARSET charset, CSTR text)
{
	assert(charset);
	assert(text);

	switch(charset->code) {
		default:
			printf("Length not implemented for charset '%s'\n", charset->name);
			return -1;
		case CHARSET_ASCII:
		case CHARSET_UTF8:
			return strlen(text);
	}
}

int MMSEncodedText_Length(ENCODEDSTRING s)
{
	assert(s);
	return GetTextLength(s->charset, s->text);
}


size_t Int32_BytesNeeded(uint32_t v)
{
	int bytes_needed = 0;
	do {
		v >>= 8u;
		bytes_needed++;
	} while(v);
	return bytes_needed;
}

MMSError MMS_EncodeNoValue(SBUFFER stream)
{
	assert(stream);
	SB_PutByte(stream, NO_VALUE);
	return MMS_ERR_NONE;
}

MMSError MMS_EncodeUintVar(SBUFFER stream, MMSUint n)
{
	assert(stream);
	static const MMSUint mask[] = {0xf0000000, 0xfe00000, 0x1fc000, 0x3f80, 0x7f};
	BYTE r = 0;
	int i = 0;

	while (i < 5 && r == 0) {
		r = (n & mask[i]) >> (7u * (4 - i));
		i++;
	}
	while (i < 5) {
		SB_PutByte(stream, r | 0x80u);
		r = (n & mask[i]) >> (7u * (4 - i));
		i++;
	}
	SB_PutByte(stream, r);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeValueLength(SBUFFER stream, size_t len)
{
	assert(stream);
	if (len > 30) {
		SB_PutByte(stream, LENGTH_QUOTE);
		return MMS_EncodeUintVar(stream, len);
	}

	SB_PutByte(stream, len & 0xffu);
	return MMS_ERR_NONE;
}

MMSError MMS_EncodeQValue(SBUFFER stream, MMSQValue value)
{
	assert(stream);
	if (value <= 0 || value >= 1.0)
		return MMS_ERR_INVALID_VALUE;

	int q = (int) (value * 1000);
	if (q % 10 == 0)
		q /= 10;
	else
		q += 100;

	return MMS_EncodeUintVar(stream, q);
}

MMSError MMS_EncodeShortInteger(SBUFFER stream, MMSShortInt value)
{
	assert(stream);
	// TODO: Don't use encoded values in tables
//	if(value > 127)
//		return MMS_ERR_INVALID_VALUE;

	value |= 0x80u;
	SB_PutByte(stream, value);
	return MMS_ERR_NONE;
}

MMSError MMS_EncodeLongInteger(SBUFFER stream, MMSLongInt value)
{
	assert(stream);
	uint8_t nbytes = Int32_BytesNeeded(value);
	SB_PutByte(stream, nbytes);
	for (int i = nbytes - 1; i >= 0; i--)
		SB_PutByte(stream, (value >> (8u * i)) & 0xffu);

	return 0;
}

MMSError MMS_EncodeInteger(SBUFFER stream, MMSLongInt value)
{
	assert(stream);
	return value <= 128 ?
	       MMS_EncodeShortInteger(stream, value) :
	       MMS_EncodeLongInteger(stream, value);
}

//MMSError MMS_EncodeWellKnownCharset(SBUFFER stream, CSTR charset)
//{
//	assert(stream);
//	if(charset == NULL || strcasecmp("*", charset) == 0)
//		return MMS_EncodeInteger(stream, CHARSET_ANY);
//
//	MMSVALUEENUM e = MMS_Charset_FindByName(charset);
//	if(e == NULL)
//		return MMS_ERR_INVALID_VALUE;
//
//	return MMS_EncodeInteger(stream, e->code);
//}

MMSError MMS_EncodeText(SBUFFER stream, CSTR text)
{
	assert(stream);
	if(IsEmptyString(text))
		return MMS_EncodeNoValue(stream);

	if(text[0] < 0)
		SB_PutByte(stream, TEXT_QUOTE);

	SB_PutString(stream, text);
	return MMS_ERR_NONE;
}

// NOTE: Only ASCII and UTF-8 are fully supported
MMSError MMS_EncodeEncodedText(SBUFFER stream, MMSCHARSET charset, CSTR text)
{
	assert(stream);
	assert(charset);

	size_t len = charset->code == CHARSET_ASCII ? strlen(text) :  UnicodeLength(text);
	SBUFFER b = SB_InitWithCapacity(len + 5);
	MMS_EncodeInteger(b, charset->code);
	SB_PutBytes(b, text, len);
	SB_PutByte(b, END_OF_STRING);

	MMS_EncodeValueLength(stream, SBUsed(b));
	SB_PutBytes(stream, SBBase(b), SBUsed(b));
	SB_Destroy(&b);

	return MMS_ERR_NONE;
}

// NOTE: Only ASCII encoded international phone numbers supported
MMSError MMS_EncodeAddress(SBUFFER stream, ENCODEDSTRING address)
{
	assert(stream);
	assert(address);
	CSTR type = "/TYPE=PLMN";
	char b[60];

	ssize_t len = MMSEncodedText_Length(address);
	if(len == -1)
		return MMS_ERR_UNSUPPORTED;

	memcpy(b, address->text, len);
	*(b + len) = 0;

	if(address->charset->code == CHARSET_UTF8)
		EncodeUTF8(b + len, type);
	else
		strcat(b, type);

	return MMS_EncodeEncodedText(stream, address->charset, b);
}


MMSError MMS_EncodeFromAddress(SBUFFER stream, ENCODEDSTRING from)
{
	assert(stream);

	if(!from || IsEmptyString(from->text)) {
		MMS_EncodeValueLength(stream, 1);
		SB_PutByte(stream, TOK_ADDRESS_INSERT);
		return MMS_ERR_NONE;
	}

	ssize_t len = MMSEncodedText_Length(from);
	SBUFFER  b = SB_InitWithCapacity(len + 20);
	SB_PutByte(b, TOK_ADDRESS_PRESENT);
	MMS_EncodeAddress(b, from);

	MMS_EncodeValueLength(stream, SBUsed(b));
	SB_PutBytes(stream, SBBase(b), SBUsed(b));
	SB_Destroy(&b);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeTokenText(SBUFFER stream, CSTR text)
{
	return MMS_EncodeText(stream, text);
}

// wap-230 8.4.2.1
MMSError MMS_EncodeExtensionMedia(SBUFFER stream, CSTR extension)
{
	if(extension[0] > 127)
		return MMS_ERR_INVALID_VALUE;

	return MMS_EncodeText(stream, extension);
}

MMSError MMS_EncodeConstrainedEncoding(SBUFFER stream, MMSVALUE value)
{
	assert(stream);
	assert(value);

	if(value->type != VT_SHORT_INT) {
		assert(value->type == VT_TEXT);
		return MMS_EncodeExtensionMedia(stream, value->v.str);
	}

	MMSVALUEENUM ct = MMS_WkContentType_FindByID(value->v.short_int);
	if(!ct)
		return MMS_ERR_INVALID_VALUE;

	return MMS_EncodeShortInteger(stream, ct->code);
}

MMSError MMS_EncodeWellKnownMedia(SBUFFER stream, MMSVALUEENUM ct)
{
	assert(stream);
	assert(ct);

	return MMS_EncodeInteger(stream, ct->code);
}

MMSError MMS_EncodeWellKnownCharset(SBUFFER stream, MMSCHARSET charset)
{
	assert(stream);
	assert(charset);

	return MMS_EncodeInteger(stream, charset->code);
}

MMSError MMS_EncodeTypedParameter(SBUFFER stream, TYPEDPARAM param)
{
	assert(stream);
	assert(param);

	MMS_EncodeInteger(stream, param->type->code);
	if(param->type->vt == VT_TEXT)
		MMS_EncodeText(stream, param->value.v.str);
	else
		MMS_EncodeInteger(stream, param->value.v.long_int);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeUntypedParameter(SBUFFER stream, UNTYPEDPARAM param)
{
	assert(stream);
	assert(param);

	MMS_EncodeTokenText(stream, param->token_text);
	if(param->value_type == VT_TEXT)
		MMS_EncodeText(stream, param->v.text);
	else
		MMS_EncodeInteger(stream, param->v.integer);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeParameters(SBUFFER stream, MMSPARAMETERS params)
{
	assert(stream);
	assert(params);

	for(int i = 0; i < params->count; i++)
		if(params->entries[i].kind == MMS_PARAM_TYPED)
			MMS_EncodeTypedParameter(stream, &params->entries[i].v.typed);
		else
			MMS_EncodeUntypedParameter(stream, &params->entries[i].v.untyped);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeMediaType(SBUFFER stream, MMSCONTENTTYPE value)
{
	assert(stream);
	assert(value);

	if(value->vt == VT_WK_MEDIA)
		return MMS_EncodeWellKnownMedia(stream, value->v.wk_media);
	else
		return MMS_EncodeExtensionMedia(stream, value->v.ext_media);
}

MMSError MMS_EncodeContentGeneralForm(SBUFFER stream, MMSCONTENTTYPE value)
{
	assert(stream);
	assert(value);

	SBUFFER b = SB_Init();
	MMS_EncodeMediaType(b, value);
	MMS_EncodeParameters(b, &value->params);

	MMS_EncodeValueLength(stream, SBUsed(b));
	SB_PutBytes(stream, SBBase(b), SBUsed(b));
	SB_Destroy(&b);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeContentType(SBUFFER stream, MMSVALUE value)
{
	assert(stream);
	assert(value);

	switch(value->type) {
		default:
			printf("Invalid value type (%d)", value->type);
			return MMS_ERR_INVALID_VALUE;
		case VT_CONSTRAINED:
			return MMS_EncodeConstrainedEncoding(stream, value);
		case VT_CONTENT_TYPE:
			return MMS_EncodeContentGeneralForm(stream, &value->v.content_type);
	}
}

MMSError MMS_EncodeMessage(SBUFFER stream, MMSMESSAGE m)
{
	assert(m);
	assert(stream);

	MMSHEADERS hdrs = m->Headers;
	MMSHEADERS hdrs_front = MMSHeaders_InitWithCapacity(3);
	MMSHEADERS hdrs_rest = MMSHeaders_InitWithCapacity(5);

	for(size_t i = 0; i != hdrs->end; i++) {
		MMSHEADER group_hdr;
		MMSHEADER h = &hdrs->entries[i];
		switch(h->id.info->code) {
			default:
				group_hdr = MMSHeaders_NewHeader(hdrs_rest);
				MMSHeader_ShallowClone(group_hdr, h);
				break;
			case MMS_MESSAGE_TYPE:
			case MMS_TRANSACTION_ID:
			case MMS_MMS_VERSION:
				group_hdr = MMSHeaders_NewHeader(hdrs_front);
				MMSHeader_ShallowClone(group_hdr, h);
				break;
		}
	}

	if(hdrs_front->end != 3) {
		MMSHeaders_Destroy(&hdrs_front);
		MMSHeaders_Destroy(&hdrs_rest);
		return MMS_ERR_REQUIRED_FIELD;
	}

	MMSHEADER ct = MMSHeaders_FindByID(hdrs_rest, MMS_HEADER, MMS_CONTENT_TYPE);
	if(!ct) {
		ct = MMSHeaders_NewHeader(hdrs_rest);
		if(!ct) {
			MMSHeaders_Destroy(&hdrs_front);
			MMSHeaders_Destroy(&hdrs_rest);
			return MMS_ERR_MEMORY;
		}

		ct->id.kind = MMS_HEADER;
		ct->id.info = MMSFields_FindByID(MMS_CONTENT_TYPE);
		ct->value.type = VT_CONTENT_TYPE;
		ct->value.v.content_type.vt = VT_WK_MEDIA;
		ct->value.v.content_type.v.wk_media = MMS_WkContentType_FindByID(0x23);
	}

	MMS_EncodeHeaders(stream, hdrs_front);
	MMS_EncodeHeaders(stream, hdrs_rest);
	MMSHeaders_Destroy(&hdrs_front);
	MMSHeaders_Destroy(&hdrs_rest);

	MMS_EncodeParts(stream, m->Parts);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeHeaders(SBUFFER stream, MMSHEADERS headers)
{
	assert(stream);
	assert(headers);

	MMSError error;

	for(size_t i = 0; i < headers->end; i++) {
		error = MMS_EncodeHeader(stream, &headers->entries[i]);
		if(error != MMS_ERR_NONE)
			return error;
	}

	return MMS_ERR_NONE;
}

MMSError MMS_EncodeHeader(SBUFFER stream, MMSHEADER header)
{
	assert(stream);
	assert(header);

	MMSError e = MMS_EncodeShortInteger(stream, header->id.info->code);
	if(e != MMS_ERR_NONE)
		return e;

	return MMSValue_Encode(stream, &header->value);
}

MMSError MMS_EncodeParts(SBUFFER stream, MMSPARTS parts)
{
	assert(stream);
	assert(parts);

	MMSError error;

	error = MMS_EncodeUintVar(stream, parts->end);
	if(error != MMS_ERR_NONE)
		return error;

	for(size_t i = 0; i < parts->end; i++)
		MMS_EncodePart(stream, &parts->entries[i]);

	return MMS_ERR_NONE;
}

MMSError MMS_EncodePart(SBUFFER stream, MMSPART part)
{
	assert(part);
	assert(stream);
	MMSError error;

	MMSHeader ct = *MMSHeaders_FindByID(part->headers, MMS_HEADER, MMS_CONTENT_TYPE);
	if(ct.id.info->vt == VT_NONE)
		return MMS_ERR_REQUIRED_FIELD;

	MMSHeaders_Remove(part->headers, &ct);

	SBUFFER b = SB_InitWithCapacity(part->data_len);
	error = MMS_EncodeContentType(b, &ct.value);
	if(error != MMS_ERR_NONE) {
		SB_Destroy(&b);
		return error;
	}

	error = MMS_EncodeHeaders(b, part->headers);
	if(error != MMS_ERR_NONE) {
		SB_Destroy(&b);
		return error;
	}

	error = MMS_EncodeUintVar(stream, SBUsed(b));
	if(error != MMS_ERR_NONE) {
		SB_Destroy(&b);
		return error;
	}

	error = MMS_EncodeUintVar(stream, part->data_len);
	if(error != MMS_ERR_NONE)
		return error;

	SB_PutBytes(stream, SBBase(b), SBUsed(b));
	SB_Destroy(&b);

	SB_PutBytes(stream, part->data, part->data_len);

	return MMS_ERR_NONE;
}
