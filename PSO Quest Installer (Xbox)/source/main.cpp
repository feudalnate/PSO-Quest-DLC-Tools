#include "main.h"

BOOL LaunchedByCDX = FALSE;
LAUNCH_DATA LaunchData; //need to hold on to this if launched by CDX, CDX needs it back
DWORD CDXConfigFileIndex = 0;

Signing* ConfigSigning = NULL;
Quest* ConfigQuests = NULL;
DWORD ConfigQuestsCount = 0;

XDEVICE_PREALLOC_TYPE Controllers[] = { { XDEVICE_TYPE_GAMEPAD, 4 } };

DWORD PrintBufferSize = 1024;
WCHAR* PrintBuffer;

VOID main() {

	//check if we were launched by CDX
	DWORD LaunchDataType = 0;
	if (XGetLaunchInfo(&LaunchDataType, &LaunchData) == ERROR_SUCCESS)
		if (LaunchDataType == LDT_TITLE && ((LD_DEMO*)&LaunchData)->dwID == LAUNCH_DATA_DEMO_ID) {
			
			DWORD index = ((LD_DEMO*)&LaunchData)->dwTimeout;
			if (index != 0 && (index % 1000) == 0) {
				
				index /= 1000;
				if (index >= CDXConfigFileMinIndex && index <= CDXConfigFileMaxIndex)
					CDXConfigFileIndex = index;
			}

			LaunchedByCDX = TRUE;
		};
	
	//init UI
	if (LaunchedByCDX && FileExists(CDX_FontFileA))
		ConsoleInit(640, 480, CDX_FontFileW, 14);
	else
		ConsoleInit(640, 480, NULL, 0);

	while(!ConsoleIsInitialized())
		Sleep(15);

	//init device stack
	XInitDevices((sizeof(Controllers) / sizeof(XDEVICE_PREALLOC_TYPE)), Controllers);

	//allocate some space to do string formatting
	PrintBuffer = (WCHAR*)malloc(PrintBufferSize);

	//start application
	ConsoleWriteLine(3);
	ConsoleWriteLine(L"          Phantasy Star Online - Quest Installer");
	ConsoleWriteLine(L"          https://github.com/feudalnate");

	//init/parse config
	ConfigSigning = new Signing;
	ConfigQuests = new Quest[MAX_QUESTS];

	for(DWORD i = 0; i < MAX_QUESTS; i++) {
		ZeroMemory(ConfigQuests[i].File, sizeof(ConfigQuests[i].File));
		ZeroMemory(ConfigQuests[i].OfferID_BUFFER, sizeof(ConfigQuests[i].OfferID_BUFFER));
		ZeroMemory(ConfigQuests[i].OfferID_STRING, sizeof(ConfigQuests[i].OfferID_STRING));
		ZeroMemory(ConfigQuests[i].SourceFilePath, sizeof(ConfigQuests[i].SourceFilePath));
		ZeroMemory(ConfigQuests[i].DestinationFilePath, sizeof(ConfigQuests[i].DestinationFilePath));
		ZeroMemory(ConfigQuests[i].ContentMetadataFilePath, sizeof(ConfigQuests[i].ContentMetadataFilePath));
		ZeroMemory(ConfigQuests[i].InternalQuestName, sizeof(ConfigQuests[i].InternalQuestName));
		ConfigQuests[i].SourceFileExists = FALSE;
		ConfigQuests[i].SourceFileIsValid = FALSE;
	}

	BOOL ParseSuccess = ParseConfig(ConfigSigning, ConfigQuestsCount, ConfigQuests);

	if (!ParseSuccess || ConfigQuestsCount == 0) { //bad parse, bad config, or no quests entries

		if (ParseSuccess && ConfigQuestsCount == 0) {

			ConsoleWriteLine(2);
			ConsoleWriteLine(L"          ERROR: No quest entries specified in config.ini");
		}

		ConsoleWriteLine(2);

		if (LaunchedByCDX) {
			ConsoleWriteLine(L"          B: Exit to installer");
			ConsoleWriteLine(L"          Y: Exit to dashboard");
		}
		else {
			ConsoleWriteLine(L"          B: Exit to dashboard");
		}

		while(1) {

			switch(AwaitButtonPress()) {

			case XINPUT_GAMEPAD_B:
				if (LaunchedByCDX)
					goto exit_to_cdx;
				else
					goto exit_to_dashboard;

			case XINPUT_GAMEPAD_Y:
				if (LaunchedByCDX)
					goto exit_to_dashboard;

			default:
				continue;

			}

		}
	}

	//build paths/check quest files

	ConsoleWriteLine(2);
	ConsoleWriteLine(L"          Checking quest files...");

	DWORD NumberOfValidQuestFiles = 0;
	for(DWORD i = 0; i < ConfigQuestsCount; i++) {

		sprintf(&ConfigQuests[i].SourceFilePath[0], "D:\\%s", &ConfigQuests[i].File[0]);

		CHAR* trailing_slash = strrchr(&ConfigQuests[i].File[0], '\\');
		sprintf(&ConfigQuests[i].DestinationFilePath[0], "T:\\$C\\%.16s\\%s", &ConfigQuests[i].OfferID_STRING[0], (trailing_slash != NULL ? (trailing_slash + 1) : &ConfigQuests[i].File[0]));
		
		sprintf(&ConfigQuests[i].ContentMetadataFilePath[0], "T:\\$C\\%.16s\\ContentMeta.xbx", &ConfigQuests[i].OfferID_STRING[0]);

		ConfigQuests[i].SourceFileExists = FileExists(&ConfigQuests[i].SourceFilePath[0]);

		if (ConfigQuests[i].SourceFileExists) {

			ValidateQuestFile(&ConfigQuests[i]);
			if (ConfigQuests[i].SourceFileIsValid)
				NumberOfValidQuestFiles++;
		}
	}

	if (NumberOfValidQuestFiles == 0) {

		ConsoleWriteLine(1);
		ConsoleWriteLine(L"          ERROR: No valid quest files available to install");

		ConsoleWriteLine(2);

		if (LaunchedByCDX) {
			ConsoleWriteLine(L"          B: Exit to installer");
			ConsoleWriteLine(L"          Y: Exit to dashboard");
		}
		else {
			ConsoleWriteLine(L"          B: Exit to dashboard");
		}

		while(1) {

			switch(AwaitButtonPress()) {

			case XINPUT_GAMEPAD_B:
				if (LaunchedByCDX)
					goto exit_to_cdx;
				else
					goto exit_to_dashboard;

			case XINPUT_GAMEPAD_Y:
				if (LaunchedByCDX)
					goto exit_to_dashboard;

			default:
				continue;

			}

		}
	}

	ConsoleWriteLine(1);

	ZeroMemory(PrintBuffer, PrintBufferSize);
	swprintf(PrintBuffer, L"          %d valid quests available to install", NumberOfValidQuestFiles);
	ConsoleWriteLine(PrintBuffer);

	if (NumberOfValidQuestFiles != ConfigQuestsCount) {

		ZeroMemory(PrintBuffer, PrintBufferSize);
		swprintf(PrintBuffer, L"          %d quests are invalid and cannot be installed", (ConfigQuestsCount - NumberOfValidQuestFiles));
		ConsoleWriteLine(PrintBuffer);
	}

	ConsoleWriteLine(2);
	ConsoleWriteLine(L"          Available Quests");
	ConsoleWriteLine(L"          ------------------------------");
	ConsoleWriteLine(1);
	for(DWORD i = 0; i < ConfigQuestsCount; i++) {
		
		if (!ConfigQuests[i].SourceFileIsValid)
			continue;

		WCHAR* internal_name_unicode = ConvertUTF8ToUnicodeSimple(ConfigQuests[i].InternalQuestName, strlen(ConfigQuests[i].InternalQuestName));

		ZeroMemory(PrintBuffer, PrintBufferSize);
		swprintf(PrintBuffer, L"          %s (%.2lf KB)", internal_name_unicode, (ConfigQuests[i].SourceFileSize / 1024.00f));
		ConsoleWriteLine(PrintBuffer);

		free(internal_name_unicode);
	}
	
	ConsoleWriteLine(2);
	ConsoleWriteLine(L"          Options");
	ConsoleWriteLine(L"          ------------------------------");
	ConsoleWriteLine(1);

	ConsoleWriteLine(L"          A: Install Quests");
	if (LaunchedByCDX) {
		ConsoleWriteLine(L"          B: Exit to installer");
		ConsoleWriteLine(L"          Y: Exit to dashboard");
	}
	else
		ConsoleWriteLine(L"          B: Exit to dashboard");

	while(1) {

		switch(AwaitButtonPress()) {

			case XINPUT_GAMEPAD_A:
				goto install_quest_dlc;

			case XINPUT_GAMEPAD_B:
				if (LaunchedByCDX)
					goto exit_to_cdx;
				else
					goto exit_to_dashboard;

			case XINPUT_GAMEPAD_Y:
				if (LaunchedByCDX)
					goto exit_to_dashboard;

			default:
				continue;
		}

	}


exit_to_dashboard:

	/*		
		XLaunchNewImage(NULL, NULL); //launch default dashboard
		
		Harcroft noted this can cause issues when executables try to boot back to the dashboard after being launched by CDX
		Instead, a soft reboot of the kernel _should_ avoid this

	*/

	CleanUp();
	HalReturnToFirmware(HalQuickRebootRoutine); //soft-reset the kernel

	Sleep(INFINITE);

exit_to_cdx:
	
	if (!LaunchedByCDX)
		goto exit_to_dashboard;

	CleanUp();
	XLaunchNewImage(((LD_DEMO*)&LaunchData)->szLauncherXBE, &LaunchData); //launch back into CDX
	
	Sleep(INFINITE);

install_quest_dlc:

	ConsoleWriteLine(2);
	ConsoleWriteLine(L"          Installing ContentImage.xbx...");
	if (InstallContentImage())
		ConsoleWriteLine(L"          SUCCESS");
	else
		ConsoleWriteLine(L"          FAILED");

	ConsoleWriteLine(2);
	ConsoleWriteLine(L"          Installing quest files...");
	ConsoleWriteLine(L"          ------------------------------");

	DWORD quests_install_success = 0;
	DWORD quests_install_failed = 0;
	DWORD quests_install_skipped = 0;

	for(DWORD i = 0; i < ConfigQuestsCount; i++) {

		if (ConfigQuests[i].SourceFileIsValid) {

			ConsoleWriteLine(1);

			WCHAR* internal_name_unicode = ConvertUTF8ToUnicodeSimple(ConfigQuests[i].InternalQuestName, strlen(ConfigQuests[i].InternalQuestName));

			ZeroMemory(PrintBuffer, PrintBufferSize);
			swprintf(PrintBuffer, L"          Installing quest: %s", internal_name_unicode);
			ConsoleWriteLine(PrintBuffer);

			free(internal_name_unicode);

			if (InstallQuest(ConfigSigning, &ConfigQuests[i])) {

				ConsoleWriteLine(L"          SUCCESS");
				quests_install_success++;
			}
			else {
				ConsoleWriteLine(L"          FAILED");
				quests_install_failed++;
			}
		}
		else quests_install_skipped++;

	}

	ConsoleWriteLine(1);
	ConsoleWriteLine(L"          ------------------------------");
	ZeroMemory(PrintBuffer, PrintBufferSize);
	swprintf(PrintBuffer, L"          %d quests installed, %d failed, %d skipped", quests_install_success, quests_install_failed, quests_install_skipped);
	ConsoleWriteLine(PrintBuffer);
	
	ConsoleWriteLine(2);
	
	if (LaunchedByCDX) {
		ConsoleWriteLine(L"          B: Exit to installer");
		ConsoleWriteLine(L"          Y: Exit to dashboard");
	}
	else {
		ConsoleWriteLine(L"          B: Exit to dashboard");
	}

	ConsoleWriteLine(1);
	
	while(1) {
        
		switch(AwaitButtonPress()) {
            
			case XINPUT_GAMEPAD_B:
				if (LaunchedByCDX)
					goto exit_to_cdx;
				else
					goto exit_to_dashboard;

			case XINPUT_GAMEPAD_Y:
				if (LaunchedByCDX)
					goto exit_to_dashboard;

			default:
				continue;
		}
	}

}

