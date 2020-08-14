
#include "stdafx.h"
#include "Text.h"
#include "resource.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "ContainerApp.h"
#include "Output.h"
#include "Engine.h"

#include <iostream>
#include <fstream>
#include "Utility.h"

// A few basic colors
const COLORREF black = RGB(0, 0, 0);
const COLORREF white = RGB(255, 255, 255);
const COLORREF green = RGB(0, 0xCC, 0xC9);
const COLORREF red = RGB(255, 0, 0);
const COLORREF blue = RGB(0, 0, 245);
const COLORREF yellow = RGB(255, 255, 0);
const COLORREF magenta = RGB(255, 0, 255);
const COLORREF cyan = RGB(0, 255, 255);
const COLORREF gray = RGB(120, 120, 180);
const COLORREF orange = RGB(255, 102, 0);

LRESULT send_editor(HWND h, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
{
    return ::SendMessage(h, Msg, wParam, lParam);
}

HWND response;
HWND transcript;
HWND inputed;


void sc_setText(HWND h, char* text) {
	int l = strlen(text);
	send_editor(h, SCI_SETTEXT, l, reinterpret_cast<LPARAM>(text));
}


void show_line_numbers(HWND h) {
    send_editor(h, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
    send_editor(h, SCI_SETMARGINWIDTHN, 0, 48);
    send_editor(h, SCI_SETMARGINWIDTHN, 1, 0);
}

void set_a_style(HWND h, int style, COLORREF fore, COLORREF back = RGB(0xFF, 0xFF, 0xEA), int size = -1, const char* face = 0, bool bold = false, bool italic = false)
{
    send_editor(h, SCI_STYLESETFORE, style, fore);
    send_editor(h, SCI_STYLESETBACK, style, back);
    if (size >= 8)
        send_editor(h, SCI_STYLESETSIZE, style, size);
    if (face)
        send_editor(h, SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
    if (bold)
        send_editor(h, SCI_STYLESETBOLD, style, 1);
    if (italic)
        send_editor(h, SCI_STYLESETITALIC, style, 1);
}


// scheme functions and macros
static const char g_scheme[] =
"abs and any append append! apply assoc assq assv begin boolean? "
"caaaar caaadr caaar caadar caaddr caadr caar cadaar cadadr cadar caddar cadddr caddr cadr car "
"case case-lambda call-with-values catch "
"cdaaar cdaadr cdaar cdadar cdaddr cdadr cdar cddaar cddadr cddar cdddar cddddr cdddr cddr cdr "
"char? char=? char>? char<? char<=? char>=? char->integer "
"cond cons cons* define define-syntax denominator "
"display do dotimes elseeq? eof-object? eqv? eval exact? expt "
"field filter find first floor for format for-each format from "
"hash "
"if in inexact? integer? integer->char import iota "
"lambda last-pair length let let* letrec let-rec list list? list-ref  list->string list-tail "
"map max member memq memv min mod"
"negative? nil not null? numerator "
"pair? positive? procedure? "
"quasiquote quote quotient or "
"random read real? remainder reverse "
"seq sequence set! set-car! set-cdr! sqrt step sublist symbol? "
"string? string=? string-append string-copy string-length string->list string-ref  string-set! substring syntax-rules "
"to try truncate  "
"unless unquote "
"values vector vector? vector-length vector->list vector-ref vector-set! "
"while when where write ";

// commands that have been added by this app.
static const char g_scheme2[] =
"after " 
"batch-draw-ellipse batch-draw-line batch-draw-rect batch-draw-scaled-rotated-sprite "
"batch-draw-sprite batch-fill-ellipse batch-fill-rect batch-render-sprite batch-render-sprite-scale-rot "
"clear-image draw-into-sprite draw-ellipse draw-line draw-rect draw-scaled-rotated-sprite draw-sprite " 
"every font graphics-keys fill-colour fill-ellipse fill-rect identity image-size "
"line-colour load-sound load-sprites make-sprite "
"pen-width play-sound render render-sprite render-sprite-scale-rot release rotate  "
"set-every-function show stop-every write-text draw-into-sprite ";

static const char g_scheme3[] =
" ";
static const char g_scheme4[] =
"#f #t '()";

struct s_scintilla_colors
{
	int         iItem;
	COLORREF    rgb;
	int			size;
	char* face;
	bool		bold;
	bool		italic;
};

static s_scintilla_colors scheme[] =
{	//	item					colour				sz	face		bold	italic
	{ SCE_C_COMMENT,          RGB(160,82,45),		11, "Consolas",	false, true },
	{ SCE_C_COMMENTLINE,      RGB(184,138,0),		11, "Consolas",	false, false },
	{ SCE_C_COMMENTDOC,       RGB(32,178,170),		11, "Consolas",	false, false },
	{ SCE_C_NUMBER,           RGB(138,43,226),		11, "Consolas",	false, false },
	{ SCE_C_STRING,           RGB(140,140,140),		11, "Consolas",	false, false },
	{ SCE_C_CHARACTER,        RGB(100,100,100),		11, "Consolas",	false, false },
	{ SCE_C_UUID,             cyan,					11, "Consolas",	false, false },
	{ SCE_C_OPERATOR,         black,				11, "Consolas",	false, false },
	{ SCE_C_WORD,             RGB(0,138,184),		11, "Consolas",	true,  false },
	{ SCE_C_WORD2,            RGB(0,138,184),		11, "Consolas",	false,  false },
	{ SCE_C_REGEX,			  RGB(186,85,211),		11, "Consolas",	false, false },
	{ -1,                     0 }
};



void show_folds(HWND h) {

	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));
	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("1"));
	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.html"), reinterpret_cast<LPARAM>("1"));

	send_editor(h, SCI_SETMARGINWIDTHN, 1, 0);
	send_editor(h, SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
	send_editor(h, SCI_SETMARGINMASKN, 1, SC_MASK_FOLDERS);
	send_editor(h, SCI_SETMARGINWIDTHN, 1, 20);
	send_editor(h, SCI_SETMARGINSENSITIVEN, 1, 1);

	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	send_editor(h, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
	send_editor(h, SCI_SETFOLDFLAGS, 16, 0);
}

void hide_folds(HWND h) {

	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("0"));
	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<LPARAM>("0"));
	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<LPARAM>("0"));
	send_editor(h, SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.html"), reinterpret_cast<LPARAM>("0"));
	send_editor(h, SCI_SETMARGINMASKN, 1, 0);
	send_editor(h, SCI_SETMARGINWIDTHN, 1, 0);
	send_editor(h, SCI_SETMARGINSENSITIVEN, 1, 0);
}


