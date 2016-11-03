
// DeskCtrlInstallerDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CDeskCtrlInstallerDlg �Ի���
class CDeskCtrlInstallerDlg : public CDialogEx
{
// ����
public:
	CDeskCtrlInstallerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DESKCTRLINSTALLER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

	BOOL PrepareForSave();
	BOOL SaveDesktopConfig(const CString & strDes);

	BOOL PrepareForRestore();
	BOOL SaveConfig();

// ʵ��
protected:
	HICON m_hIcon;
	CListBox m_listData;
	CEdit m_edtDes;

	CString m_strCfgPath;
	std::vector<std::string> m_arrDesktopCfg;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBtnSaveDesktop();
	afx_msg void OnBnClickedBtnRestoreDesktop();
	afx_msg void OnLbnSelchangeListData();
	afx_msg void OnBnClickedBtnDelete();
};
