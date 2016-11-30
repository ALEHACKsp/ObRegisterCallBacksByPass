
// RestoreAccessAppDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RestoreAccessApp.h"
#include "RestoreAccessAppDlg.h"
#include "afxdialogex.h"
#include <windows.h>  
#include <winsvc.h>  
#include <conio.h>  
#include <stdio.h>
#include <winioctl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DRIVER_NAME "RestoreAccessEx"
#define DRIVER_PATH ".\\RestoreAccess.sys"
#define DRIVER_PATH_WIN7 ".\\RestoreAccessExWin7x64.sys"

#define IOCTRL_BASE 0x800  

#define IOCTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTRL_BASE + i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CTL_HELLO IOCTL_CODE(0)  
#define CTL_ULONG IOCTL_CODE(1)  
#define CTL_WCHAR IOCTL_CODE(2)  
#define CTL_RESTORE_OBJECT_ACCESS IOCTL_CODE(3)

#define MAKELONG64(a, b)	((LONG64)(((DWORD)(((DWORD_PTR)(a)) & 0xffffffff)) | \
	((ULONG64)((DWORD)(((DWORD_PTR)(b)) & 0xffffffff))) << 32))

HANDLE g_hDevice = NULL;

//װ��NT��������
BOOL LoadDriver(char* lpszDriverName, char* lpszDriverPath, char* lpszDriverPathEx)
{
	//char szDriverImagePath[256] = "D:\\DriverTest\\ntmodelDrv.sys";
	char szDriverImagePath[256] = { 0 };
	//�õ�����������·��
	GetFullPathName(lpszDriverPath, 256, szDriverImagePath, NULL);
	AfxMessageBox(szDriverImagePath);
	if (!PathFileExists(szDriverImagePath))
	{
		GetFullPathName(lpszDriverPathEx, 256, szDriverImagePath, NULL);
		AfxMessageBox(szDriverImagePath,MB_OK);
	}
		
	

	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM�������ľ��
	SC_HANDLE hServiceDDK = NULL;//NT��������ķ�����

								 //�򿪷�����ƹ�����
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hServiceMgr == NULL)
	{
		//OpenSCManagerʧ��
		printf("OpenSCManager() Failed %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		////OpenSCManager�ɹ�
		printf("OpenSCManager() ok ! \n");
	}

	//������������Ӧ�ķ���
	hServiceDDK = CreateService(hServiceMgr,
		lpszDriverName, //�����������ע����е�����  
		lpszDriverName, // ע������������ DisplayName ֵ  
		SERVICE_ALL_ACCESS, // ������������ķ���Ȩ��  
		SERVICE_KERNEL_DRIVER,// ��ʾ���صķ�������������  
		SERVICE_DEMAND_START, // ע������������ Start ֵ  �Զ����� �������� �ֶ����� ����
		SERVICE_ERROR_IGNORE, // ע������������ ErrorControl ֵ  
		szDriverImagePath, // ע������������ ImagePath ֵ  
		NULL,  //GroupOrder HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GroupOrderList
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//�жϷ����Ƿ�ʧ��
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			//��������ԭ�򴴽�����ʧ��
			printf("CrateService() Failed %d ! \n", dwRtn);
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			//���񴴽�ʧ�ܣ������ڷ����Ѿ�������
			printf("CrateService() Failed Service is ERROR_IO_PENDING or ERROR_SERVICE_EXISTS! \n");
			AfxMessageBox("CrateService() Failed Service is ERROR_IO_PENDING or ERROR_SERVICE_EXISTS!",MB_OK);
		}

		// ���������Ѿ����أ�ֻ��Ҫ��  
		hServiceDDK = OpenService(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			//����򿪷���Ҳʧ�ܣ�����ζ����
			dwRtn = GetLastError();
			printf("OpenService() Failed %d ! \n", dwRtn);
			AfxMessageBox("OpenService failed !\n");
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			printf("OpenService() ok ! \n");
		}
	}
	else
	{
		printf("CrateService() ok ! \n");
	}

	//�����������
	bRet = StartService(hServiceDDK, NULL, NULL);
	if (!bRet)
	{
		DWORD dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("StartService() Failed %d ! \n", dwRtn);
			AfxMessageBox("StartService() failed !");
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			if (dwRtn == ERROR_IO_PENDING)
			{
				//�豸����ס
				printf("StartService() Failed ERROR_IO_PENDING ! \n");
				AfxMessageBox("StartService() Failed ERROR_IO_PENDING ! !");
				bRet = FALSE;
				goto BeforeLeave;
			}
			else
			{
				//�����Ѿ�����
				printf("StartService() Failed ERROR_SERVICE_ALREADY_RUNNING ! \n");
				AfxMessageBox("StartService() Failed ERROR_SERVICE_ALREADY_RUNNING !");
				bRet = TRUE;
				goto BeforeLeave;
			}
		}
	}
	bRet = TRUE;
	//�뿪ǰ�رվ��
BeforeLeave:
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