void initialize_editor(HWND h) {

	inputed = h;

	send_editor(h, SCI_SETCODEPAGE, SC_CP_UTF8);
	send_editor(h, SCI_SETLEXER, SCLEX_LISP);

	// Set tab width
	send_editor(h, SCI_SETTABWIDTH, 4);

	// line wrap (avoid h scroll bars)
	send_editor(h, SCI_SETWRAPMODE, 1);
	send_editor(h, SCI_SETWRAPVISUALFLAGS, 2);
	send_editor(h, SCI_SETWRAPSTARTINDENT, 6);

	// auto sel
	send_editor(h, SCI_AUTOCSETSEPARATOR, ',');
	send_editor(h, SCI_AUTOCSETMAXHEIGHT, 9);

	// dwell/ used for brackets
	send_editor(h, SCI_SETMOUSEDWELLTIME, 500);
	set_a_style(h, STYLE_DEFAULT, gray, RGB(0xFF, 0xFF, 0xEA), 11, "Consolas");

	// Set caret foreground color
	send_editor(h, SCI_SETCARETFORE, RGB(0, 0, 255));

	// Set selection color
	send_editor(h, SCI_SETSELBACK, TRUE, RGB(0xF0, 0xF0, 0xFE));

	// Set all styles
	send_editor(h, SCI_STYLECLEARALL);

	for (long i = 0; scheme[i].iItem != -1; i++)
		set_a_style(h, scheme[i].iItem,
			scheme[i].rgb,
			RGB(0xFF, 0xFF, 0xEA),
			scheme[i].size,
			scheme[i].face,
			scheme[i].bold,
			scheme[i].italic);

	set_a_style(h, STYLE_BRACELIGHT, orange, RGB(0xFF, 0xFF, 0xEA), 11, "Consolas");
	set_a_style(h, STYLE_BRACEBAD, red, RGB(0xFF, 0xFF, 0xEA), 11, "Consolas");


	send_editor(h, SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(g_scheme));
	send_editor(h, SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(g_scheme2));
	send_editor(h, SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(g_scheme3));
	send_editor(h, SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(g_scheme4));


	send_editor(h, SCI_ENSUREVISIBLEENFORCEPOLICY, 2);
	send_editor(h, SCI_GOTOLINE, 2);

 
}



