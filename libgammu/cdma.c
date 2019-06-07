#include <gammu-statemachine.h>
#include <gammu.h>
#include "cdma.h"
#include "misc/coding/coding.h"
#include "debug.h"
#include "gsmstate.h"

#include <math.h>
#include <assert.h>
#include <netinet/in.h>

typedef struct {
  unsigned char* stream;
  int stream_pos;
  unsigned int nibbler;
  int nibbler_len;
} BitStream;

typedef BitStream BitReader;
typedef BitStream BitWriter;

typedef BitReader *BITREADER;
typedef BitWriter *BITWRITER;

void BitReader_SetStart(BITREADER reader, unsigned char *buffer)
{
  assert(reader != NULL);
  assert(buffer != NULL);

  reader->stream = buffer;
  reader->stream_pos = 0;
  reader->nibbler = 0;
  reader->nibbler_len = 0;
}

unsigned int BitReader_ReadBits(BITREADER reader, int length)
{
  int i = 0;
  int bytesToRead = 0;
  unsigned bitOffset = 0;
  unsigned resultMask = (1 << length) - 1;
  unsigned int result = 0;

  if (length > reader->nibbler_len) {
    bytesToRead = ceil(((double)length - reader->nibbler_len) / 8);
    for (i = 0; i < bytesToRead; i++) {
      reader->nibbler = (reader->nibbler << 8) | (reader->stream[reader->stream_pos++] & 0xFF);
      reader->nibbler_len += 8;
    }
  }

  bitOffset = reader->nibbler_len - length;
  result = (reader->nibbler >> bitOffset) & resultMask;
  reader->nibbler_len -= length;

  return result;
}

int BitReader_GetPosition(BITREADER reader)
{
  return reader->stream_pos;
}

void BitWriter_SetStart(BITWRITER writer, unsigned char *buffer)
{
  assert(writer != NULL);
  assert(buffer != NULL);

  writer->stream = buffer;
  writer->stream_pos = 0;
  writer->nibbler = 0;
  writer->nibbler_len = 0;
}

void BitWriter_WriteBits(BITWRITER writer, unsigned int value, int length)
{
  unsigned value_mask = (1 << length) - 1;
  int merge_length = 8 - writer->nibbler_len;
  int total_length = length + writer->nibbler_len;

  if(total_length < 8) {
    writer->nibbler = (writer->nibbler << length) | (value & value_mask);
    writer->nibbler_len += length;
    return;
  }

  if(writer->nibbler_len) {
    value_mask = (1 << merge_length) -1;
    writer->nibbler = (writer->nibbler << merge_length) | ((value >> (length - merge_length)) & value_mask);
    writer->stream[writer->stream_pos++] = writer->nibbler & 0xFF;
    length -= merge_length;
  }

  while(length >= 8) {
    length -= 8;
    writer->stream[writer->stream_pos++] = (value >> length) & 0xFF;
  }

  writer->nibbler = value & ((1 << length) - 1);
  writer->nibbler_len = length;
}

void BitWriter_Flush(BITWRITER writer)
{
  if(writer->nibbler_len)
    writer->stream[writer->stream_pos++] = writer->nibbler << (8 - writer->nibbler_len);

  writer->nibbler = 0;
  writer->nibbler_len = 0;
}

int BitWriter_GetPosition(BITWRITER writer)
{
  return writer->stream_pos;
}

int Encode7bitASCII(unsigned char *dest, const char *src, int src_length)
{
  BitWriter writer;
  int i;

  BitWriter_SetStart(&writer, dest);
  for(i = 0; i < src_length; i++) {
    BitWriter_WriteBits(&writer, src[i], 7);
  }
  BitWriter_Flush(&writer);
  return BitWriter_GetPosition(&writer);
}

int CDMA_DecodeWithBCDAlphabet(unsigned char value)
{
  return 10*(value >> 4u) + (value & 0x0fu);
}

