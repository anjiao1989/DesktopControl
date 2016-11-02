
// DeskCtrlInstallerDlg.cpp : ʵ���ļ�
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

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CDeskCtrlInstallerDlg �Ի���



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
		/// д������
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
		/// д������
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


// CDeskCtrlInstallerDlg ��Ϣ�������

BOOL CDeskCtrlInstallerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
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

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CDeskCtrlInstallerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CDeskCtrlInstallerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDeskCtrlInstallerDlg::OnBnClickedBtnSaveDesktop()
{
	/// ��ȡ������Ϣ
	CString strDes;
	m_edtDes.GetWindowText(strDes);
	if(strDes.IsEmpty()) {
		MessageBox(_T("������������Ϣ"), _T("����"), MB_OK | MB_ICONERROR);
		return;
	}
	if(m_listData.FindString(-1, strDes) != -1) {
		MessageBox(_T("�����������Ϣ����"), _T("����"), MB_OK | MB_ICONERROR);
		return;
	}

	/// ׼��ִ�в���
	if(!PrepareForSave()) {
		MessageBox( _T("��ʼ��ʧ��"), _T("����"), MB_OK | MB_ICONERROR);
		return;
	}

	/// ִ�б��湦��
	CString strAppPath;
	GetAppFolderPath(strAppPath);
	strAppPath += _T("DesktopControl.dll");

	HINSTANCE hInstance = NULL;

	/// ����Զ��ģ��
	if(!RemoteLoadLibrary(GetDesktopProcessId(), 
		strAppPath.GetString(), &hInstance))
	{
		MessageBox(_T("����Զ���߳�ʧ��"), _T("����"), MB_ICONERROR | MB_OK);
		return;
	}

	/// ֪ͨ����ִ�����
	HANDLE hNotifyEvent = CreateEvent(nullptr, TRUE, FALSE,
		_T("DesktopControl\\FuncOKEvent"));
	WaitForSingleObject(hNotifyEvent, 5 * 1000);
	ResetEvent(hNotifyEvent);
	CloseHandle(hNotifyEvent);

	/// ж��Զ��ģ��
	RemoteFreeLibrary(GetDesktopProcessId(), hInstance);

	/// ���������ļ�
	SaveDesktopConfig(strDes);

	m_edtDes.SetWindowText(_T(""));
	MessageBox(_T("�����ɹ����"), _T("�ɹ�"), MB_OK | MB_ICONINFORMATION);
}

void CDeskCtrlInstallerDlg::OnBnClickedBtnRestoreDesktop()
{
	int nSel = m_listData.GetCurSel();
	if(nSel == -1) {
		MessageBox(_T("��ѡ��Ҫ��ԭ����Ŀ"), _T("����"), MB_OK);
		return;
	}

	/// ׼��ִ�в���
	if(!PrepareForRestore()) {
		MessageBox(_T("��ʼ��ʧ��"), _T("����"), MB_OK | MB_ICONERROR);
		return;
	}

	/// ִ�б��湦��
	CString strAppPath;
	GetAppFolderPath(strAppPath);
	strAppPath += _T("DesktopControl.dll");

	HINSTANCE hInstance = NULL;

	/// ����Զ��ģ��
	if(!RemoteLoadLibrary(GetDesktopProcessId(),
		strAppPath.GetString(), &hInstance)) {
		MessageBox(_T("����Զ���߳�ʧ��"), _T("����"), MB_ICONERROR | MB_OK);
		return;
	}

	/// ֪ͨ����ִ�����
	HANDLE hNotifyEvent = CreateEvent(nullptr, TRUE, FALSE,
		_T("DesktopControl\\FuncOKEvent"));
	WaitForSingleObject(hNotifyEvent, 5 * 1000);
	ResetEvent(hNotifyEvent);
	CloseHandle(hNotifyEvent);

	/// ж��Զ��ģ��
	RemoteFreeLibrary(GetDesktopProcessId(), hInstance);

	MessageBox(_T("�����ɹ����"), _T("�ɹ�"), MB_OK | MB_ICONINFORMATION);
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
		MessageBox(_T("����ѡ��һ����Ŀ"), _T("����"), MB_OK);
		return;
	}
	m_listData.DeleteString(nSel);
	m_arrDesktopCfg.erase(m_arrDesktopCfg.begin() + nSel);

	SaveConfig();
}
