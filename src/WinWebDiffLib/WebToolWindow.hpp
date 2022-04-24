#include <WindowsX.h>
#include <CommCtrl.h>
#include <tchar.h>
#include "WebDiffWindow.hpp"
#include "WinWebDiffLib.h"
#include "resource.h"

#pragma once

class CWebToolWindow : public IWebToolWindow
{
public:
	CWebToolWindow() :
		  m_hWnd(NULL)
		, m_hInstance(NULL)
		, m_pWebDiffWindow(NULL)
		, m_bInSync(false)
	{
	}

	~CWebToolWindow()
	{
	}

	bool Create(HINSTANCE hInstance, HWND hWndParent)
	{
		m_hInstance = hInstance;
		m_hWnd = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOGBAR), hWndParent, DlgProc, reinterpret_cast<LPARAM>(this));
		return m_hWnd ? true : false;
	}

	bool Destroy()
	{
		BOOL bSucceeded = DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		return !!bSucceeded;
	}

	HWND GetHWND() const override
	{
		return m_hWnd;
	}

	void Sync() override
	{
		if (!m_pWebDiffWindow)
			return;

		m_bInSync = true;

		TCHAR buf[256];
		wsprintf(buf, _T("(%d)"), m_pWebDiffWindow->GetDiffBlockSize());
		SetDlgItemText(m_hWnd, IDC_DIFF_BLOCKSIZE_STATIC, buf);
		wsprintf(buf, _T("(%d)"), static_cast<int>(m_pWebDiffWindow->GetDiffColorAlpha() * 100));
		SetDlgItemText(m_hWnd, IDC_DIFF_BLOCKALPHA_STATIC, buf);
		wsprintf(buf, _T("(%d)"), static_cast<int>(m_pWebDiffWindow->GetColorDistanceThreshold()));
		SetDlgItemText(m_hWnd, IDC_DIFF_CDTHRESHOLD_STATIC, buf);
		wsprintf(buf, _T("(%d)"), static_cast<int>(m_pWebDiffWindow->GetOverlayAlpha() * 100));
		SetDlgItemText(m_hWnd, IDC_OVERLAY_ALPHA_STATIC, buf);
		wsprintf(buf, _T("(%d%%)"), static_cast<int>(100 * m_pWebDiffWindow->GetZoom()));
		SetDlgItemText(m_hWnd, IDC_ZOOM_STATIC, buf);

		SendDlgItemMessage(m_hWnd, IDC_DIFF_HIGHLIGHT, BM_SETCHECK, m_pWebDiffWindow->GetShowDifferences() ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(m_hWnd, IDC_DIFF_BLOCKSIZE_SLIDER, TBM_SETPOS, TRUE, m_pWebDiffWindow->GetDiffBlockSize());
		SendDlgItemMessage(m_hWnd, IDC_DIFF_BLOCKALPHA_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_pWebDiffWindow->GetDiffColorAlpha() * 100));
		SendDlgItemMessage(m_hWnd, IDC_DIFF_CDTHRESHOLD_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_pWebDiffWindow->GetColorDistanceThreshold()));
		SendDlgItemMessage(m_hWnd, IDC_OVERLAY_ALPHA_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_pWebDiffWindow->GetOverlayAlpha() * 100));
		SendDlgItemMessage(m_hWnd, IDC_ZOOM_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_pWebDiffWindow->GetZoom() * 8 - 8));
		SendDlgItemMessage(m_hWnd, IDC_DIFF_INSERTION_DELETION_DETECTION_MODE, CB_SETCURSEL, m_pWebDiffWindow->GetInsertionDeletionDetectionMode(), 0);
		SendDlgItemMessage(m_hWnd, IDC_OVERLAY_MODE, CB_SETCURSEL, m_pWebDiffWindow->GetOverlayMode(), 0);
		SendDlgItemMessage(m_hWnd, IDC_PAGE_SPIN, UDM_SETRANGE, 0, MAKELONG(1, m_pWebDiffWindow->GetMaxPageCount()));
		SendDlgItemMessage(m_hWnd, IDC_PAGE_SPIN, UDM_SETPOS, 0, MAKELONG(m_pWebDiffWindow->GetCurrentMaxPage() + 1, 0));

		int w = static_cast<CWebDiffWindow *>(m_pWebDiffWindow)->GetDiffImageWidth();
		int h = static_cast<CWebDiffWindow *>(m_pWebDiffWindow)->GetDiffImageHeight();

		RECT rc;
		GetClientRect(m_hWnd, &rc);
		int cx = rc.right - rc.left;
		int cy = rc.bottom - rc.top;

		RECT rcTmp;
		HWND hwndDiffMap = GetDlgItem(m_hWnd, IDC_DIFFMAP);
		GetWindowRect(hwndDiffMap, &rcTmp);
		POINT pt = { rcTmp.left, rcTmp.top };
		ScreenToClient(m_hWnd, &pt);
		int mw = 0;
		int mh = 0;
		if (w > 0 && h > 0)
		{
			mh = h * (cx - 8) / w;
			if (mh + pt.y > cy - 8)
				mh = cy - 8 - pt.y;
			mw = mh * w / h;
		}
		RECT rcDiffMap = { (cx - mw) / 2, pt.y, (cx + mw) / 2, pt.y + mh };
		SetWindowPos(hwndDiffMap, NULL, rcDiffMap.left, rcDiffMap.top, 
			rcDiffMap.right - rcDiffMap.left, rcDiffMap.bottom - rcDiffMap.top, SWP_NOZORDER);

		InvalidateRect(GetDlgItem(m_hWnd, IDC_DIFFMAP), NULL, TRUE);

		m_bInSync = false;
	}

	void SetWebDiffWindow(IWebDiffWindow *pWebDiffWindow)
	{
		m_pWebDiffWindow = pWebDiffWindow;
		m_pWebDiffWindow->AddEventListener(OnEvent, this);
	}

	void Translate(TranslateCallback translateCallback) override
	{
		auto translateString = [&](int id)
		{
			wchar_t org[256];
			wchar_t translated[256];
			::LoadString(m_hInstance, id, org, sizeof(org)/sizeof(org[0]));
			if (translateCallback)
				translateCallback(id, org, sizeof(translated)/sizeof(translated[0]), translated);
			else
				wcscpy_s(translated, org);
			return std::wstring(translated);
		};

		struct StringIdControlId
		{
			int stringId;
			int controlId;
		};
		static const StringIdControlId stringIdControlId[] = {
			{IDS_DIFF_GROUP, IDC_DIFF_GROUP},
			{IDS_DIFF_HIGHLIGHT, IDC_DIFF_HIGHLIGHT},
			{IDS_DIFF_BLINK, IDC_DIFF_BLINK},
			{IDS_DIFF_BLOCKSIZE, IDC_DIFF_BLOCKSIZE},
			{IDS_DIFF_BLOCKALPHA, IDC_DIFF_BLOCKALPHA},
			{IDS_DIFF_CDTHRESHOLD, IDC_DIFF_CDTHRESHOLD},
			{IDS_DIFF_INSERTION_DELETION_DETECTION, IDC_DIFF_INSERTION_DELETION_DETECTION},
			{IDS_OVERLAY_GROUP, IDC_OVERLAY_GROUP},
			{IDS_OVERLAY_ALPHA, IDC_OVERLAY_ALPHA},
			{IDS_ZOOM, IDC_ZOOM},
			{IDS_PAGE, IDC_PAGE},
		};
		for (auto& e: stringIdControlId)
			::SetDlgItemText(m_hWnd, e.controlId, translateString(e.stringId).c_str());

		// translate ComboBox list
		static const StringIdControlId stringIdControlId2[] = {
			{IDS_DIFF_INSERTION_DELETION_DETECTION_NONE, IDC_DIFF_INSERTION_DELETION_DETECTION_MODE},
			{IDS_OVERLAY_MODE_NONE, IDC_OVERLAY_MODE},
		};
		for (auto& e: stringIdControlId2)
		{
			int cursel = static_cast<int>(::SendDlgItemMessage(m_hWnd, e.controlId, CB_GETCURSEL, 0, 0));
			int count = static_cast<int>(::SendDlgItemMessage(m_hWnd, e.controlId, CB_GETCOUNT, 0, 0));
			::SendDlgItemMessage(m_hWnd, e.controlId, CB_RESETCONTENT, 0, 0);
			for (int i = 0; i < count; ++i)
				::SendDlgItemMessage(m_hWnd, e.controlId, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(translateString(e.stringId + i).c_str()));
			::SendDlgItemMessage(m_hWnd, e.controlId, CB_SETCURSEL, static_cast<WPARAM>(cursel), 0);
		}
	}

