#include <assert.h>

#include <dynamic-buffer.h>
#include "mms-data.h"
#include "mms-tables.h"


size_t MMS_DecodeUintVar(CDBUFFER stream)
{
	size_t uint = 0;
	uint8_t byte = DynaBuf_NextByte(stream);
	while(byte >> 7u) {
		uint <<= 7u;
		uint |= byte & 0x7fu;
		byte = DynaBuf_NextByte(stream);
	}
	uint <<= 7u;
	uint |= byte & 0x7fu;

	return uint;
}

ssize_t DecodeValueLength(CDBUFFER stream)
{
	size_t len = DynaBuf_PeekByte(stream);
	if(len > 31)
		return -1;
	DynaBuf_NextByte(stream);

	if(len < 31)
		return len;

	return MMS_DecodeUintVar(stream);
}

int IsShortInteger(MMSShortInt v)
{
	return (v & 0x80u) != 0;
}

MMSError MMS_DecodeShortInteger(CDBUFFER stream, MMSVALUE out)
{
	if(!IsShortInteger(DynaBuf_PeekByte(stream)))
		return MME_INVALID_DATA;

	out->allocated = 0;
	out->type = VT_SHORT_INT;
	out->v.short_int = (DynaBuf_NextByte(stream) & 0x7fu);
	return MME_NONE;
}

MMSError MMS_DecodeLongInteger(CDBUFFER stream, MMSVALUE out)
{
	int len = DynaBuf_PeekByte(stream);
	if(len > 30)
		return MME_BADLENGTH;

	DynaBuf_NextByte(stream);
	printf("len(%d)\n", len);
	assert(len <= 4);

	MMSLongInt n = 0;
	for(int i = 0; i < len; i++) {
		n <<= 8u;
		n |= DynaBuf_NextByte(stream);
	}

	out->allocated = 0;
	out->type = VT_LONG_INT;
	out->v.long_int = n;
	return MME_NONE;
}

MMSError MMS_DecodeInteger(CDBUFFER stream, MMSVALUE out)
{
	if(MMS_DecodeShortInteger(stream, out) != MME_NONE)
		return MMS_DecodeLongInteger(stream, out);

	return MME_NONE;
}

MMSVALUEENUM DecodeWellKnownCharset(CDBUFFER stream)
{
	if(DynaBuf_PeekByte(stream) == ANY_CHARSET) {
		DynaBuf_NextByte(stream);
		return Charset_FindByID(0);
	}

	MMSValue v;
	MMS_DecodeInteger(stream, &v);

	MMSVALUEENUM e = Charset_FindByID(
		v.type == VT_SHORT_INT ? (int)v.v.short_int : (int)v.v.long_int);

	return e;
}

MMSError MMS_DecodeText(CDBUFFER stream, MMSVALUE out)
{
	BYTE b = DynaBuf_PeekByte(stream);
	if(b < 32 || b > 127)
		return MME_INVALID_DATA;

	out->allocated = 0;
	out->type = VT_TEXT;
	out->v.str = DynaBufPtr(stream);
	DynaBuf_Seek(stream, DynaBuf_FindNext(stream, 0) + 1, SEEK_CUR);

	return MME_NONE;
}

MMSError MMS_DecodeQuoteText(CDBUFFER stream, MMSVALUE out)
{
	if(DynaBuf_PeekByte(stream) == TEXT_QUOTE)
		DynaBuf_NextByte(stream);

	out->allocated = 0;
	out->type = VT_TEXT;
	out->v.str = DynaBufPtr(stream);
	DynaBuf_Seek(stream, DynaBuf_FindNext(stream, 0) + 1, SEEK_CUR);

	return MME_NONE;
}

MMSError MMS_DecodeEncodedText(CDBUFFER stream, MMSVALUE out)
{
	ssize_t len = DecodeValueLength(stream);
	if(len == -1)
		return MMS_DecodeQuoteText(stream, out);

	MMSVALUEENUM charset = DecodeWellKnownCharset(stream);
	MMSValue v;
	MMS_DecodeQuoteText(stream, &v);

	out->allocated = 0;
	out->type = VT_ENCODED_STRING;
	out->v.encoded_string.charset = charset;
	out->v.encoded_string.string = v.v.str;

	return MME_NONE;
}

