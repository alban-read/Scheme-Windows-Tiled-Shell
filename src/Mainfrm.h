 
#ifndef MAINFRM_H
#define MAINFRM_H

 
 

enum DockIDs
{
    ID_DOCK_TEXT1 = 1,
    ID_DOCK_TEXT2 = 2,
    ID_DOCK_TEXT3 = 3,
    ID_DOCK_IMAGE1 = 4,
    ID_DOCK_BROWSER1 = 5
};


// Declaration of the CMainFrame class
class CMainFrame : public CDockFrame
{
public:
    CMainFrame();
    virtual ~CMainFrame();
    void LoadDefaultDockers();
    BOOL OnContainerTabsAtTop();
    BOOL OnDockCloseAll();
    BOOL OnDockDefault();
    BOOL OnFileExit();
    BOOL OnHideSingleTab();
    
 
    void load_image_dockers();
    void load_full_image_dockers();
    void load_browser_dockers();

    BOOL on_dock_close_all();
    BOOL on_dock_default();
    BOOL on_dock_image();
    BOOL OnDockBrowser();
 
    CDocker* input{};
    CDocker* output{};
    CDocker* image{};
    CDocker* trans{};
    CDocker* browser{};

 



protected:
    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    virtual CDocker* NewDockerFromID(int id);
    virtual BOOL OnCommand(WPARAM wparam, LPARAM lparam);
    virtual int  OnCreate(CREATESTRUCT& cs);
    virtual void OnDestroy();
    void Stopping();
    virtual void OnInitialUpdate();
    virtual void OnMenuUpdate(UINT id);
    virtual void PreCreate(CREATESTRUCT& cs);
    virtual BOOL SaveRegistrySettings();
    virtual void SetupToolBar();

    inline void SetStatusText(int n, LPCTSTR s) {
        s0 = s;
        GetStatusBar().SetPartText(n, (LPCTSTR)s0.c_str());
    }

private:
    void HideSingleContainerTab(BOOL hideSingle);
    void SetContainerTabsAtTop(BOOL isAtTop);

    CDockContainer m_view;
    BOOL m_isContainerTabsAtTop;
    BOOL m_hideSingleTab;
    std::wstring s0;

};

#endif //MAINFRM_H

