// DMG.c : This file contains the 'main' function. Program execution begins and ends there.
//
//   Date:18-09-2021 

#include <stdio.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>
#include "dmgParser.h"
#include "apfs.h"
#include "pList.h"

//Portable endianness conversion
#if defined(WIN32) || defined(__WIN32) ||defined(__WIN32__) || defined(__NT__) ||defined(_WIN64)    
#include <Windows.h>
#define be64toh(x) _byteswap_uint64(x)    
#elif __linux__
#include <endian.h>
#elif __unix__ // all unixes not caught above
#include <endian.h>
#endif

#define INFLATE_BUF_LEN 4096
#define ENTRY_TYPE_ZLIB	0x80000005

command_line_args args;

FILE* readImageFile(FILE* stream, char* dmg_path)
{
#if defined(WIN32) || defined(__WIN32) ||defined(__WIN32__) || defined(__NT__) ||defined(_WIN64)   
        size_t bufferSize;
        errno_t e=_dupenv_s(&dmg_path, &bufferSize,"bigandsmall");

        if (e || dmg_path == nullptr)
                printf("The path is not found.");     
#endif

        stream = fopen(dmg_path, "r");

        if (stream == 0)
        {
                printf("Failed to open the file '%s'\n", dmg_path);
        }

        return stream;
}

FILE* parseDMGTrailer(FILE* stream, UDIFResourceFile* dmgTrailer)
{
        int trailerSize = 512;

        if (fseek(stream, -trailerSize, SEEK_END)) {
                printf("Couldn't seek to the desired position in the file.\n");
        }
        else {
                int bytesRead = fread(dmgTrailer, trailerSize, 1, stream);
        }

        return stream;
}

FILE* readXMLOffset(FILE* stream, UDIFResourceFile* dmgTrailer, char** plist)
{
        fseek(stream, be64toh(dmgTrailer->XMLOffset), SEEK_SET);     // Seek to the offset of xml.   
        *plist = (char*)malloc(be64toh(dmgTrailer->XMLLength));      // Allocate memory to plist char array.
        fread(*plist, be64toh(dmgTrailer->XMLLength), 1, stream);    // Read the xml.

        return stream;
}

BLKXTable* decodeDataBlk(const char* data)
{
        size_t decode_size = strlen(data);
        unsigned char* decoded_data = base64_decode(data, decode_size, &decode_size);

        BLKXTable* dataBlk = NULL;
        dataBlk = (BLKXTable*)decoded_data;

        return dataBlk;
}

int decompress_to_file(char *compressed, unsigned long comp_size, char* filename)
{
        z_stream inflate_stream = {0};
        FILE *output_buf = NULL;
        char decompressed[INFLATE_BUF_LEN] = {0};
        unsigned long remaining = 0;
        int ret = 0;

        inflate_stream.zalloc = Z_NULL;
        inflate_stream.zfree  = Z_NULL;
        inflate_stream.opaque = Z_NULL;

        inflate_stream.next_in  = compressed;
        inflate_stream.avail_in = comp_size;

        if ( (ret = inflateInit(&inflate_stream)) != Z_OK ) {
                printf("Error Initializing Inflate! [Ecode - %d]\n", ret);
                return -1;
        }

        inflate_stream.next_out  = decompressed;
        inflate_stream.avail_out = INFLATE_BUF_LEN;

        if ((output_buf = fopen(filename, "a")) == NULL) {
                printf("Unable to create file decompressed!\n");
                ret = -1;
                goto inflate_end;
        }

        while ((ret = inflate(&inflate_stream,  Z_FINISH )) != Z_STREAM_END) {
                if ((ret == Z_OK || ret == Z_BUF_ERROR ) && inflate_stream.avail_out == 0) {
                        if (fwrite(decompressed, 1, INFLATE_BUF_LEN, output_buf) == 0) {
                                printf("Error Writing to output file!\n");
                                goto close;
                        }

                        memset(decompressed, 0, sizeof(decompressed));
                        inflate_stream.next_out  = decompressed;
                        inflate_stream.avail_out = INFLATE_BUF_LEN;
                }
                else {
                        printf("Error Inflating! [Code - %d]\n", ret);
                        goto close;
                }
        }

        remaining = inflate_stream.total_out % INFLATE_BUF_LEN;
        if (remaining == 0 && inflate_stream.total_out > 0)
                remaining = INFLATE_BUF_LEN;

        if (fwrite(decompressed, 1, remaining, output_buf) == 0)
                printf("Error Writing to output file!\n");

close:
        fclose(output_buf);
inflate_end:
        inflateEnd (&inflate_stream);
        return ret;
}


