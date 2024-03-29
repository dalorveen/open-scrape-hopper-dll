// yeux_v2.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "yeux_v2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

// The one and only application object

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		MessageBoxA(NULL, "ERROR", "MFC initialization failed", MB_OK | MB_ICONERROR);
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
	}

	return nRetCode;
}

CLobbyScraper::CLobbyScraper()
{
}

CLobbyScraper::~CLobbyScraper()
{
	UnloadTableMap();
	print_log_to_file();
}

void CLobbyScraper::UnloadTableMap() {
	if (p_tablemap) {
		// First fixed memory-leak
		log_delete(0);
		delete p_tablemap;
		p_tablemap = NULL;
	}
}

bool CLobbyScraper::Load(CString filename)
{
	UnloadTableMap();

	if (p_tablemap != NULL) {
		// Release p_tablemap first before (re)allocation,
		// otherwise we get a memory_leak
		log_delete(0);
		delete p_tablemap;
		p_tablemap = NULL;
	}

	log_malloc(0);
	p_tablemap = new CTablemap();
	int line = 0;
	int ret = p_tablemap->LoadTablemap(filename, VER_OPENSCRAPE_2, false, &line, false);

	if (ret != SUCCESS)
	{
		MessageBox(NULL, "ERROR", "Hopper C++ : ERROR Loading tablemap", MB_OK | MB_ICONERROR);

		return false;
	}

	return true;
}

void CLobbyScraper::GetRegionPos(const CString name, int& posl, int& post, int& posr, int& posb)
{
	posl = post = posr = posb = -1;
	RMapCI r_it = p_tablemap->r$()->find(name);
	if (r_it == p_tablemap->r$()->end())
		return;
	posl = r_it->second.left;
	post = r_it->second.top;
	posr = r_it->second.right;
	posb = r_it->second.bottom;

}

bool CLobbyScraper::GetSymbol(const CString name, CString& text)
{
	SMapCI it = p_tablemap->s$()->find(name);
	if (it == p_tablemap->s$()->end())
		return false;
	text = it->second.text;
	return true;
}

bool CLobbyScraper::ReadRegion(HWND hwnd, const CString name, char*& result, int xOffset, int yOffset)
{
	RMapCI r_it = p_tablemap->r$()->find(name.GetString());

	if (r_it == p_tablemap->r$()->end())
	{
		return false;
	}

	STablemapRegion region = r_it->second;

	HDC WinDC = GetDC(hwnd);
	HDC CopyDC = CreateCompatibleDC(WinDC);
	HBITMAP subBmp;
	RECT rt =
	{
		region.left + xOffset,
		region.top + yOffset,
		region.right + xOffset,
		region.bottom + yOffset
	};

	//Create a bitmap compatible with the DC
	subBmp = CreateCompatibleBitmap(WinDC,
		rt.right - rt.left + 1,		//width
		rt.bottom - rt.top + 1);	//height

	//Associate the bitmap with the DC
	SelectObject(CopyDC, subBmp);

	//Copy the window DC to the compatible DC
	BitBlt(CopyDC,				//destination
		0, 0,
		rt.right - rt.left + 1,	//width
		rt.bottom - rt.top + 1,	//height
		WinDC,					//source
		rt.left, rt.top,
		SRCCOPY);

	CTransform trans;
	HBITMAP oldbmp = (HBITMAP)SelectObject(CopyDC, subBmp);
	CString text;
	CString separation;
	COLORREF cr_avg;

	int ret = trans.DoTransform(r_it, CopyDC, &text, &separation, &cr_avg);

	strcpy_s(result, 256, text);

	//We don`t need the DCs anymore
	ReleaseDC(hwnd, WinDC);
	ReleaseDC(hwnd, CopyDC);

	DeleteDC(WinDC);
	DeleteDC(CopyDC);
	DeleteObject(subBmp);

	return ret;
}

bool CLobbyScraper::ReadRegion(HDC hdc, const CString name, char*& result, int xOffset, int yOffset)
{
	RMapCI r_it = p_tablemap->r$()->find(name.GetString());

	if (r_it == p_tablemap->r$()->end())
	{
		return false;
	}

	STablemapRegion region = r_it->second;

	HDC WinDC = hdc;
	HDC CopyDC = CreateCompatibleDC(WinDC);
	HBITMAP subBmp;
	RECT rt =
	{
		region.left + xOffset,
		region.top + yOffset,
		region.right + xOffset,
		region.bottom + yOffset
	};

	//Create a bitmap compatible with the DC
	subBmp = CreateCompatibleBitmap(WinDC,
		rt.right - rt.left + 1,		//width
		rt.bottom - rt.top + 1);	//height

	//Associate the bitmap with the DC
	SelectObject(CopyDC, subBmp);

	//Copy the window DC to the compatible DC
	BitBlt(CopyDC,				//destination
		0, 0,
		rt.right - rt.left + 1,	//width
		rt.bottom - rt.top + 1,	//height
		WinDC,					//source
		rt.left, rt.top,
		SRCCOPY);

	CTransform trans;
	HBITMAP oldbmp = (HBITMAP)SelectObject(CopyDC, subBmp);
	CString text;
	CString separation;
	COLORREF cr_avg;

	int ret = trans.DoTransform(r_it, CopyDC, &text, &separation, &cr_avg);

	strcpy_s(result, 256, text);

	DeleteDC(CopyDC);
	DeleteObject(subBmp);

	return ret;
}

CLobbyScraper	scraper;

YEUX_V2_API int OpenTablemap(const wchar_t* filename)
{
	return scraper.Load(CString(filename));
}

YEUX_V2_API int ReadRegionFromHWND(HWND hWnd, const char* name, char*& result, int xOffset, int yOffset)
{
	return scraper.ReadRegion(hWnd, (CString)name, result, xOffset, yOffset);
}

YEUX_V2_API int ReadRegionFromHDC(HDC hdc, const char* name, char*& result, int xOffset, int yOffset)
{
	return scraper.ReadRegion(hdc, (CString)name, result, xOffset, yOffset);
}

YEUX_V2_API void GetRegionPos(const char* name, int& posleft, int& postop, int& posright, int& posbottom)
{
	scraper.GetRegionPos(name, posleft, postop, posright, posbottom);
}

YEUX_V2_API bool GetSymbol(const char* name, const char*& text)
{
	CString result;
	bool ret = scraper.GetSymbol(CString(name), result);
	text = result;
	return ret;
}