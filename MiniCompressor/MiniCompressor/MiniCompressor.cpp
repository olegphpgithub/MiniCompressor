#include "MiniCompressor.h"

#include <tchar.h>
#include <stdio.h>

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"
#include "base64.h"

#define BUF_SIZE 1024
static unsigned char s_inbuf[BUF_SIZE];
static unsigned char s_outbuf[BUF_SIZE];

MiniCompressor::MiniCompressor(void)
{
}

MiniCompressor::~MiniCompressor(void)
{
}

std::string MiniCompressor::CompressString(std::string source_string)
{
    std::string compressed_string;
    uLong compressed_length = compressBound(source_string.length());
    unsigned char *pDest;
    pDest = (mz_uint8 *)malloc((size_t)compressed_length);

    int cmp_status = compress(pDest, &compressed_length, (const unsigned char *)source_string.c_str(), source_string.length());
    if (cmp_status == Z_OK)
    {
        compressed_string = base64_encode2(pDest, compressed_length);
    }
    free(pDest);

    return compressed_string;
}

std::string MiniCompressor::DecompressString(std::string comressed_string)
{
    std::string decompressed_string, decomded_string;
    uLong decompressed_length = 1024;
    unsigned char *pDest;
    pDest = (mz_uint8 *)malloc((size_t)decompressed_length);
    memset(pDest, 0, (size_t)decompressed_length);
    decomded_string = base64_decode2(comressed_string);
    int cmp_status = uncompress(pDest, &decompressed_length, (const unsigned char *)decomded_string.c_str(), decomded_string.length());
    if (cmp_status == Z_OK)
    {
        std::string tmp_string((char*)pDest);
        decompressed_string.assign(tmp_string);
    }
    free(pDest);
    return decompressed_string;
}

errno_t MiniCompressor::DecompressFromFileToFile(LPTSTR infile, LPTSTR outfile)
{
    errno_t err;
    FILE *pInfile, *pOutfile;
    unsigned __int64 infile_size;
    int level = Z_BEST_COMPRESSION;
    z_stream stream;
    
    err = _tfopen_s(&pInfile, infile, TEXT("rb"));
    if (err != 0)
    {
        printf("Failed opening input file!\n");
        return EXIT_FAILURE;
    }

    // Determine input file's size.
    fseek(pInfile, 0, SEEK_END);
    unsigned __int32 file_loc = ftell(pInfile);
    fseek(pInfile, 0, SEEK_SET);

    if ((file_loc < 0) || (file_loc > INT_MAX))
    {
        // This is not a limitation of miniz or tinfl, but this example.
        printf("File is too large to be processed by this example.\n");
        return EXIT_FAILURE;
    }

    infile_size = file_loc;
    
    // Open output file.
    err = _tfopen_s(&pOutfile, outfile, "wb");
    if (err != 0)
    {
        //printf("Failed opening output file!\n");
        return EXIT_FAILURE;
    }

    // Init the z_stream
    memset(&stream, 0, sizeof(stream));
    stream.next_in = s_inbuf;
    stream.avail_in = 0;
    stream.next_out = s_outbuf;
    stream.avail_out = BUF_SIZE;

    // Decompression.
    unsigned __int32 infile_remaining = (unsigned __int32)infile_size;

    if (inflateInit(&stream))
    {
      //printf("inflateInit() failed!\n");
      return EXIT_FAILURE;
    }

    for ( ; ; )
    {
      int status;
      if (!stream.avail_in)
      {
        // Input buffer is empty, so read more bytes from input file.
        unsigned __int32 n = min(BUF_SIZE, infile_remaining);

        if (fread(s_inbuf, 1, (size_t)n, pInfile) != n)
        {
          //printf("Failed reading from input file!\n");
          return EXIT_FAILURE;
        }

        stream.next_in = s_inbuf;
        stream.avail_in = n;

        infile_remaining -= n;
      }

      status = inflate(&stream, Z_SYNC_FLUSH);

      if ((status == Z_STREAM_END) || (!stream.avail_out))
      {
        // Output buffer is full, or decompression is done, so write buffer to output file.
        unsigned __int32 n = BUF_SIZE - stream.avail_out;
        if (fwrite(s_outbuf, 1, n, pOutfile) != n)
        {
          //printf("Failed writing to output file!\n");
          return EXIT_FAILURE;
        }
        stream.next_out = s_outbuf;
        stream.avail_out = BUF_SIZE;
      }

      if (status == Z_STREAM_END)
        break;
      else if (status != Z_OK)
      {
        //printf("inflate() failed with status %i!\n", status);
        return EXIT_FAILURE;
      }
    }

    if (inflateEnd(&stream) != Z_OK)
    {
      //printf("inflateEnd() failed!\n");
      return EXIT_FAILURE;
    }
    fclose(pInfile);
    fclose(pOutfile);

    return EXIT_SUCCESS;
}