private:
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SendDlgItemMessage(hwnd, IDC_DIFF_BLOCKSIZE_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(1, 64));
		SendDlgItemMessage(hwnd, IDC_DIFF_BLOCKALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
		SendDlgItemMessage(hwnd, IDC_OVERLAY_ALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
		SendDlgItemMessage(hwnd, IDC_ZOOM_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(-7, 56));
		SendDlgItemMessage(hwnd, IDC_DIFF_INSERTION_DELETION_DETECTION_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("None")));
		SendDlgItemMessage(hwnd, IDC_DIFF_INSERTION_DELETION_DETECTION_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("Vertical")));
		SendDlgItemMessage(hwnd, IDC_DIFF_INSERTION_DELETION_DETECTION_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("Horizontal")));
		SendDlgItemMessage(hwnd, IDC_OVERLAY_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("None")));
		SendDlgItemMessage(hwnd, IDC_OVERLAY_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("XOR")));
		SendDlgItemMessage(hwnd, IDC_OVERLAY_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("Alpha Blend")));
		SendDlgItemMessage(hwnd, IDC_OVERLAY_MODE, CB_ADDSTRING, 0, (LPARAM)(_T("Alpha Animation")));
		return TRUE;
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		if (m_bInSync)
			return;

		switch (id)
		{
		case IDC_DIFF_HIGHLIGHT:
			if (codeNotify == BN_CLICKED)
				m_pWebDiffWindow->SetShowDifferences(Button_GetCheck(hwndCtl) == BST_CHECKED);
			break;
		case IDC_DIFF_BLINK:
			if (codeNotify == BN_CLICKED)
				m_pWebDiffWindow->SetBlinkDifferences(Button_GetCheck(hwndCtl) == BST_CHECKED);
			break;
		case IDC_DIFF_INSERTION_DELETION_DETECTION_MODE:
			if (codeNotify == CBN_SELCHANGE)
				m_pWebDiffWindow->SetInsertionDeletionDetectionMode(static_cast<IWebDiffWindow::INSERTION_DELETION_DETECTION_MODE>(ComboBox_GetCurSel(hwndCtl)));
			break;
		case IDC_OVERLAY_MODE:
			if (codeNotify == CBN_SELCHANGE)
				m_pWebDiffWindow->SetOverlayMode(static_cast<IWebDiffWindow::OVERLAY_MODE>(ComboBox_GetCurSel(hwndCtl)));
			break;
		case IDC_PAGE_EDIT:
			if (codeNotify == EN_CHANGE)
			{
				int page = static_cast<int>(SendDlgItemMessage(hwnd, IDC_PAGE_SPIN, UDM_GETPOS, 0, 0));
				m_pWebDiffWindow->SetCurrentPageAll(page - 1);
			}
			break;
		case IDC_DIFFMAP:
			if (codeNotify == STN_CLICKED)
			{
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwndCtl, &pt);
				RECT rc;
				GetClientRect(hwndCtl, &rc);
				CWebDiffWindow *pWebDiffWindow = static_cast<CWebDiffWindow *>(m_pWebDiffWindow);
				pWebDiffWindow->ScrollTo(
					pt.x * pWebDiffWindow->GetDiffImageWidth() / rc.right,
					pt.y * pWebDiffWindow->GetDiffImageHeight() / rc.bottom,
					true);
			}
			break;
		}
	}

	void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
	{
		int val = static_cast<int>(SendMessage(hwndCtl, TBM_GETPOS, 0, 0));
		switch (GetDlgCtrlID(hwndCtl))
		{
		case IDC_DIFF_BLOCKALPHA_SLIDER:
			m_pWebDiffWindow->SetDiffColorAlpha(val / 100.0);
			break;
		case IDC_DIFF_BLOCKSIZE_SLIDER:
			m_pWebDiffWindow->SetDiffBlockSize(val);
			break;
		case IDC_DIFF_CDTHRESHOLD_SLIDER:
			m_pWebDiffWindow->SetColorDistanceThreshold(val);
			break;
		case IDC_OVERLAY_ALPHA_SLIDER:
			m_pWebDiffWindow->SetOverlayAlpha(val / 100.0);
			break;
		case IDC_ZOOM_SLIDER:
			m_pWebDiffWindow->SetZoom(1.0 + val * 0.125);
			break;
		}
		Sync();
	}

	void OnSize(HWND hwnd, UINT nType, int cx, int cy)
	{
		static const int nIDs[] = {
			IDC_DIFF_GROUP,
			IDC_DIFF_HIGHLIGHT,
			IDC_DIFF_BLINK,
			IDC_OVERLAY_GROUP,
			IDC_VIEW_GROUP,
			IDC_DIFF_BLOCKSIZE,
			IDC_DIFF_BLOCKALPHA,
			IDC_DIFF_BLOCKALPHA_SLIDER,
			IDC_DIFF_BLOCKSIZE_SLIDER,
			IDC_DIFF_CDTHRESHOLD,
			IDC_DIFF_CDTHRESHOLD_SLIDER,
			IDC_DIFF_INSERTION_DELETION_DETECTION,
			IDC_DIFF_INSERTION_DELETION_DETECTION_MODE,
			IDC_OVERLAY_ALPHA,
			IDC_OVERLAY_ALPHA_SLIDER,
			IDC_OVERLAY_MODE,
			IDC_ZOOM,
			IDC_ZOOM_SLIDER,
			IDC_PAGE,
		};
		HDWP hdwp = BeginDeferWindowPos(static_cast<int>(std::size(nIDs)));
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		for (int id: nIDs)
		{
			RECT rcCtrl;
			HWND hwndCtrl = GetDlgItem(m_hWnd, id);
			GetWindowRect(hwndCtrl, &rcCtrl);
			POINT pt = { rcCtrl.left, rcCtrl.top };
			ScreenToClient(m_hWnd, &pt);
			DeferWindowPos(hdwp, hwndCtrl, nullptr, pt.x, pt.y, rc.right - pt.x * 2, rcCtrl.bottom - rcCtrl.top, SWP_NOMOVE | SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);

		static const int nID2s[] = {
			IDC_DIFF_BLOCKSIZE_STATIC,
			IDC_DIFF_BLOCKALPHA_STATIC,
			IDC_DIFF_CDTHRESHOLD_STATIC,
			IDC_OVERLAY_ALPHA_STATIC,
			IDC_ZOOM_STATIC,
			IDC_PAGE_SPIN,
		};
		hdwp = BeginDeferWindowPos(static_cast<int>(std::size(nID2s)));
		RECT rcSlider;
		GetWindowRect(GetDlgItem(m_hWnd, IDC_DIFF_BLOCKALPHA_SLIDER), &rcSlider);
		for (int id: nID2s)
		{
			RECT rcCtrl;
			HWND hwndCtrl = GetDlgItem(m_hWnd, id);
			GetWindowRect(hwndCtrl, &rcCtrl);
			POINT pt = { rcSlider.right - (rcCtrl.right - rcCtrl.left), rcCtrl.top };
			ScreenToClient(m_hWnd, &pt);
			DeferWindowPos(hdwp, hwndCtrl, nullptr, pt.x, pt.y, rcCtrl.right - rcCtrl.left, rcCtrl.bottom - rcCtrl.top, SWP_NOSIZE | SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);

		static const int nID3s[] = {
			IDC_PAGE_EDIT,
		};
		hdwp = BeginDeferWindowPos(static_cast<int>(std::size(nID3s)));
		RECT rcSpin;
		GetWindowRect(GetDlgItem(m_hWnd, IDC_PAGE_SPIN), &rcSpin);
		for (int id: nID3s)
		{
			RECT rcCtrl;
			HWND hwndCtrl = GetDlgItem(m_hWnd, id);
			GetWindowRect(hwndCtrl, &rcCtrl);
			POINT pt = { rcSpin.left - (rcCtrl.right - rcCtrl.left), rcCtrl.top };
			ScreenToClient(m_hWnd, &pt);
			DeferWindowPos(hdwp, hwndCtrl, nullptr, pt.x, pt.y, rcCtrl.right - rcCtrl.left, rcCtrl.bottom - rcCtrl.top, SWP_NOSIZE | SWP_NOZORDER);
		}
		EndDeferWindowPos(hdwp);

		Sync();
	}

	void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *pDrawItem)
	{
		if (!m_pWebDiffWindow || m_pWebDiffWindow->GetPaneCount() == 0)
			return;
		RECT rc;
		GetClientRect(pDrawItem->hwndItem, &rc);
		Image *pImage = static_cast<CWebDiffWindow *>(m_pWebDiffWindow)->GetDiffMapImage(rc.right - rc.left, rc.bottom - rc.top);
		RGBQUAD bkColor = { 0xff, 0xff, 0xff, 0xff };
		pImage->getFipImage()->drawEx(pDrawItem->hDC, rc, false, &bkColor);
		HWND hwndLeftPane = m_pWebDiffWindow->GetPaneHWND(0);

		SCROLLINFO sih{ sizeof(sih), SIF_POS | SIF_PAGE | SIF_RANGE };
		GetScrollInfo(hwndLeftPane, SB_HORZ, &sih);
		SCROLLINFO siv{ sizeof(siv), SIF_POS | SIF_PAGE | SIF_RANGE };
		GetScrollInfo(hwndLeftPane, SB_VERT, &siv);

		if (static_cast<int>(sih.nPage) < sih.nMax || static_cast<int>(siv.nPage) < siv.nMax)
		{
			RECT rcFrame;
			rcFrame.left = rc.left + (rc.right - rc.left) * sih.nPos / sih.nMax;
			rcFrame.right = rcFrame.left + (rc.right - rc.left) * sih.nPage / sih.nMax;
			rcFrame.top = rc.top + (rc.bottom - rc.top) * siv.nPos / siv.nMax;
			rcFrame.bottom = rcFrame.top + (rc.bottom - rc.top) * siv.nPage / siv.nMax;
			FrameRect(pDrawItem->hDC, &rcFrame, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
		}
	}

	INT_PTR OnWndMsg(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (iMsg)
		{
		case WM_INITDIALOG:
			return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);
		case WM_COMMAND:
			HANDLE_WM_COMMAND(hwnd, wParam, lParam, OnCommand);
			break;
		case WM_HSCROLL:
			HANDLE_WM_HSCROLL(hwnd, wParam, lParam, OnHScroll);
			break;
		case WM_SIZE:
			HANDLE_WM_SIZE(hwnd, wParam, lParam, OnSize);
			break;
		case WM_DRAWITEM:
			HANDLE_WM_DRAWITEM(hwnd, wParam, lParam, OnDrawItem);
			break;
		}
		return 0;
	}

	static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
	{
		if (iMsg == WM_INITDIALOG)
			SetWindowLongPtr(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(reinterpret_cast<CWebToolWindow *>(lParam)));
		CWebToolWindow *pWebWnd = reinterpret_cast<CWebToolWindow *>(GetWindowLongPtr(hwnd, DWLP_USER));
		if (pWebWnd)
			return pWebWnd->OnWndMsg(hwnd, iMsg, wParam, lParam);
		else
			return FALSE;
	}

	static void OnEvent(const IWebDiffWindow::Event& evt)
	{
		switch (evt.eventType)
		{
		case IWebDiffWindow::HSCROLL:
		case IWebDiffWindow::VSCROLL:
		case IWebDiffWindow::SIZE:
		case IWebDiffWindow::MOUSEWHEEL:
		case IWebDiffWindow::REFRESH:
		case IWebDiffWindow::SCROLLTODIFF:
		case IWebDiffWindow::OPEN:
			reinterpret_cast<CWebToolWindow *>(evt.userdata)->Sync();
			break;
		}
	}

	HWND m_hWnd;
	HINSTANCE m_hInstance;
	IWebDiffWindow *m_pWebDiffWindow;
	bool m_bInSync;
};
