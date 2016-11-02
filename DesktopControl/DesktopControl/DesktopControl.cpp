// DesktopControl.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "strtool.h"

HINSTANCE g_hDllHandle;

//void Test()
//{
//	HWND  hwndParent = ::FindWindow("Progman", "Program Manager");
//	HWND  hwndSHELLDLL_DefView = ::FindWindowEx(hwndParent, NULL, "SHELLDLL_DefView", NULL);
//	HWND  hwndSysListView32 = ::FindWindowEx(hwndSHELLDLL_DefView, NULL, 
//		"SysListView32", "FolderView");
//
//	int Nm = ListView_GetItemCount(hwndSysListView32);
//
//	int sNm = 0;
//	if (Nm >= 10)
//	{
//		sNm = 10;
//	}
//	else {
//		sNm = Nm;
//	}
//
//	for (int i = 0; i < sNm; i++)
//	{
//		int x = 400 + 150 * cos(i * 36 * 3.1415926 / 180);
//		int y = 400 + 150 * sin(i * 36 * 3.1415926 / 180);
//
//		::SendMessage(hwndSysListView32, LVM_SETITEMPOSITION, i, MAKELPARAM(x, y));
//	}
//	ListView_RedrawItems(hwndSysListView32, 0, ListView_GetItemCount(hwndSysListView32) - 1);
//	::UpdateWindow(hwndSysListView32);
//}

class CDesktopList
{
public:
	bool Initialize()
	{
		HWND hwndParent = ::FindWindow(_T("Progman"), _T("Program Manager"));
		if(hwndParent == nullptr)
			return false;

		HWND hwndSHELLDLL_DefView = ::FindWindowEx(hwndParent, NULL,
			_T("SHELLDLL_DefView"), NULL);
		if(hwndSHELLDLL_DefView == nullptr)
			return false;

		m_hDesktpoList = ::FindWindowEx(hwndSHELLDLL_DefView, NULL,
			_T("SysListView32"), _T("FolderView"));
		if(m_hDesktpoList == nullptr)
			return false;

		return true;
	}
	void Destroy()
	{}

	int GetItemCount()
	{
		return ListView_GetItemCount(m_hDesktpoList);
	}
	bool GetItemInfo(int nIndex, CString & strText, int & x, int & y)
	{
		ListView_GetItemText(m_hDesktpoList, nIndex, 0, strText.GetBuffer(512), 512);
		strText.ReleaseBuffer();

		POINT pt = {0};
		ListView_GetItemPosition(m_hDesktpoList, nIndex, &pt);

		x = pt.x;
		y = pt.y;
		return !strText.IsEmpty();
	}
	bool SetItemPosition(int nIndex, int x, int y)
	{
		return ListView_SetItemPosition(m_hDesktpoList, nIndex, x, y) != FALSE;
	}
	void RedrawDesktop()
	{
		ListView_RedrawItems(m_hDesktpoList, 0,
			ListView_GetItemCount(m_hDesktpoList) - 1);
		::UpdateWindow(m_hDesktpoList);
	}

private:
	HWND m_hDesktpoList;
};

class CDesktopIcons
{
	struct IconInfo
	{
		int nIndex;
		CString strText;
		int x;
		int y;
	};

public:
	CDesktopIcons(CDesktopList & DeskList) :
		m_DeskList(DeskList)
	{
		int nCount = DeskList.GetItemCount();
		for(int i = 0; i < nCount; i++) {
			IconInfo Info;
			Info.nIndex = i;
			DeskList.GetItemInfo(i, Info.strText, Info.x, Info.y);
			m_arrIconInfo.push_back(Info);
		}

		// 设置交换临时点的位置
		int cx = GetSystemMetrics(SM_CXSCREEN);
		int cy = GetSystemMetrics(SM_CYSCREEN);

		m_ptTmp.x = cx / 2;
		m_ptTmp.y = cy / 2;
	}

	IconInfo * FindItemByText(const CString & strText)
	{
		for(auto ite = m_arrIconInfo.begin(); ite != m_arrIconInfo.end(); ++ite) {
			if(ite->strText == strText)
				return ite._Ptr;
		}
		return nullptr;
	}
	IconInfo * FindItemByPoint(int x, int y)
	{
		for(auto ite = m_arrIconInfo.begin(); ite != m_arrIconInfo.end(); ++ite) {
			if(ite->x == x && ite->y == y)
				return ite._Ptr;
		}
		return nullptr;
	}

	void SetIconPoint(const CString & strText, int x, int y)
	{
		IconInfo * pSrcIconInfo = FindItemByText(strText);
		IconInfo * pTargetIconInfo = FindItemByPoint(x, y);

		if(pSrcIconInfo == nullptr) {
			// 当前没有这一项
			return;
		}
		if(pTargetIconInfo != nullptr) {
			m_DeskList.SetItemPosition(pTargetIconInfo->nIndex, m_ptTmp.x, m_ptTmp.y);
		}
		m_DeskList.SetItemPosition(pSrcIconInfo->nIndex, x, y);
		if(pTargetIconInfo != nullptr) {
			m_DeskList.SetItemPosition(pTargetIconInfo->nIndex,
				pSrcIconInfo->x, pSrcIconInfo->y);
		}
	}

private:
	std::vector<IconInfo> m_arrIconInfo;
	POINT m_ptTmp;

