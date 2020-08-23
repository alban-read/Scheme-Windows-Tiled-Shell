////////////////////////////////////////////////////
// Mainfrm.cpp

#include "stdafx.h"
#include "mainfrm.h"
#include "Classes.h"
#include "Files.h"
#include "Output.h"
#include "Text.h"
#include "resource.h"
#include "WebViewer.h"
#include "Engine.h"
#include "Utility.h"

// Definitions for the CMainFrame class
CMainFrame::CMainFrame() : m_isContainerTabsAtTop(FALSE), m_hideSingleTab(TRUE)
{
	SetView(m_view);
	LoadRegistrySettings(_T("Win32++\\SchemeShellDirect2020"));
}

CMainFrame::~CMainFrame()
{

}

void CMainFrame::HideSingleContainerTab(BOOL hideSingle)
{
	m_hideSingleTab = hideSingle;
	std::vector<CDocker*>::const_iterator iter;

	// Set the Tab position for each container
	for (iter = GetAllDockers().begin(); iter < GetAllDockers().end(); ++iter)
	{
		CDockContainer* pContainer = (*iter)->GetContainer();
		if (pContainer && pContainer->IsWindow())
		{
			pContainer->SetHideSingleTab(hideSingle);
		}
	}
}


void CMainFrame::LoadEvalDockers()
{
	const DWORD dw_style = DS_CLIENTEDGE; // The style added to each docker
	const auto width = GetWindowRect().Size().cx;
	auto in = AddDockedChild(new CDockText, DS_DOCKED_LEFT | dw_style, width / 2.0, ID_DOCK_TEXT1);
	in->AddDockedChild(new CDockResponseText, DS_DOCKED_BOTTOM | dw_style, 200, ID_DOCK_TEXT3);
	AddDockedChild(new CDockTranscriptText, DS_DOCKED_LEFT | DS_DOCKED_RIGHT | dw_style, 200, ID_DOCK_TEXT2);
}


void CMainFrame::LoadDefaultDockers()
{
	const DWORD dw_style = DS_CLIENTEDGE; // The style added to each docker
	const auto width = GetWindowRect().Size().cx;
	auto in = AddDockedChild(new CDockText, DS_DOCKED_LEFT | dw_style, width / 2.2, ID_DOCK_TEXT1);
	in->AddDockedChild(new CDockResponseText, DS_DOCKED_BOTTOM | dw_style, 200, ID_DOCK_TEXT3);
	auto im = AddDockedChild(new CDockImage, DS_DOCKED_LEFT | DS_DOCKED_RIGHT | dw_style, width / 3, ID_DOCK_IMAGE1);
	im->AddDockedChild(new CDockTranscriptText, DS_DOCKED_BOTTOM | dw_style, 200, ID_DOCK_TEXT2);
}

void CMainFrame::load_browser_dockers()
{
	const auto width = GetWindowRect().Size().cx;
	const DWORD dw_style = DS_CLIENTEDGE; // The style added to each docker
	AddDockedChild(new CDockWebViewer, DS_DOCKED_LEFT | dw_style, width / 3, ID_DOCK_BROWSER1);
	auto in = AddDockedChild(new CDockText, DS_DOCKED_LEFT | dw_style, width / 3, ID_DOCK_TEXT1);
	in->AddDockedChild(new CDockResponseText, DS_DOCKED_BOTTOM | dw_style, 200, ID_DOCK_TEXT3);
	auto im = AddDockedChild(new CDockImage, DS_DOCKED_LEFT | DS_DOCKED_RIGHT | dw_style, width / 3, ID_DOCK_IMAGE1);
	im->AddDockedChild(new CDockTranscriptText, DS_DOCKED_CONTAINER | dw_style, width / 3, ID_DOCK_TEXT2);
}


void CMainFrame::load_browser_image_dockers()
{
	const auto width = GetWindowRect().Size().cx;
	const DWORD dw_style = DS_CLIENTEDGE; // The style added to each docker
	input=AddDockedChild(new CDockText, DS_DOCKED_LEFT | dw_style, width / 3, ID_DOCK_TEXT1);
	input->AddDockedChild(new CDockResponseText, DS_DOCKED_CONTAINER | dw_style, 200, ID_DOCK_TEXT3);
	input->AddDockedChild(new CDockTranscriptText, DS_DOCKED_CONTAINER | dw_style, 200, ID_DOCK_TEXT2);
	image=AddDockedChild(new CDockImage, DS_DOCKED_TOP | DS_DOCKED_RIGHT | dw_style, width / 3, ID_DOCK_IMAGE1);
	browser=input->AddDockedChild(new CDockWebViewer, DS_DOCKED_CONTAINER | dw_style, width / 3, ID_DOCK_BROWSER1);
}

