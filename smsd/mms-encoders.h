#ifndef GAMMU_MMS_ENCODERS_H
#define GAMMU_MMS_ENCODERS_H

MMSError MMS_EncodeNoValue(SBUFFER stream);
MMSError MMS_EncodeUintVar(SBUFFER stream, MMSUint n);
MMSError MMS_EncodeValueLength(SBUFFER stream, size_t len);
MMSError MMS_EncodeQValue(SBUFFER stream, MMSQValue value);
MMSError MMS_EncodeShortInteger(SBUFFER stream, MMSShortInt value);
MMSError MMS_EncodeLongInteger(SBUFFER stream, MMSLongInt value);
MMSError MMS_EncodeInteger(SBUFFER stream, MMSLongInt value);
MMSError MMS_EncodeEncodedText(SBUFFER stream, int charset, ENCODEDSTRING text);
MMSError MMS_EncodeAddress(SBUFFER stream, ENCODEDSTRING);
MMSError MMS_EncodeFromAddress(SBUFFER stream, ENCODEDSTRING from);
MMSError MMS_EncodeTokenText(SBUFFER stream, CSTR text);
MMSError MMS_EncodeExtensionMedia(SBUFFER stream, CSTR extension);
MMSError MMS_EncodeConstrainedEncoding(SBUFFER stream, MMSVALUE in);
MMSError MMS_EncodeWellKnownMedia(SBUFFER stream, MMSVALUEENUM ct);
MMSError MMS_EncodeWellKnownCharset(SBUFFER stream, MMSCHARSET charset);
MMSError MMS_EncodeTypedParameter(SBUFFER stream, TYPEDPARAM param);
MMSError MMS_EncodeUntypedParameter(SBUFFER stream, UNTYPEDPARAM param);
MMSError MMS_EncodeParameters(SBUFFER stream, MMSPARAMETERS params);
MMSError MMS_EncodeMediaType(SBUFFER stream, MMSCONTENTTYPE value);
MMSError MMS_EncodeContentGeneralForm(SBUFFER stream, MMSCONTENTTYPE value);
MMSError MMS_EncodeContentType(SBUFFER stream, MMSVALUE value);
MMSError MMS_EncodeHeader(SBUFFER stream, MMSHEADER header);
MMSError MMS_EncodeHeaders(SBUFFER stream, MMSHEADERS headers);

#endif //GAMMU_MMS_ENCODERS_H
