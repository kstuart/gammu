#ifndef GAMMU_CDMA_H
#define GAMMU_CDMA_H

#include <gammu-message.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * CDMA SMS Encoding
 *
 * \ingroup SMS
 */
typedef enum {
  SMS_ENC_OCTET = 0,
  SMS_ENC_ASCII = 2,
  SMS_ENC_UNICODE = 4,
  SMS_ENC_GSM = 9
} SMS_ENCODING;

typedef enum {
  CDMA_MSG_TYPE_P2P = 0,
  CDMA_MSG_TYPE_BCAST = 1,
  CDMA_MSG_TYPE_ACK = 2
} CDMA_MSG_TYPE;

typedef enum {
  CDMA_PARAM_TELESERVICE_ID = 0,
  CDMA_PARAM_SERVICE_CATEGORY = 1,
  CDMA_PARAM_ORIG_ADDRESS = 2,
  CDMA_PARAM_ORIG_SUBADDRESS = 3,
  CDMA_PARAM_DEST_ADDRESS = 4,
  CDMA_PARAM_DEST_SUBADDRESS = 5,
  CDMA_PARAM_BEARER_REPLY_OPT = 6,
  CDMA_PARAM_CAUSE_CODES = 7,
  CDMA_PARAM_BEARER_DATA = 8
} CDMA_PARAMETER_ID;

// N.S0005 Table 175
typedef enum {
  TELESERVICE_ID_PAGE = 4097,
  TELESERVICE_ID_SMS = 4098,
  TELESERVICE_ID_VOICEMAIL = 4099,
  TELESERVICE_ID_SMS_MULTI = 4101
} TELESERVICE_ID;

GSM_Error ATCDMA_DecodePDUFrame(GSM_Debug_Info *di, GSM_SMSMessage *SMS,
  const unsigned char *buffer, size_t length, size_t *final_pos);

GSM_Error ATCDMA_EncodePDUFrame(GSM_Debug_Info *di, GSM_SMSMessage *SMS,
  unsigned char *buffer, int *length);

const char *NetworkTypeToString(NETWORK_TYPE networkType);
const char *TeleserviceIdToString(TELESERVICE_ID teleserviceId);
const char *CDMA_SMSPriorityToString(SMS_PRIORITY priority);
const char *CDMA_SMSEncodingToString(SMS_ENCODING encoding);
const char *CDMA_MsgTypeToString(CDMA_MSG_TYPE msgType);
const char *CDMA_ParameterIDToString(CDMA_PARAMETER_ID parameterId);

#ifdef  __cplusplus
}
#endif

#endif //GAMMU_CDMA_H
