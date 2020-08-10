//////////////////////////////////////////////
// ContainerApp.cpp


#include "stdafx.h"
#include "ContainerApp.h"

CDockContainerApp::CDockContainerApp() : m_pDirect2dFactory(NULL)
{
}

CDockContainerApp::~CDockContainerApp()
{
    SafeRelease(&m_pDirect2dFactory);
    CoUninitialize();
}

HRESULT CDockContainerApp::CreateDeviceIndependentResources()
{
    // Create a Direct2D factory.
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);

    return hr;
}
 
 

BOOL CDockContainerApp::InitInstance()
{
 
    m_frame.Create();    
 
    return TRUE;
}


