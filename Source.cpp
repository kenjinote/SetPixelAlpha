#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"gdiplus")
#pragma comment(lib,"shlwapi")

#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>

#include <stdio.h>

using namespace Gdiplus;

TCHAR szClassName[] = TEXT("Window");

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0, size = 0;
	GetImageEncodersSize(&num, &size);
	if (!size)return -1;
	ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)GlobalAlloc(GPTR, size);
	if (!pImageCodecInfo)return -1;
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j<num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			GlobalFree(pImageCodecInfo);
			return j;
		}
	}
	GlobalFree(pImageCodecInfo);
	return -1;
}

BOOL SaveBitmapAs(LPCWSTR pszFileType, LPCWSTR pszFileName, Image* pImage)
{
	CLSID clsid;
	if (GetEncoderClsid(pszFileType, &clsid) >= 0)
	{
		pImage->Save(pszFileName, &clsid, 0);
		return TRUE;
	}
	return FALSE;
}

BOOL SetPixelAlpha(LPCWSTR lpszFilePath, int nPosition, BOOL bReSize)
{
	Bitmap *pBitmap = Bitmap::FromFile(lpszFilePath);
	if (pBitmap)
	{
		int nWidth = pBitmap->GetWidth();
		int nHeight = pBitmap->GetHeight();
		Bitmap *pSaveBitmap = 0;
		if (bReSize)
		{
			if (nWidth > nHeight && nWidth > 1024)
			{
				nHeight = nHeight * 1024 / nWidth;
				nWidth = 1024;
			}
			else if (nWidth < nHeight && nHeight > 1024)
			{
				nWidth = nWidth * 1024 / nHeight;
				nHeight = 1024;
			}
			pSaveBitmap = new Bitmap(nWidth, nHeight, PixelFormat32bppARGB);
			if (!IsAlphaPixelFormat(pBitmap->GetPixelFormat()))
			{
				Gdiplus::BitmapData bmpData;
				pSaveBitmap->LockBits(&Rect(0, 0, nWidth, nHeight), ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &bmpData);
				int srcStride = bmpData.Stride;
				UINT* srcPixel = (UINT*)bmpData.Scan0;
				byte* pSrc = (byte*)srcPixel;
				for (int y = 0; y<nHeight; y++)
				{
					for (int x = 0; x<nWidth; x++)
					{
						pSrc[4 * x + 0 + y*srcStride] = 0; //R
						pSrc[4 * x + 1 + y*srcStride] = 0; //G
						pSrc[4 * x + 2 + y*srcStride] = 0; //B
						pSrc[4 * x + 3 + y*srcStride] = 255; //A
					}
				}
				pSaveBitmap->UnlockBits(&bmpData);
			}
			Graphics g(pSaveBitmap);
			g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
			g.DrawImage(pBitmap, 0.0f, 0.0f, (float)nWidth, (float)nHeight);
		}
		else
		{
			pSaveBitmap = pBitmap->Clone(0, 0, nWidth, nHeight, PixelFormat32bppARGB);
		}
		delete pBitmap;
		{
			Gdiplus::BitmapData bmpData;
			pSaveBitmap->LockBits(&Rect(0, 0, nWidth, nHeight), ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &bmpData);
			int srcStride = bmpData.Stride;
			UINT* srcPixel = (UINT*)bmpData.Scan0;
			byte* pSrc = (byte*)srcPixel;
			BOOL bAlphaExists = FALSE;
			for (int y = 0; y<nHeight; y++)
			{
				for (int x = 0; x<nWidth; x++)
				{
					if (pSrc[4 * x + 3 + y*srcStride] < 255)
					{
						bAlphaExists = TRUE;
						goto ESCAPE;
					}
				}
			}
		ESCAPE:
			if (!bAlphaExists)
			{
				switch (nPosition)
				{
				case 0:
					pSrc[4 * 0 + 3 + 0 * srcStride] = 254; //左上
					break;
				case 1:
					pSrc[4 * 0 + 3 + (nHeight - 1)*srcStride] = 254; //左下
					break;
				case 2:
					pSrc[4 * (nWidth - 1) + 3 + 0 * srcStride] = 254; //右上
					break;
				case 3:
					pSrc[4 * (nWidth - 1) + 3 + (nHeight - 1) * srcStride] = 254; //右下
					break;
				}
			}
			pSaveBitmap->UnlockBits(&bmpData);
		}
		WCHAR szFilePath[MAX_PATH];
		lstrcpyW(szFilePath, lpszFilePath);
		PathRenameExtensionW(szFilePath, L".png");
		SaveBitmapAs(L"image/png", szFilePath, pSaveBitmap);
		delete pSaveBitmap;
	}
	return TRUE;
}

