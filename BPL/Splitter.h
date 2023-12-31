#ifndef Spliter
#define Spliter

#include <windows.h>
#include "myInit.h"
#define WC_CONTSPLITER	_T("ContSpliterClass")

class ContSpliter {
protected:
	HWND W1, W2;
	WORD dwSplitterPos;
	bool horis, bSplitterMoving = false;
	byte width;
	HCURSOR hCur;
	void* LP_Data =nullptr;
	ContSpliter(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width);
	~ContSpliter();
	static LRESULT CALLBACK static_Procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	virtual void Resize(HWND hwnd, WPARAM wParam, LPARAM lParam);
	virtual void Move(HWND hwnd, WPARAM wParam, LPARAM lParam);
	virtual void UpM(HWND hwnd, WPARAM wParam, LPARAM lParam);
	virtual void DownM(HWND hwnd, WPARAM wParam, LPARAM lParam);
	
	static ContSpliter *New(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width = 4);
	static void Reg_Class(HINSTANCE hInst);
	static ContSpliter* GetObject(HWND Item);
};


class ContSpliterProc : public ContSpliter {
public:
	void Resize(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void UpM(HWND hwnd, WPARAM wParam, LPARAM lParam);
	static ContSpliter *New(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width = 4, byte Defproc = 50);
protected:
	ContSpliterProc(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width);
};

class ContSpliterProc_RT : public ContSpliterProc {
public:
	void Move(HWND hwnd, WPARAM wParam, LPARAM lParam);
	static ContSpliter *New(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width = 4, byte Defproc = 50);
protected:
	ContSpliterProc_RT(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width);
};


/////////////////////////////////////////////////////////ContSpliter///////////////////////////////////////////////////////////////////////////
ContSpliter::~ContSpliter() {
	if (!LP_Data) delete[] LP_Data;
};// ---------------------------------------------------------------

ContSpliter::ContSpliter(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte wid) {
	W1 = Ch1;
	W2 = Ch2;
	horis = hor;
	width = wid;
	auto s = SetWindowLong(Item, GWL_USERDATA, (LONG)this);
	s = GetWindowLong(Item, GWL_USERDATA);
	hCur = LoadCursor(NULL, hor ? IDC_SIZEWE : IDC_SIZENS);
	dwSplitterPos = 0;
	SetWindowLong(Item, GWL_WNDPROC, (LONG)&static_Procedure);
}; // ---------------------------------------------------------------

ContSpliter*  ContSpliter::GetObject(HWND hwnd) {
	auto R = GetWindowLong(hwnd, GWL_USERDATA);
	return re_cast(ContSpliter*, R);
}	// ---------------------------------------------------------------

void ContSpliter::Reg_Class(HINSTANCE hInst) {
	WNDCLASS C;
	if (GetClassInfo(hInst, WC_CONTSPLITER, &C)) return;
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = static_Procedure;
	wcex.hInstance = hInst;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszClassName = WC_CONTSPLITER;
	_ASSERTE(RegisterClassEx(&wcex) != 0);
};	// ---------------------------------------------------------------

ContSpliter* ContSpliter::New(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width) {
	return new ContSpliter(Item, Ch1, Ch2, hor, width);
}; // ---------------------------------------------------------------

void ContSpliter::Move(HWND hwnd, WPARAM wParam, LPARAM lParam) {

	if ((horis ? LOWORD(lParam) : HIWORD(lParam)) > width * 2) {// do not allow above this mark
		SetCursor(hCur);
		if (LP_Data)
		{
			LPRECT rect = re_cast(LPRECT, LP_Data)+ 1;
			if ((horis ? LOWORD(lParam) : HIWORD(lParam)) + width * 2 > (horis ? rect->right : rect->bottom))
				return;
			
			auto hdc = GetDC(hwnd);
			LPRECT focusrect = re_cast(LPRECT, LP_Data);
			if (dwSplitterPos) DrawFocusRect(hdc, focusrect);

			dwSplitterPos = (horis ? LOWORD(lParam) : HIWORD(lParam));

			if (horis) {
				SetRect(focusrect, dwSplitterPos, 0, dwSplitterPos + width, rect->bottom);
			} else {
				SetRect(focusrect, 0, dwSplitterPos, rect->right, dwSplitterPos + width);
			}
			DrawFocusRect(hdc, focusrect);
			ReleaseDC(hwnd, hdc);
		}
		
	}
};//---------------------------------------------------------------

void ContSpliter::Resize(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	if (!dwSplitterPos) dwSplitterPos = (horis ? LOWORD(lParam) : HIWORD(lParam)) / 2;

	if (horis) {
		if ((wParam != 1) && (LOWORD(lParam) < dwSplitterPos))
			dwSplitterPos = LOWORD(lParam) - width * 2;

		/* Adjust the children's size and position */
		HDWP hdwp = BeginDeferWindowPos(2);
		DeferWindowPos(hdwp, W1, nullptr, 0, 0, dwSplitterPos - 1, HIWORD(lParam), 0);
		DeferWindowPos(hdwp, W2, nullptr, dwSplitterPos + width, 0, LOWORD(lParam) - dwSplitterPos, HIWORD(lParam), 0);
		if (hdwp) EndDeferWindowPos(hdwp);
	}
	else {
		if ((wParam != 1) && (HIWORD(lParam) < dwSplitterPos))
			dwSplitterPos = HIWORD(lParam) - width * 2;

		/* Adjust the children's size and position */
		HDWP hdwp = BeginDeferWindowPos(2);
		DeferWindowPos(hdwp, W1, nullptr, 0, 0, LOWORD(lParam), dwSplitterPos - 1, 0);
		DeferWindowPos(hdwp, W2, nullptr, 0, dwSplitterPos + width, LOWORD(lParam), HIWORD(lParam) - dwSplitterPos - width, 0);
		if (hdwp) EndDeferWindowPos(hdwp);

	}
};//---------------------------------------------------------------

void ContSpliter::DownM(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	if (LP_Data) delete[] LP_Data;
	LP_Data = new RECT[2];
	GetClientRect(hwnd, re_cast(LPRECT, LP_Data)+1);
	
	bSplitterMoving = TRUE;
	dwSplitterPos = 0;
	SetCapture(hwnd);
};//---------------------------------------------------------------

void ContSpliter::UpM(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	auto hdc = GetDC(hwnd);
	LPRECT focusrect = re_cast(LPRECT, LP_Data), rect = re_cast(LPRECT, LP_Data)+ 1;
	DrawFocusRect(hdc, focusrect);
	ReleaseDC(hwnd, hdc);
	
	ReleaseCapture();
	bSplitterMoving = FALSE;
	Resize(hwnd, 10, MAKELPARAM(rect->right, rect->bottom)); 

	delete[] LP_Data;
	LP_Data = nullptr;
};//---------------------------------------------------------------

LRESULT CALLBACK ContSpliter::static_Procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_SIZE: if (auto A = ContSpliter::GetObject(hwnd)) { A->Resize(hwnd, wParam, lParam); }
					  break;
		case WM_MOUSEMOVE:
		reinterpret_cast<ContSpliter*>(GetWindowLong(hwnd, GWL_USERDATA))->Move(hwnd, wParam, lParam);
		break;
		case WM_LBUTTONDOWN: ContSpliter::GetObject(hwnd)->DownM(hwnd, wParam, lParam);
			break;
		case WM_LBUTTONUP: ContSpliter::GetObject(hwnd)->UpM(hwnd, wParam, lParam);
			break;
		case WM_DESTROY: delete ContSpliter::GetObject(hwnd);
			break;
		case WM_COMMAND: SendMessage(GetParent(hwnd), WM_COMMAND, wParam, lParam);
			break;
		default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}//---------------------------------------------------------------