MMSError MMS_DecodeFromAddress(CDBUFFER stream, MMSVALUE out)
{
	ssize_t len = DecodeValueLength(stream);
	if(len == -1)
		return MME_BADLENGTH;

	BYTE token = DynaBuf_NextByte(stream);
	if(token == TOK_ADDRESS_PRESENT)
		return MMS_DecodeEncodedText(stream, out);

	out->allocated = 0;
	out->type = VT_FROM;
	out->v.str = (char*)"";

	return MME_NONE;
}

/*
 * Token-text = Token End-of-string
 */
MMSError MMS_DecodeTokenText(CDBUFFER stream, MMSVALUE out)
{
	return MMS_DecodeText(stream, out);
}


/*
 * Extension-media = *TEXT End-of-string
 */
MMSError MMS_DecodeExtensionMedia(CDBUFFER stream, MMSVALUE out)
{
	return MMS_DecodeText(stream, out);
}

/* Constrained-encoding = Extension-Media | Short-integer
 * Extension-media = *TEXT End-of-string
 */
MMSError MMS_DecodeConstrainedEncoding(CDBUFFER stream, MMSVALUE out)
{
	MMSError error = MMS_DecodeShortInteger(stream, out);
	if(error != MME_NONE)
		return MMS_DecodeExtensionMedia(stream, out);

	MMSVALUEENUM ct = MMS_WkContentType_FindByID(out->v.short_int);
	if(ct) {
		MMSValue_SetEnum(out, VT_ENUM, ct);
		return MME_NONE;
	}

	return MME_LOOKUP_FAILED;
}

/* Well-known-media = Integer-value */
MMSError MMS_DecodeWellKnownMedia(CDBUFFER stream, MMSVALUE out)
{
	MMSError error;
	error = MMS_DecodeInteger(stream, out);
	if(error != MME_NONE)
		return error;

	MMSVALUEENUM media = MMS_WkContentType_FindByID(out->type == VT_SHORT_INT ? (int)out->v.short_int : (int)out->v.long_int);
	if(!media)
		return MME_LOOKUP_FAILED;

	MMSValue_SetEnum(out, VT_WK_MEDIA, media);

	return MME_NONE;
}

/*
 * Typed-parameter = Well-known-parameter-token Typed-value
 * ; the actual expected type of the value is implied by the well-known parameter
 * Well-known-parameter-token = Integer-value
 * ; the code values used for parameters are specified in the Assigned Numbers appendix
 * Typed-value = Compact-value | Text-value
 * ; In addition to the expected type, there may be no value.
 * ; If the value cannot be encoded using the expected type, it shall be encoded as text.
 * Compact-value = Integer-value |
 * Date-value | Delta-seconds-value | Q-value | Version-value |
 * Uri-value
 */
MMSError MMS_DecodeTypedParameter(CDBUFFER stream, MMSVALUE out)
{
	MMSError error;
	TypedParam p;
	MMSValue v;
	MMSFIELDINFO fi;

	error = MMS_DecodeInteger(stream, &v);
	if(error != MME_NONE)
		return error;

	fi = MMSWellKnownParams_FindByID(v.v.short_int);

}

/*
 * Untyped-parameter = Token-text Untyped-value
 * ; the type of the value is unknown, but it shall be encoded as an integer, if that is possible.
 * Untyped-value = Integer-value | Text-value
 */
MMSError MMS_DecodeUntypedParameter(CDBUFFER stream, MMSVALUE out)
{
	MMSError error;
	UntypedParam p;
	MMSValue v;

	error = MMS_DecodeTokenText(stream, &v);
	if(error != MME_NONE)
		return error;

	assert(v.type == VT_TEXT);
	p.token_text = v.v.str;

	error = MMS_DecodeInteger(stream, &v);
	if(error != MME_NONE)
		error = MMS_DecodeText(stream, &v);

	if(error != MME_NONE)
		return error;

	p.value_type = v.type;

	return error;
}

/*
 * Parameter = Typed-parameter | Untyped-parameter
 */
