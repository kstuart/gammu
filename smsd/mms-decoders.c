#include <assert.h>

#include "streambuffer.h"
#include "mms-data.h"
#include "mms-tables.h"
#include <malloc.h>
#include <string.h>

MMSError MMS_DecodeFieldValue(SBUFFER stream, MMSFIELDINFO fi, MMSValue *out);

size_t MMS_DecodeUintVar(SBUFFER stream)
{
	size_t uint = 0;
	uint8_t byte = SB_NextByte(stream);
	while(byte >> 7u) {
		uint <<= 7u;
		uint |= byte & 0x7fu;
		byte = SB_NextByte(stream);
	}
	uint <<= 7u;
	uint |= byte & 0x7fu;

	return uint;
}

ssize_t DecodeValueLength(SBUFFER stream)
{
	size_t len = SB_PeekByte(stream);
	if(len > 31)
		return -1;
	SB_NextByte(stream);

	if(len < 31)
		return len;

	return MMS_DecodeUintVar(stream);
}

MMSError MMS_DecodeQValue(SBUFFER stream, MMSVALUE out)
{
	size_t ui = MMS_DecodeUintVar(stream);
	if (ui > 1100)
		ui -= 100;
	else
		ui *= 10;

	ui /= 1000.0f;
	out->allocated = 0;
	out->type = VT_QVALUE;
	out->v.float_v = ui;
	return MMS_ERR_NONE;
}

MMSError MMS_DecodeShortInteger(SBUFFER stream, MMSVALUE out)
{
	MMSShortInt v = SB_PeekByte(stream);
	if((v & 0x80u) == 0)
		return MMS_ERR_INVALID_DATA;

	out->allocated = 0;
	out->type = VT_SHORT_INT;
	out->v.short_int = (SB_NextByte(stream) & 0x7fu);
	return MMS_ERR_NONE;
}

MMSError MMS_DecodeLongInteger(SBUFFER stream, MMSVALUE out)
{
	int len = SB_PeekByte(stream);
	if(len > 30)
		return MMS_ERR_BADLENGTH;

	SB_NextByte(stream);
//	assert(len <= 4);

	MMSLongInt n = 0;
	for(int i = 0; i < len; i++) {
		n <<= 8u;
		n |= SB_NextByte(stream);
	}

	out->allocated = 0;
	out->type = VT_LONG_INT;
	out->v.long_int = n;
	return MMS_ERR_NONE;
}

MMSError MMS_DecodeInteger(SBUFFER stream, MMSVALUE out)
{
	if(MMS_DecodeShortInteger(stream, out) != MMS_ERR_NONE)
		return MMS_DecodeLongInteger(stream, out);

	return MMS_ERR_NONE;
}

// Expiry-value =
//  Value-length (Absolute-token Date-value | Relative-token Delta-seconds-value)
MMSError MMS_DecodeExpiry(SBUFFER stream, MMSVALUE out)
{
	MMSError error;

	ssize_t len = DecodeValueLength(stream);
	if(len == -1)
		return MMS_ERR_INVALID_DATA;

	MMSExpiryToken tok = SB_PeekByte(stream);
	if(!(tok == DTIME_ABSOLUTE || tok == DTIME_RELATIVE))
		return MMS_ERR_INVALID_DATA;

	SB_NextByte(stream);
	error = MMS_DecodeLongInteger(stream, out);
	if(error != MMS_ERR_NONE)
		return error;

	out->allocated = 0;
	out->type = VT_EXPIRY;
	out->v.expiry.dtime = out->v.long_int;
	out->v.expiry.token = tok;

	return MMS_ERR_NONE;
}


MMSVALUEENUM DecodeWellKnownCharset(SBUFFER stream)
{
	if(SB_PeekByte(stream) == CHARSET_ANY) {
		SB_NextByte(stream);
		return MMS_Charset_FindByID(0);
	}

	MMSValue v;
	MMS_DecodeInteger(stream, &v);

	MMSVALUEENUM e = MMS_Charset_FindByID(
		v.type == VT_SHORT_INT ? (int) v.v.short_int : (int) v.v.long_int);

	return e;
}

MMSError MMS_DecodeQuoteText(SBUFFER stream, MMSVALUE out)
{
	if(SB_PeekByte(stream) == TEXT_QUOTE)
		SB_NextByte(stream);

	out->allocated = 0;
	out->type = VT_TEXT;
	out->v.str = SBPtr(stream);
	SB_Seek(stream, SB_FindNext(stream, 0) + 1, SEEK_CUR);

	return MMS_ERR_NONE;
}

