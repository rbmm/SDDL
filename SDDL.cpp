// SDDL.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "resource.h"

_NT_BEGIN

#include "sd.h"

class VDbgPrint : public VPrint
{
	PWSTR _buf = 0, _ptr = 0;
	ULONG _cch = 0;
public:

	~VDbgPrint()
	{
		if (_buf)
		{
			LocalFree(_buf);
		}
	}

	BOOL InitBuf(ULONG cch)
	{
		if (_buf)
		{
			LocalFree(_buf), _buf = 0, _cch = 0;
		}

		if (_buf = (PWSTR)LocalAlloc(0, cch*sizeof(WCHAR)))
		{
			_ptr = _buf, _cch = cch;

			return TRUE;
		}

		return FALSE;
	}

	void SetToWnd(HWND hwnd)
	{
		if (PVOID buf = (PVOID)SendMessageW(hwnd, EM_GETHANDLE, 0, 0))
		{
			LocalFree(buf);
		}

		SendMessageW(hwnd, EM_SETHANDLE, (WPARAM)_buf, 0);
		_buf = 0, _cch = 0;
	}

	virtual VPrint& operator ()(PCWSTR format, ...)
	{
		va_list args;
		va_start(args, format);

		int len = _vsnwprintf_s(_ptr, _cch, _TRUNCATE, format, args);

		if (0 < len)
		{
			_ptr += len, _cch -= len;
		}

		va_end(args);

		return *this;
	}
};

struct RegEdit_HexData
{
	ULONG cb;
	CHAR buf[];
};

class SDialog : public LSA_LOOKUP, VDbgPrint
{
	HFONT _hFont;
	HICON _hi[2];
	HWND _hwnd[2];

	INT_PTR OnInitDialog(HWND hwndDlg);

	void OnOk(HWND hwndDlg);
	void OnDestroy();

	INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static INT_PTR CALLBACK _DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return reinterpret_cast<SDialog*>(GetWindowLongPtrW(hwndDlg, DWLP_USER))->DialogProc(hwndDlg, uMsg, wParam, lParam);
	}

	static INT_PTR CALLBACK StartDialogProc(HWND hwndDlg, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
	{
		if (uMsg == WM_INITDIALOG)
		{
			SetWindowLongPtrW(hwndDlg, DWLP_USER, lParam);
			SetWindowLongPtrW(hwndDlg, DWLP_DLGPROC, (LONG_PTR)_DialogProc);
			return reinterpret_cast<SDialog*>(lParam)->OnInitDialog(hwndDlg);
		}

		return 0;
	}

	BOOL FixClipData()
	{
		ULONG f = 0;
		while (f = EnumClipboardFormats(f))
		{
			WCHAR name[sizeof("RegEdit_HexData")];
			if (GetClipboardFormatNameW(f, name, _countof(name)) && !wcscmp(L"RegEdit_HexData", name))
			{
				if (HANDLE h = GetClipboardData(f))
				{
					SIZE_T s = GlobalSize(h);

					if (s > sizeof(RegEdit_HexData) + 3)
					{
						union {
							RegEdit_HexData* p;
							PVOID pv;
						};

						if (pv = GlobalLock(h))
						{
							if (p->cb + 3 + sizeof(RegEdit_HexData) == s && RtlValidSecurityDescriptor(p->buf))
							{
								if (InitBuf(MAXUSHORT+1))
								{
									DumpSecurityDescriptor(p->buf);
									SetToWnd(_hwnd[0]);
								}

								PWSTR psz;
								if (ConvertSecurityDescriptorToStringSecurityDescriptorW(p->buf, 
									SDDL_REVISION, 
									DACL_SECURITY_INFORMATION|
									SACL_SECURITY_INFORMATION|
									LABEL_SECURITY_INFORMATION|
									OWNER_SECURITY_INFORMATION|
									GROUP_SECURITY_INFORMATION, &psz, 0))
								{
									SetWindowTextW(_hwnd[1], psz);
									LocalFree(psz);
								}
							}

							GlobalUnlock(h);
						}
					}
				}

				return TRUE;
			}
		}

		return FALSE;
	}

	static LRESULT CALLBACK sSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		switch (uIdSubclass)
		{
		case IDC_EDIT2:
			switch (uMsg)
			{
			case WM_CONTEXTMENU:
				if (OpenClipboard(hWnd))
				{
					BOOL b = reinterpret_cast<SDialog*>(dwRefData)->FixClipData();
					if (b) EmptyClipboard();
					CloseClipboard();
					if (b)
					{
						return 0;
					}
				}
				break;
			}
			break;
		case IDC_EDIT1:
			switch (uMsg)
			{
			case WM_GETDLGCODE:
				return DLGC_WANTCHARS|DLGC_HASSETSEL|DLGC_WANTALLKEYS|DLGC_WANTARROWS;
			}
			break;
		default:
			__debugbreak();
		}

		if (uMsg == WM_NCDESTROY)
		{
			RemoveWindowSubclass(hWnd, sSubclassProc, uIdSubclass);
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}
public:
	SDialog () : LSA_LOOKUP(*static_cast<VDbgPrint*>(this)), _hi{}, _hFont(0)
	{
	}

	INT_PTR DoModal()
	{
		return DialogBoxParamW((HINSTANCE)&__ImageBase, MAKEINTRESOURCEW(IDD_DIALOG1), HWND_DESKTOP, StartDialogProc, (LPARAM)this);
	}
};

