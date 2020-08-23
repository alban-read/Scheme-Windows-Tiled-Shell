#pragma once



class WebViewer : public CWnd
{

public:

	WebViewer();
	virtual ~WebViewer();
    void OnInitialUpdate();
    virtual BOOL OnCommand(WPARAM wparam, LPARAM lparam);
    LRESULT OnSize(UINT, WPARAM, LPARAM lparam);
 
    virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);
    static void EvalSelected(HWND hwnd);
    static void Start();
    static void Stop();

};

class CContainWebViewer : public CDockContainer
{
public:
    CContainWebViewer();
    ~CContainWebViewer() {}

public:
    WebViewer m_WebViewer;
};

 
class CDockWebViewer : public CDocker
{
public:
    CDockWebViewer();
    virtual ~CDockWebViewer() {}

public:
    CContainWebViewer m_View;

};