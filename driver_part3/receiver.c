/**********************************************************************
 *
 *  Toby Opferman
 *
 *  Example Communicating with a driver
 *
 *  This example is for educational purposes only.  I license this source
 *  out for use in learning how to write a device driver.
 *
 *
 **********************************************************************/


#include <windows.h>
#include <stdio.h>

/*********************************************************
 *   Main Function Entry
 *
 *********************************************************/
int _cdecl main(int argc, char **argv) {
	HANDLE hFile;
	DWORD dwReturn;
	char szTemp[256] = {0};

	sprintf(szTemp, "\\\\.\\YTDevice\\tmp_file");

	hFile = CreateFile(szTemp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if(hFile == INVALID_HANDLE_VALUE) {
		printf("invalid handle\n");
		goto cleanup;
	}

	while(szTemp[0] != 'q' && szTemp[0] != 'Q') {
		printf("Press enter to get a string from the driver or 'Q' to quit\n");
		gets(szTemp);

		if(szTemp[0] != 'q' &&  szTemp[0] != 'Q') {
			memset(szTemp, 0, sizeof(szTemp));
			dwReturn = 0;
			ReadFile(hFile, szTemp, sizeof(szTemp), &dwReturn, NULL);
			printf("%d bytes read\n", dwReturn);
			printf("'%.*s'\n", dwReturn, szTemp);
		}
	}

	CloseHandle(hFile);
cleanup:
	return 0;
}