void initialize_response_editor(HWND h) {

	response = h;

	send_editor(h, SCI_SETCODEPAGE, SC_CP_UTF8);
	send_editor(h, SCI_SETLEXER, SCLEX_LISP);

	// Set tab width
	send_editor(h, SCI_SETTABWIDTH, 4);

	// line wrap (avoid h scroll bars)
	send_editor(h, SCI_SETWRAPMODE, 1);
	send_editor(h, SCI_SETWRAPVISUALFLAGS, 2);
	send_editor(h, SCI_SETWRAPSTARTINDENT, 6);

	// auto sel
	send_editor(h, SCI_AUTOCSETSEPARATOR, ',');
	send_editor(h, SCI_AUTOCSETMAXHEIGHT, 9);

	// dwell/ used for brackets
	send_editor(h, SCI_SETMOUSEDWELLTIME, 500);
	set_a_style(h, STYLE_DEFAULT, gray, RGB(0xFF, 0xFF, 0xEA), 11, "Consolas");

	// Set caret foreground color
	send_editor(h, SCI_SETCARETFORE, RGB(0, 0, 255));

	// Set selection color
	send_editor(h, SCI_SETSELBACK, TRUE, RGB(0xF0, 0xF0, 0xFE));

	// Set all styles
	send_editor(h, SCI_STYLECLEARALL);

	for (long i = 0; scheme[i].iItem != -1; i++)
		set_a_style(h, scheme[i].iItem,
			scheme[i].rgb,
			RGB(0xFF, 0xFF, 0xEA),
			scheme[i].size,
			scheme[i].face,
			scheme[i].bold,
			scheme[i].italic);

	set_a_style(h, STYLE_BRACELIGHT, orange, RGB(0xFF, 0xFF, 0xEA), 11, "Consolas");
	set_a_style(h, STYLE_BRACEBAD, red, RGB(0xFF, 0xFF, 0xEA), 11, "Consolas");


	send_editor(h, SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(g_scheme));
	send_editor(h, SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(g_scheme2));
	send_editor(h, SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(g_scheme3));
	send_editor(h, SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(g_scheme4));


	send_editor(h, SCI_ENSUREVISIBLEENFORCEPOLICY, 2);
	send_editor(h, SCI_GOTOLINE, 2);
 

}


void initialize_transcript_editor(HWND h) {

	transcript = h;

	send_editor(h, SCI_SETCODEPAGE, SC_CP_UTF8);
	// Set tab width
	send_editor(h, SCI_SETTABWIDTH, 4);

	// line wrap (avoid h scroll bars)
	send_editor(h, SCI_SETWRAPMODE, 1);
	send_editor(h, SCI_SETWRAPVISUALFLAGS, 2);
	send_editor(h, SCI_SETWRAPSTARTINDENT, 6);

	// auto sel
	send_editor(h, SCI_AUTOCSETSEPARATOR, ',');
	send_editor(h, SCI_AUTOCSETMAXHEIGHT, 9);

	// dwell
	send_editor(h, SCI_SETMOUSEDWELLTIME, 500);
	set_a_style(h, STYLE_DEFAULT, gray, RGB(0xFF, 0xFF, 0xEA), 10, "Consolas");

	// Set caret foreground color
	send_editor(h, SCI_SETCARETFORE, RGB(0, 0, 255));

	// Set selection color
	send_editor(h, SCI_SETSELBACK, TRUE, RGB(0xF0, 0xF0, 0xFE));

	// Set all styles
	send_editor(h, SCI_STYLECLEARALL);
 

}

static bool is_brace(const int c)
{
	switch (c)
	{
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
		return true;
	default:
		return false;
	}
}


// this is hooked into the notify event in the header files.
// this just adds brace highlighting.

