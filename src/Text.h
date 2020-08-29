///////////////////////////////////////////////////////
// Text.h - Declaration of the CViewText, CContainText, 
//          and CDockText classes

#ifndef TEXT_H
#define TEXT_H

#include "D2DView.h"

// Declaration of the CViewText class
class CViewText : public CWnd
{
public:
    CViewText();
    virtual ~CViewText();

    static void ClearAll(HWND hwnd);

    static void Start();
    static void EvalSelected(HWND hwnd);
    static void Eval(HWND hwnd);
    // sc_setEditorFromFile(char* fname)
    static void LoadFile(char* fname);
    static void LoadFile(std::wstring fname);
    static void NewFile();
    static void SaveFile();
    static void SaveFileAs(std::wstring fname);
    static void transcriptln(char* s);
    static void TakeSnapShot();
    static void RestoreSnapShot();

protected:
    virtual void PreCreate(CREATESTRUCT& cs); 
    virtual void OnAttach(); 
    virtual int OnCreate(CREATESTRUCT&);
    
    virtual void OnInitialUpdate();
    virtual BOOL OnCommand(WPARAM wparam, LPARAM lparam);
    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT on_drop_files(UINT uMsg, WPARAM wParam, LPARAM lParam);

  
};




class CViewResponseText : public CViewText
{
public:
    CViewResponseText() {}
    virtual ~CViewResponseText() {}

protected:
    virtual void PreCreate(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();
};

class CViewTranscriptText : public CViewText
{
public:
    CViewTranscriptText() {}
    virtual ~CViewTranscriptText() {}

protected:
    virtual void PreCreate(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();

};


 
// Declaration of the CContainText class
class CContainText : public CDockContainer
{
public:
    CContainText();
    ~CContainText() {}

private:
    CViewText m_viewText;
};



// Declaration of the CContainText class
class CContainResponseText : public CDockContainer
{
public:
    CContainResponseText();
    ~CContainResponseText() {}

private:
    CViewResponseText m_ViewResponseText;
};

// Declaration of the CContainText class
class CContainTranscriptText : public CDockContainer
{
public:
    CContainTranscriptText();
    ~CContainTranscriptText() {}

private:
    CViewTranscriptText m_ViewTranscriptText;
};

class CDockText : public CDocker
{
public:
    CDockText();
    virtual ~CDockText() {}

private:
    CContainText m_View;

};

class CDockResponseText : public CDocker
{
public:
    CDockResponseText();
    virtual ~CDockResponseText() {}

private:
    CContainResponseText m_View;

};

class CDockTranscriptText : public CDocker
{
public:
    CDockTranscriptText();
    virtual ~CDockTranscriptText() {}

private:
    CContainTranscriptText m_View;

};

class CContainImage : public CDockContainer
{
public:
    CContainImage();
    ~CContainImage() {}

public:
    CD2DView m_ViewImage;
};

////
class CDockImage : public CDocker
{
public:
    CDockImage();
    virtual ~CDockImage() {}

public:
    CContainImage m_View;

};



#endif // TEXT_H

