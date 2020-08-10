//////////////////////////////////////////////////
// ContainerApp.h

#ifndef CONTAINERAPP_H
#define CONTAINERAPP_H

#include "Mainfrm.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include "D2DView.h"

// Declaration of the CDockContainerApp class
class CDockContainerApp : public CWinApp
{
public:
    CDockContainerApp();
    virtual ~CDockContainerApp();
    virtual BOOL InitInstance();
    CMainFrame& GetMainFrame() { return m_frame; }
    ID2D1Factory* GetD2DFactory() { return m_pDirect2dFactory; }

private:
    CMainFrame m_frame;
    HRESULT CreateDeviceIndependentResources();

    CD2DView m_view;
    ID2D1Factory* m_pDirect2dFactory;
};


// returns a pointer to the CDockContainerApp object
inline CDockContainerApp* GetContainerApp() { return static_cast<CDockContainerApp*>(GetApp()); }

inline ID2D1Factory* GetD2DFactory()
{
    return ((CDockContainerApp*)GetApp())->GetD2DFactory();
}



#endif // CONTAINERAPP_H