MMSError MMS_DecodeText(SBUFFER stream, MMSVALUE out)
{
	BYTE b = SB_PeekByte(stream);
	if(b < 32)
		return MMS_ERR_INVALID_DATA;

	return MMS_DecodeQuoteText(stream, out);
}

// Encoded-string-value = Text -string | Value-length Char-set Text -string
MMSError MMS_DecodeEncodedText(SBUFFER stream, MMSVALUE out)
{
	MMSError error = MMS_DecodeText(stream, out);
	if(error == MMS_ERR_NONE)
		return MMS_ERR_NONE;

	ssize_t vlsize = SBOffset(stream);
	ssize_t len = DecodeValueLength(stream);
	vlsize = SBOffset(stream) - vlsize;
	if(len == -1)
		return MMS_DecodeQuoteText(stream, out);

	MMSVALUEENUM charset = DecodeWellKnownCharset(stream);
	if(!charset)
		return MMS_ERR_INVALID_DATA;

	if(charset->code == CHARSET_ASCII) {
		MMS_DecodeQuoteText(stream, out);
		out->v.encoded_string.text = out->v.str;
	}
	else {
		out->v.encoded_string.text = SBPtr(stream);
		SB_Seek(stream, len - vlsize, SEEK_CUR);
	}

	out->allocated = 0;
	out->type = VT_ENCODED_STRING;
	out->v.encoded_string.charset = charset;

	return MMS_ERR_NONE;
}

MMSError MMS_DecodeAddress(SBUFFER stream, MMSVALUE out)
{
	MMSError e = MMS_DecodeEncodedText(stream, out);
	if(e != MMS_ERR_NONE)
		return e;

	return MMS_ERR_NONE;
}

MMSError MMS_DecodeFromAddress(SBUFFER stream, MMSVALUE out)
{
	ssize_t len = DecodeValueLength(stream);
	if(len == -1)
		return MMS_ERR_BADLENGTH;

	MMSFromAddressToken tok = SB_NextByte(stream);
	if(tok != TOK_ADDRESS_PRESENT) {
		out->allocated = 0;
		out->type = VT_FROM;
		out->v.str = (char*)"";
		return MMS_ERR_NONE;
	}

	MMSError e = MMS_DecodeAddress(stream, out);
	if(e != MMS_ERR_NONE)
		return e;

	out->type = VT_FROM;
	return MMS_ERR_NONE;
}

/*
 * Token-text = Token End-of-string
 */
MMSError MMS_DecodeTokenText(SBUFFER stream, MMSVALUE out)
{
	return MMS_DecodeText(stream, out);
}


/*
 * Extension-media = *TEXT End-of-string
 */
MMSError MMS_DecodeExtensionMedia(SBUFFER stream, MMSVALUE out)
{
	return MMS_DecodeText(stream, out);
}

/* Constrained-encoding = Extension-Media | Short-integer
 * Extension-media = *TEXT End-of-string
 */
MMSError MMS_DecodeConstrainedEncoding(SBUFFER stream, MMSVALUE out)
{
	MMSError error = MMS_DecodeShortInteger(stream, out);
	if(error != MMS_ERR_NONE)
		return MMS_DecodeExtensionMedia(stream, out);

	MMSVALUEENUM ct = MMS_WkContentType_FindByID(out->v.short_int);
	if(ct) {
		MMSValue_SetEnum(out, VT_WK_MEDIA, ct);
		return MMS_ERR_NONE;
	}

	return MMS_ERR_LOOKUP_FAILED;
}

/* Well-known-media = Integer-value */
MMSError MMS_DecodeWellKnownMedia(SBUFFER stream, MMSVALUE out)
{
	MMSError error;
	error = MMS_DecodeInteger(stream, out);
	if(error != MMS_ERR_NONE)
		return error;

	MMSVALUEENUM media = MMS_WkContentType_FindByID(out->type == VT_SHORT_INT ? (int)out->v.short_int : (int)out->v.long_int);
	if(!media)
		return MMS_ERR_LOOKUP_FAILED;

	MMSValue_SetEnum(out, VT_WK_MEDIA, media);

	return MMS_ERR_NONE;
}

typedef MMSVALUEENUM (*MMS_FindByIDFn)(int id);

