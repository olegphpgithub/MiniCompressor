#pragma once

#include <windows.h>
#include <string>

class MiniCompressor
{
public:
	MiniCompressor(void);
	~MiniCompressor(void);
	static std::string CompressString(std::string source_string);
	static std::string DecompressString(std::string comressed_string);
	static errno_t DecompressFromFileToFile(LPTSTR infile, LPTSTR outfile);
    static errno_t DecompressFromFileToMemory(LPTSTR infile, unsigned char **dest, size_t *out_len);
    static unsigned char * MiniCompressor::base64_encode(const unsigned char *src,
        size_t len,
        size_t *out_len);
    static unsigned char * base64_decode(const unsigned char *src, size_t len,
        size_t *out_len);
    static const unsigned char base64_table[65];
};
