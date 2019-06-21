#include <gammu-statemachine.h>
#include <gammu.h>
#include "misc/coding/coding.h"
#include "debug.h"
#include "gsmstate.h"

#include "bitstream.h"
#include "cdma.h"

#include <assert.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

int CDMA_Decode7bit(unsigned char *dest, const unsigned char *src, int src_length)
{
  BitReader reader;
  int i;

  BitReader_SetStart(&reader, src);
  for(i = 0; i < src_length; i++)
    dest[i] = BitReader_ReadBits(&reader, 7);

  return BitReader_GetPosition(&reader);
}

int CDMA_Encode7bit(unsigned char *dest, const unsigned char *src, int src_length)
{
  BitWriter writer;
  int i;

  BitWriter_SetStart(&writer, dest);
  for(i = 0; i < src_length; i++)
    BitWriter_WriteBits(&writer, src[i], 7);

  BitWriter_Flush(&writer);
  return BitWriter_GetPosition(&writer);
}

int CDMA_DecodeWithBCDAlphabet(unsigned char value)
{
  return 10*(value >> 4u) + (value & 0x0fu);
}

GSM_Error CDMA_CheckTeleserviceID(GSM_Debug_Info *di, const unsigned char *pos, int length)
{
  const int supported_ids[] = {
    TELESERVICE_ID_SMS,
    TELESERVICE_ID_SMS_MULTI
  };
  const int num_ids = sizeof(supported_ids) / sizeof(int);
  int i;
  int teleservice_id;

  if(length != 2) {
    smfprintf(di, "Invalid Teleservice ID parameter length, expected 2 but given %d\n", length);
    return ERR_CORRUPTED;
  }
  teleservice_id = ntohs(*(unsigned short*)pos);

  for(i = 0; i < num_ids; i++) {
    if(teleservice_id == supported_ids[i]) {
      smfprintf(di, "Teleservice: [%d] %s\n", teleservice_id, TeleserviceIdToString(teleservice_id));
      return ERR_NONE;
    }
  }

  smfprintf(di, "Unsupported teleservice identifier: %d\n", teleservice_id);
  return ERR_NOTSUPPORTED;
}

GSM_Error ATCDMA_DecodeSMSDateTime(GSM_Debug_Info *di, GSM_DateTime *DT, const unsigned char *req)
{
  DT->Year    = CDMA_DecodeWithBCDAlphabet(req[0]);
  if (DT->Year < 90) {
    DT->Year = DT->Year + 2000;
  } else {
    DT->Year = DT->Year + 1990;
  }
  DT->Month   = CDMA_DecodeWithBCDAlphabet(req[1]);
  DT->Day     = CDMA_DecodeWithBCDAlphabet(req[2]);
  DT->Hour    = CDMA_DecodeWithBCDAlphabet(req[3]);
  DT->Minute  = CDMA_DecodeWithBCDAlphabet(req[4]);
  DT->Second  = CDMA_DecodeWithBCDAlphabet(req[5]);
  DT->Timezone = GSM_GetLocalTimezoneOffset();

  if (!CheckDate(DT) || !CheckTime(DT)) {
    smfprintf(di, "Invalid date & time!\n");
    DT->Year = 0;
    return ERR_NONE;
  }

  smfprintf(di, "Decoding date & time: %s\n", OSDateTime(*DT, TRUE));

  return ERR_NONE;
}