	CDesktopList & m_DeskList;
};

bool SaveDesktopIcons(LPCSTR lpszPath)
{
	std::ofstream ofs(lpszPath);
	if(!ofs.is_open())
		return false;

	CDesktopList Desktop;
	if(!Desktop.Initialize()) {
		OutputDebugString(_T("Desktop.Initialize() return Error\n"));
		return false;
	}

	int nCount = Desktop.GetItemCount();
	for(int i = 0; i < nCount; i++) {
		CString strText;
		int x, y;
		Desktop.GetItemInfo(i, strText, x, y);

		ofs << strText.GetString() << ";" << x << ";" << y << std::endl;
	}
	return true;
}
bool RestoreDesktopIcons(LPCSTR lpszPath)
{
	std::ifstream ifs(lpszPath);
	if(!ifs.is_open())
		return false;

	CDesktopList Desktop;
	if(!Desktop.Initialize()) {
		OutputDebugString(_T("Desktop.Initialize() return Error\n"));
		return false;
	}

	CDesktopIcons DesktopIcons(Desktop);

	std::string strLine;
	std::getline(ifs, strLine);
	while(!strLine.empty()) {

		OutputDebugString(strLine.c_str());

		std::vector<std::string> arrData;
		strtool::split(strLine, arrData, ";");
		if(arrData.size() != 3) {
			OutputDebugString(_T("数据格式错误\n"));
			return false;
		}

		DesktopIcons.SetIconPoint(arrData[0].c_str(),
			std::stoi(arrData[1]), std::stoi(arrData[2]));

		strLine.clear();
		std::getline(ifs, strLine);
	}
	Desktop.RedrawDesktop();
	return true;
}

void GetAppFolderPath(CString & strPath)
{
	GetModuleFileName(g_hDllHandle, strPath.GetBuffer(MAX_PATH), MAX_PATH);
	strPath.ReleaseBuffer();
	strPath = strPath.Left(strPath.ReverseFind(_T('\\')));
	strPath += _T("\\");
}

void TidyDesktopIcons()
{
	/// 计算配置项的全路径
	CString strAppPath;
	GetAppFolderPath(strAppPath);

	CString strCfgPath = strAppPath + _T("TidyDesktopIcons.cfg");
	CString strFuncPath = strAppPath + _T("func.cfg");

	/// 读取要执行的功能
	std::ifstream ifs(strFuncPath.GetString());
	char Func = 0;
	ifs.read(&Func, 1);
	if(ifs.gcount() != 1) {
		OutputDebugString(_T("读取功能描述文件失败\n"));
		return;
	}
	ifs.close();

	/// 执行相应功能
	if(Func == '1') {
		if(!SaveDesktopIcons(strCfgPath)) {
			OutputDebugString(_T("SaveDesktopIcons return Error\n"));
		}
	} else if(Func == '2') {
		if(!RestoreDesktopIcons(strCfgPath)) {
			OutputDebugString(_T("RestoreDesktopIcons return Error\n"));
		}
	} else {
		OutputDebugString(_T("Unknown command\n"));
	}
	
	/// 通知功能执行完毕
	HANDLE hNotifyEvent = CreateEvent(nullptr, TRUE, FALSE, 
		_T("DesktopControl\\FuncOKEvent"));
	SetEvent(hNotifyEvent);
	CloseHandle(hNotifyEvent);
}

DWORD __stdcall UnloadDllThread(LPVOID)
{
	Sleep(3 * 1000);
	FreeLibrary(g_hDllHandle);
	return 0;
}

BOOL WINAPI DllMain(IN HINSTANCE hDllHandle,
	IN DWORD     nReason,
	IN LPVOID    Reserved)
{
	switch(nReason) {
	case DLL_PROCESS_ATTACH:
	{
		g_hDllHandle = hDllHandle;

		OutputDebugString("hehe\n");
		// DebugBreak();

		DisableThreadLibraryCalls(hDllHandle);
		TidyDesktopIcons();

		/*
		/// 卸载自己
		DWORD dwThreadId = 0;
		HANDLE hThread = CreateThread(NULL, 0, UnloadDllThread,
			NULL, 0, &dwThreadId);
		CloseHandle(hThread);
		*/

		break;
	}

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}


int main()
{
	CDesktopList Desktop;
	if(!Desktop.Initialize())
		return -1;

	Desktop.SetItemPosition(0, 0, 0);
	Desktop.SetItemPosition(1, 1, 1);

	//// int nCount = Desktop.GetItemCount();
	//// Desktop.SetItemPosition(0, 0, 0);

	//CString strText;
	//int x, y;
	//Desktop.GetItemInfo(1, strText, x, y);

	//_tprintf(_T("%s, %d, %d"), strText.GetString(), x, y);

	system("pause");
	return 0;
}