MMSError MMS_DecodeParameters(CDBUFFER stream, CPTR end, MMSVALUE out)
{
	MMSValue v;
	MMSError error = MME_INVALID_DATA;

	while(DynaBufPtr(stream) != end) {
		error = MMS_DecodeTypedParameter(stream, &v);
		if(error != MME_NONE)
			error = MMS_DecodeUntypedParameter(stream, &v);

		if(error != MME_NONE)
			return error;

		// TODO: add parameter v to out
	}

	return error;
}



/*
 * Media-type = (Well-known-media | Extension-Media) *(Parameter)
 */
MMSError MMS_DecodeMediaType(CDBUFFER stream, CPTR end, MMSVALUE out)
{
	MMSError error;

	error = MMS_DecodeWellKnownMedia(stream, out);
	if(error != MME_NONE)
		error = MMS_DecodeExtensionMedia(stream, out);

	if(error != MME_NONE)
		return error;

	return MME_NONE;
	// TODO: complete parameter decoding
	// return MMS_DecodeParameters(stream, end, out);
}

MMSError MMS_DecodeContentGeneralForm(CDBUFFER stream, MMSVALUE out)
{
	MMSValue v;
	MMSError error = MME_NONE;
	ssize_t len = DecodeValueLength(stream);
	if(len == -1)
		return MME_BADLENGTH;

	// TODO: Complete CT decoding
	error = MMS_DecodeMediaType(stream, DynaBufPtr(stream) + len, /*&v*/out);

	return error;
}

/*
 * Content-type-value = Constrained-media | Content-general-form
 * Content-general-form = Value-length Media-type
 * Media-type = (Well-known-media | Extension-Media) *(Parameter)
 * Constrained-media = Constrained-encoding
 */
MMSError MMS_ContentTypeDecode(CDBUFFER stream, MMSVALUE out)
{
	if(MMS_DecodeConstrainedEncoding(stream, out) != MME_NONE)
		return MMS_DecodeContentGeneralForm(stream, out);

	return MME_NONE;
}

MMSError _EnumLookup(CDBUFFER stream, MMSVALUE out, MMSValueType vt, MMSVALUEENUM tbl, int count);

MMSValueEnum MMSYesNoEnum[] = {
	{ "Yes", 128 },
	{ "No", 129 },
};
size_t MMSYesNoEnumSize = sizeof(MMSYesNoEnum) / sizeof(MMSYesNoEnum[0]);

MMSError MMS_YesNoDecode(CDBUFFER stream, MMSVALUE out)
{
	return _EnumLookup(stream, out, VT_YESNO,
	                   MMSYesNoEnum, MMSYesNoEnumSize);
}

MMSValueEnum MMSPriorityEnum[] = {
	{"Low", 128},
	{"Normal", 129},
	{"High", 130},
};
size_t MMSPriorityEnumSize = sizeof(MMSPriorityEnum) / sizeof(MMSPriorityEnum[0]);

MMSError MMS_PriorityDecode(CDBUFFER stream, MMSVALUE out)
{
	return _EnumLookup(stream, out, VT_PRIORITY,
	                   MMSPriorityEnum, MMSPriorityEnumSize);
}

MMSValueEnum MMSMessageClassEnum[] = {
	{"Personal", 128},
	{"Advertisement", 129},
	{"Informational", 130},
	{"Auto", 131},
};
size_t MMSMessageClassEnumSize = sizeof(MMSMessageClassEnum) / sizeof(MMSMessageClassEnum[0]);

MMSError MMS_MessageClassDecode(CDBUFFER stream, MMSVALUE out)
{
	return _EnumLookup(stream, out, VT_MESSAGE_CLASS,
	                   MMSMessageClassEnum, MMSMessageClassEnumSize);
}

MMSValueEnum MMSMessageTypeEnum[] = {
	{"m-send-req", 128},
	{"m-send-conf", 129},
	{"m-notification-ind", 130},
	{"m-notifyresp-ind", 131},
	{"m-retrieve-conf", 132},
	{"m-acknowledge-ind", 133},
	{"m-delivery-ind", 134},
};
size_t MMSMessageTypeEnumSize = sizeof(MMSMessageTypeEnum) / sizeof(MMSMessageTypeEnum[0]);

MMSError MMS_MessageTypeDecode(CDBUFFER stream, MMSVALUE out)
{
	return _EnumLookup(stream, out, VT_MESSAGE_TYPE,
	                   MMSMessageTypeEnum, MMSMessageTypeEnumSize);
}