void CMainFrame::load_image_dockers()
{
	const auto width = GetWindowRect().Size().cx;
	const DWORD dw_style = DS_CLIENTEDGE; // The style added to each docker
	input = AddDockedChild(new CDockText, DS_DOCKED_LEFT | dw_style, width / 3, ID_DOCK_TEXT1);
	auto x=AddDockedChild(new CDockResponseText, DS_DOCKED_BOTTOM | dw_style, 160, ID_DOCK_TEXT3);
	x->AddDockedChild(new CDockTranscriptText, DS_DOCKED_RIGHT | dw_style, 160, ID_DOCK_TEXT2);
    AddDockedChild(new CDockImage, DS_DOCKED_TOP | DS_DOCKED_RIGHT | dw_style, width / 3, ID_DOCK_IMAGE1);
}

void CMainFrame::load_full_image_dockers()
{
}


CDocker* CMainFrame::NewDockerFromID(int id)
{
	CDocker* pDock = NULL;
	switch (id)
	{
	case ID_DOCK_TEXT1:
		input = new CDockText;
		break;
	case ID_DOCK_TEXT2:
		output = new CDockResponseText;
		break;
	case ID_DOCK_TEXT3:
		trans = new CDockTranscriptText;
		break;
	case ID_DOCK_IMAGE1:
		image = new CDockImage;
		break;
	case ID_DOCK_BROWSER1:
		browser = new CDockWebViewer;
		break;
	default:
		TRACE("Unknown Dock ID\n");
		break;
	}

	return pDock;
}

DWORD e0 = 0;
BOOL CMainFrame::OnCommand(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(lparam);

	// OnCommand responds to menu and and toolbar input
	UINT id = LOWORD(wparam);
	switch (id)
	{
	case IDM_CONTAINER_TOP:     return OnContainerTabsAtTop();
	case IDM_FILE_EXIT:         return OnFileExit();
	case IDM_DOCK_DEFAULT:      return OnDockDefault();
	case ID_DOCKING_BROWSERLAYOUT: return OnDockBrowser();
	case ID_DOCKING_IMAGELAYOUT: return on_dock_image();
	case ID_DOCKING_IMAGE_EVALUATOR: return on_dock_image_eval();
	case ID_DOCKING_EVALUATOR: return on_dock_eval();
	case IDM_DOCK_CLOSEALL:     return OnDockCloseAll();
	case IDW_VIEW_STATUSBAR:    return OnViewStatusBar();
	case IDW_VIEW_TOOLBAR:      return OnViewToolBar();
	case IDM_HELP_ABOUT:        return OnHelp();
	case IDM_HIDE_SINGLE_TAB:   return OnHideSingleTab();


	case IDM_EDIT_COPY:
	{
		const auto focus = ::GetFocus();
		SendMessage(focus, WM_COPY, 0, 0);
		break;
	}
	case IDM_EDIT_CUT:
	{
		const auto focus = ::GetFocus();
		SendMessage(focus, WM_CUT, 0, 0);
		break;
	}
	case IDM_EDIT_UNDO:
	{
		const auto focus = ::GetFocus();
		SendMessage(focus, WM_UNDO, 0, 0);
		break;
	}

	case IDM_EDIT_PASTE:
	{
		const auto focus = ::GetFocus();
		SendMessage(focus, WM_PASTE, 0, 0);
		break;
	}

	case IDM_ACTIONS_EXECUTE_SELECTED:
	{
		CViewText::EvalSelected(::GetFocus());
		break;
	}

	case IDM_ACTIONS_EXECUTE:
	{
		CViewText::Eval(::GetFocus());
		break;
	}

	case IDM_VIEW_CLEAR:
	{
		CViewText::ClearAll(::GetFocus());
		break;
	}

	}

	return FALSE;
}

BOOL CMainFrame::OnContainerTabsAtTop()
// Reposition the tabs in the containers
{
	SetContainerTabsAtTop(!m_isContainerTabsAtTop);
	return TRUE;
}

int CMainFrame::OnCreate(CREATESTRUCT& cs)
{

	// call the base class function
	return CDockFrame::OnCreate(cs);
}

void CMainFrame::OnDestroy()
{
	Stopping();
	PostQuitMessage(0);
}

void CMainFrame::Stopping()
{
	Engine::Stop();
	Sleep(100);
	CD2DView::Stop();
	Sleep(100);

}

BOOL CMainFrame::OnDockDefault()
{
	Stopping();
	SetRedraw(FALSE);
	CloseAllDockers();
	LoadDefaultDockers();
	HideSingleContainerTab(m_hideSingleTab);
	SetContainerTabsAtTop(m_isContainerTabsAtTop);
	SetRedraw(TRUE);
	RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
	return TRUE;
}

BOOL CMainFrame::OnDockCloseAll()
{
	Stopping();
	CloseAllDockers();
	return TRUE;
}

