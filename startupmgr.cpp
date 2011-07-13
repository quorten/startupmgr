//Startup Manager - helps you log on to your Windows XP computer faster

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0510
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include "resource.h"

#include <vector>
#include <cstring>

using namespace std;

HINSTANCE g_hInstance;
vector<string> processDescs;
vector<string> processCmds;
vector<bool> startProcess;
vector<string> serviceDescs;
vector<string> serviceNames;
vector<bool> startService;
vector<bool> offlProcChecks;
vector<bool> netProcChecks;
vector<bool> offlSvcChecks;
vector<bool> netSvcChecks;

INT_PTR CALLBACK SelectTaskProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CustomizeSheetProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int StartChosenServices();
int StartChosenProcesses();
int LoadDataFile(bool useEmbedded = false);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nShowCmd)
{
	//Load the data file
	int errorCode = LoadDataFile();
	switch (errorCode)
	{
	case 1:
		MessageBox(NULL, "Could not read startup.txt, so the embedded file will be used instead.", "Warning", MB_ICONEXCLAMATION | MB_OK);
		break;
	case 2:
		MessageBox(NULL, "Syntax error in startup.txt, so the embedded file will be used instead.", "Warning", MB_ICONEXCLAMATION | MB_OK);
		break;
	}
	if (errorCode != 0)
		LoadDataFile(true);

	InitCommonControls();

	//Globally save the application instance
	g_hInstance = hInstance;

	//Create the wizard window
	PROPSHEETPAGE propPage;
	HPROPSHEETPAGE createdPages[2];
	PROPSHEETHEADER propHeader;

	propPage.dwSize = sizeof(PROPSHEETPAGE);
	propPage.dwFlags = PSP_DEFAULT;
	propPage.hInstance = hInstance;
	propPage.pszTemplate = (LPCTSTR)WIZ_01;
	propPage.pfnDlgProc = SelectTaskProc;
	propPage.lParam = 0;
	createdPages[0] = CreatePropertySheetPage(&propPage);

	propPage.pszTemplate = (LPCTSTR)WIZ_02;
	propPage.pfnDlgProc = CustomizeSheetProc;
	createdPages[1] = CreatePropertySheetPage(&propPage);

	propHeader.dwSize = sizeof(PROPSHEETHEADER);
	propHeader.dwFlags = /*PSH_MODELESS | */PSH_USEICONID | PSH_WIZARD;
	propHeader.hwndParent = NULL;
	propHeader.hInstance = hInstance;
	propHeader.pszIcon = (LPCTSTR)MAIN_ICON;
	//propHeader.pszCaption = "Startup Manager";
	propHeader.nPages = 2;
	propHeader.nStartPage = 0;
	propHeader.phpage = createdPages;
	PropertySheet(&propHeader);
#ifdef _DEBUG
	return 0;
#endif

	int result1 = StartChosenServices();
	int result2 = StartChosenProcesses();
	if (result1 == -1)
	{
		MessageBox(NULL, "Could not open the service control manager database; no services were started.", "Error", MB_ICONERROR | MB_OK);
		return 2;
	}
	if (result1 > 0 && result2 != 0)
	{
		MessageBox(NULL, "Some processes and services did not start properly.", "Warning", MB_ICONINFORMATION | MB_OK);
		return 1;
	}
	if (result1 > 0)
	{
		MessageBox(NULL, "Some services did not start properly.", "Warning", MB_ICONINFORMATION | MB_OK);
		return 1;
	}
	if (result2 != 0)
	{
		MessageBox(NULL, "Some processes did not start properly.", "Warning", MB_ICONINFORMATION | MB_OK);
		return 1;
	}

	return 0;
}

