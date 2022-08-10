#include <xtl.h>
#include <stdio.h>
#include <string.h>

typedef unsigned __int64 QWORD;

#include "Xbox\kernel.h"
#include "Xbox\Console\Console.h"
#include "Crypto\HMAC\HMACSHA1.h"
#include "Resources\ContentImage.h"

extern "C" 
{ 
#include "INI\ini.h"
}

//standalone
#define ConfigFileA "D:\\config.ini"

//CDX
#define CDX_FontFileA "D:\\CDXMedia\\arialuni.ttf"
#define CDX_FontFileW L"D:\\CDXMedia\\arialuni.ttf"

#define CDXConfigFileMinIndex 0
#define CDXConfigFileMaxIndex 4

const CHAR* CDXConfigFilesA[] = {
	"D:\\CDXU\\config_en.inx",
	"D:\\CDXU\\config_jp.inx",
	"D:\\CDXU\\config_ger.inx",
	"D:\\CDXU\\config_fr.inx",
	"D:\\CDXU\\config_spa.inx"
};

#define MAX_QUESTS 100

struct Quest {

	//ini stuff
	CHAR File[MAX_PATH];
	CHAR OfferID_STRING[16];
	BYTE OfferID_BUFFER[8];

	//built stuff
	CHAR SourceFilePath[MAX_PATH];
	CHAR DestinationFilePath[MAX_PATH];

	CHAR ContentMetadataFilePath[MAX_PATH];
	CHAR InternalQuestName[0x20];

	DWORD SourceFileSize;
	BOOL SourceFileExists;
	BOOL SourceFileIsValid;
};

struct Signing {
	CHAR TitleID_STRING[8];
	BYTE TitleID_BUFFER[4];
	BYTE TitleSignatureKey[0x10];
};

//application
VOID CleanUp();
BOOL ParseConfig(Signing* signing, DWORD& quest_count, Quest* quests);

//content/quest data
BOOL InstallContentImage();
BOOL InstallQuest(Signing* signing, Quest* quest);
VOID SignQuest(Signing* signing, BYTE* quest_data, DWORD quest_data_length, BYTE* signature);
VOID ValidateQuestFile(Quest* quest);
DWORD CRC32(BYTE* buffer, DWORD length);

//strings
BOOL ValidHexString(CHAR* string, DWORD length);
VOID CopyHexToBytes(CHAR* string, DWORD length, BYTE* buffer, BOOL reverse);
WCHAR* ConvertUTF8ToUnicodeSimple(CHAR* string, DWORD string_length);

//contentmeta data
BOOL CreateContentMetadata(CHAR* display_name, DWORD title_id, QWORD offer_id, DWORD content_flags, BYTE* &result, DWORD &result_length);
BOOL CalculateContentHeaderSignature(DWORD TitleId, BYTE* buffer, DWORD length, BYTE* result);

//input
DWORD AwaitButtonPress();

//filesystem
VOID CreateOfferDirectory(CHAR* OfferID_STRING);
VOID DeleteFileIfExists(CHAR* FilePath);
BOOL FileExists(CHAR* FilePath);