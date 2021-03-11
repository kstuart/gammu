#include "test_helper.h"

#define BUFFSIZE 1024

int main(void)
{
	GSM_Error error;
	GSM_Debug_Info *di = set_debug_info();
	GSM_SMSMessage sms;
	unsigned char pdu[BUFFSIZE] = { 0 };
	size_t pos = 0;

	const char *pdu_hex = "07918497900000F6400B918406290000F4000002219221351140A005000363080140CCB7BCDC06A5E1F37A1B447EB3DF72D03C4D0785DB653A0B347EBBE7E531BD4CAFCB4161721A9E9E8FD3EE33A8CC4ED35D206899CD2EBBE9E579BC5E06D9D3F47019E42EC7EB6550F95C9ED3C37316C81CAE8FD3E2FA1C5496BFE7A0B49B054A87C775767A0E4AC3E7F5B60B34ADCFE16537393D9F9741E4F4D99D9ECFD36D10FC2DA7D3D3";
	const size_t pdu_len = strlen(pdu_hex);

	//sms.State = SMS_UnRead;
	test_result(DecodeHexBin(pdu, pdu_hex, pdu_len));
	error = GSM_DecodePDUFrame(di, &sms, pdu, pdu_len / 2, &pos, TRUE);

	test_result(error == ERR_NONE);
	test_result(pos == pdu_len / 2);
	test_result(sms.Length == 153);

	test_result(sms.UDH.Type == UDH_ConcatenatedMessages);
	test_result(sms.UDH.PartNumber == 1);
	test_result(sms.UDH.AllParts == 8);

	test_result(sms.PDU == SMS_Deliver);
	test_result(sms.Coding == SMS_Coding_Default_No_Compression);
	test_result(strcmp("+48790900006", DecodeUnicodeString(sms.SMSC.Number)) == 0);
	test_result(strcmp("+48609200004", DecodeUnicodeString(sms.Number)) == 0);

	char* decoded_msg = DecodeUnicodeString(sms.Text);
	test_result(strcmp(" Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque vitae neque egestas, faucibus eros in, iaculis ipsum. Suspendisse dignissim portti", decoded_msg) == 0);
}