INT_PTR CALLBACK SelectTaskProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, D_NETSTART, BST_CHECKED);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case D_CUSTOMIZE:
			if (IsDlgButtonChecked(hDlg, D_CUSTOMIZE))
				PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
			else
				PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
			break;
		}
		break;
	case WM_NOTIFY:
		NMHDR* notifyInfo;
		notifyInfo = (NMHDR*)lParam;
		switch (notifyInfo->code)
		{
		case PSN_SETACTIVE:
			if (IsDlgButtonChecked(hDlg, D_CUSTOMIZE))
				PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
			else
				PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
			break;
		case PSN_WIZBACK:
			break;
		case PSN_WIZNEXT:
		case PSN_WIZFINISH:
			if (IsDlgButtonChecked(hDlg, D_LOADNOTHING))
			{
				//Clear everything
				for (unsigned i = 0; i < startProcess.size(); i++)
					startProcess[i] = false;
				for (unsigned i = 0; i < startService.size(); i++)
					startService[i] = false;
			}
			if (IsDlgButtonChecked(hDlg, D_OFFLLOAD))
			{
				for (unsigned i = 0; i < startProcess.size(); i++)
					startProcess[i] = offlProcChecks[i];
				for (unsigned i = 0; i < startService.size(); i++)
					startService[i] = offlSvcChecks[i];
			}
			if (IsDlgButtonChecked(hDlg, D_NETSTART))
			{
				for (unsigned i = 0; i < startProcess.size(); i++)
					startProcess[i] = netProcChecks[i];
				for (unsigned i = 0; i < startService.size(); i++)
					startService[i] = netSvcChecks[i];
			}
			if (IsDlgButtonChecked(hDlg, D_LOADALL))
			{
				//Set everything
				for (unsigned i = 0; i < startProcess.size(); i++)
					startProcess[i] = true;
				for (unsigned i = 0; i < startService.size(); i++)
					startService[i] = true;
			}
			break;
		case PSN_QUERYCANCEL:
			int userChoice;
			userChoice = MessageBox(GetParent(hDlg), "Do you want startup to proceed as normal?", "Confirm", MB_ICONEXCLAMATION | MB_YESNO);
			if (userChoice == IDYES)
				SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			else
				SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
			return TRUE;
		/*case PSN_WIZFINISH:
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			return TRUE;*/
		}
		break;
	case WM_DESTROY:
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK CustomizeSheetProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//Temporary variables
	HWND procBox;
	HWND svcBox;
	NMHDR* notifyInfo;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		procBox = GetDlgItem(hDlg, D_PROCBOX);
		svcBox = GetDlgItem(hDlg, D_SVCBOX);
		for (unsigned i = 0; i < processDescs.size(); i++)
			SendMessage(procBox, LB_INSERTSTRING, -1, (LPARAM)processDescs[i].c_str());
		for (unsigned i = 0; i < serviceDescs.size(); i++)
			SendMessage(svcBox, LB_INSERTSTRING, -1, (LPARAM)serviceDescs[i].c_str());
		return TRUE;
	case WM_NOTIFY:
		notifyInfo = (NMHDR*)lParam;
		switch (notifyInfo->code)
		{
		case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
			//Set the list box selections
			procBox = GetDlgItem(hDlg, D_PROCBOX);
			svcBox = GetDlgItem(hDlg, D_SVCBOX);
			for (unsigned i = 0; i < startProcess.size(); i++)
			{
				BOOL selState;
				if (startProcess[i] == true) selState = TRUE;
				else selState = FALSE;
				SendMessage(procBox, LB_SETSEL, selState, i);
			}
			for (unsigned i = 0; i < startService.size(); i++)
			{
				BOOL selState;
				if (startService[i] == true) selState = TRUE;
				else selState = FALSE;
				SendMessage(svcBox, LB_SETSEL, selState, i);
			}
			SendMessage(procBox, LB_SETCARETINDEX, 0, FALSE);
			SendMessage(svcBox, LB_SETCARETINDEX, 0, FALSE);
			//PropSheet_SetFinishText(GetParent(hDlg), "&Finish");
			break;
		case PSN_WIZBACK:
			break;
		case PSN_WIZNEXT:
			break;
		case PSN_QUERYCANCEL:
			int userChoice;
			userChoice = MessageBox(GetParent(hDlg), "Do you want startup to proceed as normal?", "Confirm", MB_ICONEXCLAMATION | MB_YESNO);
			if (userChoice == IDYES)
				SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			else
				SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
			return TRUE;
		case PSN_WIZFINISH:
			//Get the selection state of the list boxes
			procBox = GetDlgItem(hDlg, D_PROCBOX);
			svcBox = GetDlgItem(hDlg, D_SVCBOX);
			for (unsigned i = 0; i < startProcess.size(); i++)
			{
				LRESULT result = SendMessage(procBox, LB_GETSEL, i, 0);
				if (result == 0) startProcess[i] = false;
				else startProcess[i] = true;
			}
			for (unsigned i = 0; i < startService.size(); i++)
			{
				LRESULT result = SendMessage(svcBox, LB_GETSEL, i, 0);
				if (result == 0) startService[i] = false;
				else startService[i] = true;
			}
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		break;
	}
	return FALSE;
}

