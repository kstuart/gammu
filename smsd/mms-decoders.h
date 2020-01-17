#ifndef GAMMU_MMS_DECODERS_H
#define GAMMU_MMS_DECODERS_H

#include <stdio.h>
#include "mms-data.h"

typedef struct _DynamicBuffer *CDBUFFER;
typedef struct _MMSValue *MMSVALUE;
typedef const char *CPTR;

size_t MMS_DecodeUintVar(CDBUFFER stream);
ssize_t DecodeValueLength(CDBUFFER stream);
MMSError MMS_DecodeShortInteger(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeLongInteger(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeInteger(CDBUFFER stream, MMSVALUE out);
MMSVALUEENUM DecodeWellKnownCharset(CDBUFFER stream);
MMSError MMS_DecodeEncodedText(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeFromAddress(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeText(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeQuoteText(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeTokenText(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeExtensionMedia(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeConstrainedEncoding(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeWellKnownMedia(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeTypedParameter(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeUntypedParameter(CDBUFFER stream, MMSVALUE out);
MMSError MMS_DecodeParameters(CDBUFFER stream, CPTR end, MMSVALUE out);
MMSError MMS_DecodeMediaType(CDBUFFER stream, CPTR end, MMSVALUE out);
MMSError MMS_DecodeContentGeneralForm(CDBUFFER stream, MMSVALUE out);
MMSError MMS_ContentTypeDecode(CDBUFFER stream, MMSVALUE out);
MMSError MMS_YesNoDecode(CDBUFFER stream, MMSVALUE out);
MMSError MMS_PriorityDecode(CDBUFFER stream, MMSVALUE out);
MMSError MMS_MessageClassDecode(CDBUFFER stream, MMSVALUE out);
MMSError MMS_MessageTypeDecode(CDBUFFER stream, MMSVALUE out);

#endif //GAMMU_MMS_DECODERS_H