BOOL CMainFrame::OnFileExit()
{
	Stopping();
	PostMessage(WM_CLOSE);
	return TRUE;
}

BOOL CMainFrame::OnHideSingleTab()
{
	HideSingleContainerTab(!m_hideSingleTab);
	return TRUE;
}



BOOL CMainFrame::on_dock_close_all()
{
	return 0;
}

BOOL CMainFrame::on_dock_default()
{
	return 0;
}

BOOL CMainFrame::on_dock_image()
{
	Stopping();
	SetRedraw(FALSE);
	CloseAllDockers();
	load_browser_image_dockers();
	Sleep(100);
	SetRedraw(TRUE);
	RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
	return 0;
}


BOOL CMainFrame::on_dock_image_eval()
{
	Stopping();
	SetRedraw(FALSE);
	CloseAllDockers();
	load_image_dockers();
	Sleep(100);
	SetRedraw(TRUE);
	RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
	return 0;
}

BOOL CMainFrame::on_dock_eval()
{
	Stopping();
	SetRedraw(FALSE);
	CloseAllDockers();
	LoadEvalDockers();
	Sleep(100);
	SetRedraw(TRUE);
	RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
	return 0;
}

BOOL CMainFrame::OnDockBrowser()
{
	Stopping();
	SetRedraw(FALSE);
	CloseAllDockers();
	load_browser_dockers();
	SetRedraw(TRUE);
	RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
	return 0;
}

void CMainFrame::OnInitialUpdate()
{
	HWND hwnd = GetHwnd();
	Utility::set_main(hwnd);

	SetDockStyle(DS_CLIENTEDGE);
	DragAcceptFiles(true);

	if (!LoadDockRegistrySettings(GetRegistryKeyName()))
		LoadDefaultDockers();

	HideSingleContainerTab(m_hideSingleTab);
	ShowWindow(GetInitValues().showCmd);
}

void CMainFrame::OnMenuUpdate(UINT id)
// Called when menu items are about to be displayed
{
	UINT check;
	switch (id)
	{
	case IDM_CONTAINER_TOP:
		check = (m_isContainerTabsAtTop) ? MF_CHECKED : MF_UNCHECKED;
		GetFrameMenu().CheckMenuItem(id, check);
		break;

	case IDM_HIDE_SINGLE_TAB:
		check = (m_hideSingleTab) ? MF_CHECKED : MF_UNCHECKED;
		GetFrameMenu().CheckMenuItem(id, check);
		break;
	}

	CDockFrame::OnMenuUpdate(id);
}

void CMainFrame::PreCreate(CREATESTRUCT& cs)
{

	CDockFrame::PreCreate(cs);
	cs.style &= ~WS_VISIBLE;
}

BOOL CMainFrame::SaveRegistrySettings()
{
	if (CDockFrame::SaveRegistrySettings())
		return SaveDockRegistrySettings(GetRegistryKeyName());
	else
		return FALSE;
}

void CMainFrame::SetContainerTabsAtTop(BOOL isAtTop)
{
	m_isContainerTabsAtTop = isAtTop;
	std::vector<CDocker*>::const_iterator iter;

	// Set the Tab position for each container
	for (iter = GetAllDockers().begin(); iter < GetAllDockers().end(); ++iter)
	{
		CDockContainer* pContainer = (*iter)->GetContainer();
		if (pContainer && pContainer->IsWindow())
		{
			pContainer->SetTabsAtTop(isAtTop);
		}
	}
}

void CMainFrame::SetupToolBar()
{

	AddToolBarButton(0);
}



LRESULT CMainFrame::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_USER + 500:
	{
		SetStatusText((int)wParam, (LPCTSTR)lParam);
		break;
	}
	case WM_USER + 501:
	{
		Invalidate();
		RedrawWindow();
		break;
	}
	case WM_DROPFILES:
	{
		const int count = DragQueryFile(reinterpret_cast<HDROP>(wParam), 0xFFFFFFFF, nullptr, 0);
		if (count == 0) { return TRUE; }
		auto index = 0;
		const auto buffer = new char[64000];

		for (auto i = 0; i < count; i++)
		{
			const auto result_size = DragQueryFileA(reinterpret_cast<HDROP>(wParam), index, buffer, 4000);
			if (result_size == 0)
			{
				delete[] buffer;
				return TRUE;
			}
			index++;
			POINT xy;
			DragQueryPoint(reinterpret_cast<HDROP>(wParam), &xy);
			if (result_size == 0)
			{
				delete[] buffer;
				return TRUE;
			}

			// set editor from file buffer.
			CViewText::LoadFile(buffer);
			delete[] buffer;
			DragFinish(reinterpret_cast<HDROP>(wParam));
			return TRUE;
		}
	}
	default:
		return WndProcDefault(uMsg, wParam, lParam);
	}

	return WndProcDefault(uMsg, wParam, lParam);
}