GSM_Error ATCDMA_DecodePDUFrame(GSM_Debug_Info *di, GSM_SMSMessage *SMS, const unsigned char *buffer, size_t length, size_t *final_pos)
{
  int udh = 0;
  size_t pos = 0;
  int datalength = 0;
  int udh_len = 0;
  unsigned char output[512] = { 0 };
  const unsigned char *data_ptr = NULL;
  SMS_ENCODING encoding;
  GSM_Error error;

  GSM_SetDefaultReceivedSMSData(SMS);

  if (SMS->State == SMS_Read || SMS->State == SMS_UnRead) {
    smfprintf(di, "SMS type: Deliver\n");
    SMS->PDU = SMS_Deliver;
  } else {
    smfprintf(di, "SMS type: Submit\n");
    SMS->PDU = SMS_Submit;
  }

  error = GSM_UnpackSemiOctetNumber(di, SMS->Number, buffer, &pos, length, FALSE);
  if (error != ERR_NONE)
    return error;

  if (SMS->PDU == SMS_Submit) {
    error = GSM_UnpackSemiOctetNumber(di, SMS->OtherNumbers[0], buffer, &pos, length, FALSE);
    if (error != ERR_NONE)
      return error;

    smfprintf(di, "Destination Address: \"%s\"\n", DecodeUnicodeString(SMS->Number));
    smfprintf(di, "Callback Address: \"%s\"\n", DecodeUnicodeString(SMS->OtherNumbers[0]));
  } else {
    smfprintf(di, "Originating Address: \"%s\"\n", DecodeUnicodeString(SMS->Number));
  }

  if (SMS->PDU == SMS_Deliver) {
    error = ATCDMA_DecodeSMSDateTime(di, &SMS->DateTime, buffer + pos);
    pos += 6;
    if (error != ERR_NONE)
      return error;
  }

  error = CDMA_CheckTeleserviceID(di, buffer + pos, 2);
  pos += 2;
  if (error != ERR_NONE)
    return error;

  SMS->Priority = buffer[pos++];
  smfprintf(di, "Priority: [%d] %s\n", SMS->Priority, CDMA_SMSPriorityToString(SMS->Priority));

  encoding = buffer[pos++];
  smfprintf(di, "Encoding: [%d] %s\n", encoding, CDMA_SMSEncodingToString(encoding));

  udh = buffer[pos++];
  smfprintf(di, "UDH Present: %s\n", udh == 0 ? "No" : "Yes");

  datalength = buffer[pos++];
  smfprintf(di, "Data length: %d\n", datalength);

  if(encoding == SMS_ENC_ASCII) {
    pos += CDMA_Decode7bit(output, buffer + pos,  datalength);
    data_ptr = output;
  } else {
    data_ptr = buffer + pos;
  }

  if (udh) {
    udh_len = *data_ptr + 1;
    SMS->UDH.Length = udh_len;
    memcpy(SMS->UDH.Text, data_ptr, SMS->UDH.Length);
    GSM_DecodeUDHHeader(di, &SMS->UDH);
    datalength -= udh_len;
  }

  switch (encoding) {
    case SMS_ENC_ASCII:
      SMS->Coding = SMS_Coding_ASCII;
      EncodeUnicode(SMS->Text, data_ptr + udh_len, datalength);
      SMS->Length = datalength;
      break;
    case SMS_ENC_UNICODE:
      SMS->Coding = SMS_Coding_Unicode_No_Compression;
      SMS->Length = datalength / 2;
      DecodeUnicodeSpecialNOKIAChars(SMS->Text, data_ptr + udh_len, SMS->Length);
      pos += datalength + udh_len;
      break;
    case SMS_ENC_GSM:
      SMS->Coding = SMS_Coding_Default_No_Compression;
      datalength--;
      SMS->Length = datalength;
      DecodeDefault(SMS->Text, data_ptr + udh_len, SMS->Length, TRUE, NULL);
      pos += datalength + udh_len;
      break;
    case SMS_ENC_OCTET:
      SMS->Coding = SMS_Coding_8bit;
      SMS->Length = datalength;
      EncodeUnicode(SMS->Text, data_ptr + udh_len, datalength);
      pos += datalength + udh_len;
      break;
    default:
      smfprintf(di, "Unsupported encoding.\n");
      error = ERR_ABORTED;
  }

  if (error != ERR_NONE)
    return error;

#ifdef DEBUG
  smfprintf(di, "SMS length %i\n",SMS->Length);
  DumpMessageText(di, SMS->Text, SMS->Length * 2);
  smfprintf(di, "%s\n", DecodeUnicodeString(SMS->Text));
#endif

  if (final_pos)
    *final_pos = pos;

  return error;
}

