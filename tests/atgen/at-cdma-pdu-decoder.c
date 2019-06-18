#include <gammu-message.h>

#include "../../libgammu/gsmstate.h"
#include "../../libgammu/misc/coding/coding.h"
#include "../../libgammu/cdma.h"

#include "test_helper.h"

void decode_pdu_ascii(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[4096] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "088110416890025043190530095939100200020014C3870E1C3870E1C387162C58B162C58B1620";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_ASCII);
  test_result(strcmp("aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_octet(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[4096] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881104168900250431906120747401002000000146161616161616161616162626262626262626262";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_8bit);
  test_result(strcmp("aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);

}

void decode_pdu_unc(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[4096] = { 0 };
  size_t pos = 0;

  const char *unc_pdu = "088010416890025043190612074740100200040024005A0061017C00F301420107002000670119015B006C01050020006A0061017A01440020";
  const size_t pdu_len = strlen(unc_pdu);

  const unsigned char unc_text[] = { // "Zażółć gęślą jaźń "
    0x00, 0x5a, 0x00, 0x61, 0x01, 0x7c, 0x00, 0xf3, 0x01, 0x42, 0x01, 0x07, 0x00, 0x20, 0x00, 0x67,
    0x01, 0x19, 0x01, 0x5b, 0x00, 0x6c, 0x01, 0x05, 0x00, 0x20, 0x00, 0x6a, 0x00, 0x61, 0x01, 0x7a,
    0x01, 0x44, 0x00, 0x20 };

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, unc_pdu, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 18);
  test_result(sms.Coding == SMS_Coding_Unicode_No_Compression);
  test_result(memcmp(unc_text, sms.Text, sms.Length * 2) == 0);
}

void decode_pdu_ascii_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[4096] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "08811041689002504319061207474010050002011A0A001DF02070E1C3870E1C3870E1C58B162C58B162C588";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.AllParts == 1);
  test_result(sms.UDH.PartNumber == 1);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_ASCII);
  test_result(strcmp("aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_gsm_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[4096] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881105184735443F2190612074740100500090117050003E702024646464646464646464646464646414141";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.AllParts == 1);
  test_result(sms.UDH.PartNumber == 1);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_Default_No_Compression);
  test_result(strcmp(
    "FFFFFFFFFFFFFFAAA",
    DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_unc_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[4096] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "08811041689002504319061207474010050004012E0500035F010100610061006100610061006100610061006100610062006200620062006200620062006200620062";
  //"0881105184735443F219061207474010050004014E0500036B0303004100410041004100410041004100410041004100410041004100410041004100410041004100460046004600460046004600460046004600460046004600460046004100410041";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.AllParts == 1);
  test_result(sms.UDH.PartNumber == 1);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_Unicode_No_Compression);
  test_result(strcmp( "aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);
  //test_result(strcmp( "AAAAAAAAAAAAAAAAAAAFFFFFFFFFFFFFFAAA", DecodeUnicodeString(sms.Text)) == 0);
}

int main(void)
{
  decode_pdu_ascii();
  decode_pdu_octet();
  decode_pdu_unc();

//  decode_pdu_gsm_multi();
  decode_pdu_unc_multi();
  decode_pdu_ascii_multi();
}