void scintillate(SCNotification* N, LPARAM lParam) {
	const auto h = static_cast<HWND>(N->nmhdr.hwndFrom);

	if (N->nmhdr.code == SCN_CHARADDED) {
		auto c = N->ch;
		const int lpos = send_editor(h, SCI_GETCURRENTPOS);
		const int line = send_editor(h, SCI_LINEFROMPOSITION, lpos);
	}

	else if (N->nmhdr.code == SCN_UPDATEUI) {
		const int lpos = send_editor(h, SCI_GETCURRENTPOS);
		const int c = send_editor(h, SCI_GETCHARAT, lpos, 0);
		if (is_brace(c)) {
			const int bracepos = send_editor(h, SCI_BRACEMATCH, lpos, 0);
			if (bracepos != -1) {
				send_editor(h, SCI_BRACEHIGHLIGHT, lpos, bracepos);
			}
			else {
				SendMessage(h, SCI_BRACEBADLIGHT, lpos, 0);
			}
		}
	}
	else if (N->nmhdr.code == SCN_DWELLSTART) {
		const int lpos = send_editor(h, SCI_GETCURRENTPOS);
		const int c = send_editor(h, SCI_GETCHARAT, lpos, 0);
		if (is_brace(c)) {
			const int bracepos = send_editor(h, SCI_BRACEMATCH, lpos, 0);
			if (bracepos != -1) {
				send_editor(h, SCI_BRACEHIGHLIGHT, lpos, bracepos);
			}
			else {
				SendMessage(h, SCI_BRACEBADLIGHT, lpos, 0);
			}
		}
	}
}



// Transcript etc

void eval_scite(HWND hc)
{
	const auto l = send_editor(hc, SCI_GETLENGTH) + 1;
	auto* cmd = new(std::nothrow) char[l];
	if (cmd == nullptr) {
		return;
	}
	memset(cmd, 0, l);
	send_editor(hc, SCI_GETTEXT, l, reinterpret_cast<LPARAM>(cmd));
	Engine::Eval(cmd);
}

void eval_selected_scite(HWND hc)
{

	if (hc == response || hc == transcript || hc == inputed) {

		const int l = send_editor(hc, SCI_GETSELTEXT) + 1;
		auto* cmd = new(std::nothrow) char[l];
		if (cmd == nullptr) {
			return;
		}
		memset(cmd, 0, l);
		send_editor(hc, SCI_GETSELTEXT, l, reinterpret_cast<LPARAM>(cmd));

		try {
			Engine::Eval(cmd);
		}
		catch (...) {

			return;
		}
		return;
	}
}


char* sc_getText(HWND hc) {
	const auto l = send_editor(hc, SCI_GETLENGTH) + 1;
	auto cmd = new(std::nothrow) char[l];
	memset(cmd, 0, l);
	send_editor(hc, SCI_GETTEXT, l, reinterpret_cast<LPARAM>(cmd));
	return cmd;
}

const char* sc_getExprText() {
	return sc_getText(inputed);
}


void sc_appendText(HWND h, char* text) {
	const int l = strlen(text);
	send_editor(h, SCI_APPENDTEXT, l, reinterpret_cast<LPARAM>(text));
	Sleep(10);
}

void sc_setTextFromFile(HWND h, char* fname) {
	std::ifstream f(fname);
	std::stringstream buffer;
	buffer << f.rdbuf();
	buffer.seekg(0, std::ios::end);
	const int size = buffer.tellg();
	send_editor(h, SCI_SETTEXT, size, reinterpret_cast<LPARAM>(buffer.str().c_str()));
}

void sc_setEditorFromFile(char* fname) {
	sc_setTextFromFile(inputed, fname);
}


// called from scheme
void set_inputed(char* s)
{
	sc_setText(inputed, s);
}

ptr get_inputed()
{
	const int l = send_editor(inputed, SCI_GETLENGTH) + 1;
	auto cmd = new(std::nothrow) char[l];
	memset(cmd, 0, l);
	send_editor(inputed, SCI_GETTEXT, l, reinterpret_cast<LPARAM>(cmd));
	const auto s = Sstring(cmd);
	delete[] cmd;
	return s;
}

void appendEditor(char* s)
{
	sc_setText(response, s);
	Sleep(10);
}
void appendTranscript(char* s)
{
	sc_appendText(transcript, s);
}
 
void appendTranscriptNL(char* s)
{
		sc_appendText(transcript, s);
		sc_appendText(transcript, "\r\n");
}