/////////////////////////////////////////////////////////ContSpliterProc///////////////////////////////////////////////////////////////////////////

ContSpliterProc::ContSpliterProc(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width) : ContSpliter(Item,  Ch1,  Ch2,  hor,  width)
{};//---------------------------------------------------------------

void ContSpliterProc::UpM(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	auto y = (WORD)(dwSplitterPos * 100 / (horis ? (re_cast(LPRECT, LP_Data)+1)->right : (re_cast(LPRECT, LP_Data)+1)->bottom));
	ContSpliter::UpM (hwnd, wParam, lParam);
	dwSplitterPos = y;
};//---------------------------------------------------------------

ContSpliter* ContSpliterProc::New(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width, byte Defproc) {
	ContSpliterProc* f = new ContSpliterProc(Item, Ch1, Ch2, hor, width);
	f->dwSplitterPos = Defproc;
	return f;
}; // ---------------------------------------------------------------

void ContSpliterProc::Resize(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	if (wParam == 10) ContSpliter::Resize(hwnd, wParam, lParam);
	else {
		auto prev = dwSplitterPos;
		dwSplitterPos = dwSplitterPos * (DWORD)(horis? LOWORD(lParam): HIWORD(lParam)) / 100;
		ContSpliter::Resize(hwnd, wParam, lParam);
		dwSplitterPos = prev;
	}
};//---------------------------------------------------------------


/////////////////////////////////////////////////////////ContSpliterProc_RT///////////////////////////////////////////////////////////////////////////
ContSpliterProc_RT::ContSpliterProc_RT(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width) : ContSpliterProc(Item, Ch1, Ch2, hor, width) {};
//---------------------------------------------------------------

ContSpliter* ContSpliterProc_RT::New(HWND Item, HWND Ch1, HWND Ch2, bool hor, byte width, byte Defproc) {
	ContSpliterProc_RT* f = new ContSpliterProc_RT(Item, Ch1, Ch2, hor, width);
	f->dwSplitterPos = Defproc;
	return f;
}; // ---------------------------------------------------------------

void ContSpliterProc_RT::Move(HWND hwnd, WPARAM wParam, LPARAM lParam) {

	if ((horis ? LOWORD(lParam) : HIWORD(lParam)) > width * 2) {// do not allow above this mark
		SetCursor(hCur);
		if (LP_Data) {
			LPRECT rect = re_cast(LPRECT, LP_Data) + 1;
			if ((horis ? LOWORD(lParam) : HIWORD(lParam)) + width * 2 > (horis ? rect->right : rect->bottom))
				return;

			dwSplitterPos = (horis ? LOWORD(lParam) : HIWORD(lParam));
			ContSpliterProc::Resize(hwnd, 10, MAKELPARAM(rect->right, rect->bottom));
		}

	}
};//---------------------------------------------------------------

#endif