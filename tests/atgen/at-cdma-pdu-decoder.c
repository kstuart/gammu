#include <gammu-message.h>

#include "../../libgammu/gsmstate.h"
#include "../../libgammu/misc/coding/coding.h"
#include "../../libgammu/cdma.h"

#include "test_helper.h"

#define BUFFSIZE 4096

void decode_pdu_ascii(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "088110416890952861190530095939100200020014C3870E1C3870E1C387162C58B162C58B1620";
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

void decode_pdu_gsm(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881104168909528611906120747401002000900156161616161616161616162626262626262626262";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_Default_No_Compression);
  test_result(strcmp("aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_unicode(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *unc_pdu = "088010416890952861190612074740100200040024005A0061017C00F301420107002000670119015B006C01050020006A0061017A01440020";
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

void decode_pdu_octet(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881104168909528611906120747401002000000146161616161616161616162626262626262626262";
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

void decode_pdu_ascii_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "08811041689095286119061207474010050002011A0A001DF02070E1C3870E1C3870E1C58B162C58B162C588";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.ID8bit == 95);
  test_result(sms.UDH.ID16bit == -1);
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
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881104168909528611906120747401005000901180500035F02024646464646464646464646464646414141";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.ID8bit == 95);
  test_result(sms.UDH.ID16bit == -1);
  test_result(sms.UDH.AllParts == 2);
  test_result(sms.UDH.PartNumber == 2);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 17);
  test_result(sms.Coding == SMS_Coding_Default_No_Compression);
  test_result(strcmp(
    "FFFFFFFFFFFFFFAAA",
    DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_unicode_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881105100035443F219061207474010050004014E0500036B0303004100410041004100410041004100410041004100410041004100410041004100410041004100460046004600460046004600460046004600460046004600460046004100410041";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.ID8bit == 107);
  test_result(sms.UDH.ID16bit == -1);
  test_result(sms.UDH.AllParts == 3);
  test_result(sms.UDH.PartNumber == 3);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 36);
  test_result(sms.Coding == SMS_Coding_Unicode_No_Compression);
  test_result(strcmp( "AAAAAAAAAAAAAAAAAAAFFFFFFFFFFFFFFAAA", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_octet_multi(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881104168909528611906120747401005000001760500035F02014C6F72656D20697073756D20646F6C6F722073697420616D65742C20636F6E73656374657475722061646970697363696E6720656C69742E204E756C6C616D2061207175616D20717569732075726E61206469676E697373696D206C616F726565742065742071756973206F7263692E";
  const size_t pdu_len = strlen(pdu_hex);

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos);

  test_result(error == ERR_NONE);
  test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
  test_result(sms.UDH.ID8bit == 95);
  test_result(sms.UDH.ID16bit == -1);
  test_result(sms.UDH.AllParts == 2);
  test_result(sms.UDH.PartNumber == 1);
  test_result(pos == pdu_len / 2);
  test_result(sms.Length == 112);
  test_result(sms.Coding == SMS_Coding_8bit);
  test_result(strcmp(
   "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam a quam quis urna dignissim laoreet et quis orci.",
    DecodeUnicodeString(sms.Text)) == 0);
}


int main(void)
{
  decode_pdu_ascii();
  decode_pdu_gsm();
  decode_pdu_unicode();
  decode_pdu_octet();

  decode_pdu_ascii_multi();
  decode_pdu_gsm_multi();
  decode_pdu_unicode_multi();
  decode_pdu_octet_multi();
}