VOID CleanUp() {

	if (ConsoleIsInitialized()) {
		
		ConsoleDestroy();
		while(ConsoleIsInitialized())
			Sleep(15);
	}

	if (PrintBuffer)
		free(PrintBuffer);

	if (ConfigSigning)
		delete ConfigSigning;

	if (ConfigQuests)
		delete[] ConfigQuests;

}


BOOL ParseConfig(Signing* signing, DWORD& quest_count, Quest* quests) {

	ini_t* ini = ini_load((LaunchedByCDX ? CDXConfigFilesA[CDXConfigFileIndex] : ConfigFileA));
	if (!ini) {

		if (ConsoleIsInitialized()) {

			CHAR* ConfigFile = (LaunchedByCDX ? CDXConfigFilesA[CDXConfigFileIndex] : ConfigFileA);
			WCHAR* ConfigFileW = ConvertUTF8ToUnicodeSimple(ConfigFile, strlen(ConfigFile));

			ZeroMemory(PrintBuffer, PrintBufferSize);
			swprintf(PrintBuffer, L"          ERROR: Failed to load configuration file \"%s\"", ConfigFileW);

			ConsoleWriteLine(2);
			ConsoleWriteLine(PrintBuffer);

			free(ConfigFileW);
		}

		return FALSE;		
	}

	BOOL result = FALSE;
	const CHAR* value;

	//TITLE ID
	if ((value = ini_get(ini, "pqi.signing", "titleid")) == NULL) {

		if (ConsoleIsInitialized()) {
			ConsoleWriteLine(2);
			ConsoleWriteLine(L"          ERROR: Failed to find value for key \"titleid\" in section \"pqi.signing\"");
		}

		goto failed_exit;
	}
	else {
		
		if (strlen(value) != 8 || !ValidHexString((CHAR*)value, 8)) {

			if (ConsoleIsInitialized()) {
				ConsoleWriteLine(2);
				ConsoleWriteLine(L"          ERROR: Invalid value for key \"titleid\" in section \"pqi.signing\"");
			}

			goto failed_exit;
		}

		strncpy(&signing->TitleID_STRING[0], value, 8);
		CopyHexToBytes(&signing->TitleID_STRING[0], 8, &signing->TitleID_BUFFER[0], TRUE);
	}

	//TITLE SIGNATURE KEY
	if ((value = ini_get(ini, "pqi.signing", "titlesignaturekey")) == NULL) {

		if (ConsoleIsInitialized()) {
			ConsoleWriteLine(2);
			ConsoleWriteLine(L"          ERROR: Failed to find value for key \"titlesignaturekey\" in section \"pqi.signing\"");
		}

		goto failed_exit;
	}
	else {

		if (strlen(value) != 32 || !ValidHexString((CHAR*)value, 32)) {

			if (ConsoleIsInitialized()) {
				ConsoleWriteLine(2);
				ConsoleWriteLine(L"          ERROR: Invalid value for key \"titlesignaturekey\" in section \"pqi.signing\"");
			}

			goto failed_exit;
		}

		CopyHexToBytes((CHAR*)value, 32, &signing->TitleSignatureKey[0], FALSE);
	}

	//QUESTS

	CHAR* quest_section_name = (CHAR*)malloc(20);
	WCHAR* error_string_w = (WCHAR*)malloc((80 * sizeof(WCHAR)));

	for(DWORD i = 0; i < MAX_QUESTS; i++) {

		ZeroMemory(quest_section_name, sizeof(quest_section_name));

		sprintf(quest_section_name, "pqi.quest.%d", i);

		if ((value = ini_get(ini, quest_section_name, "file")) != NULL) {

			if (strlen(value) == 0) {

				if (ConsoleIsInitialized()) {

					ZeroMemory(error_string_w, sizeof(error_string_w));
					swprintf(error_string_w, L"          ERROR: Invalid value for key \"file\" in section \"pqi.quest.%d\"", i);

					ConsoleWriteLine(2);
					ConsoleWriteLine(error_string_w);
				}

				goto failed_exit;
			}

			strncpy(quests[i].File, value, strlen(value));

			if ((value = ini_get(ini, quest_section_name, "offerid")) != NULL) {

				if (strlen(value) != 16 || !ValidHexString((CHAR*)value, 16)) {
					
					if (ConsoleIsInitialized()) {
						ZeroMemory(error_string_w, sizeof(error_string_w));
						swprintf(error_string_w, L"          ERROR: Invalid value for key \"offerid\" in section \"pqi.quest.%d\"", i);
                        
						ConsoleWriteLine(2);
						ConsoleWriteLine(error_string_w);
					}

					goto failed_exit;
				}
				
				strncpy(&quests[i].OfferID_STRING[0], value, 16);
				CopyHexToBytes((CHAR*)value, 16, &quests[i].OfferID_BUFFER[0], TRUE);
			}
			else {
				
				if (ConsoleIsInitialized()) {
					
					ZeroMemory(error_string_w, sizeof(error_string_w));
					swprintf(error_string_w, L"          ERROR: Invalid value for key \"offerid\" in section \"pqi.quest.%d\"", i);
                    
					ConsoleWriteLine(2);
					ConsoleWriteLine(error_string_w);
				}
				
				goto failed_exit;
			}

			quest_count++;
		}
		else
			break;

	}

	result = TRUE;

failed_exit:

	if (ini)
		ini_free(ini);

	if (quest_section_name)
		free(quest_section_name);

	if (error_string_w)
		free(error_string_w);

	return result;
}