GSM_Error ATCDMA_EncodePDUFrame(GSM_Debug_Info *di, GSM_SMSMessage *SMS, unsigned char *buffer, int *length)
{
  GSM_Error error = ERR_NONE;
  int datalength_ofs;
  int encoding_ofs;
  int udh_len = 0;
  char *sms_text = NULL;
  unsigned char *sms_ptr = NULL;
  size_t sms_len = 0;

  *length = 0;

  sms_len = GSM_PackSemiOctetNumber(SMS->Number, buffer + 1, FALSE);
  *buffer = sms_len;
  *length += sms_len + 1;
  smfprintf(di, "Destination Address: \"%s\"\n", DecodeUnicodeString(SMS->Number));

  if(SMS->CallbackIndex >= 0) {
    assert(SMS->OtherNumbersNum > 0);
    sms_len = GSM_PackSemiOctetNumber(SMS->OtherNumbers[SMS->CallbackIndex], buffer + *length + 1, FALSE);
    *(buffer + *length) = sms_len;
    *length += sms_len + 1;
    smfprintf(di, "Callback Address: \"%s\"\n", DecodeUnicodeString(SMS->OtherNumbers[SMS->CallbackIndex]));
  } else {
    *(buffer + (*length)++) = 0;
  }

  switch (SMS->UDH.Type) {
    case UDH_NoUDH:
      *((unsigned short*)&buffer[*length]) = htons(TELESERVICE_ID_SMS);
      break;
    case UDH_ConcatenatedMessages:
    case UDH_ConcatenatedMessages16bit:
    case UDH_UserUDH:
      *((unsigned short*)&buffer[*length]) = htons(TELESERVICE_ID_SMS_MULTI);
      break;
    default:
      smfprintf(di, "Unknown UDH type (%d : %s), cannot set teleservice id.\n", SMS->UDH.Type, SMS->UDH.Text);
      return ERR_NOTSUPPORTED;
  }
  *length += 2;

  buffer[(*length)++] = SMS->Priority;

  encoding_ofs = (*length)++;

  if(SMS->UDH.Type != UDH_NoUDH) {
    buffer[(*length)++] = 1;
    if (SMS->UDH.Length == 0) {
      udh_len = SMS->UDH.Text[0] + 1;
      smfprintf(di, "UDL passed from API is 0, using UDHL+1 (%i)\n", udh_len);
    } else {
      udh_len = SMS->UDH.Length;
      smfprintf(di, "UDL: %i, UDHL: %i\n", udh_len, SMS->UDH.Text[0]);
    }
    memcpy(buffer + *length + 1, SMS->UDH.Text, udh_len);
    smfprintf(di, "UDH, length %i\n", udh_len);
    DumpMessageText(di, SMS->UDH.Text, udh_len);
  }
  else {
    buffer[(*length)++] = 0;
  }

  datalength_ofs = (*length)++;

  sms_text = DecodeUnicodeString(SMS->Text);
  sms_len = UnicodeLength(SMS->Text);

  sms_ptr = buffer + *length + udh_len;
  switch(SMS->Coding) {
    case SMS_Coding_Default_No_Compression:
      buffer[encoding_ofs] = SMS_ENC_GSM;
      sms_len = MIN(sms_len, 160);
      EncodeDefault(sms_ptr, SMS->Text, &sms_len, TRUE, NULL);
      *length += sms_len + udh_len;
      buffer[datalength_ofs] = 1 + sms_len + udh_len;
      break;
    case SMS_Coding_Unicode_No_Compression:
      buffer[encoding_ofs] = SMS_ENC_UNICODE;
      sms_len = MIN(sms_len, 70);
      EncodeUnicodeSpecialNOKIAChars(sms_ptr, SMS->Text, sms_len);
      buffer[datalength_ofs] = sms_len * 2 + udh_len;
      *length += buffer[datalength_ofs];
      break;
    case SMS_Coding_8bit:
      buffer[encoding_ofs] = SMS_ENC_OCTET;
      sms_len = MIN(sms_len, 140);
      memcpy(sms_ptr, sms_text, sms_len);
      buffer[datalength_ofs] = sms_len + udh_len;
      *length += buffer[datalength_ofs];
      break;
    case SMS_Coding_ASCII:
      buffer[encoding_ofs] = SMS_ENC_ASCII;
      sms_len = MIN(sms_len, 160);
      memcpy(sms_ptr, sms_text, sms_len);
      *length += CDMA_Encode7bit(sms_ptr - udh_len, sms_ptr - udh_len, sms_len + udh_len);
      buffer[datalength_ofs] = sms_len + udh_len;
      break;
    default:
      smfprintf(di, "Unsupported encoding. (%d)\n", SMS->Coding);
      error = ERR_NOTSUPPORTED;
  }

  return error;
}

