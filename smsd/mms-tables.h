#ifndef GAMMU_MMS_TABLES_H
#define GAMMU_MMS_TABLES_H

#include "mms-data.h"

MMSFIELDINFO WSPFields_FindByID(int id);
MMSFIELDINFO MMSFields_FindByID(int id);
MMSFIELDINFO MMS_WellKnownParams_FindByID(int id);
MMSVALUEENUM MMS_Charset_FindByID(int id);
MMSVALUEENUM MMS_WkContentType_FindByID(int id);
MMSVALUEENUM MMS_MessageType_FindByID(int id);
MMSVALUEENUM MMS_MessageClass_FindByID(int id);
MMSVALUEENUM MMS_Priority_FindByID(int id);
MMSVALUEENUM MMS_YesNo_FindByID(int id);

#endif //GAMMU_MMS_TABLES_H