// called from scheme engine.
void appendTranscript1(char* s)
{
		sc_appendText(transcript, s);
}

void appendTranscript2(char* s, char* s1)
{
		sc_appendText(transcript, s);
		sc_appendText(transcript, " ");
		sc_appendText(transcript, s1);
}


void clear_transcript()
{
	send_editor(transcript, SCI_CLEARALL);
}
 
void add_transcript_commands() {
	Sforeign_symbol("eval_respond", static_cast<void*>(appendEditor));
	Sforeign_symbol("append_transcript", static_cast<void*>(appendTranscript1));
	Sforeign_symbol("setInputed", static_cast<void*>(set_inputed));
	Sforeign_symbol("getInputed", static_cast<ptr>(get_inputed));
}


///////////////////////////////////////////////
// CViewText functions
CViewText::CViewText()
{
}

CViewText::~CViewText()
{
}

void CViewText::OnAttach()
{
 
}

void CViewText::PreCreate(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("Scintilla");
}

int CViewText::OnCreate(CREATESTRUCT&)
{
	return 0;
}

void CViewText::OnInitialUpdate()
{
	HWND h = this->GetHwnd();
	initialize_editor(h);
}


BOOL CViewText::OnCommand(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(lparam);
 
	return 0;
}

LRESULT CViewText::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	default:;
	}
	return WndProcDefault(uMsg, wParam, lParam);
}


LRESULT CViewText::on_drop_files(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0L;
}

void CViewText::Start()
{
	add_transcript_commands();
}

 
void CViewText::EvalSelected(HWND hwnd)
{
	if (hwnd == inputed || hwnd == transcript || hwnd == response) {
		eval_selected_scite(hwnd);
	}

}

void CViewText::Eval(HWND hwnd)
{
	eval_scite(hwnd);
}

void CViewText::LoadFile(char* fname)
{
	sc_setEditorFromFile(fname);
}

void CViewText::transcriptln(char* s)
{
	appendTranscriptNL(s);
}
 
CDockText::CDockText()
{
	// Set the view window to our edit control
	SetView(m_View);

	// Set the width of the splitter bar
	SetBarWidth(8);
}

void CViewResponseText::PreCreate(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("Scintilla");
}

void CViewResponseText::OnInitialUpdate()
{

	const auto h = this->GetHwnd();
	initialize_response_editor(h);
}

void CViewTranscriptText::PreCreate(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("Scintilla");
}

void CViewTranscriptText::OnInitialUpdate()
{
	const auto h = this->GetHwnd();
	initialize_transcript_editor(h);
}

 
CContainText::CContainText()
{
	SetDockCaption(_T("Evaluator View"));
	SetTabText(_T("Evaluator"));
	SetTabIcon(IDI_LAMBDA);
	SetView(m_viewText);
}

 
CContainResponseText::CContainResponseText()
{
	SetDockCaption(_T("Response View"));
	SetTabText(_T("Response"));
	SetTabIcon(IDI_TEXT);
	SetView(m_ViewResponseText);
}

 
CContainTranscriptText::CContainTranscriptText()
{
	SetDockCaption(_T("Transcript View"));
	SetTabText(_T("Transcript"));
	SetTabIcon(IDI_TEXT);
	SetView(m_ViewTranscriptText);
}

 

 
CDockResponseText::CDockResponseText()
{
	// Set the view window to our edit control
	CDocker::SetView(m_View);

	// Set the width of the splitter bar
	SetBarWidth(4);
}
 
CDockTranscriptText::CDockTranscriptText()
{
	// Set the view window to our edit control
	CDocker::SetView(m_View);

	// Set the width of the splitter bar
	SetBarWidth(4);
}


CContainImage::CContainImage()
{
	SetDockCaption(_T("Image View"));
	SetTabText(_T("Image"));
	SetTabIcon(IDI_TEXT);
	SetView(m_ViewImage);
}

//////////////////////////////////////////////
//  Definitions for the CDockText class
CDockImage::CDockImage()
{
	// Set the view window to our edit control
	CDocker::SetView(m_View);

	// Set the width of the splitter bar
	SetBarWidth(3);
}
void CViewText::ClearAll(HWND hwnd) {
	send_editor(hwnd, SCI_CLEARALL);
}