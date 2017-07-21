// WaveformTest.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "WaveformWindow.h"


#define MAX_LOADSTRING 100

#include "Playback.h"
#include "PlainView.h"
#include "Utility.h"

#include "mmsystem.h"
#include "Commdlg.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define CreateFileDialog(hwnd, ofn, filepath, filters, flags) \
	OPENFILENAME ofn; \
	ZeroMemory(&ofn, sizeof(OPENFILENAME)); \
	ofn.lStructSize = sizeof(OPENFILENAME); \
	ofn.hwndOwner = hwnd; \
	ofn.lpstrFile = filepath; \
	ofn.nMaxFile = MAX_PATH; \
	ofn.lpstrFilter = filters; \
	ofn.lpstrDefExt = _T("wav"); \
	ofn.Flags = flags

#define CreateOpenFileDialog(hwnd, ofn, filepath, filters)	\
	CreateFileDialog(hwnd, ofn, filepath, filters, \
	OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST)

#define CreateSaveFileDialog(hwnd, ofn, filepath, filters)	\
	CreateFileDialog(hwnd, ofn, filepath, filters, \
	OFN_OVERWRITEPROMPT)

#define RECT_WIDTH(rc) (rc.right - rc.left)
#define RECT_HEIGHT(rc) (rc.bottom - rc.top)

struct NewWave newWave;

Waveform* wave = NULL;
DrawContext dc;
static TCHAR waveName[64];

GenWaveParams genWave;
AdjustVolume vol;
FFTInfo fft;

INT_PTR CALLBACK GenerateWaveDialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewWaveDialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK VolumeDialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam);

static HDC memdc = NULL, wavedc = NULL;
static HBITMAP membmp = NULL, wavebmp = NULL;
static HFONT normalFont = NULL;
HBRUSH backgroundBrush = NULL;
static SYSTEMTIME now;

struct OperationParameters op;

WaveViewport* waveVp = NULL;
IndicatorViewport* indicVp = NULL;
FFTViewport* fftVp = NULL;
PartialWaveViewport* partialWaveVp = NULL;

WindowAgent windowAgent;

static bool CloseFile(HWND hwnd);

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WAVEFORMBOX, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WAVEFORMBOX));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WAVEFORMTEST));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WAVEFORMTEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_HSCROLL,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	//SetWindowPos(hWnd, NULL, 100, 100, 700, 300, SWP_NOZORDER);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

void UpdateScrollBar(HWND hwnd)
{
	SCROLLINFO scr;
	scr.cbSize = sizeof(SCROLLINFO);

	scr.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_HORZ, &scr);

	scr.nMin = 0;
	scr.nMax = dc.totalPixelWidth + 100;
	scr.nPage = RECT_WIDTH(dc.viewBounds);
	scr.fMask = SIF_RANGE | SIF_PAGE;

	if (wave == NULL)
	{
		scr.nPos = 0;
		scr.fMask |= SIF_POS;
	}
	else if (scr.nPos > scr.nMax - (int)scr.nPage)
	{
		scr.nPos = scr.nMax - (int)scr.nPage;
		scr.fMask |= SIF_POS;
	}

	SetScrollInfo(hwnd, SB_HORZ, &scr, TRUE);
	Invalidate(hwnd, true);
}

void UpdateWaveform(HWND hwnd)
{
	dc.wf = wave;
	fft.wf = wave;
	fft.maxFreq = 0;
	RenewDrawInfo(dc);
	UpdateScrollBar(hwnd);
}