int StartChosenServices()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	int errorCount = 0;

	//Open a handle to the SC Manager database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

	if (NULL == schSCManager)
	{
		//MessageBox(NULL, "OpenSCManager failed", NULL, MB_OK);
		return -1;
	}

	for (unsigned i = 0; i < serviceNames.size(); i++)
	{
		if (startService[i] == false)
			continue;

		schService = OpenService(schSCManager, serviceNames[i].c_str(), SERVICE_START);

		if (schService == NULL)
		{
			//MessageBox(NULL, "OpenService failed", NULL, MB_OK);
			//CloseServiceHandle(schSCManager);
			errorCount++;
			continue;
		}

		if (!StartService(schService, 0, NULL))
			errorCount++;
		//else MessageBox(NULL, "Service start pending.", NULL, MB_OK);
		CloseServiceHandle(schService);
	}

	CloseServiceHandle(schSCManager);

	return errorCount;
}

int StartChosenProcesses()
{
	string exeName;
	string parameters;
	int errorCount = 0;

	for (unsigned i = 0; i < processCmds.size(); i++)
	{
		if (startProcess[i] == false)
			continue;

		//Parse the command line into the program name and parameters
		unsigned lastQuotPos = processCmds[i].find('"', 1);
		exeName = processCmds[i].substr(1, lastQuotPos - 1);
		if (lastQuotPos + 2 < processCmds[i].size())
			parameters = processCmds[i].substr(lastQuotPos + 2);
		else
			parameters = "";
		LRESULT result = (LRESULT)ShellExecute(NULL, "open", exeName.c_str(), parameters.c_str(), NULL, SW_SHOWDEFAULT);
		if (result <= 32)
			errorCount++;
	}

	return errorCount;
}