const char *NetworkTypeToString(NETWORK_TYPE networkType)
{
  switch(networkType) {
    case NETWORK_CDMA : return "CDMA";
    case NETWORK_GSM  : return "GSM";
    case NETWORK_AUTO : return "Undetermined";
    default           : return "Unknown";
  }
}

const char *CDMA_MsgTypeToString(CDMA_MSG_TYPE msgType)
{
  switch(msgType) {
    case CDMA_MSG_TYPE_P2P   : return "Point to Point";
    case CDMA_MSG_TYPE_BCAST : return "Broadcast";
    case CDMA_MSG_TYPE_ACK   : return "Acknowledge";
    default           : return "Unknown";
  }
}

const char *CDMA_ParameterIDToString(CDMA_PARAMETER_ID parameterID)
{
  switch(parameterID) {
    case CDMA_PARAM_TELESERVICE_ID   : return "Teleservice Identifier";
    case CDMA_PARAM_SERVICE_CATEGORY : return "Service Category";
    case CDMA_PARAM_ORIG_ADDRESS     : return "Originating Address";
    case CDMA_PARAM_ORIG_SUBADDRESS  : return "Originating Subaddress";
    case CDMA_PARAM_DEST_ADDRESS     : return "Destination Address";
    case CDMA_PARAM_DEST_SUBADDRESS  : return "Destination Subaddress";
    case CDMA_PARAM_BEARER_REPLY_OPT : return "Bearer Reply Option";
    case CDMA_PARAM_CAUSE_CODES      : return "Cause Codes";
    case CDMA_PARAM_BEARER_DATA      : return "Bearer Data";
    default           : return "Unknown";
  }
}

const char *TeleserviceIdToString(TELESERVICE_ID teleserviceId)
{
  switch(teleserviceId) {
    case TELESERVICE_ID_SMS       : return "CDMA Messaging";
    case TELESERVICE_ID_SMS_MULTI : return "Segmented SMS";
    case TELESERVICE_ID_VOICEMAIL : return "CDMA Voice Mail Notification";
    case TELESERVICE_ID_PAGE      : return "CDMA Paging";
    default           : return "Unknown";
  }
}

const char *CDMA_SMSPriorityToString(SMS_PRIORITY priority)
{
  switch(priority) {
    case SMS_PRIORITY_NORMAL      : return "NORMAL";
    case SMS_PRIORITY_INTERACTIVE : return "INTERACTIVE";
    case SMS_PRIORITY_URGENT      : return "URGENT";
    case SMS_PRIORITY_EMERGENCY   : return "EMERGENCY";
    default           : return "Unknown";
  }
}

const char *CDMA_SMSEncodingToString(SMS_ENCODING encoding)
{
  switch(encoding) {
    case SMS_ENC_ASCII   : return "ASCII";
    case SMS_ENC_GSM     : return "GSM";
    case SMS_ENC_UNICODE : return "Unicode";
    case SMS_ENC_OCTET   : return "8-bit";
    default              : return "Unknown";
  }
}
