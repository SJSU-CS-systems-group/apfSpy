// DMG.c : This file contains the 'main' function. Program execution begins and ends there.
//
//   Date:18-09-2021 

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "dmgParser.h"
#include "apfs.h"

#if defined(WIN32) || defined(__WIN32) ||defined(__WIN32__) || defined(__NT__) ||defined(_WIN64)    
           #	include <Windows.h>
           #define be64toh(x) _byteswap_uint64(x)    
#elif __linux__
      #	include <endian.h>
#elif __unix__ // all unixes not caught above
      #	include <endian.h>
#endif

#define INFLATE_BUF_LEN 4096
#define ENTRY_TYPE_ZLIB	0x80000005

FILE* readImageFile(FILE* stream, char* dmg_path)
{
 #if defined(WIN32) || defined(__WIN32) ||defined(__WIN32__) || defined(__NT__) ||defined(_WIN64)   
    size_t bufferSize;
    errno_t e=_dupenv_s(&dmg_path, &bufferSize,"bigandsmall");
     if (e || dmg_path == nullptr)
         printf("The path is not found.");     
#endif
    stream = fopen(dmg_path, "r");

    if (stream != 0)
    {
        printf("The file '%s' was opened\n", dmg_path);
    }
    else
    {
        printf("The file '%s' was not opened\n", dmg_path);
    }
    return stream;
}

FILE* parseDMGTrailer(FILE* stream, UDIFResourceFile* dmgTrailer)
{
    int trailerSize = 512;

    if (fseek(stream, -trailerSize, SEEK_END)) {
        printf("Couldn't seek to the desired position in the file.\n");
    }
    else
    {
        // printf("Seeked to the beginning of Trailer.\n");

        int bytesRead = fread(dmgTrailer, trailerSize, 1, stream);
        // printf("The bytes read %d\n", bytesRead);
        
    }
    return stream;
}

FILE* readXMLOffset(FILE* stream, UDIFResourceFile* dmgTrailer, char** plist)
{
    fseek(stream, be64toh(dmgTrailer->XMLOffset), SEEK_SET);     // Seek to the offset of xml.   
    *plist = (char*)malloc(be64toh(dmgTrailer->XMLLength));      // Allocate memory to plist char array.
    fread(*plist, be64toh(dmgTrailer->XMLLength), 1, stream);    // Read the xml.

    //printf("The xml is : %s", *plist);
    return stream;
}

BLKXTable* decodeDataBlk(const char* data)
{
    size_t decode_size = strlen(data);
    printf("The size of data is : %lu\n", decode_size);
    unsigned char* decoded_data = base64_decode(data, decode_size, &decode_size);
    printf("DEcode structure is %s\n", decoded_data);

    BLKXTable* dataBlk = NULL;
    dataBlk = (BLKXTable*)decoded_data;

    // we can remove these print statements later. Keeping them for reference.
    printf("Version: %u\n", be32toh(dataBlk->Version));
    printf("Sector count: %lu \n", be64toh(dataBlk->SectorCount));
    printf("No. Block Chunks: %u \n", be32toh(dataBlk->NumberOfBlockChunks));

    printf("BLKXChunkEntry :\n");
    printf("BLK0 Entry type: %x\n", be32toh(dataBlk->chunk[0].EntryType));
    printf("BLK0 SectorNumber: %lu\n", be64toh(dataBlk->chunk[0].SectorNumber));
    printf("BLK0 Sector Count: %lu\n", be64toh(dataBlk->chunk[0].SectorCount));
    printf("BLK0 Compressed Offset: %lu\n", be64toh(dataBlk->chunk[0].CompressedOffset));
    printf("BLK0 Compressed Length: %lu\n", be64toh(dataBlk->chunk[0].CompressedLength));
    printf("\nBLK1 Entry type: %x\n", be32toh(dataBlk->chunk[1].EntryType));
    printf("BLK1 SectorNumber: %lu\n", be64toh(dataBlk->chunk[1].SectorNumber));
    printf("BLK1 Sector Count: %lu\n", be64toh(dataBlk->chunk[1].SectorCount));
    printf("BLK1 Compressed Offset: %lu\n", be64toh(dataBlk->chunk[1].CompressedOffset));
    printf("BLK1 Compressed Length: %lu\n", be64toh(dataBlk->chunk[1].CompressedLength));

    return dataBlk;
}

