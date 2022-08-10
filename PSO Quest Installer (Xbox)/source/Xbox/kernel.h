extern "C" {

	typedef enum _FIRMWARE_REENTRY {
		HalHaltRoutine = 0,
		HalRebootRoutine = 1,
		HalQuickRebootRoutine = 2,
		HalKdRebootRoutine = 3,
		HalFatalErrorRebootRoutine = 4,
		HalMaximumRoutine = 5
	} FIRMWARE_REENTRY;

	XBOXAPI VOID NTAPI HalReturnToFirmware(FIRMWARE_REENTRY Routine);
	XBOXAPI UCHAR __declspec(dllimport) XboxHDKey[0x10];
}

