#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#include "log.h"
#include "streambuffer.h"
#include "mms-service.h"
#include "mms-data.h"
#include "mms-encoders.h"

MMSHEADER MMS_CreateHeader(SBUFFER stream, MMSHEADERS headers, MMSFieldKind kind, MMSFIELDINFO fi)
{
	MMSError error;
	MMSValue value;
	MMSHEADER header = NULL;

	MMSValue_Init(&value);

	error = MMS_DecodeFieldValue(stream, fi, &value);
	if(error != MMS_ERR_NONE)
		return NULL;

	header = MMSHeaders_NewHeader(headers);
	if(!header) {
		puts("failed to create new header");
		return NULL;
	}

	header->id.kind = kind;
	header->id.info = fi;
	header->value = value;

	return header;
}

int MMS_MapEncodedHeaders(SBUFFER stream, MMSHEADERS headers)
{
	MMSValue fid;
	MMSFIELDINFO fi = NULL;
	MMSFieldKind kind = MMS_HEADER;

	while(MMS_DecodeShortInteger(stream, &fid) == MMS_ERR_NONE) {
		fi = MMSFields_FindByID(fid.v.short_int);
		if(!fi) {
			fi = WSPFields_FindByID(fid.v.short_int);
			kind = WSP_HEADER;
		}

		if (!fi) {
			printf("Unknown field (0x%X)\n", fid.v.short_int);
			return -1;
		}

		MMS_CreateHeader(stream, headers, kind, fi);
		// stop mapping when we have content-type
		if(fi->code == MMS_CONTENT_TYPE)
			return 0;
	}

	return 0;
}

MMSPART MMS_CreatePart(SBUFFER stream, MMSPARTS parts)
{
	MMSError error;
	size_t headers_len = MMS_DecodeUintVar(stream);
	size_t data_len = MMS_DecodeUintVar(stream);
	MMSValue v;

	long mark = SBOffset(stream);
	const char *headers_end = SBPtr(stream) + headers_len;

	error = MMS_DecodeContentType(stream, &v);
	if(error != MMS_ERR_NONE)
		return NULL;

	MMSPART part = MMSParts_NewPart(parts);
	if(!part)
		return NULL;

	MMSHEADER header = MMSHeaders_NewHeader(part->headers);
	header->id.kind = MMS_HEADER;
	header->id.info = MMSFields_FindByID(MMS_CONTENT_TYPE);
	header->value = v;

	// FIXME: Skip part headers until enough parameters/fields are supported for success
//	if(SBPtr(stream) < headers_end)
//		MMS_MapEncodedHeaders(stream, part->headers);

	// NOTE: shouldn't be needed if all headers parsed
	if(SBPtr(stream) != headers_end)
		SB_Seek(stream, mark + (int)headers_len, SEEK_SET);

	assert(SBPtr(stream) == headers_end);

	part->data_len = data_len;
	part->data = SBPtr(stream);

	SB_Seek(stream, data_len, SEEK_CUR);

	return part;
}

MMSError MMS_MapEncodedParts(SBUFFER stream, MMSPARTS parts)
{
	size_t num_parts = MMS_DecodeUintVar(stream);

	for(size_t i = 0; i < num_parts; i++)
		MMS_CreatePart(stream, parts);

	return MMS_ERR_NONE;
}

