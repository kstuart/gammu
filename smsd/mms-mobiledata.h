#ifndef GAMMU_MMS_MOBILEDATA_H
#define GAMMU_MMS_MOBILEDATA_H

#include <gammu-error.h>
#include "gammu-smsd.h"

typedef struct _StreamBuffer *SBUFFER;
extern struct _MMSConveyor *MMSMobileDataConveyor;

GSM_Error MobileDataStart(GSM_SMSDConfig *Config);
GSM_Error MobileDataStop(GSM_SMSDConfig *Config);

GSM_Error FetchMMS(GSM_SMSDConfig *Config, SBUFFER Buffer, GSM_MMSIndicator *MMSIndicator);
GSM_Error SendMMS(GSM_SMSDConfig *Config, SBUFFER Buffer);

#endif //GAMMU_MMS_MOBILEDATA_H
