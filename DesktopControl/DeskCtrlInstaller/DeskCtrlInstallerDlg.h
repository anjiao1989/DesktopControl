
// DeskCtrlInstallerDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CDeskCtrlInstallerDlg 对话框
class CDeskCtrlInstallerDlg : public CDialogEx
{
// 构造
public:
	CDeskCtrlInstallerDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DESKCTRLINSTALLER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	BOOL PrepareForSave();
	BOOL SaveDesktopConfig(const CString & strDes);

	BOOL PrepareForRestore();
	BOOL SaveConfig();

// 实现
protected:
	HICON m_hIcon;
	CListBox m_listData;
	CEdit m_edtDes;

	CString m_strCfgPath;
	std::vector<std::string> m_arrDesktopCfg;

	// 生成的消息映射函数
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