static MMSError _EncodedQuickFindByID(SBUFFER stream, MMSValueType vt, MMSVALUE out, MMS_FindByIDFn fn)
{
	MMSShortInt id = SB_PeekByte(stream);
	if(!(id & 0x80u))
		return MMS_ERR_INVALID_DATA;

	MMSVALUEENUM entry = fn(id);
	if(!entry)
		return MMS_ERR_LOOKUP_FAILED;

	SB_NextByte(stream);
	return MMSValue_SetEnum(out, vt, entry);
}

MMSError MMS_DecodeYesNo(SBUFFER stream, MMSVALUE out)
{
	return _EncodedQuickFindByID(stream, VT_YESNO, out, MMS_YesNo_FindByID);
}

MMSError MMS_DecodePriority(SBUFFER stream, MMSVALUE out)
{
	return _EncodedQuickFindByID(stream, VT_PRIORITY, out, MMS_Priority_FindByID);
}

MMSError MMS_DecodeMessageClass(SBUFFER stream, MMSVALUE out)
{
	return _EncodedQuickFindByID(stream, VT_MESSAGE_CLASS, out, MMS_MessageClass_FindByID);
}

MMSError MMS_DecodeMessageType(SBUFFER stream, MMSVALUE out)
{
	return _EncodedQuickFindByID(stream, VT_MESSAGE_TYPE, out, MMS_MessageType_FindByID);
}