void SDialog::OnDestroy()
{
	if (HFONT hFont = _hFont)
	{
		DeleteObject(hFont);
	}

	int i = _countof(_hi);
	do 
	{
		if (HICON hi = _hi[--i])
		{
			DestroyIcon(hi);
		}
	} while (i);
}

INT_PTR SDialog::OnInitDialog(HWND hwndDlg)
{
	SetWindowSubclass(GetDlgItem(hwndDlg, IDC_EDIT1), sSubclassProc, IDC_EDIT1, (ULONG_PTR)this);
	SetWindowSubclass(GetDlgItem(hwndDlg, IDC_EDIT2), sSubclassProc, IDC_EDIT2, (ULONG_PTR)this);
	NTSTATUS status = Init();
	if (0 > status)
	{
		EndDialog(hwndDlg, status);
		return 0;
	}

	static const int 
		X_index[] = { SM_CXSMICON, SM_CXICON }, 
		Y_index[] = { SM_CYSMICON, SM_CYICON },
		icon_type[] = { ICON_SMALL, ICON_BIG};

	ULONG i = _countof(icon_type) - 1;
	do 
	{
		HICON hi;

		if (0 <= LoadIconWithScaleDown((HINSTANCE)&__ImageBase, MAKEINTRESOURCE(IDI_ICON1), 
			GetSystemMetrics(X_index[i]), GetSystemMetrics(Y_index[i]), &hi))
		{
			_hi[i] = hi;
			SendMessage(hwndDlg, WM_SETICON, icon_type[i], (LPARAM)hi);
		}
	} while (i--);

	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
	{
		ncm.lfCaptionFont.lfWeight = FW_NORMAL;
		ncm.lfCaptionFont.lfQuality = CLEARTYPE_QUALITY;
		ncm.lfCaptionFont.lfPitchAndFamily = FIXED_PITCH|FF_MODERN;
		wcscpy(ncm.lfCaptionFont.lfFaceName, L"Courier New");//

		if (_hFont = CreateFontIndirect(&ncm.lfCaptionFont))
		{
			SendMessageW(GetDlgItem(hwndDlg, IDC_EDIT1), WM_SETFONT, (WPARAM)_hFont, 0);
		}
	}

	_hwnd[0] = GetDlgItem(hwndDlg, IDC_EDIT1);
	_hwnd[1] = GetDlgItem(hwndDlg, IDC_EDIT2);

	return 0;
}

void SDialog::OnOk(HWND hwndDlg)
{
	HWND hwnd = GetDlgItem(hwndDlg, IDC_EDIT2);

	if (ULONG len = GetWindowTextLengthW(hwnd))
	{
		ULONG cb = ++len * sizeof(WCHAR);
		if (PWSTR StringSecurityDescriptor = (PWSTR)_malloca(cb))
		{
			if (GetWindowTextW(hwnd, StringSecurityDescriptor, len))
			{
				if (InitBuf(MAXUSHORT+1))
				{
					DumpStringSecurityDescriptor(StringSecurityDescriptor);
					SetToWnd(GetDlgItem(hwndDlg, IDC_EDIT1));
				}
			}
			_freea(StringSecurityDescriptor);
		}
	}
}

INT_PTR SDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		OnDestroy();
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			OnOk(hwndDlg);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		break;
	}
	return 0;
}

void WINAPI ep(void*)
{
	{
		SDialog dlg;
		dlg.DoModal();
	}
	ExitProcess(0);
}

_NT_END