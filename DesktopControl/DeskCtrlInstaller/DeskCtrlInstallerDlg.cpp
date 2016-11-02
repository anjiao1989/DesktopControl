
// DeskCtrlInstallerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "DeskCtrlInstaller.h"
#include "DeskCtrlInstallerDlg.h"
#include "afxdialogex.h"
#include "Base64.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define APP_NAME    _T("DeskCtrlInstaller")

void GetAppFolderPath(CString & strPath)
{
	GetModuleFileName(NULL, strPath.GetBuffer(MAX_PATH), MAX_PATH);
	strPath.ReleaseBuffer();
	strPath = strPath.Left(strPath.ReverseFind(_T('\\')));
	strPath += _T("\\");
}

BOOL RemoteLoadLibrary(DWORD dwPid, LPCSTR lpszPath,
	HINSTANCE * pInstance = NULL)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if(hProcess == NULL)
		return FALSE;

	DWORD dwNeedSize = (DWORD)strlen(lpszPath) + 1;
	LPVOID lpRemoteMem = VirtualAllocEx(hProcess, NULL, dwNeedSize,
		MEM_COMMIT, PAGE_READWRITE);
	if(lpRemoteMem == NULL)
		return FALSE;

	SIZE_T dwWrited = 0;
	BOOL bResult = WriteProcessMemory(hProcess, lpRemoteMem, lpszPath,
		dwNeedSize, &dwWrited);
	if(!bResult || dwWrited != dwNeedSize)
		return FALSE;

	HINSTANCE hKernel32 = LoadLibrary("Kernel32.dll");
	LPVOID lpFunc = GetProcAddress(hKernel32, "LoadLibraryA");
	if(lpFunc == NULL)
		return FALSE;

	DWORD dwThreadId = 0;
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)lpFunc, lpRemoteMem, 0, &dwThreadId);
	if(hRemoteThread == NULL)
		return FALSE;

	WaitForSingleObject(hRemoteThread, INFINITE);
	if(pInstance != NULL) {
		DWORD dwExitCode = 0;
		GetExitCodeThread(hRemoteThread, &dwExitCode);
#ifdef _AMD64_
		*pInstance = (HINSTANCE)(0x7FE00000000i64 | dwExitCode);
#else
		*pInstance = (HINSTANCE)dwExitCode;
#endif
	}

	CloseHandle(hRemoteThread);
	VirtualFreeEx(hProcess, lpRemoteMem, dwNeedSize, MEM_DECOMMIT);
	CloseHandle(hProcess);

	return TRUE;
}

BOOL RemoteFreeLibrary(DWORD dwPid, HINSTANCE Instance)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if(hProcess == NULL)
		return FALSE;

	HINSTANCE hKernel32 = LoadLibrary("Kernel32.dll");
	LPVOID lpFunc = GetProcAddress(hKernel32, "FreeLibrary");
	if(lpFunc == NULL)
		return FALSE;

	DWORD dwThreadId = 0;
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)lpFunc, Instance, 0, &dwThreadId);
	if(hRemoteThread == NULL)
		return FALSE;

	WaitForSingleObject(hRemoteThread, INFINITE);
	CloseHandle(hRemoteThread);
	CloseHandle(hProcess);

	return TRUE;
}

DWORD GetDesktopProcessId()
{
	DWORD dwPid = 0;
	HWND hDesktop = ::FindWindow(_T("Progman"), _T("Program Manager"));
	if(hDesktop == NULL)
		return 0;
	GetWindowThreadProcessId(hDesktop, &dwPid);
	return dwPid;
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDeskCtrlInstallerDlg 对话框



CDeskCtrlInstallerDlg::CDeskCtrlInstallerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DESKCTRLINSTALLER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDeskCtrlInstallerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DATA, m_listData);
	DDX_Control(pDX, IDC_EDT_DESC, m_edtDes);
}

BOOL CDeskCtrlInstallerDlg::PrepareForSave()
{
	CString strFuncPath;
	GetAppFolderPath(strFuncPath);
	strFuncPath.Append(_T("func.cfg"));

	std::ofstream ofs(strFuncPath.GetString());
	if(!ofs.is_open())
		return FALSE;
	ofs << "1";
	ofs.close();
	return TRUE;
}

