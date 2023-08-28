#include "base64_cpp.h"
#include "base64.h"
std::string LiMao::Encryption::Base64::Encode(RbsLib::IBuffer const& buffer) noexcept
{
	char* encode_out = new char[BASE64_ENCODE_OUT_SIZE(buffer.GetLength())];
	base64_encode((unsigned char*)buffer.Data(), buffer.GetLength(), encode_out);
	std::string result(encode_out);
	delete[] encode_out;
	return result;
}
#include <cstring>
RbsLib::Buffer LiMao::Encryption::Base64::Decode(const std::string& str)
{
	char* decode_out = new char[BASE64_DECODE_OUT_SIZE(str.length())];
	if (0 == base64_decode(str.c_str(), str.length(), (unsigned char*)decode_out))
	{
		delete[] decode_out;
		throw EncryptionException("Base64 decode error");
	}
	RbsLib::Buffer result(decode_out, BASE64_DECODE_OUT_SIZE(str.length()));
	delete[] decode_out;
	return result;
}
