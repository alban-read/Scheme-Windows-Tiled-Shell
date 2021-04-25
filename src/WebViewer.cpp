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

#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)


ComPtr<ICoreWebView2> web_view_window;
ComPtr<ICoreWebView2Controller>web_view_controller;

HANDLE g_messages_mutex;

bool allow_eval = true;


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
 


WebViewer::WebViewer()
{
}

WebViewer::~WebViewer()
{
}

bool startsWith(std::string mainStr, std::string toMatch)
{
    if (mainStr.find(toMatch) == 0)
        return true;
    else
        return false;
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

                        web_view_window->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                PWSTR uri;
                                args->get_Uri(&uri);
                                const std::wstring source(uri);
                              
                                CoTaskMemFree(uri);
                                return S_OK;
                            }).Get(), &token);


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

                        const auto locate_base= Utility::GetFullPathFor(Utility::widen("scripts/base.js").c_str());
                        if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributes(locate_base.c_str()) &&
                            GetLastError() == ERROR_FILE_NOT_FOUND))
                        {
                            const auto startup_script = load_utf8_file_to_string(locate_base);
                            web_view_window->AddScriptToExecuteOnDocumentCreated(locate_base.c_str(), nullptr);
                        }


                        // listen for messages to evaluate scheme from the web view
                        web_view_window->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {

                                PWSTR source;
                                PWSTR message;
                                args->TryGetWebMessageAsString(&message);
                                args->get_Source(&source);

                                std::string text = Utility::ws_2s(message);
                                std::string uri = Utility::ws_2s(source);

                                //CViewText::transcriptln((char*)fmt::format("Message from {}", uri).c_str());
                                
                                bool safe_ish = false;
                                if (startsWith(uri, "https://github.com/alban-read/")) safe_ish = true;
                                if (startsWith(uri, "https://www.scheme.com/")) safe_ish = true;
                                if (startsWith(uri, "http://cisco.github.io/ChezScheme/csug9.5/")) safe_ish = true;
                                if (startsWith(uri, "file:")) safe_ish = true; //local

                                if (uri.find("Scheme-Windows-Tiled-Shell\docs") != std::string::npos) {
                                    safe_ish = true;
                                }
                                const char* eval_cmd = "::eval:";
                                if (allow_eval == true && safe_ish==true) {
                            
                                    if (text.rfind(eval_cmd, 0) == 0) {
                                        std::string command = text.c_str() + strlen(eval_cmd);
                                        Engine::Eval(_strdup(command.c_str()));
                                        //CViewText::transcriptln((char*)command.c_str());
                                        Sleep(0);
                                        CoTaskMemFree(message);
                                        return S_OK;
                                    }

                                }

                                const char* api_cmd = "::api:";
                                if (text.rfind(api_cmd, 0) == 0) {
                                    char* end_ptr;
                                    long int n = static_cast<int>(strtol(text.c_str() + strlen(api_cmd), &end_ptr, 10));

                                    if (n < 63) {
                                        std::string param = end_ptr;
                                        const ptr scheme_string = Engine::CALL2byName("api-call", Sfixnum(n), Sstring(param.c_str()));

                                        std::string result;
                                        if (scheme_string != Snil && Sstringp(scheme_string))
                                        {
                                            result = Utility::Ss2s(scheme_string);
                                        }
                                        std::wstring response;
                                        if (result.rfind("::", 0) == std::string::npos)
                                            response = Utility::s2_ws(fmt::format("::api_reply:{0}:", n));
                                        response += Utility::s2_ws(result);
                                        webview->PostWebMessageAsString(response.c_str());

                                        return S_OK;

                                    }

                                    switch (n)
                                    {
                                    case 1064:
                                        Engine::Stop();
                                        return S_OK;
                                        break;

                                    default:
                                        break;
                                    }
                                }

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

std::deque<std::wstring> post_messages;

ptr post_message_to_webview(const char* msg) {
    WaitForSingleObject(g_messages_mutex, INFINITE);
    post_messages.emplace_back(Utility::widen(msg));
    ReleaseMutex(g_messages_mutex);
    return Strue;
}

DWORD WINAPI process_postmessages(LPVOID x) {

    while (true) {

        if (GetAsyncKeyState(VK_ESCAPE) != 0)
        {
            while (!post_messages.empty())
            {
                post_messages.pop_front();
            }
            post_messages.shrink_to_fit();
            ReleaseMutex(g_messages_mutex);
        }
        
        WaitForSingleObject(g_messages_mutex, INFINITE);
        if (post_messages.empty())
        {
            ReleaseMutex(g_messages_mutex);
            Sleep(20);
        }
        else {
            web_view_window->PostWebMessageAsString(post_messages.front().c_str());
            post_messages.pop_front();
            Sleep(1);
            ReleaseMutex(g_messages_mutex);
        }
    }
}


void add_webview_commands() {
    Sforeign_symbol("post_message_to_webview", static_cast<ptr>(post_message_to_webview));
}

void WebViewer::Start()
{
    if (g_messages_mutex == nullptr) {
        g_messages_mutex = CreateMutex(nullptr, FALSE, nullptr);
    }
 
    static auto cmd_thread = CreateThread(
        nullptr,
        0,
        process_postmessages,
        nullptr,
        0,
        nullptr);

    add_webview_commands();
}

void WebViewer::Stop()
{


}