BOOL CDeskCtrlInstallerDlg::SaveDesktopConfig(const CString & strDes)
{
	CString strDataPath;
	GetAppFolderPath(strDataPath);
	strDataPath.Append(_T("TidyDesktopIcons.cfg"));

	std::ifstream ifs(strDataPath.GetString());
	if(!ifs.is_open())
		return FALSE;

	ifs.seekg(0, std::ios::end);
	size_t len = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<char> arrData(len + 1);
	ifs.read(&arrData[0], len);
	ifs.close();
	
	std::vector<char> arrBase64Target(Base64_TargetLen(len) + 1);
	Base64_Encode(&arrBase64Target[0], &arrData[0], len);
	arrBase64Target[Base64_TargetLen(len)] = 0;

	m_listData.AddString(strDes);
	m_arrDesktopCfg.push_back(&arrBase64Target[0]);

	SaveConfig();
	return TRUE;
}

BOOL CDeskCtrlInstallerDlg::PrepareForRestore()
{
	{
		/// 写功能项
		CString strFuncPath;
		GetAppFolderPath(strFuncPath);
		strFuncPath.Append(_T("func.cfg"));

		std::ofstream ofs(strFuncPath.GetString());
		if(!ofs.is_open())
			return FALSE;
		ofs << "2";
		ofs.close();
	}

	{
		/// 写数据项
		int nSel = m_listData.GetCurSel();
		const CString & strBase64 = m_arrDesktopCfg[nSel];
		CString strData;
		Base64_Decode(strData.GetBuffer((int)Base64_SrcLen(strBase64.GetLength()) + 1),
			strBase64.GetString(), strBase64.GetLength());
		strData.ReleaseBuffer();

		CString strFuncPath;
		GetAppFolderPath(strFuncPath);
		strFuncPath.Append(_T("TidyDesktopIcons.cfg"));

		std::ofstream ofs(strFuncPath.GetString());
		if(!ofs.is_open()) {
			return FALSE;
		}
		ofs << strData.GetString();
		ofs.close();
	}
	
	return TRUE;
}

BOOL CDeskCtrlInstallerDlg::SaveConfig()
{
	DeleteFile(m_strCfgPath);
	int nCount = m_listData.GetCount();

	CString strCount;
	strCount.Format(_T("%d"), nCount);
	WritePrivateProfileString(APP_NAME, _T("count"), strCount, m_strCfgPath);

	for(int i = 0; i < nCount; i++) {
		CString strKey;
		strKey.Format(_T("des_%d"), i);

		CString strValue;
		m_listData.GetText(i, strValue);
		WritePrivateProfileString(APP_NAME, strKey, strValue, m_strCfgPath);
		
		strKey.Format(_T("file_%d"), i);
		strValue = m_arrDesktopCfg[i];
		WritePrivateProfileString(APP_NAME, strKey, strValue, m_strCfgPath);
	}
	return TRUE;
}

BEGIN_MESSAGE_MAP(CDeskCtrlInstallerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SAVE_DESKTOP, &CDeskCtrlInstallerDlg::OnBnClickedBtnSaveDesktop)
	ON_BN_CLICKED(IDC_BTN_RESTORE_DESKTOP, &CDeskCtrlInstallerDlg::OnBnClickedBtnRestoreDesktop)
	ON_LBN_SELCHANGE(IDC_LIST_DATA, &CDeskCtrlInstallerDlg::OnLbnSelchangeListData)
	ON_BN_CLICKED(IDC_BTN_DELETE, &CDeskCtrlInstallerDlg::OnBnClickedBtnDelete)
END_MESSAGE_MAP()


// CDeskCtrlInstallerDlg 消息处理程序