void Initialize(HWND hwnd)
{
	_tcscpy_s(waveName, 64, _T("Untitled"));

	RECT rect;
	GetClientRect(hwnd, &rect);

	HDC hdc = GetDC(hwnd);

	memdc = CreateCompatibleDC(dc.hdc);
	membmp = CreateCompatibleBitmap(dc.hdc, RECT_WIDTH(rect), RECT_HEIGHT(rect));
	SelectObject(memdc, membmp);

	dc.hdc = memdc;

	backgroundBrush = CreateSolidBrush(RGB(40, 40, 40));

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(LOGFONT));
	//_tcscpy_s(lf.lfFaceName, _T("Consolas"));
	lf.lfHeight = 14;
	//lf.lfWidth = 6;
	normalFont = CreateFontIndirect(&lf);

	newWave.format.channels = 1;
	newWave.format.samplesPerSecond = 8000;
	newWave.format.bitsPerSample = 8;
	newWave.format.blockAlign = 1;
	newWave.seconds = 3;

	genWave.freq = 100;
	genWave.volume = 50;
	genWave.phase = 0;
	genWave.flags = GenerateFlag::Set;

	vol.volumePercent = 100;

	dc.scrollToCursor = true;
	dc.fftAfterSelect = true;
	dc.fftShowCursor = true;

	wave = CreateWave(1, 8000, 8);
	//GenerateWave(*wave, genWave);
	UpdateWaveform(hwnd);

	for (int i = 0; i < MAX_SUPPORT_CHANNELS; i++)
	{
		dc.channelColors[i] = RGB(120, 120, 120);
	}

	dc.selectionPen = CreatePen(PS_SOLID, 1, RGB(100, 200, 220));
	dc.selectionBrush = CreateSolidBrush(RGB(60, 60, 60));
	dc.cursorPen = CreatePen(PS_SOLID, 1, RGB(200, 40, 40));

	// initialize sprites
	windowAgent.hwnd = hwnd;
	
	windowAgent.AddSprite(reinterpret_cast<Sprite*>(indicVp = new IndicatorViewport(dc)));
	windowAgent.AddSprite(reinterpret_cast<Sprite*>(waveVp = new WaveViewport(dc, hdc)));
	windowAgent.AddSprite(reinterpret_cast<Sprite*>(fftVp = new FFTViewport(dc)));
	windowAgent.AddSprite(reinterpret_cast<Sprite*>(partialWaveVp = new PartialWaveViewport(dc)));
}

void Uninitialize(HWND hwnd)
{
	if (dc.selectionPen != NULL) DeleteObject(dc.selectionPen);
	if (dc.selectionBrush != NULL) DeleteObject(dc.selectionBrush);
	if (dc.cursorPen != NULL) DeleteObject(dc.cursorPen);

	if (membmp != NULL) DeleteObject(membmp);
	if (memdc != NULL) DeleteDC(memdc);

	windowAgent.DestroyAllSprites();

	ReleaseDC(hwnd, dc.hdc);

	if (backgroundBrush != NULL) DeleteObject(backgroundBrush);
	if (normalFont != NULL) DeleteObject(normalFont);
}

void NewFile(HWND hwnd)
{
	if (DialogBox(hInst, MAKEINTRESOURCE(IDD_NEW_WAVE), hwnd, NewWaveDialogProc) != IDOK)
	{
		return;
	}

	if (CloseFile(hwnd))
	{
		wave = CreateWave(newWave.format.channels, newWave.format.samplesPerSecond,
			newWave.format.bitsPerSample, newWave.seconds);

		_tcsncpy_s(waveName, _T("Untitled"), 64);

		UpdateWaveform(hwnd);
	}
}

void OpenFile(HWND hwnd)
{
	TCHAR filepath[MAX_PATH];
	filepath[0] = '\0';

	CreateOpenFileDialog(hwnd, ofn, filepath, _T("Waveform file(*.wav)\0*.wav\0All files(*.*)\0*.*\0"));

	if (GetOpenFileName(&ofn) == TRUE)
	{
		if (!CloseFile(hwnd))
		{
			return;
		}

		wave = WaveOpen(ofn.lpstrFile);

		if (wave == NULL)
		{
			MessageBox(hwnd, _T("Cannot open specified file."), NULL, MB_OK);
			return;
		}

		GetFileNameWithoutExtension(ofn.lpstrFile, waveName, 64);

		UpdateWaveform(hwnd);
	}
}

