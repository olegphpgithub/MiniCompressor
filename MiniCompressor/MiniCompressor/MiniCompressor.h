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
};