MMSError MMS_MapEncodedMessage(GSM_SMSDConfig *Config, SBUFFER Stream, MMSMESSAGE *out)
{
	MMSError error;
	MMSHEADERS headers;
	MMSMESSAGE m = MMSMessage_Init();
	if(!m)
		return MMS_ERR_MEMORY;

	error = MMS_MapEncodedHeaders(Stream, m->Headers);
	if(error != MMS_ERR_NONE) {
		MMSMessage_Destroy(&m);
		return error;
	}

	headers = m->Headers;
	MMSHEADER h = MMSHeader_FindByID(headers, MMS_HEADER, MMS_MESSAGE_TYPE);
	if(!h) {
		MMSMessage_Destroy(&m);
		return MMS_ERR_REQUIRED_FIELD;
	}

	m->MessageType = h->value.v.enum_v;

	h = MMSHeader_FindByID(headers, MMS_HEADER, MMS_MMS_VERSION);
	if(!h) {
		MMSMessage_Destroy(&m);
		return MMS_ERR_REQUIRED_FIELD;
	}

	m->Version = h->value.v.short_int;

	error = MMS_MapEncodedParts(Stream, m->Parts);
	if(error != MMS_ERR_NONE) {
		MMSMessage_Destroy(&m);
		return error;
	}

	*out = m;

	return MMS_ERR_NONE;
}

void MMS_DumpHeaders(SBUFFER buffer, MMSHEADERS headers)
{
	for(size_t i = 0; i != headers->end; i++) {
		if(headers->entries[i].id.kind != MMS_HEADER)
			continue;

		MMSHeader_AsString(buffer, &headers->entries[i]);
	}
}

void SaveSBufferToTempFile(GSM_SMSDConfig *Config, SBUFFER Buffer)
{
	char fname[] = "/tmp/GMMS_XXXXXX";
	int fd = mkstemp(fname);
	if(fd == -1) {
		SMSD_LogErrno(Config, "Could not create MMS file");
		return;
	}

	SMSD_Log(DEBUG_INFO, Config, "Saving MMS Buffer to: %s", fname);

	ssize_t written = write(fd, SBBase(Buffer), SBUsed(Buffer));
	close(fd);

	if((size_t)written != SBUsed(Buffer))
		SMSD_Log(DEBUG_ERROR, Config,
		         "Failed to flush buffer to file: BufferSize(%zd), BytesWritten(%zu)", SBUsed(Buffer), written);
}

void MMS_ContentTypeAsString(SBUFFER buffer, MMSContentType *ct)
{
	assert(ct);
	switch(ct->vt) {
		default:
			SB_PutString(buffer, "<Unrecognized>");
			break;
		case VT_WK_MEDIA:
			SB_PutString(buffer, ct->v.wk_media->name);
			break;
		case VT_TEXT:
			SB_PutString(buffer, ct->v.ext_media);
			break;
	}

	// Note: Avoid parameters that may cause SQL insert to fail until sanitization/escaping implemented
	// TODO: Add untyped parameters
	if(ct->params.count) {
		MMSPARAMETERS params = &ct->params;
		SB_PutString(buffer, " ");
		for(int i = 0; i < params->count; i++) {
			MMSPARAMETER p = &params->entries[i];
			if (p->kind == MMS_PARAM_TYPED) {
				SB_PutFormattedString(buffer, "%s=", p->v.typed.type->name);
				MMSValue_AsString(buffer, &p->v.typed.value);
				SB_PutString(buffer, ", ");
			}
		}
		SB_Truncate(buffer, 2);
	}
}

LocalTXID MMS_CreateTransactionID(void)
{
	LocalTXID txid;
	int fd = open("/dev/urandom", O_RDONLY);
	size_t bytes_read = read(fd, &txid, sizeof(LocalTXID));
	if(bytes_read != sizeof(LocalTXID))
		return -1;

	return txid;
}

ssize_t MMS_NextToken(CSTR str, size_t end, size_t offset, char token)
{
	while(offset < end) {
		if(str[offset] == token)
			break;
		offset++;
	}

	return offset >= end ? -1 : (ssize_t)offset + 1;
}