int decompress_to_file(char *compressed, unsigned long comp_size, int block_no)
{
        z_stream inflate_stream = {0};
        FILE *output_buf = NULL;
        char decompressed[INFLATE_BUF_LEN] = {0}, filename[16] = {0};
        int ret = 0;

        snprintf(filename, sizeof(filename), "decompressed%d", block_no);

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

	printf("avail_in - %u, avail_out - %u\n", inflate_stream.avail_in, inflate_stream.avail_out);

        while ((ret = inflate(&inflate_stream,  Z_FINISH )) != Z_STREAM_END) {
                printf("Did not reach end. ret = %d, total out - %lu avail_in - %u (ZOK- %d, buf error - %d)\n", ret, inflate_stream.total_out, inflate_stream.avail_in, Z_OK, Z_BUF_ERROR);
                if ((ret == Z_OK || ret == Z_BUF_ERROR ) && inflate_stream.avail_out == 0) {
                        if (fwrite(decompressed, 1, INFLATE_BUF_LEN, output_buf) == 0) {
                                printf("Error Writing to output file!\n");
                                goto close;
                        }
                        memset(decompressed, 0, sizeof(decompressed));
                        inflate_stream.next_out  = decompressed;
                        inflate_stream.avail_out = INFLATE_BUF_LEN;
                } else {
			printf("Error Inflating! [Code - %d]\n", ret);
			goto close;
		}
	}

	remaining = inflate_stream.total_out % INFLATE_BUF_LEN;
	if (remaining == 0 && inflate_stream.total_out > 0)
		remaining = INFLATE_BUF_LEN;
		
	if (fwrite(decompressed, 1, remaining, output_buf) == 0)
		printf("Error Writing to output file!\n");
	else
		printf("Decompressed Successfully to file 'decompressed' size - %lu\n", inflate_stream.total_out);
close:
	fclose(output_buf);
inflate_end:
	inflateEnd (&inflate_stream);
	return ret;
}

void dump_hex( char *stream, unsigned long size )
{
        int i;

        printf("\n");
        for(i = 0; i < size; ++i) {
                if (i%16==0)
                        printf("\n");
                printf("%02x ", stream[i]);
        }
        printf("\n");
}

// This function will be called by printdmgBlocks
void readDataBlks(BLKXTable* dataBlk,FILE* stream, int block_no)
{
	int sectorSize = 512;
	uint8_t* compressedBlk = NULL; 
	fseek(stream, sectorSize * be64toh(dataBlk->SectorNumber), SEEK_SET);  //Seeking to the start od the data block
	printf("\nThe offset  is %lu\n", be64toh(dataBlk->chunk[0].CompressedOffset));

	fseek(stream, be64toh(dataBlk->chunk[0].CompressedOffset)*sectorSize , SEEK_CUR);
	for (int noOfChunks = 0; noOfChunks < (be32toh(dataBlk->NumberOfBlockChunks)); noOfChunks++)
	{
		printf("The offset length is %lu:", be64toh(dataBlk->chunk[noOfChunks].CompressedLength));
		compressedBlk = (uint8_t*)malloc(be64toh(dataBlk->chunk[noOfChunks].CompressedLength));
		size_t result=fread(compressedBlk, 1, be64toh(dataBlk->chunk[noOfChunks].CompressedLength), stream);
		printf("The result of fread is %lu\n", result);
		dump_hex(compressedBlk, result);			
		if (be32toh(dataBlk->chunk[noOfChunks].EntryType) != ENTRY_TYPE_ZLIB) {
			printf("Ignoring Non zlib compression for now\n");
		} else if (decompress_to_file(compressedBlk, result, block_no) < 0) {
			printf("Failed to decompress block\n");
		}	
    }	
}

//Searches through the siblings of a node for the given type
xmlNode* findSiblingByType(xmlNode *node, char* type)
{
	const xmlChar *nodeType = (const xmlChar*)type;

	//Advance the pointer to its sibling
	node = node->next;

	while (node != NULL && !xmlStrEqual(node->name, nodeType))
		node = node->next;

	return node;
}