// This function will be called by printdmgBlocks
void readDataBlks(BLKXTable* dataBlk, FILE* stream, char* filename)
{
        int sectorSize = 512;
        uint8_t* compressedBlk = NULL; 

        for (int noOfChunks = 0; noOfChunks < (be32toh(dataBlk->NumberOfBlockChunks)); noOfChunks++)
        {
                fseek(stream, be64toh(dataBlk->chunk[noOfChunks].CompressedOffset), SEEK_SET);

                compressedBlk = (uint8_t*)malloc(be64toh(dataBlk->chunk[noOfChunks].CompressedLength));
                size_t result=fread(compressedBlk, 1, be64toh(dataBlk->chunk[noOfChunks].CompressedLength), stream);

                //Only supports zlib compression
                //To-Do: support other compression types
                if (be32toh(dataBlk->chunk[noOfChunks].EntryType) == ENTRY_TYPE_ZLIB) {
                        if (decompress_to_file(compressedBlk, result, filename) < 0) {
                                printf("Failed to decompress block\n");
                        }
                }
        }
}

int checkCommandLineArguments(char** argv, int argc)
{
        int result = 0, i = 0;

        if (argc < 2 || argc > 6) {
                printf("Invalid number of arguments!\n");
                result = 1;
        } else if (argc == 2) {
                args.fs_structure = 1;
                args.volume = 1;
                args.volume_ID = 1026;
        } else {
                switch (argv[2][1]) {
                        case 'c':
                        case 'C':
                                if (argc == 3) {
                                        args.container = 1;
                                } else {
                                        printf("Container Superblock takes no arguments\n");
                                        result = 1;
                                }
                                break;
                        case 'v':
                        case 'V':
                                if (argc == 3) {
                                        args.volume = 1;
                                } else {
                                        args.volume = 1;
                                        /* Make sure that argument contains nothing but digits */
                                        if (argv[3]) {
                                                if(strspn(argv[3], "0123456789") != strlen(argv[3])) {
                                                        printf("Volume id has to be an interger!\n");
                                                        result = 1;
                                                } else {
                                                        args.volume_ID = atoi(argv[3]);
                                                }
                                        }
                                        if (argv[4]) {
                                                if ((strcmp(argv[4], "-fs") == 0) || (strcmp(argv[4], "-FS") == 0)) {
                                                        args.fs_structure = 2;
                                                } else {
                                                        printf("Please use \"-fs\" to see the file system structure\n");
                                                        result = 1;
                                                }
                                        }
                                }
                                break;
                        default:
                                printf("Invalid parameter!\n");
                                result = 1;
                }
        }

        for (i = 1; i < argc; ++i)
                if (strcmp(argv[i], "-d") == 0)
                        args.debug_mode = 1;

        return result;
}

void printUsage(char **argv)
{
        printf("Usage :\n\n%s <DMG_FILE>     	         	Prints the Disk Image Structure\n \
                        -c                      Container Superblock Information\n \
                        -v [ Volume ID ]        All Vol SuperBlock Information | Specified Volume's Information \n \
                        -f <file_name>          Displays file content\n \
                        -v <Volume_ID> -fs      Displays File system Structure\n \
                        -d			Debug Mode\n", argv[0]);	
}

int main(int argc, char** argv)
{
        FILE* stream = NULL;
        UDIFResourceFile dmgTrailer;
        char *plist;

        // check command line arguments 
        if (checkCommandLineArguments( argv , argc) == 1) {
                printUsage(argv);
                return 1;
        }

        stream = readImageFile(stream, argv[1]);
        parseDMGTrailer(stream, &dmgTrailer);      // reference of dmgTrailer is passed.
        readXMLOffset(stream,&dmgTrailer,&plist);  //reference of dmgTrailer and plist is passed
        char* apfsData = parsePlist(plist, stream);  //Parse the pList and find the APFS data block

        if (apfsData != NULL)
        {
                char *filename = "APFS Image Decompressed";

                dprintf("\nDecompressing DMG file...\n\n");

                //Decompress the APFS data blocks
                BLKXTable* dataBlk = NULL;
                dataBlk = decodeDataBlk(apfsData);   // decode the data block.
                readDataBlks(dataBlk, stream, filename);    // loop through the chunks to decompress each.

                //Parse the APFS image
                parse_APFS(filename);
        }
        else
        {
                printf("Failed to parse the DMG pList\n");
        }

        free(apfsData);
        free(plist);

        printf("\n");

        return 0;
}