void SaveFile(HWND hwnd)
{
	if (wave == NULL || wave->buffer == NULL || wave->bufferLength <= 0)
	{
		MessageBox(hwnd, _T("Waveform content is empty."), _T("Waveform Box"), MB_OK);
		return;
	}

	TCHAR filepath[MAX_PATH];
	filepath[0] = '\0';

	CreateSaveFileDialog(hwnd, ofn, filepath, _T("Waveform file(*.wav)\0*.wav\0All files(*.*)\0*.*\0"));

	if (GetSaveFileName(&ofn) == TRUE)
	{
		SaveWave(*wave, filepath);
	}
}

bool CloseFile(HWND hwnd)
{
	if (IsPlaying())
	{
		PlaybackStop(true);
	}

	if (wave != NULL)
	{
		DestroyWave(wave);
		wave = NULL;
	}

	UpdateWaveform(hwnd);

	return true;
}

void GenerateWaveDialog(HWND hwnd)
{
	if (wave != NULL)
	{
		if (DialogBox(hInst, MAKEINTRESOURCE(IDD_GENERATE_WAVE), hwnd, GenerateWaveDialogProc) == IDOK)
		{
			GenerateWave(*wave, genWave);
			Invalidate(hwnd, true);
		}
	}
}

void AdjustVolumeDialog(HWND hwnd)
{
	if (wave != NULL)
	{
		if (DialogBox(hInst, MAKEINTRESOURCE(IDD_VOLUME), hwnd, VolumeDialogProc) == IDOK)
		{
			WaveVolume(*wave, vol, 
				vol.applyToSelection ? dc.selection.start : -1,
				vol.applyToSelection ? dc.selection.end : -1);

			Invalidate(hwnd, true);
		}
	}
}

void Render(HWND hwnd, HDC hdc, RECT& rect)
{
	int clientWidth = RECT_WIDTH(rect);
	int clientHeight = RECT_HEIGHT(rect);

	HGDIOBJ oldFont = SelectObject(hdc, normalFont);

	FillRect(hdc, &rect, backgroundBrush);

	SetTextColor(hdc, RGB(160, 160, 160));
	SetBkMode(hdc, TRANSPARENT);

	windowAgent.Render();

	if (wave != NULL)
	{
		TCHAR msg[128];

		_sntprintf_s(msg, 128, _T("%s - %d Hz, %d Bits, %.2f s, %d kbps"),
			waveName,
			wave->format.samplesPerSecond, wave->format.bitsPerSample,
			wave->secondLength, wave->format.bitsPerSample * wave->format.samplesPerSecond / 1000);
		TextOut(hdc, 10, 5, msg, _tcsnlen(msg, 128));

		_sntprintf_s(msg, 128, _T("%.2f s"), dc.cursorInSecond);
		TextOut(hdc, rect.right - 40, 5, msg, _tcsnlen(msg, 128));
	}

	SelectObject(hdc, oldFont);
}

void UpdatePlaybackCursor(HWND hwnd, double cursorInSecond)
{
	dc.cursorInSecond = cursorInSecond;
	
	bool repainted = false;

	if (dc.scrollToCursor)
	{
		if (dc.cursorInSecond > dc.viewStartSecond + dc.viewDisplaySeconds)
		{
			SendMessage(hwnd, WM_HSCROLL, SB_PAGEDOWN, NULL);
			Invalidate(hwnd, true);
			repainted = true;
		}
	}
		
	if (!repainted)
	{
		Invalidate(hwnd, false);
	}
}

void ProcessHorScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	
	int nScrollCode = (int)LOWORD(wParam);
	bool processed = false;

	SCROLLINFO si;
	ZeroMemory(&si, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

	GetScrollInfo(hwnd, SB_HORZ, &si);
	int pos = si.nTrackPos;

	int newPos = si.nPos;

	switch (nScrollCode)
	{
	case SB_PAGEUP:
		GetClientRect(hwnd, &rect);

		newPos = si.nPos - (rect.right - rect.left);
		processed = true;
		break;

	case SB_PAGEDOWN:
		RECT rect;
		GetClientRect(hwnd, &rect);

		newPos = si.nPos + (rect.right - rect.left);
		processed = true;
		break;

	case SB_LINEUP:
		newPos = si.nPos - 10;
		processed = true;
		break;

	case SB_LINEDOWN:
		newPos = si.nPos + 20;
		processed = true;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newPos = pos + si.nMin;
		processed = true;
		break;
	}

	if (processed)
	{
		if (newPos > (int)(si.nMax - (int)si.nPage)) newPos = (int)(si.nMax - (int)si.nPage);
		if (newPos < si.nMin) newPos = si.nMin;

		if (newPos != si.nPos)
		{
			si.fMask = SIF_POS;
			si.nPos = newPos;
			SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

			dc.viewStartSecond = (double)newPos / dc.totalPixelWidth * wave->secondLength;
			Invalidate(hwnd, true);
		}
	}
}