//ж����������  
BOOL UnloadDriver(char * szSvrName)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;//SCM�������ľ��
	SC_HANDLE hServiceDDK = NULL;//NT��������ķ�����
	SERVICE_STATUS SvrSta;
	//��SCM������
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//����SCM������ʧ��
		printf("OpenSCManager() Failed %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//����SCM������ʧ�ܳɹ�
		printf("OpenSCManager() ok ! \n");
	}
	//����������Ӧ�ķ���
	hServiceDDK = OpenService(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);

	if (hServiceDDK == NULL)
	{
		//����������Ӧ�ķ���ʧ��
		printf("OpenService() Failed %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		printf("OpenService() ok ! \n");
	}
	//ֹͣ�����������ֹͣʧ�ܣ�ֻ�������������ܣ��ٶ�̬���ء�  
	if (!ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta))
	{
		printf("ControlService() Failed %d !\n", GetLastError());
	}
	else
	{
		//����������Ӧ��ʧ��
		printf("ControlService() ok !\n");
	}


	//��̬ж����������  

	if (!DeleteService(hServiceDDK))
	{
		//ж��ʧ��
		printf("DeleteSrevice() Failed %d !\n", GetLastError());
	}
	else
	{
		//ж�سɹ�
		printf("DelServer:deleteSrevice() ok !\n");
	}

	bRet = TRUE;
BeforeLeave:
	//�뿪ǰ�رմ򿪵ľ��
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}


BOOL RestoreAccess(DWORD ActiveId, DWORD PassiveId)
{
	ULONG64 ProcessId = 0;
	DWORD dwRet = 0;

	ProcessId = MAKELONG64(PassiveId, ActiveId);

	if (g_hDevice)
	{
		if (!DeviceIoControl(g_hDevice,
			CTL_RESTORE_OBJECT_ACCESS,
			&ProcessId,
			sizeof(ActiveId) + sizeof(PassiveId),
			NULL,
			0,
			&dwRet,
			NULL))
		{
			return FALSE;
		}
	}

	return TRUE;
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


// CRestoreAccessAppDlg �Ի���



CRestoreAccessAppDlg::CRestoreAccessAppDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_RESTOREACCESSAPP_DIALOG, pParent)
	, m_RestorePid(0)
	, m_GamePid(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRestoreAccessAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_AESTORE_PID, m_RestorePid);
	DDX_Text(pDX, IDC_EDIT_GAME_PID, m_GamePid);
}

BEGIN_MESSAGE_MAP(CRestoreAccessAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_LOAD, &CRestoreAccessAppDlg::OnBnClickedBtnLoad)
	ON_BN_CLICKED(IDC_BTN_UNLOAD, &CRestoreAccessAppDlg::OnBnClickedBtnUnload)
	ON_BN_CLICKED(IDC_BTN_RESTORE, &CRestoreAccessAppDlg::OnBnClickedBtnRestore)
END_MESSAGE_MAP()


// CRestoreAccessAppDlg ��Ϣ�������

BOOL CRestoreAccessAppDlg::OnInitDialog()
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

	//ShowWindow(SW_MINIMIZE);

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CRestoreAccessAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CRestoreAccessAppDlg::OnPaint()
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
HCURSOR CRestoreAccessAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRestoreAccessAppDlg::OnBnClickedBtnLoad()
{
	BOOL bRet = LoadDriver(DRIVER_NAME, DRIVER_PATH, DRIVER_PATH_WIN7);
	if (!bRet)
	{
		AfxMessageBox("LoadNtDriver error !", MB_OK);
		return;
	}

	//AfxMessageBox("LoadDriver Success !",MB_OK);

	Sleep(1000);
	
	g_hDevice = CreateFile("\\\\.\\RestoreAccess",
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (g_hDevice != INVALID_HANDLE_VALUE)
	{
		//AfxMessageBox("Create Device ok !");
	}
	else
	{
		AfxMessageBox("Create Device Failed  !");
		return;
	}

	this->GetDlgItem(IDC_BTN_UNLOAD)->EnableWindow();
	this->GetDlgItem(IDC_BTN_RESTORE)->EnableWindow();
}


void CRestoreAccessAppDlg::OnBnClickedBtnUnload()
{
	if (g_hDevice)
		CloseHandle(g_hDevice);

	BOOL bRet = UnloadDriver(DRIVER_NAME);
	if (!bRet)
	{
		AfxMessageBox("UnloadNTDriver error !", MB_OK);
		return;
	}

	AfxMessageBox("UnloadNTDriver Success !", MB_OK);
}


void CRestoreAccessAppDlg::OnBnClickedBtnRestore()
{
	UpdateData(TRUE);

	DWORD ResPid = m_RestorePid;
	DWORD GamePid = m_GamePid;
	if (!RestoreAccess(ResPid, GamePid))
	{
		AfxMessageBox("Restore failed !",MB_OK);
	}
	else
	{
		AfxMessageBox("Restore success !",MB_OK);
	}
}
