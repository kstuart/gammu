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

  const char *pdu_hex = "088110000890952861190530095939100200020014C3870E1C3870E1C387162C58B162C58B1620";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
  test_result(pos == pdu_len);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_ASCII);
  test_result(strcmp("aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_ascii_2(void)
{
	GSM_Error error;
	GSM_Debug_Info *di = set_debug_info();
	GSM_SMSMessage sms;
	unsigned char pdu[BUFFSIZE] = { 0 };
	size_t pos = 0;

	const char *pdu_hex = "098110514294453190F621030313543910020002001691A5053E997B655883A68D3CD069E6820ECCBE00";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

	puts(__func__);

	sms.State = SMS_UnRead;
	test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
	error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
	test_result(pos == pdu_len);
	test_result(sms.Length == 22);
	test_result(sms.Coding == SMS_Coding_ASCII);
	test_result(strcmp("Hi Steve, this is Alex", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_gsm(void)
{
	GSM_Error error;
	GSM_Debug_Info *di = set_debug_info();
	GSM_SMSMessage sms;
	unsigned char pdu[BUFFSIZE] = { 0 };
	size_t pos = 0;

	const char *pdu_hex = "06808781971531201028173823100200090024486F6F7261792120204974277320776F726B696E67201B6501031B3C1B3E1B281B292325";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

	puts(__func__);

	sms.State = SMS_UnRead;
	test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
	error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
	test_result(pos == pdu_len);
	test_result(sms.Length == 31);
	test_result(sms.Coding == SMS_Coding_Default_No_Compression);
	test_result(strcmp("Hooray!  It's working ???[]{}#%", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_gsm_2(void)
{
	GSM_Error error;
	GSM_Debug_Info *di = set_debug_info();
	GSM_SMSMessage sms;
	unsigned char pdu[BUFFSIZE] = { 0 };
	size_t pos = 0;

	const char *pdu_hex = "0781090008002610210303131637100500090138050003940202E8BDDACEDE3E47EA72760F4DA7C3E7BAD7CBDE3EBBECFABB6BFC6EBFACCD63B2397D599BD3B2DC9E1E97E7";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

	puts(__func__);

	sms.State = SMS_UnRead;
	test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
	error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
	test_result(pos == pdu_len);
	test_result(sms.Length == 49);
	test_result(sms.Coding == SMS_Coding_Default_No_Compression);
	test_result(sms.UDH.PartNumber == 2);
	test_result(strcmp("t=5;vmg_url=https://vmg.vzw.com/VMGIMS/VMServices", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_unicode(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *unc_pdu = "088010000890952861190612074740100200040024005A0061017C00F301420107002000670119015B006C01050020006A0061017A01440020";
	const size_t pdu_len_hex = strlen(unc_pdu);
	const size_t pdu_len = pdu_len_hex / 2;

  const unsigned char unc_text[] = { // "Zażółć gęślą jaźń "
    0x00, 0x5a, 0x00, 0x61, 0x01, 0x7c, 0x00, 0xf3, 0x01, 0x42, 0x01, 0x07, 0x00, 0x20, 0x00, 0x67,
    0x01, 0x19, 0x01, 0x5b, 0x00, 0x6c, 0x01, 0x05, 0x00, 0x20, 0x00, 0x6a, 0x00, 0x61, 0x01, 0x7a,
    0x01, 0x44, 0x00, 0x20 };

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, unc_pdu, pdu_len_hex));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
  test_result(pos == pdu_len);
  test_result(sms.Length == 18);
  test_result(sms.Coding == SMS_Coding_Unicode_No_Compression);
  test_result(memcmp(unc_text, sms.Text, sms.Length * 2) == 0);
}

void decode_pdu_latin_1(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881000168909528611906120747401002000800084665636B206F6666";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

  test_result(error == ERR_NONE);
  test_result(pos == pdu_len);
  test_result(sms.Length == 8);
  test_result(sms.Coding == SMS_Coding_8bit);
  test_result(strcmp("Feck off", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_latin_2(void)
{
	GSM_Error error;
	GSM_Debug_Info *di = set_debug_info();
	GSM_SMSMessage sms;
	unsigned char pdu[BUFFSIZE] = { 0 };
	size_t pos = 0;

	const char *pdu_hex = "0681077458307221030408013410020008000E576879206E6F20616E7377657220";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

	puts(__func__);

	sms.State = SMS_UnRead;
	test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
	error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
	test_result(pos == pdu_len);
	test_result(sms.Length == 14);
	test_result(sms.Coding == SMS_Coding_8bit);
	test_result(strcmp("Why no answer ", DecodeUnicodeString(sms.Text)) == 0);
}

void decode_pdu_octet(void)
{
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();
  GSM_SMSMessage sms;
  unsigned char pdu[BUFFSIZE] = { 0 };
  size_t pos = 0;

  const char *pdu_hex = "0881000168909528611906120747401002000000146161616161616161616162626262626262626262";
	const size_t pdu_len_hex = strlen(pdu_hex);
	const size_t pdu_len = pdu_len_hex / 2;

  puts(__func__);

  sms.State = SMS_UnRead;
  test_result(DecodeHexBin(pdu, pdu_hex, pdu_len_hex));
  error = ATCDMA_DecodePDUFrame(di, &sms, pdu, pdu_len, &pos);

	test_result(error == ERR_NONE);
  test_result(pos == pdu_len);
  test_result(sms.Length == 20);
  test_result(sms.Coding == SMS_Coding_8bit);
  test_result(strcmp("aaaaaaaaaabbbbbbbbbb", DecodeUnicodeString(sms.Text)) == 0);

}

int main(void)
{
	decode_pdu_ascii();
	decode_pdu_ascii_2();
  decode_pdu_latin_1();
  decode_pdu_latin_2();
	decode_pdu_gsm();
	decode_pdu_gsm_2();
  decode_pdu_unicode();
  decode_pdu_octet();
}
