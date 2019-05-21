#ifndef _BASE64_H_INCLUDED
#define _BASE64_H_INCLUDED

#include <string>

std::string base64_encode2(const unsigned char*, unsigned int len);
std::string base64_decode2(std::string const& s);

#endif