static void PlayWave(HWND hwnd, double startSeconds)
{
	Playback(hwnd, *wave, dc.selection.start);

	dc.playStartInSecond = startSeconds;
	GetSystemTime(&now);
}

static bool ToggleMenuItemCheck(HWND hwnd, UINT menuItem)
{
	HMENU menu = GetMenu(hwnd);

	MENUITEMINFO mi;
	ZeroMemory(&mi, sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_STATE;
	GetMenuItemInfo(menu, menuItem, FALSE, &mi);

	if ((mi.fState & MFS_CHECKED) == MFS_CHECKED)
	{
		mi.fState &= ~MFS_CHECKED;
	}
	else
	{
		mi.fState |= MFS_CHECKED;
	}

	SetMenuItemInfo(menu, menuItem, FALSE, &mi);
	return ((mi.fState & MFS_CHECKED) == MFS_CHECKED);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_FILE_NEW:
			NewFile(hWnd);
			break;

		case IDM_FILE_OPEN:
			OpenFile(hWnd);
			break;

		case IDM_FILE_SAVE:
			SaveFile(hWnd);
			break;

		case IDM_EDIT_SELECTALL:
			if (wave != NULL)
			{
				dc.selection.start = 0;
				dc.selection.end = wave->secondLength;
			}
			windowAgent.RefreshUI();
			break;

		case IDM_VIEW_SHOWCURSORDETAILS:
			partialWaveVp->SetVisible(ToggleMenuItemCheck(hWnd, IDM_VIEW_SHOWCURSORDETAILS));
			windowAgent.RefreshUI();
			break;

		case IDM_VIEW_SHOWFFTPANEL:
			{
				fftVp->SetVisible(ToggleMenuItemCheck(hWnd, IDM_VIEW_SHOWFFTPANEL));
				
				RECT rect;
				GetClientRect(hWnd, &rect);
				windowAgent.Resize(Size((float)RECT_WIDTH(rect), (float)RECT_HEIGHT(rect)));

				Invalidate(hWnd, true);
			}
			break;

		case IDM_WAVE_GENERATE:
			GenerateWaveDialog(hWnd);
			break;

		case IDM_WAVE_VOLUME:
			AdjustVolumeDialog(hWnd);
			break;

		case IDM_FFT_PERFORMANALYSIS:
			fftVp->DoAnalysis();
			break;

		case IDM_FFT_AUTOANALYSISAFTERSELECT:
			dc.fftAfterSelect = ToggleMenuItemCheck(hWnd, IDM_FFT_AUTOANALYSISAFTERSELECT);
			break;

		case IDM_FFT_SHOWHOVERCURSOR:
			dc.fftShowCursor = ToggleMenuItemCheck(hWnd, IDM_FFT_SHOWHOVERCURSOR);
			break;

		case IDM_PLAY_PLAY:
			PlayWave(hWnd, dc.selection.start);
			break;

		case IDM_PLAY_PLAYFROMBEGIN:
			PlayWave(hWnd, 0);
			break;

		case IDM_PLAY_STOP:
			PlaybackStop(true);
			break;

		case IDM_PLAY_SCROLLTOCURSOR:
			dc.scrollToCursor = ToggleMenuItemCheck(hWnd, IDM_PLAY_SCROLLTOCURSOR);
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_CREATE:
		Initialize(hWnd);
		break;

	case WM_LBUTTONDOWN:
		{
			op.leftButtonDown = true;
			short x = LOWORD(lParam);
			short y = HIWORD(lParam);

			windowAgent.DispatchMouseDown(MouseButtons::Left, x, y);
		}
		break;

	case WM_MOUSEMOVE:
		{
			MouseButtons buttons = None;
			
			if (op.leftButtonDown)
			{
				buttons |= Left;
			}

			short x = LOWORD(lParam);
			short y = HIWORD(lParam);

			windowAgent.DispatchMouseMove(buttons, Point(x, y));
		}
		break;

	case WM_LBUTTONUP:
		{
			short x = LOWORD(lParam);
			short y = HIWORD(lParam);
			windowAgent.DispatchMouseUp(MouseButtons::Right, Point(x, y));

			//op.lastMoveX = -1;
			op.leftButtonDown = false;

			Invalidate(hWnd, false);
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE:
			if (wave != NULL)
			{
				if (!IsPlaying())
				{
					PlayWave(hWnd, GetKeyState(VK_CONTROL) < 0 ? 0 : dc.selection.start);
				}
				else
				{
					PlaybackStop(true);
				}
			}
		}
		break;

	case WUM_PLAYSTART:
		SetTimer(hWnd, 0, 40, NULL);
		UpdatePlaybackCursor(hWnd, dc.playStartInSecond);
		break;

	case WUM_PLAYING:
		{
			double cursor = (double)wParam;

			UpdatePlaybackCursor(hWnd, cursor);

			if (dc.selection.start < dc.selection.end
				&& cursor > dc.selection.end)
			{
				PlaybackStop(true);
				KillTimer(hWnd, 0);
			}
		}
		break;

	case WUM_PLAYSTOP:
		KillTimer(hWnd, 0);
		break;

	case WM_TIMER:
		{
			int eventId = (int)wParam;

			switch (eventId)
			{
				case 0:
					if (dc.selection.start < dc.selection.end
						&& dc.cursorInSecond > dc.selection.end)
					{
						PlaybackStop(true);
					}
					else
					{
						SYSTEMTIME time;
						GetSystemTime(&time);

						double elapsedSeconds =
							((double)time.wMilliseconds / 1000 + time.wSecond + (double)time.wMinute * 60)
							- ((double)now.wMilliseconds / 1000 + now.wSecond + (double)now.wMinute * 60);
				
						UpdatePlaybackCursor(hWnd, dc.playStartInSecond + elapsedSeconds);
					}
				break;
			}
		}
		break;

	case WM_HSCROLL:
		ProcessHorScroll(hWnd, wParam, lParam);
		break;

	case WM_MOUSEWHEEL:
		if (wave != NULL)
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			delta /= (GetKeyState(VK_SHIFT) < 0) ? 2 : 60;

			int pixelPerSecond = dc.pixelPerSecond + delta * wave->format.bitsPerSample * wave->format.channels;

			UpdatePixelPerSecond(dc, pixelPerSecond);
			UpdateScrollBar(hWnd);
		}
		break;

	case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			RECT rect;
			GetClientRect(hWnd, &rect);

			Render(hWnd, memdc, rect);
			BitBlt(hdc, rect.left, rect.top, RECT_WIDTH(rect), RECT_HEIGHT(rect), memdc, 0, 0, SRCCOPY);

			EndPaint(hWnd, &ps);
		}
		break;

	case WM_ERASEBKGND:
		// do nothing
		break;

	case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			SetRect(&dc.viewBounds, 20, 20, width - 20, height - 20);

			dc.viewDisplaySeconds = (double)width / dc.pixelPerSecond;

			hdc = GetDC(hWnd);

			if (memdc != NULL)
			{
				if (membmp != NULL)
				{
					DeleteObject(membmp);
				}

				membmp = CreateCompatibleBitmap(hdc, width, height);
				SelectObject(memdc, membmp);
			}

			windowAgent.Resize(Size((float)width, (float)height));

			ReleaseDC(hWnd, hdc);

			UpdateScrollBar(hWnd);
		}
		break;

	case WM_DESTROY:
		CloseFile(hWnd);
		PlaybackClose();
		Uninitialize(hWnd);
		
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for dialog boxes

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