GSM_Error CDMA_CheckTeleserviceID(GSM_Debug_Info *di, const unsigned char *pos, int length)
{
  int teleservice_id;

  if(length != 2) {
    smfprintf(di, "Invalid Teleservice ID parameter length, expected 2 but given %d\n", length);
    return ERR_CORRUPTED;
  }
  teleservice_id = ((*(unsigned short*)pos >> 8) & 0xff) | (*(unsigned short*)pos & 0xff) << 8;

  if(teleservice_id != 4098) {
    smfprintf(di, "Unsupported teleservice identifier: %d\n", teleservice_id);
    return ERR_NOTSUPPORTED;
  }

  smfprintf(di, "Teleservice: [%d] %s\n", teleservice_id, TeleserviceIdToString(teleservice_id));

  return ERR_NONE;
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
  unsigned char output[161];
  int out_pos = 0;
  GSM_Error error;
  BitReader reader;

  GSM_SetDefaultReceivedSMSData(SMS);

  if(SMS->State == SMS_Read || SMS->State == SMS_UnRead) {
    smfprintf(di, "SMS type: Deliver\n");
    SMS->PDU = SMS_Deliver;
  } else {
    smfprintf(di, "SMS type: Submit\n");
    SMS->PDU = SMS_Submit;
  }

  error = GSM_UnpackSemiOctetNumber(di, SMS->Number, buffer, &pos, length, FALSE);
  if(error != ERR_NONE)
    return error;

  if(SMS->PDU == SMS_Submit) {
    error = GSM_UnpackSemiOctetNumber(di, SMS->OtherNumbers[0], buffer, &pos, length, FALSE);
    if(error != ERR_NONE)
      return error;

    smfprintf(di, "Destination Address: \"%s\"\n",DecodeUnicodeString(SMS->Number));
    smfprintf(di, "Callback Address: \"%s\"\n",DecodeUnicodeString(SMS->OtherNumbers[0]));
  }
  else
    smfprintf(di, "Originating Address: \"%s\"\n",DecodeUnicodeString(SMS->Number));


  if(SMS->PDU == SMS_Deliver) {
    error = ATCDMA_DecodeSMSDateTime(di, &SMS->DateTime, buffer + pos);
    pos += 6;
    if (error != ERR_NONE)
      return error;
  }

  error = CDMA_CheckTeleserviceID(di, buffer + pos, 2);
  pos += 2;
  if(error != ERR_NONE)
    return error;

  SMS->Priority = buffer[pos++];
  smfprintf(di, "Priority: [%d] %s\n", SMS->Priority, CDMA_SMSPriorityToString(SMS->Priority));

  SMS->Encoding = buffer[pos++];
  smfprintf(di, "Encoding: [%d] %s\n", SMS->Encoding, CDMA_SMSEncodingToString(SMS->Encoding));

  // TODO: [KS] Map encodings to GSM
  SMS->Coding = SMS_Coding_Default_No_Compression;

  udh = buffer[pos++];
  smfprintf(di, "UDH Present: %s\n", udh == 0 ? "No" : "Yes");

  datalength = buffer[pos++];
  smfprintf(di, "Data length: %d\n", datalength);

  switch(SMS->Encoding) {
    case SMS_ENC_ASCII:
      BitReader_SetStart(&reader, buffer + pos);
      for(out_pos = 0; out_pos < datalength; out_pos++)
        output[out_pos] = BitReader_ReadBits(&reader, 7);
      pos += BitReader_GetPosition(&reader);
      break;

    case SMS_ENC_OCTET:
      memcpy(output, buffer + pos, datalength);
      pos += datalength;
      break;
  }

  SMS->Length = datalength;
  DecodeDefault(SMS->Text, output, SMS->Length, TRUE, NULL);

  if(final_pos)
    *final_pos = pos;

  return error;
}

GSM_Error ATCDMA_EncodePDUFrame(GSM_Debug_Info *di, GSM_SMSMessage *SMS, unsigned char *buffer, int *length)
{
  GSM_Error error = ERR_NONE;
  int encoding_ofs = -1;
  char *sms_text = NULL;
  unsigned char *dest_ptr = NULL;
  size_t len = 0;

  len = GSM_PackSemiOctetNumber(SMS->Number, buffer + 1, FALSE);
  *buffer = len;
  *length += len + 1;
  smfprintf(di, "Destination number: \"%s\"\n", DecodeUnicodeString(SMS->Number));

  len = GSM_PackSemiOctetNumber(SMS->SMSC.Number, buffer + *length + 1, FALSE);
  *(buffer + *length) = len;
  *length += len + 1;
  smfprintf(di, "Callback number: \"%s\"\n", DecodeUnicodeString(SMS->SMSC.Number));

  *((unsigned short*)&buffer[*length]) = htons(TELESERVICE_ID_SMS);
  *length += 2;

  // TODO: [KS] support priorities
  buffer[(*length)++] = SMS_PRIORITY_NORMAL;

  encoding_ofs = (*length)++;

  // TODO: [KS] support UDH
  buffer[(*length)++] = 0;
  buffer[(*length)++] = SMS->Length;

  sms_text = DecodeUnicodeString(SMS->Text);
  len = strlen(sms_text);
  dest_ptr = buffer + *length;
  switch(SMS->Coding) {
    case SMS_Coding_Default_No_Compression:
      buffer[encoding_ofs] = SMS_ENC_GSM;
      EncodeDefault(dest_ptr, SMS->Text, &len, TRUE, NULL);
      *length += Encode7bitASCII(dest_ptr, dest_ptr, len);
      break;
    case SMS_Coding_8bit:
      buffer[encoding_ofs] = SMS_ENC_OCTET;
      memcpy(dest_ptr, sms_text, len);
      *length += len;
      break;
    default:
      buffer[encoding_ofs] = SMS_ENC_ASCII;
      *length += Encode7bitASCII(dest_ptr, sms_text, len);
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
    case SMS_ENC_OCTET: return "8-bit Octet";
    case SMS_ENC_ASCII: return "7-bit ASCII";
    case SMS_ENC_UNICODE: return "16-bit Unicode";
    case SMS_ENC_GSM: return "GSM 7-bit";
    default           : return "Unknown";
  }
}