MMSError MMS_DecodeWellKnownCharset(SBUFFER stream, MMSVALUE out)
{
	MMSError error = MMS_DecodeInteger(stream, out);
	if(error != MMS_ERR_NONE)
		return error;

	MMSVALUEENUM charset = MMS_Charset_FindByID(
		out->type == VT_SHORT_INT ? out->v.short_int : out->v.long_int);
	if(!charset)
		return MMS_ERR_INVALID_DATA;
	MMSValue_SetEnum(out, VT_WK_CHARSET, charset);

	return MMS_ERR_NONE;
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
MMSError MMS_DecodeTypedParameter(SBUFFER stream, MMSPARAMETER param)
{
	assert(param);

	MMSError error;
	MMSValue v;

	error = MMS_DecodeInteger(stream, &v);
	if(error != MMS_ERR_NONE)
		return error;

	MMSFIELDINFO fi = MMS_WkParams_FindByID(v.v.short_int);
	if(!fi)
		return MMS_ERR_INVALID_PARAM;

	error = MMS_DecodeFieldValue(stream, fi, &v);
	if(error != MMS_ERR_NONE)
		return error;

	param->kind = MMS_PARAM_TYPED;
	param->v.typed.type = fi;
	param->v.typed.value = v;

	return MMS_ERR_NONE;
}

/*
 * Untyped-parameter = Token-text Untyped-value
 * ; the type of the value is unknown, but it shall be encoded as an integer, if that is possible.
 * Untyped-value = Integer-value | Text-value
 */
MMSError MMS_DecodeUntypedParameter(SBUFFER stream, MMSPARAMETER param)
{
	assert(param);

	MMSError error;
	STR token_text;
	MMSValue v;

	error = MMS_DecodeTokenText(stream, &v);
	if(error != MMS_ERR_NONE)
		return error;

	assert(v.type == VT_TEXT);
	token_text = v.v.str;

	error = MMS_DecodeInteger(stream, &v);
	if(error != MMS_ERR_NONE)
		error = MMS_DecodeText(stream, &v);

	if(error != MMS_ERR_NONE)
		return error;

	param->kind = MMS_PARAM_UNTYPED;
	param->v.untyped.token_text = token_text;
	param->v.untyped.value_type = v.type;

	return MMS_ERR_NONE;
}

/*
 * Parameter = Typed-parameter | Untyped-parameter
 */
MMSError MMS_DecodeParameters(SBUFFER stream, CPTR end, MMSPARAMETERS out)
{
	MMSError error = MMS_ERR_INVALID_DATA;
	MMSParameters params;

	assert(!out->entries);

	if(SBPtr(stream) == (char*)end)
		return MMS_ERR_NONE;

	params.entries = malloc(MMS_MAX_PARAMS * sizeof(MMSParameter));
	if(!params.entries)
		return MMS_ERR_MEMORY;

	memset(params.entries, 0, MMS_MAX_PARAMS * sizeof(MMSParameter));

	params.count = 0;
	while(SBPtr(stream) < (STR)end) {
		error = MMS_DecodeTypedParameter(stream, &params.entries[params.count]);
		if(error != MMS_ERR_NONE)
			error = MMS_DecodeUntypedParameter(stream, &params.entries[params.count]);

		if(error != MMS_ERR_NONE) {
			free(params.entries);
			return error;
		}

		params.count++;
	}

	*out = params;

	return error;
}

/*
 * Media-type = (Well-known-media | Extension-Media) *(Parameter)
 */
MMSError MMS_DecodeMediaType(SBUFFER stream, MMSVALUE out)
{
	if(MMS_DecodeWellKnownMedia(stream, out) != MMS_ERR_NONE)
		return MMS_DecodeExtensionMedia(stream, out);

	return MMS_ERR_NONE;
}

MMSError MMS_DecodeContentGeneralForm(SBUFFER stream, MMSVALUE out)
{
	assert(out);

	MMSValue v;
	MMSContentType *ct = &out->v.content_type;
	MMSError error;

	ssize_t len = DecodeValueLength(stream);
	if(len == -1)
		return MMS_ERR_BADLENGTH;

	error = MMS_DecodeMediaType(stream, &v);
	if(error != MMS_ERR_NONE)
		return error;

	error = MMS_DecodeParameters(stream, SBPtr(stream) + len - 1, &ct->params);
	if(error != MMS_ERR_NONE)
		return error;

	out->allocated = 0;
	out->type = VT_CONTENT_TYPE;

	ct->vt = v.type;
	switch(v.type) {
		default:
			ct->vt = VT_UNSUPPORTED;
			break;
		case VT_WK_MEDIA:
			ct->v.wk_media = v.v.enum_v;
			break;
		case VT_TEXT:
			ct->v.ext_media = v.v.str;
			break;
	}

	return error;
}

/*
 * Content-type-value = Constrained-media | Content-general-form
 * Content-general-form = Value-length Media-type
 * Media-type = (Well-known-media | Extension-Media) *(Parameter)
 * Constrained-media = Constrained-encoding
 */
MMSError MMS_DecodeContentType(SBUFFER stream, MMSVALUE out)
{
	MMSError error;
	MMSValue v;

	memset(out, 0, sizeof(MMSValue));

	error = MMS_DecodeConstrainedEncoding(stream, &v);
	if(error != MMS_ERR_NONE)
		return MMS_DecodeContentGeneralForm(stream, out);

	out->allocated = 0;
	out->type = VT_CONTENT_TYPE;
	out->v.content_type.vt = v.type;

	if(v.type == VT_WK_MEDIA)
		out->v.content_type.v.wk_media = v.v.enum_v;
	else
		out->v.content_type.v.ext_media = v.v.str;

	return error;
}

MMSError MMS_DecodeFieldValue(SBUFFER stream, MMSFIELDINFO fi, MMSVALUE out)
{
	assert(stream);
	assert(fi);
	assert(out);

	switch(fi->vt) {
		default:
			printf("unsupported codec (%d, %s)\n", fi->code, fi->name);
			return MMS_ERR_NOCODEC;
		case VT_SHORT_INT:
			MMS_DecodeShortInteger(stream, out);
			switch(fi->code) {
				default:
					break;
				case MMS_MMS_VERSION:
					out->type = VT_VERSION;
					break;
			}
			break;
		case VT_MESSAGE_TYPE:
			MMS_DecodeMessageType(stream, out);
			break;
		case VT_TEXT:
			MMS_DecodeQuoteText(stream, out);
			break;
		case VT_LONG_INT:
			MMS_DecodeLongInteger(stream, out);
			break;
		case VT_FROM:
			MMS_DecodeFromAddress(stream, out);
			break;
		case VT_ENCODED_STRING:
			MMS_DecodeEncodedText(stream, out);
			switch(fi->code) {
				default:
					break;
				case MMS_FROM:
					out->type = VT_FROM;
					break;
				case MMS_TO:
				case MMS_BCC:
				case MMS_CC:
					out->type = VT_ADDRESS;
					break;
			}
			break;
		case VT_MESSAGE_CLASS:
			MMS_DecodeMessageClass(stream, out);
			break;
		case VT_PRIORITY:
			MMS_DecodePriority(stream, out);
			break;
		case VT_YESNO:
			MMS_DecodeYesNo(stream, out);
			break;
		case VT_CONTENT_TYPE:
			MMS_DecodeContentType(stream, out);
			break;
		case VT_WK_CHARSET:
			MMS_DecodeWellKnownCharset(stream, out);
			break;
		case VT_CONSTRAINED:
			MMS_DecodeConstrainedEncoding(stream, out);
			break;
		case VT_QVALUE:
			MMS_DecodeQValue(stream, out);
			break;
		case VT_EXPIRY:
			MMS_DecodeExpiry(stream, out);
	}

	return MMS_ERR_NONE;
}
