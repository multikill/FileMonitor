#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#define FILE_CHANGE \
	FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |\
	FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY


int LogChange(HANDLE hDir)
{
	BYTE buf[1024];
	memset(buf, 0, sizeof(buf));
	DWORD offset = 0, bRet = 0;
	PFILE_NOTIFY_INFORMATION pFni = NULL;

	int readDirFail = ReadDirectoryChangesW(hDir, buf, sizeof(buf), TRUE, FILE_CHANGE, &bRet, NULL, NULL);
	if (readDirFail == FALSE)
	{
		DWORD err = GetLastError();
		printf("ReadDirectoryChangesW failed 0x%X\n", GetLastError());
		return 1;
	}

	do {
		pFni = (PFILE_NOTIFY_INFORMATION) &buf[offset];
		offset += pFni->NextEntryOffset;

		switch (pFni->Action)
		{
		case FILE_ACTION_ADDED:				printf("[ADDED] %ls\n", pFni->FileName); break;
		case FILE_ACTION_REMOVED:			printf("[REMOVED] %ls\n", pFni->FileName); break;
		case FILE_ACTION_MODIFIED:			printf("[MODIFIED] %ls\n", pFni->FileName); break;
		case FILE_ACTION_RENAMED_OLD_NAME:	printf("[RENAMED OLD] %ls\n", pFni->FileName); break;
		case FILE_ACTION_RENAMED_NEW_NAME:	printf("[RANAMED NEW] %ls\n", pFni->FileName); break;
		default:							printf("[UNSPECIFIED] %ls\n", pFni->FileName); break;
		}

	} while (pFni->NextEntryOffset != 0);

	return 0;
}

int _tmain(int argc, TCHAR* argv[])
{
	HANDLE  hChange, hDir;
	DWORD	dwWaitStatus;
	TCHAR   lpDir[_MAX_PATH]; // = L"C:\\temp";
	GetCurrentDirectory(_MAX_PATH, lpDir);

	hChange = FindFirstChangeNotification(lpDir, FALSE, FILE_CHANGE);
	if (hChange == INVALID_HANDLE_VALUE)
	{
		printf("FindFirstChangeNotification failed 0x%X\n", GetLastError());
		return 1;
	}

	hDir = CreateFile(lpDir, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if (hDir == INVALID_HANDLE_VALUE) {
		FindCloseChangeNotification(hChange);
		return 1;
	}

	while (true)
	{
		dwWaitStatus = WaitForSingleObject(hChange, INFINITE);
		if (dwWaitStatus != WAIT_OBJECT_0)
		{
			printf("WaitForSingleObject failed 0x%X\n", GetLastError());
			FindCloseChangeNotification(hChange);
			CloseHandle(hDir);
			return 1;
		}

		LogChange(hDir);

		int findNextFail = FindNextChangeNotification(hChange);
		if (findNextFail == FALSE)
		{
			SetEvent(hChange);
			continue;
		}
	}

	FindCloseChangeNotification(hChange);
	CloseHandle(hDir);
	return 0;
}