#ifndef GAMMU_MMS_SERVICE_H
#define GAMMU_MMS_SERVICE_H

#include "mms-data.h"
#include "mms-tables.h"
#include "mms-decoders.h"
#include "../include/gammu-smsd.h"

typedef struct _MMSContext {
	SBUFFER MMSBuffer;
	MMSMESSAGE MappedMMS;
} MMSContext;
typedef struct _MMSContext *MMSCTX;

typedef struct _MMSConveyor {
	GSM_Error (*FetchMMS)(GSM_SMSDConfig*, SBUFFER, GSM_MMSIndicator*);
	GSM_Error (*SendMMS)(GSM_SMSDConfig*, SBUFFER);
} MMSConveyor;
typedef MMSConveyor *MMSCONVEYOR;

int MMS_MapEncodedHeaders(SBUFFER stream, MMSHEADERS headers);
MMSError MMS_MapEncodedParts(SBUFFER stream, MMSPARTS parts);
MMSError MMS_MapEncodedMessage(GSM_SMSDConfig *Config, SBUFFER Stream, MMSMESSAGE *out);

void MMS_DumpHeaders(SBUFFER buffer, MMSHEADERS headers);
void SaveSBufferToTempFile(GSM_SMSDConfig *Config, SBUFFER Buffer);

void MMS_ContentTypeAsString(SBUFFER buffer, MMSContentType *ct);

#endif //GAMMU_MMS_SERVICE_H