int LoadDataFile(bool useEmbedded)
{
	unsigned fileSize;
	char* buffer;
	if (useEmbedded == false)
	{
		//Read the whole file into memory
		HANDLE hFile;
		DWORD bytesRead;
		hFile = CreateFile("C:\\Program Files\\Startup Manager\\startup.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return 1;
		fileSize = GetFileSize(hFile, NULL);
		buffer = new char[fileSize+1];
		ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
		buffer[fileSize] = '\0';
		CloseHandle(hFile);
	}
	else
	{
		HRSRC hRes = FindResource(NULL, (LPCTSTR)STARTUP_INFO, RT_RCDATA);
		HGLOBAL hResData = LoadResource(NULL, hRes);
		fileSize = SizeofResource(NULL, hRes);
		buffer = (char*)LockResource(hResData);
	}

	//Since we read the whole file this way, we will have to properly format newlines.
	string fileContents;
	for (unsigned i = 0, j = 0; i < fileSize; i++)
	{
		if (buffer[i] != '\r')
			fileContents.push_back(buffer[i]);
		else
		{
			if (buffer[i+1] == '\n') //CR+LF
				i++;
			fileContents.push_back('\n');
		}
	}
	if (useEmbedded == false)
		delete []buffer;

	//Parse the data
	unsigned curPos = 0;
	unsigned contentSize = fileContents.size();

	//Read the process list
	if (fileContents.compare(curPos, 11, "Processes\n\n") == 0)
		curPos += 11;
	else
		return 2;
	while (curPos < contentSize &&
		fileContents.compare(curPos, 11, "\nServices\n\n") != 0)
	{
		//Read the process description
		unsigned nextPos = curPos;
		while (nextPos < contentSize &&
			fileContents[nextPos] != '\t' &&
			fileContents[nextPos] != '\n')
			nextPos++;
		if (nextPos >= contentSize || fileContents[nextPos] == '\n')
			return 2;
		string procDesc = fileContents.substr(curPos, nextPos - curPos);
		processDescs.push_back(procDesc);
		nextPos++;
		curPos = nextPos;

		//Read the process command line
		while (nextPos < contentSize &&
			fileContents[nextPos] != '\t' &&
			fileContents[nextPos] != '\n')
			nextPos++;
		if (nextPos >= contentSize || fileContents[nextPos] == '\t')
			return 2;
		string procCmd = fileContents.substr(curPos, nextPos - curPos);
		processCmds.push_back(procCmd);
		nextPos++;
		curPos = nextPos;

		//Add an selection state entry
		startProcess.push_back(false);
	}
	curPos += 11;

	//Read the service list
	while (curPos < contentSize &&
		fileContents.compare(curPos, 12, "\nChecklist\n\n") != 0)
	{
		//Read the service description
		unsigned nextPos = curPos;
		while (nextPos < contentSize &&
			fileContents[nextPos] != '\t' &&
			fileContents[nextPos] != '\n')
			nextPos++;
		if (nextPos >= contentSize || fileContents[nextPos] == '\n')
			return 2;
		string svcDesc = fileContents.substr(curPos, nextPos - curPos);
		serviceDescs.push_back(svcDesc);
		nextPos++;
		curPos = nextPos;

		//Read the service name
		while (nextPos < contentSize &&
			fileContents[nextPos] != '\t' &&
			fileContents[nextPos] != '\n')
			nextPos++;
		if (nextPos >= contentSize || fileContents[nextPos] == '\t')
			return 2;
		string svcName = fileContents.substr(curPos, nextPos - curPos);
		serviceNames.push_back(svcName);
		nextPos++;
		curPos = nextPos;

		//Add an selection state entry
		startService.push_back(false);
	}
	curPos += 12;

	//Read the process checklists
	while (curPos < contentSize &&
		fileContents.compare(curPos, 1, "\n") != 0)
	{
		bool checkValue;
		if (fileContents[curPos] == 'X') checkValue = true;
		else checkValue = false;
		offlProcChecks.push_back(checkValue);
		curPos += 2;
		if (curPos >= contentSize)
			return 2;
		if (fileContents[curPos] == 'X') checkValue = true;
		else checkValue = false;
		netProcChecks.push_back(checkValue);
		curPos += 2;
	}
	curPos++;

	//Read the service checklists
	while (curPos < contentSize)
	{
		bool checkValue;
		if (fileContents[curPos] == 'X') checkValue = true;
		else checkValue = false;
		offlSvcChecks.push_back(checkValue);
		curPos += 2;
		if (curPos >= contentSize)
			return 2;
		if (fileContents[curPos] == 'X') checkValue = true;
		else checkValue = false;
		netSvcChecks.push_back(checkValue);
		curPos += 2;
	}

	if (offlProcChecks.size() != startProcess.size() ||
		offlSvcChecks.size() != startService.size())
		return 2;

	return 0;
}