//Searches through the siblings of a node for the given content
xmlNode* findSiblingByContent(xmlDoc *doc, xmlNode *node, char* content)
{
	const xmlChar *nodeContent = (const xmlChar*)content;

	for(xmlNode* sibling = node->next; sibling != NULL; sibling = sibling->next)
	{
		if (sibling->type == XML_ELEMENT_NODE)
		{
			const xmlChar *nodeText = xmlNodeListGetString(doc, sibling->children, 1);
		
			if (xmlStrEqual(nodeText, nodeContent))
				return sibling;
		}

	}

	return NULL;
}

//A breadth-first search for an xmlNode with the given contents
xmlNode* findNodeByText(xmlDoc *doc, xmlNode *root, char *searchText)
{
	//Search all siblings
	xmlNode* sibling = findSiblingByContent(doc, root, searchText);

	if (sibling != NULL)
		return sibling;

	//Search all children
	for (xmlNode* sibling = root; sibling != NULL; sibling = sibling->next)
	{
		if (sibling->type == XML_ELEMENT_NODE)
		{
			xmlNode* node = findNodeByText(doc, sibling->children, searchText);

			if (node != NULL)
				return node;
		}
	}

	//Item not found in this branch
	return NULL;
}

char* removeWhiteSpace(char* string)
{
	char* copy;
	int length = strlen(string);
	int wsCount = 0;
	int newLength;

	int strIndex = 0;
	int copyIndex = 0;

	//Count the number of white spaces
	for (strIndex = 0; strIndex < length; strIndex++)
	{
		if (string[strIndex] == ' ' ||
			string[strIndex] == '\t'||
			string[strIndex] == '\n'||
			string[strIndex] == '\r')
			wsCount++;
	}

	//Allocate memory for a new string
	//Add an extra byte for the null terminator character
	newLength = length - wsCount + 1;
	copy = (char*) malloc(newLength * sizeof(char));

	//Copy all non-whitespace characters
	for (strIndex = 0; strIndex <= length; strIndex++)
	{
		if (string[strIndex] != ' ' &&
			string[strIndex] != '\t'&&
			string[strIndex] != '\n'&&
			string[strIndex] != '\r')
		{
			copy[copyIndex] = string[strIndex];
			copyIndex++;
		}
	}

	return copy;
}

int printDmgBlocks(xmlDoc *doc, xmlNode *blkxNode,FILE* stream,command_line_args args)
{
	xmlNode *array = findSiblingByType(blkxNode, "array");

	if (array == NULL) {
		printf("plist format is incorrect");
		return 1;
	}

	xmlNode *block = findSiblingByType(array->children, "dict");

	if (block == NULL) {
		printf("plist format is incorrect");
		return 1;
	}

	//Print all blocks
	for (int i = 0; block != NULL; i++)
	{
		printf("\nBlock %i\n", i);

		//Find the CFName key
		xmlNode *node = findSiblingByContent(doc, block->children, "CFName");

		if (node == NULL) {
			printf("plist format is incorrect");
			return 1;
		}

		//Find the CFName content node
		node = findSiblingByType(node, "string");

		if (node == NULL) {
			printf("plist format is incorrect");
			return 1;
		}

		//Print the block CFName
		const xmlChar *name = xmlNodeListGetString(doc, node->children, 1);
		printf("CFName: %s\n", name);

		//Find the data node
		node = findSiblingByType(node, "data");

		if (node == NULL) {
			printf("plist format is incorrect");
			return 1;
		}

		//Print the block data
		char *data = xmlNodeListGetString(doc, node->children, 1);
		char *dataNoWs = removeWhiteSpace(data);
		printf("Data:\n%s\n", dataNoWs);
		BLKXTable* dataBlk = NULL; 
		dataBlk=decodeDataBlk(dataNoWs);   // decode the data block.
		readDataBlks(dataBlk, stream, i);     // loop through the chunks to decompress each.
		
		if (strstr(name, "Apple_APFS") != NULL) {
			printf("Parsing APFS\n");
			parse_APFS(i,args);
		}

		free(data);
		free(dataNoWs);

		//Go to the next block
		block = findSiblingByType(block, "dict");
	}
}