BOOL InstallContentImage() {

	CHAR* File = "T:\\ContentImage.xbx";
	HANDLE Destination = CreateFile(File, GENERIC_ALL, GENERIC_ALL, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (Destination == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD ContentImageSize = sizeof(ContentImage);
	DWORD BytesWritten = 0;

	if (!WriteFile(Destination, ContentImage, ContentImageSize, &BytesWritten, NULL) || BytesWritten != ContentImageSize) {

		CloseHandle(Destination);
		return FALSE;
	}

	FlushFileBuffers(Destination);
	CloseHandle(Destination);

	return TRUE;
}

BOOL InstallQuest(Signing* signing, Quest* quest) {

	if (!quest)
		return FALSE;

	CreateOfferDirectory(&quest->OfferID_STRING[0]);

	HANDLE Source = CreateFile(&quest->SourceFilePath[0], GENERIC_ALL, GENERIC_ALL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(Source == INVALID_HANDLE_VALUE)
		return FALSE;

	DeleteFileIfExists(&quest->DestinationFilePath[0]);
	DeleteFileIfExists(&quest->ContentMetadataFilePath[0]);

	HANDLE Destination = CreateFile(&quest->DestinationFilePath[0], GENERIC_ALL, GENERIC_ALL, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (Destination == INVALID_HANDLE_VALUE) {
		
		DWORD err = GetLastError();
		CloseHandle(Source);
		return FALSE;
	}

	DWORD FileSize = GetFileSize(Source, NULL);
	BYTE* FileBuffer = (BYTE*)malloc(FileSize);

	DWORD BytesRead = 0;
	if (!ReadFile(Source, FileBuffer, FileSize, &BytesRead, NULL) || BytesRead != FileSize) {

		if (Source)
			CloseHandle(Source);

		if (Destination)
			CloseHandle(Destination);

		if (FileBuffer)
			free(FileBuffer);

		return FALSE;
	}

	CloseHandle(Source);

	DWORD QuestDataLength = 0x3FEC + *(DWORD*)(FileBuffer + 0x14); //(header size - signature size) + block size
	SignQuest(signing, (FileBuffer + 0x14), QuestDataLength, FileBuffer);

	DWORD BytesWritten = 0;
	if (!WriteFile(Destination, FileBuffer, FileSize, &BytesWritten, NULL) || BytesWritten != FileSize) {

		if (Destination)
			CloseHandle(Destination);

		if (FileBuffer)
			free(FileBuffer);

		return FALSE;
	}

	FlushFileBuffers(Destination);
	CloseHandle(Destination);

	free(FileBuffer);

	//install contentmeta.xbx
	Destination = CreateFile(&quest->ContentMetadataFilePath[0], GENERIC_ALL, GENERIC_ALL, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (Destination == INVALID_HANDLE_VALUE)
		return FALSE;

	BYTE* ContentMetadata = NULL;
	DWORD ContentMetadataSize = 0;

	if (!CreateContentMetadata(&quest->InternalQuestName[0], *(DWORD*)(&signing->TitleID_BUFFER[0]), *(QWORD*)(&quest->OfferID_BUFFER[0]), 1, ContentMetadata, ContentMetadataSize)) {

		if (Destination)
			CloseHandle(Destination);

		if (ContentMetadata)
			free(ContentMetadata);

		return FALSE;
	}

	if (!ContentMetadata || ContentMetadataSize == 0) {
		
		if (Destination)
			CloseHandle(Destination);

		if (ContentMetadata)
			free(ContentMetadata);

		return FALSE;
	}

	BytesWritten = 0;
	if (!WriteFile(Destination, ContentMetadata, ContentMetadataSize, &BytesWritten, NULL)) {

		if (Destination)
			CloseHandle(Destination);

		if (ContentMetadata)
			free(ContentMetadata);

		return FALSE;
	}

	FlushFileBuffers(Destination);
	CloseHandle(Destination);

	free(ContentMetadata);

	return TRUE;
}

VOID SignQuest(Signing* signing, BYTE* quest_data, DWORD quest_data_length, BYTE* signature) {

	if (!quest_data || quest_data_length == 0 || !signature)
		return;

	//standard non-roamable signature

	BYTE* XboxCERTKey = &XboxHDKey[-0x10]; //cert key is stored 0x10 bytes behind XboxHDKey

	BYTE* Hash = (BYTE*)malloc(0x14);
	BYTE* ContentKey = (BYTE*)malloc(0x10);

	sha1_context* context = new sha1_context();

	//calc content key (aka "authkey")
	hmacsha1_init(context, XboxCERTKey, 0x10);
	hmacsha1_update(context, &signing->TitleSignatureKey[0], 0x10);
	hmacsha1_final(context, XboxCERTKey, 0x10, Hash);

	memcpy(ContentKey, Hash, 0x10); //copy first 0x10 bytes (trim last 4 bytes of hash)

	//calc roamable signature
	hmacsha1_init(context, ContentKey, 0x10);
	hmacsha1_update(context, quest_data, quest_data_length);
	hmacsha1_final(context, ContentKey, 0x10, Hash);

	//convert roamable signature to non-roamable
    hmacsha1_init(context, &XboxHDKey[0], 0x10);
	hmacsha1_update(context, Hash, 0x14);
	hmacsha1_final(context, &XboxHDKey[0], 0x10, Hash);

	memcpy(signature, Hash, 0x14);

	delete context;
	free(Hash);
	free(ContentKey);	
}

VOID ValidateQuestFile(Quest* quest) {

	if (!quest)
		return;

	FILE* file = fopen(quest->SourceFilePath, "rb");
	if (!file)
		return;

	fseek(file, 0, SEEK_END);
	DWORD file_size = (DWORD)ftell(file);

	quest->SourceFileSize = file_size;

	if (file_size < 0x4000) {

		fclose(file);
		return;
	}

	fseek(file, 0x14, SEEK_SET);
	DWORD block_size = 0;
	fread(&block_size, 1, 4, file);

	if ((block_size + 0x4000) != file_size || block_size <= 0x2048) {
		
		fclose(file);
		return;
	}

	BYTE* block = (BYTE*)malloc(block_size);
	fseek(file, 0x4000, SEEK_SET);
	fread(block, 1, block_size, file);
	fclose(file);

	BYTE magic_bytes[] = { 0x82, 0x6F, 0x82, 0x72, 0x82, 0x6E, 0x20, 0x83, 0x47, 0x83, 0x73, 0x83, 0x5C, 0x81, 0x5B, 0x83, 0x68, 0x81, 0x40, 0x82, 0x50, 0x81, 0x95, 0x82, 0x51, 0x00 };
	for(DWORD i = 0; i < 0x1A; i++) {
		if (block[i] != magic_bytes[i]) {

			free(block);
			return;
		}
	}

	CHAR* internal_quest_name = (CHAR*)(block + 0x20);
	CHAR* forward_slash = strrchr(internal_quest_name, '/'); //skip PSO/ or PSOV2/ prefixes in internal name

	if (forward_slash == NULL) {

		strncpy(&quest->InternalQuestName[0], internal_quest_name, 0x20);
	}
	else {

		DWORD copy_from_index = ((forward_slash + 1) - internal_quest_name);
		DWORD copy_length = 0x20 - copy_from_index;
		strncpy(&quest->InternalQuestName[0], (CHAR*)(internal_quest_name + copy_from_index), copy_length);
	}

    DWORD current_checksum = *(DWORD*)(block + 0x2044);
	*(DWORD*)(block + 0x2044) = 0;

	DWORD calculated_checksum = CRC32(block, 0x2048);

	quest->SourceFileIsValid = (calculated_checksum == current_checksum);
	
	free(block);
}

DWORD CRC32(BYTE* buffer, DWORD length) {

	const DWORD Table[] = 
	{
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4,
		0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
		0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
		0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A,
		0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
		0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
		0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
		0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5,
		0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6,
		0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
		0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
		0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C,
		0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31,
		0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
		0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
		0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7,
		0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
		0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
	};

	DWORD Checksum = 0xFFFFFFFF;

	for(DWORD i = 0; i < length; i++)
		Checksum = ((Checksum >> 8) ^ Table[((Checksum & 0xFF) ^ buffer[i])]);

	return ~Checksum;
}

BOOL ValidHexString(CHAR* string, DWORD length) {

	//this could be done better/faster

	if (!string || length == 0 || (length % 2) != 0)
		return FALSE;

	const CHAR* valid_chars = "0123456789abcdefABCDEF";
	const DWORD valid_chars_len = 22;

	BOOL valid;
	CHAR c, v;
	for(DWORD i = 0; i < length; i++) {

		valid = FALSE;
		c = *(CHAR*)(string + i);

		for(DWORD x = 0; x < valid_chars_len; x++) {

			v = *(CHAR*)(valid_chars + x);

			if (c == v) {
				valid = TRUE;
				break;
			}

		}

		if (!valid)
			return FALSE;

	}

	return TRUE;
}

VOID CopyHexToBytes(CHAR* string, DWORD length, BYTE* buffer, BOOL reverse) {

	if (string && length > 0 && (length % 2) == 0 && buffer) {

		CHAR c;
		BYTE b;

		DWORD buffer_index = 0;

		for(DWORD i = 0; i < length; i += 2) {

			//first nibble
			c = *(CHAR*)(string + i);
			
			if (c >= '0' && c <= '9')
				b = (BYTE)((c - '0') << 4);
			else if (c >= 'A' && c <= 'F')
				b = (BYTE)((c - 'A' + 10) << 4);
			else
				b = (BYTE)((c - 'a' + 10) << 4);

			//second nibble
			c = *(CHAR*)(string + (i + 1));
			
			if (c >= '0' && c <= '9')
				b = (BYTE)(b | (c - '0'));
			else if (c >= 'A' && c <= 'F')
				b = (BYTE)(b | (c - 'A' + 10));
			else
				b = (BYTE)(b | (c - 'a' + 10));

			*(BYTE*)(buffer + buffer_index) = b;
			buffer_index++;
		}

		if (reverse) {

			DWORD x = 0;
			DWORD y = (length / 2);

			while (x < y) {
				b = *(BYTE*)(buffer + x);
				*(BYTE*)(buffer + x) = *(BYTE*)(buffer + (y - 1));
				*(BYTE*)(buffer + (y - 1)) = b;
				x++;
				y--;
			}

		}

	}
}

WCHAR* ConvertUTF8ToUnicodeSimple(CHAR* string, DWORD string_length) {

	//dont recommend anyone using this, definitely not ideal
	//this just converts uint8 to uint16 with no character map checking
	//importing the kernel function would be a better idea
	//int __stdcall RtlAnsiStringToUnicodeString(_UNICODE_STRING *DestinationString, _STRING *SourceString, char AllocateDestinationString)

    if (string && string_length > 0) {

		DWORD buffer_size = ((string_length * sizeof(WORD)) + sizeof(WORD)); //include null term
        BYTE* buffer = (BYTE*)malloc(buffer_size);

        if (!buffer)
            return NULL;

		ZeroMemory(buffer, buffer_size);

        for (DWORD i = 0; i < string_length; i++)
            *(WORD*)(buffer + (i * sizeof(WORD))) = (WORD)string[i];

        return (WCHAR*)buffer;
    }

    return NULL;
}

BOOL CreateContentMetadata(CHAR* display_name, DWORD title_id, QWORD offer_id, DWORD content_flags, BYTE* &result, DWORD &result_length) {

	/*
		Equivalent to the XCreateContentSimple function within the XOnline/content library

		This is the _OLD_ version of XCreateContentSimple(), calling the kernel function when linked against a higher version lib
		would result in unparseable contentmeta for PSO

		This is from the 1.0.4831.9 version. Passes the created metadata buffer out instead of automatically writing to disk like the standard function
	*/

	if (display_name && strlen(display_name) > 0) {

		const DWORD HeaderSize = 0x6C;
		const DWORD UTF8Size = 1024;

		BYTE* Header = (BYTE*)malloc(HeaderSize);
		CHAR* UTF8 = (CHAR*)malloc(UTF8Size);

		ZeroMemory(Header, HeaderSize);
		ZeroMemory(UTF8, UTF8Size);

		sprintf(UTF8, "[Default]\r\nName=%s\r\n", display_name);

		WCHAR* Unicode = ConvertUTF8ToUnicodeSimple(UTF8, strlen(UTF8));

		if (!Unicode) {

			free(UTF8);
			free(Header);

			return FALSE;
		}

		DWORD LanguageSectionSize = ((wcslen(Unicode) * sizeof(WCHAR)) + sizeof(WCHAR));
		BYTE* LanguageSection = (BYTE*)malloc(LanguageSectionSize);

		*(WORD*)LanguageSection = (WORD)0xFEFF;
		memcpy((LanguageSection + 2), Unicode, (wcslen(Unicode) * sizeof(WCHAR)));

		free(UTF8);
		free(Unicode);

		*(DWORD*)(Header + 0x14) = (DWORD)0x46534358; //magic
		*(DWORD*)(Header + 0x18) = HeaderSize; //header size
		*(DWORD*)(Header + 0x1C) = (DWORD)1; //unk (always 1) (possibly the type of offer? free/paid/subscription? setting this to anything has no effect)
		*(DWORD*)(Header + 0x20) = content_flags; //content flags
		*(DWORD*)(Header + 0x24) = title_id; //title id
		*(QWORD*)(Header + 0x28) = offer_id; //offer id
		*(DWORD*)(Header + 0x30) = (DWORD)0; //publisher flags

		*(DWORD*)(Header + 0x34) = HeaderSize; //language section offset (directly after header)
		*(DWORD*)(Header + 0x38) = LanguageSectionSize; //language section size

		//language section hash (standard SHA1)
		HANDLE context = XCalculateSignatureBegin(XCALCSIG_FLAG_CONTENT);
		if (context) {
			XCalculateSignatureUpdate(context, LanguageSection, LanguageSectionSize);
			XCalculateSignatureEnd(context, (Header + 0x3C));
		}

		if (!CalculateContentHeaderSignature(title_id, (Header + 0x14), (HeaderSize - 0x14), Header)) {
			
			free(Header);
			free(LanguageSection);

			return FALSE;
		}

		//everything done, pull the pieces together and return

		result_length = (HeaderSize + LanguageSectionSize);
		result = (BYTE*)malloc(result_length);

		memcpy(result, Header, HeaderSize);
		memcpy((result + HeaderSize), LanguageSection, LanguageSectionSize);

		free(Header);
		free(LanguageSection);

		return TRUE;
	}
	
	return FALSE;
}

BOOL CalculateContentHeaderSignature(DWORD TitleId, BYTE* buffer, DWORD length, BYTE* result) {

	if (buffer && length > 0 && result) {

		sha1_context* context = new sha1_context();
		BYTE* content_key = new BYTE[0x14];

		//calc content key (HMAC SHA1 hash of TitleId bytes using XboxHDKey as the key)
		hmacsha1_init(context, &XboxHDKey[0], 0x10);
		hmacsha1_update(context, (BYTE*)&TitleId, 4);
		hmacsha1_final(context, &XboxHDKey[0], 0x10, content_key);

		//calc signature (HMAC SHA1 hash of supplied data using "content key" as the key)
		hmacsha1_init(context, content_key, 0x14);
		hmacsha1_update(context, buffer, length);
		hmacsha1_final(context, content_key, 0x14, result);

		delete context;
		delete content_key;

		return TRUE;
	}

	return FALSE;
}

DWORD AwaitButtonPress() {

	DWORD Result = UINT_MAX;

	CONST DWORD ControllerCount = 4;
	HANDLE Controller[ControllerCount] = { 0 };

	DWORD LastStatePacketNumber[ControllerCount];
	XINPUT_STATE CurrentState[ControllerCount];

	for(DWORD i = 0; i < ControllerCount; i++) {
		ZeroMemory(&CurrentState[i], sizeof(XINPUT_STATE));
		LastStatePacketNumber[i] = 0;
	}

	DWORD Insertions = 0;
	DWORD Removals = 0;

	for(DWORD i = 0; i < ControllerCount; i++)
		Controller[i] = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);

	while(true) {

		//check if controllers were unplugged/plugged in
		if (XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &Insertions, &Removals)) {

			//wait for enumeration to complete
			while(XGetDeviceEnumerationStatus() != XDEVICE_ENUMERATION_IDLE)
				Sleep(15);

			//close current device handles
			for(DWORD i = 0; i < ControllerCount; i++)
				if (Controller[i]) XInputClose(Controller[i]);

			//open new device handles
			for(DWORD i = 0; i < ControllerCount; i++)
				Controller[i] = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);

		}

		//poll all controllers
		for (DWORD i = 0; i < ControllerCount; i++) {

			//get current input state
			if (Controller[i] && XInputGetState(Controller[i], &CurrentState[i]) == ERROR_SUCCESS) {

				//if packet id changed then process input
				if (CurrentState[i].dwPacketNumber != LastStatePacketNumber[i]) {

					LastStatePacketNumber[i] = CurrentState[i].dwPacketNumber;

					//checking for button presses only

					//START
					if ((CurrentState[i].Gamepad.wButtons & XINPUT_GAMEPAD_START) == XINPUT_GAMEPAD_START) {
						Result = XINPUT_GAMEPAD_START;
						break;
					}

					//BACK
					if ((CurrentState[i].Gamepad.wButtons & XINPUT_GAMEPAD_BACK) == XINPUT_GAMEPAD_BACK) {
						Result = XINPUT_GAMEPAD_BACK;
						break;
					}

					//LEFT STICK (CLICK)
					if ((CurrentState[i].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) == XINPUT_GAMEPAD_LEFT_THUMB) {
						Result = XINPUT_GAMEPAD_LEFT_THUMB;
						break;
					}


					//RIGHT STICK (CLICK)
					if ((CurrentState[i].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) == XINPUT_GAMEPAD_RIGHT_THUMB) {
						Result = XINPUT_GAMEPAD_RIGHT_THUMB;
						break;
					}

					//A BUTTON
					if (CurrentState[i].Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK) {
						Result = XINPUT_GAMEPAD_A;
						break;
					}

					//B BUTTON
					if (CurrentState[i].Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK) {
						Result = XINPUT_GAMEPAD_B;
						break;
					}

					//X BUTTON
					if (CurrentState[i].Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] > XINPUT_GAMEPAD_MAX_CROSSTALK) {
						Result = XINPUT_GAMEPAD_X;
						break;
					}

					//Y BUTTON
					if (CurrentState[i].Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] > XINPUT_GAMEPAD_MAX_CROSSTALK) {
						Result = XINPUT_GAMEPAD_Y;
						break;
					}

					//BLACK BUTTON
					if (CurrentState[i].Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > XINPUT_GAMEPAD_MAX_CROSSTALK) {
						Result = XINPUT_GAMEPAD_BLACK;
						break;
					}

					//WHITE BUTTON
					if (CurrentState[i].Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > XINPUT_GAMEPAD_MAX_CROSSTALK) {
						Result = XINPUT_GAMEPAD_WHITE;
						break;
					}

				}
			
			}

		}

		if (Result != UINT_MAX)
			break;

		Sleep(100); //prevent spam
	}

	//close handles
	for(int i = 0; i < ControllerCount; i++)
		if (Controller[i]) XInputClose(Controller[i]);

	return Result;
}

VOID CreateOfferDirectory(CHAR* OfferID_STRING) {

	if (!OfferID_STRING)
		return;

	//it's okay to call CreateDirectory on a potentially already existing directory
	//if the directory already exists, GetLastError() will return ERROR_ALREADY_EXISTS (aka fails successfully)

	//Content folder
	CreateDirectory("T:\\$C", NULL);

	//OfferID
	CHAR* DirectoryPath = (CHAR*)malloc(MAX_PATH);
	ZeroMemory(DirectoryPath, MAX_PATH);

	sprintf(DirectoryPath, "T:\\$C\\%.16s", OfferID_STRING);
	
	CreateDirectory(DirectoryPath, NULL);

	free(DirectoryPath);
}

VOID DeleteFileIfExists(CHAR* FilePath) {

	if (FileExists(FilePath))
		DeleteFile(FilePath);
}

BOOL FileExists(CHAR* FilePath) {

	if (!FilePath)
		return FALSE;

	WIN32_FIND_DATA context;
	HANDLE handle = FindFirstFile(FilePath, &context);
	BOOL result = (handle != INVALID_HANDLE_VALUE);
	FindClose(handle);

	return result;
}
