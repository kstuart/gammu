#include <gammu-message.h>

#include "../../libgammu/gsmstate.h"
#include "../../libgammu/misc/coding/coding.h"
#include "../../libgammu/cdma.h"

#include "test_helper.h"
#include "../../libgammu/service/sms/gsmmulti.h"

void encode_pdu_ascii(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[4096] = { 0 };
  unsigned char pdu[4096] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609200534", 14);
  EncodeUnicode(sms.Text, "aaaaaaaaaabbbbbbbbbb", 20);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_ASCII;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "08811041689002504300100200020014C3870E1C3870E1C387162C58B162C58B1620",
    pos * 2) == 0);
}

void encode_pdu_octet(void)
{
  GSM_Error error;
  GSM_Debug_Info *di;
  GSM_SMSMessage sms;
  unsigned char pdu_hex[4096] = { 0 };
  unsigned char pdu[4096] = { 0 };
  int pos = 0;

  puts(__func__);

  di = GSM_GetGlobalDebug();
  GSM_SetDebugFileDescriptor(stderr, FALSE, di);
  GSM_SetDebugLevel("textall", di);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609200534", 14);
  EncodeUnicode(sms.Text, "aaaaaaaaaabbbbbbbbbb", 20);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_8bit;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "088110416890025043001002000000146161616161616161616162626262626262626262",
    pos * 2) == 0);
}

void encode_pdu_unicode(void)
{
  GSM_Error error;
  GSM_Debug_Info *di;
  GSM_SMSMessage sms;
  unsigned char pdu_hex[4096] = { 0 };
  unsigned char pdu[4096] = { 0 };
  int pos = 0;

  const unsigned char unc_text[] = { // "Zażółć gęślą jaźń  "
    0x00, 0x5a, 0x00, 0x61, 0x01, 0x7c, 0x00, 0xf3, 0x01, 0x42, 0x01, 0x07, 0x00, 0x20, 0x00, 0x67,
    0x01, 0x19, 0x01, 0x5b, 0x00, 0x6c, 0x01, 0x05, 0x00, 0x20, 0x00, 0x6a, 0x00, 0x61, 0x01, 0x7a,
    0x01, 0x44, 0x00, 0x20, 0x00, 0x00 };

  puts(__func__);

  di = GSM_GetGlobalDebug();
  GSM_SetDebugFileDescriptor(stderr, FALSE, di);
  GSM_SetDebugLevel("textall", di);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609200534", 14);
  memcpy(sms.Text, unc_text, sizeof(unc_text));
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_Unicode_No_Compression;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "08811041689002504300100200040024005A0061017C00F301420107002000670119015B006C01050020006A0061017A01440020",
    pos * 2) == 0);
}

void encode_pdu_ascii_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[4096] = { 0 };
  unsigned char pdu[4096] = { 0 };
  int pos = 0;

  puts(__func__);

  memset(&sms, 0, sizeof(GSM_SMSMessage));
  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609200534", 14);
  EncodeUnicode(sms.Text, "aaaaaaaaaabbbbbbbbbb", 20);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_ConcatenatedMessages;
  sms.UDH.Length = 6;
  sms.UDH.Text[0] = 5;
  sms.UDH.Text[2] = 3;
  sms.UDH.Text[3] = 95;
  sms.UDH.Text[4] = 1;
  sms.UDH.Text[5] = 1;
  sms.Coding = SMS_Coding_ASCII;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  smfprintf(di, "%s\n", pdu_hex);
  test_result(memcmp(
    pdu_hex,
    "0881104168900250430010050002011A0A001DF02070E1C3870E1C3870E1C58B162C58B162C588",
    pos * 2) == 0);
}

void encode_pdu_unicode_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[4096] = { 0 };
  unsigned char pdu[4096] = { 0 };
  int pos = 0;

  puts(__func__);

  memset(&sms, 0, sizeof(GSM_SMSMessage));
  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609200534", 14);
  EncodeUnicode(sms.Text, "aaaaaaaaaabbbbbbbbbb", 20);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_ConcatenatedMessages;
  sms.UDH.Length = 6;
  sms.UDH.Text[0] = 5;
  sms.UDH.Text[2] = 3;
  sms.UDH.Text[3] = 95;
  sms.UDH.Text[4] = 1;
  sms.UDH.Text[5] = 1;
  sms.Coding = SMS_Coding_Unicode_No_Compression;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  smfprintf(di, "%s\n", pdu_hex);
  test_result(memcmp(
    pdu_hex,
    "0881104168900250430010050004012E0500035F010100610061006100610061006100610061006100610062006200620062006200620062006200620062",
    pos * 2) == 0);
}
int main(void)
{
  encode_pdu_ascii();

  encode_pdu_octet();
  encode_pdu_unicode();

  encode_pdu_ascii_multi();
  encode_pdu_unicode_multi();
}

