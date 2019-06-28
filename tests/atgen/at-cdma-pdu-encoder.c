#include <gammu-message.h>

#include "../../libgammu/gsmstate.h"
#include "../../libgammu/misc/coding/coding.h"
#include "../../libgammu/cdma.h"

#include "test_helper.h"

#define BUFFSIZE 4096

void encode_pdu_ascii(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
  EncodeUnicode(sms.Text, "Lorem ipsum dolor sit amet.", 27);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_ASCII;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  smfprintf(di, "%s\n", pdu_hex);
  test_result(memcmp(
    pdu_hex,
   "0881104168909528610010020002001B99BF965DA834F0E7D76A0C9BF66FE4839E9E8830EDCBD170",
    pos * 2) == 0);
}

void encode_pdu_gsm(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
  EncodeUnicode(sms.Text, "Lorem ipsum dolor sit amet.", 27);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_Default_No_Compression;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  // NOTE: [KS] see ATCDMA_EncodePDUFrame, GSM appears unsupported by network
  //  so encoder gets switched to ASCII (CDMA Network preferred)
  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "0881104168909528610010020002001B99BF965DA834F0E7D76A0C9BF66FE4839E9E8830EDCBD170",
    pos * 2) == 0);
}

void encode_pdu_unicode(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  const unsigned char unc_text[] = { // "Zażółć gęślą jaźń "
    0x00, 0x5a, 0x00, 0x61, 0x01, 0x7c, 0x00, 0xf3, 0x01, 0x42, 0x01, 0x07, 0x00, 0x20, 0x00, 0x67,
    0x01, 0x19, 0x01, 0x5b, 0x00, 0x6c, 0x01, 0x05, 0x00, 0x20, 0x00, 0x6a, 0x00, 0x61, 0x01, 0x7a,
    0x01, 0x44, 0x00, 0x20, 0x00, 0x00 };

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
  memcpy(sms.Text, unc_text, sizeof(unc_text));
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_Unicode_No_Compression;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  smfprintf(di, "%s\n", pdu_hex);
  test_result(memcmp(
    pdu_hex,
    "08811041689095286100100200040024005A0061017C00F301420107002000670119015B006C01050020006A0061017A01440020",
    pos * 2) == 0);
}

void encode_pdu_octet(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
  EncodeUnicode(sms.Text, "Lorem ipsum dolor sit amet.", 27);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_NoUDH;
  sms.Coding = SMS_Coding_8bit;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "088110416890952861001002000000146161616161616161616162626262626262626262",
    pos * 2) == 0);
}

void encode_pdu_ascii_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  memset(&sms, 0, sizeof(GSM_SMSMessage));
  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
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
  test_result(memcmp(
    pdu_hex,
    "0881104168909528610010050002011A0A001DF02070E1C3870E1C3870E1C58B162C58B162C588",
    pos * 2) == 0);
}

void encode_pdu_gsm_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
  EncodeUnicode(sms.Text, "FFFFFFFFFFFFFFAAA", 17);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_ConcatenatedMessages;
  sms.UDH.Length = 6;
  sms.UDH.Text[0] = 5;
  sms.UDH.Text[2] = 3;
  sms.UDH.Text[3] = 95;
  sms.UDH.Text[4] = 2;
  sms.UDH.Text[5] = 2;
  sms.Coding = SMS_Coding_Default_No_Compression;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "088110416890952861001005000901180500035F02024646464646464646464646464646414141",
    pos * 2) == 0);
}

void encode_pdu_unicode_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
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
  test_result(memcmp(
    pdu_hex,
    "0881104168909528610010050004012E0500035F010100610061006100610061006100610061006100610062006200620062006200620062006200620062",
    pos * 2) == 0);
}

void encode_pdu_octet_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu_hex[BUFFSIZE] = { 0 };
  unsigned char pdu[BUFFSIZE] = { 0 };
  int pos = 0;

  puts(__func__);

  GSM_SetDefaultSMSData(&sms);

  EncodeUnicode(sms.Number, "01148609598216", 14);
  EncodeUnicode(sms.Text, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam a quam quis urna dignissim laoreet et quis orci.", 112);
  sms.Length = UnicodeLength(sms.Text);
  sms.UDH.Type = UDH_ConcatenatedMessages;
  sms.UDH.Length = 6;
  sms.UDH.Text[0] = 5;
  sms.UDH.Text[2] = 3;
  sms.UDH.Text[3] = 95;
  sms.UDH.Text[4] = 2;
  sms.UDH.Text[5] = 1;
  sms.Coding = SMS_Coding_8bit;

  error = ATCDMA_EncodePDUFrame(di, &sms, pdu, &pos);
  test_result(error == ERR_NONE);

  EncodeHexBin(pdu_hex, pdu, pos);
  test_result(memcmp(
    pdu_hex,
    "088110416890952861001005000001760500035F02014C6F72656D20697073756D20646F6C6F722073697420616D65742C20636F6E73656374657475722061646970697363696E6720656C69742E204E756C6C616D2061207175616D20717569732075726E61206469676E697373696D206C616F726565742065742071756973206F7263692E",
    pos * 2) == 0);
}

int main(void)
{
  encode_pdu_ascii();
  encode_pdu_gsm();
  encode_pdu_unicode();
//  encode_pdu_octet();

//  encode_pdu_ascii_multi();
//  encode_pdu_gsm_multi();
//  encode_pdu_unicode_multi();
//  encode_pdu_octet_multi();
}

