#include "MiniCompressor.h"

#include <tchar.h>
#include <stdio.h>

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

#define BUF_SIZE 1024
static unsigned char s_inbuf[BUF_SIZE];
static unsigned char s_outbuf[BUF_SIZE];

const unsigned char MiniCompressor::base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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
        size_t size;
        unsigned char *usz_encoded;
        usz_encoded = base64_encode(pDest, compressed_length, &size);
        std::string tmpstr((char *)usz_encoded);
        compressed_string.assign(tmpstr);
        free(usz_encoded);
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

    size_t size;
    unsigned char *usz_decoded;
    usz_decoded = base64_decode((const unsigned char *)comressed_string.c_str(), comressed_string.length(), &size);

    int cmp_status = uncompress(pDest, &decompressed_length, (const unsigned char *)usz_decoded, size);
    if (cmp_status == Z_OK)
    {
        std::string tmp_string((char*)pDest);
        decompressed_string.assign(tmp_string);
    }
    free(usz_decoded);
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

unsigned char * MiniCompressor::base64_encode(const unsigned char *src, size_t len,
    size_t *out_len)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	int line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len)
		return NULL; /* integer overflow */
	out = (unsigned char *)malloc(olen);
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}


unsigned char * MiniCompressor::base64_decode(const unsigned char *src, size_t len,
    size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = (unsigned char *)malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