BOOL CDeskCtrlInstallerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	GetAppFolderPath(m_strCfgPath);
	m_strCfgPath.Append(_T("DeskCtrlInstallerCfg.ini"));

	int nCount = GetPrivateProfileInt(APP_NAME, _T("count"), 0, m_strCfgPath);
	for(int i = 0; i < nCount; i++) {
		CString strValue;
		CString strKey;

		strKey.Format(_T("des_%d"), i);
		GetPrivateProfileString(APP_NAME, strKey, _T(""),
			strValue.GetBuffer(MAX_PATH), MAX_PATH, m_strCfgPath);
		strValue.ReleaseBuffer();
		m_listData.AddString(strValue);

		strKey.Format(_T("file_%d"), i);
		GetPrivateProfileString(APP_NAME, strKey, _T(""),
			strValue.GetBuffer(MAX_PATH), MAX_PATH, m_strCfgPath);
		strValue.ReleaseBuffer();
		m_arrDesktopCfg.push_back(strValue);
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDeskCtrlInstallerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDeskCtrlInstallerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDeskCtrlInstallerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDeskCtrlInstallerDlg::OnBnClickedBtnSaveDesktop()
{
	/// 获取描述信息
	CString strDes;
	m_edtDes.GetWindowText(strDes);
	if(strDes.IsEmpty()) {
		MessageBox(_T("请输入描述信息"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}
	if(m_listData.FindString(-1, strDes) != -1) {
		MessageBox(_T("输入的描述信息重名"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}

	/// 准备执行参数
	if(!PrepareForSave()) {
		MessageBox( _T("初始化失败"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}

	/// 执行保存功能
	CString strAppPath;
	GetAppFolderPath(strAppPath);
	strAppPath += _T("DesktopControl.dll");

	HINSTANCE hInstance = NULL;

	/// 加载远程模块
	if(!RemoteLoadLibrary(GetDesktopProcessId(), 
		strAppPath.GetString(), &hInstance))
	{
		MessageBox(_T("创建远程线程失败"), _T("错误"), MB_ICONERROR | MB_OK);
		return;
	}

	/// 通知功能执行完毕
	HANDLE hNotifyEvent = CreateEvent(nullptr, TRUE, FALSE,
		_T("DesktopControl\\FuncOKEvent"));
	WaitForSingleObject(hNotifyEvent, 5 * 1000);
	ResetEvent(hNotifyEvent);
	CloseHandle(hNotifyEvent);

	/// 卸载远程模块
	RemoteFreeLibrary(GetDesktopProcessId(), hInstance);

	/// 保存配置文件
	SaveDesktopConfig(strDes);

	m_edtDes.SetWindowText(_T(""));
	MessageBox(_T("操作成功完成"), _T("成功"), MB_OK | MB_ICONINFORMATION);
}

void CDeskCtrlInstallerDlg::OnBnClickedBtnRestoreDesktop()
{
	int nSel = m_listData.GetCurSel();
	if(nSel == -1) {
		MessageBox(_T("请选择要还原的项目"), _T("错误"), MB_OK);
		return;
	}

	/// 准备执行参数
	if(!PrepareForRestore()) {
		MessageBox(_T("初始化失败"), _T("错误"), MB_OK | MB_ICONERROR);
		return;
	}

	/// 执行保存功能
	CString strAppPath;
	GetAppFolderPath(strAppPath);
	strAppPath += _T("DesktopControl.dll");

	HINSTANCE hInstance = NULL;

	/// 加载远程模块
	if(!RemoteLoadLibrary(GetDesktopProcessId(),
		strAppPath.GetString(), &hInstance)) {
		MessageBox(_T("创建远程线程失败"), _T("错误"), MB_ICONERROR | MB_OK);
		return;
	}

	/// 通知功能执行完毕
	HANDLE hNotifyEvent = CreateEvent(nullptr, TRUE, FALSE,
		_T("DesktopControl\\FuncOKEvent"));
	WaitForSingleObject(hNotifyEvent, 5 * 1000);
	ResetEvent(hNotifyEvent);
	CloseHandle(hNotifyEvent);

	/// 卸载远程模块
	RemoteFreeLibrary(GetDesktopProcessId(), hInstance);

	MessageBox(_T("操作成功完成"), _T("成功"), MB_OK | MB_ICONINFORMATION);
}

void CDeskCtrlInstallerDlg::OnLbnSelchangeListData()
{
	int nSel = m_listData.GetCurSel();
	if(nSel == -1)
		return;
	CString strText;
	m_listData.GetText(nSel, strText);

	m_edtDes.SetWindowText(strText);
}


void CDeskCtrlInstallerDlg::OnBnClickedBtnDelete()
{
	int nSel = m_listData.GetCurSel();
	if(nSel == -1) {
		MessageBox(_T("请先选择一个项目"), _T("错误"), MB_OK);
		return;
	}
	m_listData.DeleteString(nSel);
	m_arrDesktopCfg.erase(m_arrDesktopCfg.begin() + nSel);

	SaveConfig();
}
