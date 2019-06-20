#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gammu.h>

#include "test_helper.h"
#include "../../libgammu/cdma.h"

int main(void)
{
  GSM_MultiPartSMSInfo SMSInfo;
  GSM_MultiSMSMessage MSMS;
  GSM_Error error;
  GSM_Debug_Info *di = set_debug_info();

  // 558
  const char sms_text[] = "Lorem Ipsum jest tekstem stosowanym jako przykladowy wypelniacz w przemysle poligraficznym. Zostal po raz pierwszy uzyty w XV w. przez nieznanego drukarza do wypelnienia tekstem probnej ksiazki. Piec wiekow pozniej zaczal byc uzywany przemysle elektronicznym, pozostajac praktycznie niezmienionym. Spopularyzowal sie w latach 60. XX w. wraz z publikacja arkuszy Letrasetu, zawierajacych fragmenty Lorem Ipsum, a ostatnio z zawierajacym rozne wersje Lorem Ipsum oprogramowaniem przeznaczonym do realizacji drukow na komputerach osobistych, jak Aldus PageMaker";
  unsigned char msg_unicode[(sizeof(sms_text) + 1) * 2];
  int i, msg_len = 0;

  GSM_ClearMultiPartSMSInfo(&SMSInfo);
  SMSInfo.Class = 1;
  SMSInfo.EntriesNum = 1;
  SMSInfo.AsciiCoding = TRUE;
  SMSInfo.Entries[0].ID = SMS_ConcatenatedTextLong;
  EncodeUnicode(msg_unicode, sms_text, strlen(sms_text));
  SMSInfo.Entries[0].Buffer = msg_unicode;

  error = GSM_EncodeMultiPartSMS(di, &SMSInfo, &MSMS);

  test_result(error == ERR_NONE);
  test_result(MSMS.Number = 4);
  test_result(MSMS.SMS[MSMS.Number].Length = 99);
  test_result(MSMS.SMS[MSMS.Number].UDH.AllParts = 4);
  test_result(MSMS.SMS[MSMS.Number].UDH.PartNumber = 4);

  for(i = 0; i < MSMS.Number; i++) {
    msg_len += MSMS.SMS[i].Length;
  }
  test_result(msg_len == 558);
}
