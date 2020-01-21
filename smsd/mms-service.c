#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "log.h"
#include "streambuffer.h"
#include "mms-service.h"
#include "mms-data.h"

MMSHEADER MMS_CreateHeader(SBUFFER stream, MMSHEADERS headers, MMSFieldKind kind, MMSFIELDINFO fi)
{
	MMSError error;
	MMSValue value;
	MMSHEADER header = NULL;

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
		if(fi->id == MMS_CONTENT_TYPE)
			return 0;
	}

	return 0;
}

MMSError MMS_MapEncodedParts(SBUFFER stream, MMSPARTS parts)
{
	size_t num_parts = MMS_DecodeUintVar(stream);

	for(int i = 0; i < num_parts; i++)
		MMS_CreatePart(stream, parts);

	return MMS_ERR_NONE;
}


MMSError MMS_MapEncodedMessage(GSM_SMSDConfig *Config, SBUFFER Stream, MMSMESSAGE *out)
{
	MMSError error;
	MMSHEADERS headers;
	MMSMESSAGE msg = MMSMessage_Init();
	if(!msg)
		return MMS_ERR_MEMORY;

	error = MMS_MapEncodedHeaders(Stream, msg->Headers);
	if(error != MMS_ERR_NONE) {
		MMSMessage_Destroy(&msg);
		return error;
	}

	headers = msg->Headers;
	MMSHEADER h = MMSHeader_FindByID(headers, MMS_HEADER, MMS_MESSAGE_TYPE);
	assert(h && h->value.type == VT_MESSAGE_TYPE);
	msg->MessageType = h->value.v.enum_v;

	error = MMS_MapEncodedParts(Stream, msg->Parts);
	if(error != MMS_ERR_NONE) {
		MMSMessage_Destroy(&msg);
		return error;
	}

	*out = msg;

	return MMS_ERR_NONE;
}

void MMS_DumpHeaders(SBUFFER buffer, MMSHEADERS headers)
{
	for(size_t i = 0; i < headers->end; i++) {
		if(headers->entries[i].id.kind != MMS_HEADER)
			continue;

		const char *name = headers->entries[i].id.info->name;
		const char *value = MMSValue_AsString(headers->entries[i].id.info->id, &headers->entries[i].value);
		SB_PutFormattedString(buffer, "%s=%s\n", name, value);
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

	// FIXME: Avoid parameters that may cause SQL insert to fail until sanitization implemented
	if(ct->params.count) {
		MMSPARAMETERS params = &ct->params;
		SB_PutString(buffer, " ");
		for(int i = 0; i < params->count; i++) {
			MMSPARAMETER p = &params->entries[i];
			if (p->kind == MMS_PARAM_TYPED)
				SB_PutFormattedString(buffer, "%s=%s, ", p->v.typed.type->name, MMSValue_AsString(-1, &p->v.typed.value));
//			SB_PutFormattedString(buffer, "%s=%s, ",
//			  p->entries[i].kind == MMS_PARAM_TYPED ? p->entries[i].v.typed.type->name
//			  : p->entries[i].v.untyped.token_text,
//			  p->entries[i].kind == MMS_PARAM_TYPED ? MMSValue_AsString(-1, &p->entries[i].v.typed.value)
//			  : p->entries->v.untyped.v.text);
		}
	}
}