void PrintUsage(HWND hWnd)
{
	LPCTSTR lpszUsage = TEXT("使用法: SetPixelAlpha [-r] [-p 半透明化するピクセル位置] ターゲットファイル名\r\n\r\n")
		TEXT("オプション:\r\n")
		TEXT("-r\t最大でも1024x1024となるようにリサイズする\r\n")
		TEXT("-p 数値\t半透明化するピクセルを数値で指定してください。(0:左上,1:左下,2:右上,3:右下)");
	MessageBox(hWnd, lpszUsage, TEXT("使用法"), 0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hCheck1;
	static HWND hRadio1;
	static HWND hRadio2;
	static HWND hRadio3;
	static HWND hRadio4;
	switch (msg)
	{
	case WM_CREATE:
		{
			int nArgs;
			LPWSTR *szArglist = CommandLineToArgvW(GetCommandLine(), &nArgs);
			BOOL bExit = FALSE;
			if (szArglist && nArgs > 1)
			{
				int nPosition = 0;
				BOOL bReSize = FALSE;
				WCHAR szFilePath[MAX_PATH] = { 0 };
				for (int i = 0; i < nArgs; ++i)
				{
					if (lstrcmpiW(szArglist[i], L"/r") == 0 || lstrcmpiW(szArglist[i], L"-r") == 0)
					{
						bReSize = TRUE;
					}
					else if (lstrcmpiW(szArglist[i], L"/p") == 0 || lstrcmpiW(szArglist[i], L"-p") == 0)
					{
						if (i + 1 >= nArgs) break;
						++i;
						nPosition = _wtoi(szArglist[i]);
					}
					else if (i > 0 && PathFileExistsW(szArglist[i]))
					{
						lstrcpyW(szFilePath, szArglist[i]);
					}
				}
				if (szFilePath[0])
				{
					SetPixelAlpha(szFilePath, nPosition, bReSize);
				}
				else
				{
					PrintUsage(hWnd);
				}
				bExit = TRUE;
			}
			LocalFree(szArglist);
			if (bExit)
			{
				return -1;
			}
		}
		hCheck1 = CreateWindow(TEXT("BUTTON"), TEXT("最大 1024 x 1024 にリサイズ"), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hRadio1 = CreateWindow(TEXT("BUTTON"), TEXT("左上"), WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hRadio2 = CreateWindow(TEXT("BUTTON"), TEXT("左下"), WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hRadio3 = CreateWindow(TEXT("BUTTON"), TEXT("右上"), WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hRadio4 = CreateWindow(TEXT("BUTTON"), TEXT("右下"), WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hCheck1, BM_SETCHECK, 1, 0);
		SendMessage(hRadio1, BM_SETCHECK, 1, 0);
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_DROPFILES:
		{
			TCHAR szFilePath[MAX_PATH];
			const UINT nFiles = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
			BOOL bReSize = (BOOL)SendMessage(hCheck1, BM_GETCHECK, 0, 0);
			int nPosition = 0;
			if (SendMessage(hRadio1, BM_GETCHECK, 0, 0))
			{
				nPosition = 0;
			}
			else if (SendMessage(hRadio2, BM_GETCHECK, 0, 0))
			{
				nPosition = 1;
			}
			else if (SendMessage(hRadio3, BM_GETCHECK, 0, 0))
			{
				nPosition = 2;
			}
			else if (SendMessage(hRadio4, BM_GETCHECK, 0, 0))
			{
				nPosition = 3;
			}
			for (UINT i = 0; i < nFiles; ++i)
			{
				DragQueryFile((HDROP)wParam, i, szFilePath, _countof(szFilePath));
				SetPixelAlpha(szFilePath, nPosition, bReSize);
			}
			DragFinish((HDROP)wParam);
		}
		break;
	case WM_SIZE:
		MoveWindow(hCheck1, 10, 10, 256, 32, TRUE);
		MoveWindow(hRadio1, 10, 50, 256, 32, TRUE);
		MoveWindow(hRadio2, 10, 90, 256, 32, TRUE);
		MoveWindow(hRadio3, 10, 130, 256, 32, TRUE);
		MoveWindow(hRadio4, 10, 170, 256, 32, TRUE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("ドラッグ & ドロップされた画像ファイルをリサイズし特定のピクセルを半透明化する"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}