//Adapted from the article:
//https://www.developer.com/database/libxml2-everything-you-need-in-an-xml-library/
void parseXML(char* xmlStr,FILE* stream,command_line_args args)
{
	xmlDoc *doc = NULL;
	xmlNode *root = NULL;

	//Parse the given string into an XML file
	doc = xmlParseDoc(xmlStr);

	if (doc == NULL) {
		printf("error: could not parse file %s\n", xmlStr);
		return;
	}

	root = xmlDocGetRootElement(doc);

	//Get the blkx node
	xmlNode *blkx = findNodeByText(doc, root, "blkx");

	if (blkx == NULL) {
		printf("error: DMG plist is formatted incorrectly\n");
		return;
	}

	//Print the plist blocks
	printDmgBlocks(doc, blkx,stream,args);

	//Free any libXML2 memory
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

int checkCommandLineArguments(char** argv, int argc)
{
	int result = 0;
	if (argc < 2) {
    	printf("Not Enough Arguments!\n");
    	return 1;
    }else if(argc > 5){
		printf("Excess arguments entered!\n");
		result =1;
	}
	else if( argc == 3){
		if(strcmp(argv[2],"-c") != 0 && strcmp(argv[2],"-C") != 0 && strcmp(argv[2],"-v") != 0 && strcmp(argv[2],"-V") != 0)
		{
			printf("Wrong parameter entered!\n");
			result =1;
		}
	}else if (argc == 4){
		if( strcmp(argv[2],"-v") == 0 || strcmp(argv[2],"-V") == 0)
		{
			// Make sure that argument contains nothing but digits
            for (int i = 0; argv[3][i] != '\0'; i++)
            {
                if (!isdigit(argv[3][i]))
                {
                    printf("Volume id has to be an interger!\n");
					result =1;
                    break;
                }//if
            }//for
		}//if 
	}else if(argc == 5 && (strcmp(argv[4],"-fs") != 0 && strcmp(argv[4],"-FS") != 0) ){
		printf("Wrong parameter entered for Volume's file structure! \n");
		result=1;
	}
	return result;
}

void printUsage()
{
	printf("Usage :              apfs_parser <DMG_FILE>               Prints the Disk Image Structure\n \
	                          -c                      Container Superblock Information\n \
                                 -v [ Volume ID ]        All Vol SuperBlock Information | Specified Volume's Information \n \
                                 -f <file_name>          Displays file content\n \
                                 -v <Volume_ID> -fs      Displays File system Structure\n");	
}

command_line_args fillCommandLineArguments(char **argv,int argc)
{
	command_line_args args={0};
	if( strlen(argv[2]) > 0 )
	{
		if(strcmp(argv[2],"-c") == 0 || strcmp(argv[2],"-C") == 0)
		    args.container = 1;
	    else if(strcmp(argv[2],"-v") == 0 || strcmp(argv[2],"-V") == 0){
	        args.volume = 1;
			if(argc == 4)
			{
				args.volume_ID = atoi(argv[3]);
			}
		}else{
			args.file = 1;
		     if(argc == 4)
			{
				strncpy(args.file_name,argv[3],strlen(argv[3]));
			}
		}
	}
	if( argc == 5 )
		args.fs_structure = 1;
	
	return args;
}
int main(int argc, char** argv)
{
    FILE* stream = NULL;
    UDIFResourceFile dmgTrailer;
    char *plist;
    command_line_args args;
    // check command line arguments 
	int result = checkCommandLineArguments( argv , argc);
	if(result == 1)
	{
		printUsage();
		return 1;
	}else{
		args=fillCommandLineArguments(argv,argc);
	}
    stream = readImageFile(stream, argv[1]);
    parseDMGTrailer(stream, &dmgTrailer);      // reference of dmgTrailer is passed.
    readXMLOffset(stream,&dmgTrailer,&plist);  //reference of dmgTrailer and plist is passed
    parseXML(plist,stream,args);
	
       
    

    free(plist);
	return 0;
}


