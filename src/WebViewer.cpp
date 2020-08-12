#include "stdafx.h"
#include "WebViewer.h"
#include <WinInet.h>
#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <algorithm>
 
#include <fmt/format.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h> 
#include <deque>
#include <locale>
#include <codecvt>
#include "resource.h"
#include <deque>
#include "Utility.h"
#include "Engine.h"
#include "scheme.h"
#include "Text.h"

#pragma comment (lib, "wininet.lib")

using namespace Microsoft::WRL;

ComPtr<ICoreWebView2> web_view_window;
ComPtr<ICoreWebView2Controller>web_view_controller;


size_t get_size_of_file(const std::wstring& path)
{
    struct _stat fileinfo {};
    _wstat(path.c_str(), &fileinfo);
    return fileinfo.st_size;
}

std::wstring load_utf8_file_to_string(const std::wstring& filename)
{
    std::wstring buffer;  
    FILE* f;
    const auto err = _wfopen_s(&f, filename.c_str(), L"rtS, ccs=UTF-8");
    // Failed to open file
    if (f == nullptr || err != 0)
    {
        return buffer;
    }
    const auto file_size = get_size_of_file(filename);
    if (file_size > 0)
    {
        buffer.resize(file_size);
        const auto chars_read = fread(&(buffer.front()), sizeof(wchar_t), file_size, f);
        buffer.resize(chars_read);
        buffer.shrink_to_fit();
    }
    fclose(f);
    return buffer;
}

 
 
ptr scheme_load_document_from_file(const char* relative_file_name)
{
    const auto locate_file = Utility::GetFullPathFor(Utility::widen(relative_file_name).c_str());
    if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributes(locate_file.c_str()) &&
        GetLastError() == ERROR_FILE_NOT_FOUND))
    {
        std::wstring document = load_utf8_file_to_string(locate_file);
        web_view_window->NavigateToString(document.c_str());
        return Strue;
    }
    return Sfalse;
}
 

void add_webview_commands() {

}


WebViewer::WebViewer()
{
}

WebViewer::~WebViewer()
{
}

void WebViewer::OnInitialUpdate()
{
    HWND hWnd = GetHwnd();
    Utility::set_browser(hWnd);
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (controller != nullptr) {
                            web_view_controller = controller;
                            web_view_controller->get_CoreWebView2(&web_view_window);
                        }
                        ICoreWebView2Settings* Settings;
                        web_view_window->get_Settings(&Settings);
                        Settings->put_IsScriptEnabled(TRUE);
                        Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        Settings->put_IsWebMessageEnabled(TRUE);
                        Settings->put_IsStatusBarEnabled(FALSE);
                    
                        EventRegistrationToken token;

                        web_view_window->add_ProcessFailed(Callback<ICoreWebView2ProcessFailedEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2ProcessFailedEventArgs* args) -> HRESULT {
                                COREWEBVIEW2_PROCESS_FAILED_KIND kind;
                                args->get_ProcessFailedKind(&kind);
                                auto new_web_view_needed =
                                    (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED);
                                if (new_web_view_needed)
                                {
                                    ::MessageBoxW(nullptr, L"This web view\r\n has sadly crashed..", L"Fatal Error", MB_OK);
                                    
                                }
                                else
                                {	// might work we guess,.
                                    webview->Reload();
                                }
                                return S_OK;
                            }).Get(), &token);

                        const auto locate_script = Utility::GetFullPathFor(Utility::widen("scripts/startup.js").c_str());
                        if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributes(locate_script.c_str()) &&
                            GetLastError() == ERROR_FILE_NOT_FOUND))
                        {
                            const auto startup_script = load_utf8_file_to_string(locate_script);
                            web_view_window->AddScriptToExecuteOnDocumentCreated(startup_script.c_str(), nullptr);
                        }

                        // listen for messages to evaluate scheme from the web view
                        web_view_window->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {


                                PWSTR message;
                                args->TryGetWebMessageAsString(&message);
                                std::string text = Utility::ws_2s(message);
                                
                                const char* eval_cmd = "::eval:";
                                if (text.rfind(eval_cmd, 0) == 0) {
                                    std::string command = text.c_str() + strlen(eval_cmd);
                                    Engine::Eval(_strdup(command.c_str()));
                                    CViewText::transcriptln((char*)command.c_str());
                                    Sleep(0);
                                    CoTaskMemFree(message);
                                    return S_OK;
                                }
                                // add more commands ..

                                std::wstring response = L"::invalid_request:";
                                response += message;
                                webview->PostWebMessageAsString(response.c_str());
                                CoTaskMemFree(message);
                                return S_OK;
                            }).Get(), &token);
                            

                        RECT bounds;
                        ::GetClientRect(hWnd, &bounds);
                        web_view_controller->put_Bounds(bounds);
                        web_view_window->Navigate(Utility::GetFullPathFor(L"docs/welcome.html").c_str());
                        return S_OK;

                    }).Get());
                return S_OK;
            }).Get());

}

 

LRESULT WebViewer::OnSize(UINT, WPARAM, LPARAM lparam)
{
    HWND hwnd = GetHwnd();
    if (web_view_window != nullptr && web_view_controller != nullptr) {
        RECT bounds;
        ::GetClientRect(hwnd, &bounds);
        bounds.bottom = bounds.bottom - 45;
        web_view_controller->put_Bounds(bounds);
    };
    return 0;
}

CContainWebViewer::CContainWebViewer()
{
    SetDockCaption(_T("Documentation"));
    SetTabText(_T("Documentation"));
    SetTabIcon(IDI_TEXT);
    SetView(m_WebViewer);
}

CDockWebViewer::CDockWebViewer()
{
    CDocker::SetView(m_View);
}


BOOL WebViewer::OnCommand(WPARAM wparam, LPARAM lparam) {
 
    UINT id = LOWORD(wparam);
    switch (id)
    {
    }
    return FALSE;
}
 
LRESULT WebViewer::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{

    switch (msg)
    {
    case WM_DISPLAYCHANGE:
    case WM_SIZE:  return OnSize(msg, wparam, lparam);
    }


    return WndProcDefault(msg, wparam, lparam);
}

 

void WebViewer::Start()
{
    add_webview_commands();
}