MMSError MMS_ParseHeaders(MMSHEADERS headers, CSTR extra_headers)
{
	assert(headers);
	assert(extra_headers);
	char buf[1024];
	MMSError error;
	MMSHEADER h;

	const size_t end = strlen(extra_headers);
	size_t begin = 0;
	ssize_t eol = MMS_NextToken(extra_headers, end, 0, '\n');
	while(eol > 0) {
		ssize_t split = MMS_NextToken(extra_headers, eol, begin, '=');
		if(split > 0) {
			size_t len = split - begin - 1;
			memcpy(buf, &extra_headers[begin], len);
			buf[len] = 0;
			MMSFIELDINFO fi = MMSFields_FindByName(buf);
			if(!fi)
				return MMS_ERR_LOOKUP_FAILED;

			h = MMSHeaders_NewHeader(headers);
			if(!h)
				return MMS_ERR_MEMORY;

			h->id.kind = MMS_HEADER;
			h->id.info = fi;

			memset(buf, 0xff, sizeof(buf));
			len = eol - split - 1;
			memcpy(buf, &extra_headers[split], len);
			buf[len] = 0;

			error = MMSValue_SetFromString(&h->value, fi, buf);
			if(error != MMS_ERR_NONE)
				return error;
		}

		begin = eol;
		eol = MMS_NextToken(extra_headers, end, eol, '\n');
	}

	return MMS_ERR_NONE;
}

MMSError MMS_ParseMediaType(CSTR mime, MMSCONTENTTYPE out)
{
	assert(mime);
	assert(out);

	MMSError e;
	const size_t end = strlen(mime);
	ssize_t pos = strchr(mime, ' ') - mime + 1;
	if(pos < 0)
		pos = end;

	MMSVALUEENUM wk = MMS_WkContentType_FindByName(mime);
	if(!wk)
		return MMS_ERR_UNKNOWN_MIME;

	// TODO: support extension media

	out->vt = VT_WK_MEDIA;
	out->v.wk_media = wk;

	if((size_t)pos == end)
		return MMS_ERR_NONE;

	MMSPARAMETERS params = &out->params;
	if(!params->entries) {
		params->entries = malloc(MMS_MAX_PARAMS * sizeof(MMSParameter));
		if (!params->entries)
			return MMS_ERR_MEMORY;

		memset(params->entries, 0, MMS_MAX_PARAMS * sizeof(MMSParameter));
		params->count = 0;
	}

	ssize_t next = MMS_NextToken(mime, end, pos, ',');

	while(next > 0) {
		MMSPARAMETER p = &params->entries[params->count];
		e = MMS_ParseParameter(p, &mime[pos], next);
		if(e != MMS_ERR_NONE)
			return e;

		params->count++;
		next = MMS_NextToken(mime, end, pos, ',');
	}

	MMSPARAMETER p = &params->entries[params->count];
	e = MMS_ParseParameter(p, &mime[pos], end);
	if(e != MMS_ERR_NONE)
		return e;

	params->count++;

	return MMS_ERR_NONE;
}

MMSError MMS_ParseParameter(MMSPARAMETER p, CSTR param, ssize_t end)
{
	assert(p);
	assert(param);
	char buf[1024];

	ssize_t split = MMS_NextToken(param, end, 0, '=');
	if(split == -1)
		return MMS_ERR_INVALID_PARAM;

	memcpy(buf, param, split - 1);
	buf[split] = 0;
	MMSFIELDINFO fi = MMS_WkParam_FindByName(buf);
	if(!fi) // TODO: support untyped parameters
		return MMS_ERR_INVALID_PARAM;

	p->kind = MMS_PARAM_TYPED;
	memcpy(buf, param + split, end);
	buf[end + 1] = 0;

	switch(fi->vt) {
		default:
			return MMS_ERR_UNSUPPORTED;
		case VT_WK_CHARSET: {
			MMSVALUEENUM ch = MMS_Charset_FindByName(buf);
			if(!ch)
				return MMS_ERR_UNSUPPORTED;

			p->v.typed.type = fi;
			p->v.typed.value.type = VT_WK_CHARSET;
			p->v.typed.value.v.enum_v = ch;
		}
		break;
	}

	return MMS_ERR_NONE;
}
