
#include "stdafx.h"
#include "Text.h"
#include "resource.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "ContainerApp.h"
#include "Output.h"
#include "Engine.h"
#include <fmt/format.h>
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
 
char* snap_shot;
int snap_shot_len = 0;
char* response_snap_shot;
int response_snap_shot_len = 0;
char* transcript_snap_shot;
int  transcript_snap_shot_len = 0;

char text_font[512];


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


// standard scheme functions and macros
static const char g_scheme[] =
"abs " 
"and any append append! apply assoc assq assv begin boolean? "
"caaaar caaadr caaar caadar caaddr caadr caar cadaar cadadr cadar caddar cadddr caddr cadr car "
"case case-lambda call-with-values catch "
"cdaaar cdaadr cdaar cdadar cdaddr cdadr cdar cddaar cddadr cddar cdddar cddddr cdddr cddr cdr "
"char? char=? char>? char<? char<=? char>=? char->integer "
"cond cons cons* define define-record-type define-syntax denominator "
"display do dotimes elseeq? eof-object? eqv? eval exact? expt "
"field fields filter find first floor for format for-each format from "
"hash "
"if immutable import in inexact? integer? integer->char iota "
"lambda last-pair length let let* letrec let-rec list list? list-ref  list->string list-tail "
"map max member memq memv min mod mutable "
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

// custom 'native' commands that have been added by this app.
static const char g_scheme2[] =
"add-clear-image add-draw-rect add-draw-ellipse add-draw-sprite  add-fill-colour add-fill-ellipse "
"add-fill-linear-ellipse add-fill-linear-rect "
"add-fill-radial-rect add-fill-radial-ellipse "
"add-fill-rect add-draw-line add-line-colour add-pen-width add-render-sprite "
"add-select-linear-brush add-select-radial-brush "
"add-scaled-rotated-sprite add-write-text "
"after api-call " 
"batch-clear-active "
"batch-draw-ellipse batch-draw-line batch-draw-rect batch-draw-scaled-rotated-sprite "
"batch-draw-sprite batch-fill-ellipse batch-fill-rect batch-render-sprite batch-render-sprite-scale-rot "
"batch-write-text "
"clear-image clear-transcript clear-sprite-command clear-sprite-commands "
"draw-batch draw-into-sprite draw-ellipse draw-line draw-rect draw-scaled-rotated-sprite draw-sprite " 
"every eval->string eval->text " 
"font fill-colour fill-ellipse fill-linear-ellipse fill-linear-rect "
"fill-radial-ellipse fill-radial-rect fill-rect graphics-keys "
"free-sprite free-sprites "
"identity image-size "
"keyboard-delay "
"line-colour linear-gradient load-sound load-sprites make-sprite "
"pen-width play-sound radial-gradient render render-sprite render-sprite-scale-rot release restart-engine rotate  "
"safely select-linear-brush select-radial-brush "
"set-draw-sprite set-every-function set-linear-brush set-radial-brush  show sprite-size stop-every "
"web-message write-text ";

static const char g_scheme3[] =
" ";
static const char g_scheme4[] =
"#f #t '()";

struct s_scintilla_colors
{
	int         iItem;
	COLORREF    rgb;
	int			size;
	char*		face;
	bool		bold;
	bool		italic;
};

const int fsz = 10;

static s_scintilla_colors scheme[] =
{	//	item					colour					sz	face	bold	italic
	{ SCE_LISP_DEFAULT,         RGB(  10,  10,  10),	fsz, text_font,	false, false },
	{ SCE_LISP_COMMENT,         RGB(  20,  20, 120),	fsz, text_font,	false, true },
	{ SCE_LISP_NUMBER,          RGB(  30,  30, 130),	fsz, text_font,	false, false },
	{ SCE_LISP_KEYWORD,         RGB(  40,  40, 110),	fsz, text_font,	true, false },
	{ SCE_LISP_KEYWORD_KW,      RGB(  50,  50, 180),	fsz, text_font,	false, false },
	{ SCE_LISP_SYMBOL,          RGB(  60,  60, 110),	fsz, text_font,	false, false },
	{ SCE_LISP_STRING,          RGB(  70,  80, 110),	fsz, text_font,	false, false },
	{ SCE_LISP_STRINGEOL,       RGB(  80,  80, 110),	fsz, text_font,	false, false },
	{ SCE_LISP_IDENTIFIER,      RGB(  25,  20, 110),	fsz, text_font,	false, false },
	{ SCE_LISP_OPERATOR,		RGB(  90,  90, 110),	fsz, text_font,	false, false },
	{ SCE_LISP_SPECIAL,		    RGB(  95,  90, 115),	fsz, text_font,	false, false },
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

	send_editor(h, SCI_SETBUFFEREDDRAW, 0);
	send_editor(h, SC_TECHNOLOGY_DIRECTWRITERETAIN, 1);

	send_editor(h, SCI_SETCODEPAGE, SC_CP_UTF8);
	send_editor(h, SCI_SETLEXER, SCLEX_LISP);

	// Set tab width
	send_editor(h, SCI_SETTABWIDTH, 2);

	// line wrap (avoid h scroll bars)
	send_editor(h, SCI_SETWRAPMODE, 1);
	send_editor(h, SCI_SETWRAPVISUALFLAGS, 2);
	send_editor(h, SCI_SETWRAPSTARTINDENT, 6);


	// auto sel
	send_editor(h, SCI_AUTOCSETSEPARATOR, ',');
	send_editor(h, SCI_AUTOCSETMAXHEIGHT, 9);

	// dwell/ used for brackets
	send_editor(h, SCI_SETMOUSEDWELLTIME, 500);
	set_a_style(h, STYLE_DEFAULT, blue, RGB(0xFF, 0xFF, 0xEA), fsz+1, text_font);

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

	set_a_style(h, STYLE_BRACELIGHT, blue, RGB(0xFF, 0xFF, 0xEA), fsz+1, text_font);
	set_a_style(h, STYLE_BRACEBAD, red, RGB(0xFF, 0xFF, 0xEA), fsz + 1, text_font);


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
	send_editor(h, SCI_SETTABWIDTH, 2);

	// line wrap (avoid h scroll bars)
	send_editor(h, SCI_SETWRAPMODE, 1);
	send_editor(h, SCI_SETWRAPVISUALFLAGS, 2);
	send_editor(h, SCI_SETWRAPSTARTINDENT, 6);

	// auto sel
	send_editor(h, SCI_AUTOCSETSEPARATOR, ',');
	send_editor(h, SCI_AUTOCSETMAXHEIGHT, 9);

	// dwell/ used for brackets
	send_editor(h, SCI_SETMOUSEDWELLTIME, 500);
	set_a_style(h, STYLE_DEFAULT, gray, RGB(0xFF, 0xFF, 0xEA), fsz + 1, text_font);

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

	set_a_style(h, STYLE_BRACELIGHT, orange, RGB(0xFF, 0xFF, 0xEA), fsz, text_font);
	set_a_style(h, STYLE_BRACEBAD, red, RGB(0xFF, 0xFF, 0xEA), fsz, text_font);

	send_editor(h, SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(g_scheme));
	send_editor(h, SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(g_scheme2));
	send_editor(h, SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(g_scheme3));
	send_editor(h, SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(g_scheme4));

	send_editor(h, SCI_ENSUREVISIBLEENFORCEPOLICY, 2);
	send_editor(h, SCI_GOTOLINE, 2);
 

}


void initialize_transcript_editor(HWND h) {

	transcript = h;

	send_editor(h, SCI_SETBUFFEREDDRAW, 0);
	send_editor(h, SC_TECHNOLOGY_DIRECTWRITERETAIN, 1);

	send_editor(h, SCI_SETCODEPAGE, SC_CP_UTF8);
	// Set tab width
	send_editor(h, SCI_SETTABWIDTH, 2);

	// line wrap (avoid h scroll bars)
	send_editor(h, SCI_SETWRAPMODE, 1);
	send_editor(h, SCI_SETWRAPVISUALFLAGS, 2);
	send_editor(h, SCI_SETWRAPSTARTINDENT, 6);

	// auto sel
	send_editor(h, SCI_AUTOCSETSEPARATOR, ',');
	send_editor(h, SCI_AUTOCSETMAXHEIGHT, 9);

	// dwell
	send_editor(h, SCI_SETMOUSEDWELLTIME, 500);
	set_a_style(h, STYLE_DEFAULT, gray, RGB(0xFF, 0xFF, 0xEA), fsz, text_font);

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
// this just adds brace highlighting; kind of essential for scheme.

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
	if (cmd != nullptr) memset(cmd, 0, l);
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

void set_inputed(char* s)
{
	sc_setText(inputed, s);
}

ptr get_inputed()
{
	if (inputed == nullptr) return Sstring("");
	const int l = send_editor(inputed, SCI_GETLENGTH) + 1;
	auto cmd = new(std::nothrow) char[l];
	if(cmd!=nullptr) memset(cmd, 0, l);
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

	if(transcript!=nullptr)
		sc_appendText(transcript, s);
}
 
void appendTranscriptNL(char* s)
{
	if (transcript != nullptr) {
		sc_appendText(transcript, s);
		sc_appendText(transcript, "\r\n");
	}
}

// called from scheme engine.
void appendTranscript1(char* s)
{
	if (transcript != nullptr) {
		const int l = strlen(s);
		send_editor(transcript, SCI_APPENDTEXT, l, reinterpret_cast<LPARAM>(s));
	}
}

void display_status(char* s)
{
	HWND h = Utility::get_main();
	::PostMessageA(h, WM_USER + 500, (WPARAM)0, (LPARAM)s);
}

void appendTranscriptln(char* s)
{
	std::string text(s);
	text += "\r\n";
	send_editor(transcript, SCI_APPENDTEXT, text.length(), reinterpret_cast<LPARAM>(text.c_str()));
}

void appendTranscript2(char* s, char* s1)
{
	if (transcript != nullptr) {
		sc_appendText(transcript, s);
		sc_appendText(transcript, " ");
		sc_appendText(transcript, s1);
	}
}

void clear_transcript()
{
	if (transcript != nullptr) {
		send_editor(transcript, SCI_CLEARALL);
	}
}

ptr cleartranscript() {
	if (transcript != nullptr) {
		send_editor(transcript, SCI_CLEARALL);
	}
	return Strue;
}
 
void add_transcript_commands() {
	Sforeign_symbol("eval_respond", static_cast<void*>(appendEditor));
	Sforeign_symbol("append_transcript", static_cast<void*>(appendTranscript1));
	Sforeign_symbol("append_transcript_ln", static_cast<void*>(appendTranscriptln));
	Sforeign_symbol("cleartranscript", static_cast<void*>(cleartranscript));
	Sforeign_symbol("setInputed", static_cast<void*>(set_inputed));
	Sforeign_symbol("getInputed", static_cast<ptr>(get_inputed));
	Sforeign_symbol("display_status", static_cast<ptr>(display_status));
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

// code fonts
bool hack_available = false;
bool consolas_available = false;
bool DejaVu_Sans_Mono_available = false;
bool Droid_Sans_Mono_available = false;
bool font_3270_available = false;



int CALLBACK EnumFontFamExProc(
	ENUMLOGFONTEX* lpelfe,
	NEWTEXTMETRICEX* lpntme,
	DWORD FontType,
	LPARAM lParam
)
{
	std::wstring font_face(lpelfe->elfFullName);

	if (font_face.find(L"Hack") != std::string::npos)
		hack_available = true;

	if (font_face.find(L"DejaVu Sans Mono") != std::string::npos)
		DejaVu_Sans_Mono_available = true;

	if (font_face.find(L"Droid Sans Mono") != std::string::npos)
		Droid_Sans_Mono_available = true;

 
	consolas_available = true;

	if (font_face.find(L"3270") != std::string::npos)
		font_3270_available = true;

	return 1;
}

void test_for_code_fonts() {
	LOGFONT lf;
	lf.lfFaceName[0] = '\0';
	lf.lfCharSet = DEFAULT_CHARSET;
	HDC hDC = GetDC(NULL);
	EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)&EnumFontFamExProc, 0, 0);
	ReleaseDC(NULL, hDC);
}


int select_code_font() {

	test_for_code_fonts();
	consolas_available = true;

 
	if (hack_available==true) {
		strncpy(text_font, "Hack", 5);
		return 0;
	}
	if (DejaVu_Sans_Mono_available==true) {
		strncpy(text_font, "DejaVu Sans Mono", 17);
		return 1;
	}
	if (Droid_Sans_Mono_available==true) {
		strncpy(text_font, "Droid Sans Mono", 16);
		return 2;
	}
	if (font_3270_available == true) {
		strncpy(text_font, "ibm3270", 8);
		return 3;
	}

	strncpy(text_font, "Consolas", 9);
	return 4;
}

void CViewText::SetFont(char* s)
{
	strncpy(text_font, s, 1+strlen(s));
	initialize_editor(inputed);
}



void CViewText::UpdateMenus(HMENU menu)
{
	test_for_code_fonts();
	consolas_available = true;


	if (hack_available == false)
		::EnableMenuItem(menu, ID_FONTS_HACK, MF_DISABLED);
	else 
		::EnableMenuItem(menu, ID_FONTS_HACK, MF_ENABLED);

	if (consolas_available == false)
		::EnableMenuItem(menu, ID_FONTS_CONS, MF_DISABLED);
	else
		::EnableMenuItem(menu, ID_FONTS_CONS, MF_ENABLED);

	if (DejaVu_Sans_Mono_available == false)
		::EnableMenuItem(menu, ID_FONTS_DEJAVI, MF_DISABLED);
	else
		::EnableMenuItem(menu, ID_FONTS_DEJAVI, MF_ENABLED);

	if (font_3270_available == false)
		::EnableMenuItem(menu, ID_FONTS_IBM3270, MF_DISABLED);
	else
		::EnableMenuItem(menu, ID_FONTS_IBM3270, MF_ENABLED);

	if (Droid_Sans_Mono_available == false)
		::EnableMenuItem(menu, ID_FONTS_DROID, MF_DISABLED);
		else 
		::EnableMenuItem(menu, ID_FONTS_DROID, MF_ENABLED);



}



void CViewText::OnInitialUpdate()
{
	HWND h = this->GetHwnd();
	select_code_font();
	initialize_editor(h);

	if (snap_shot == nullptr) return;
	send_editor(h, SCI_SETTEXT, snap_shot_len, reinterpret_cast<LPARAM>(snap_shot));
	snap_shot_len = 0;
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
	else {
		INPUT ip;
		ip.type = INPUT_KEYBOARD;
		ip.ki.wScan = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
		ip.ki.wVk = 0;
		ip.ki.dwFlags = 0;
		ip.ki.time = 0;
		ip.ki.dwExtraInfo = 0;
		ip.ki.dwFlags = KEYEVENTF_SCANCODE;
		SendInput(1, &ip, sizeof(INPUT));
		ip.ki.wScan = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
		SendInput(1, &ip, sizeof(INPUT));
		Sleep(30);
		ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
		SendInput(1, &ip, sizeof(INPUT));
		ip.ki.wScan = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
		SendInput(1, &ip, sizeof(INPUT));
	}
}

void CViewText::Eval(HWND hwnd)
{
	eval_scite(hwnd);
}

// used in messages
std::wstring loaded_file_name(L"");
std::wstring loaded_message(L"");
std::wstring saving_message(L"");
std::wstring saved_message(L"");

void CViewText::LoadFile(char* fname)
{
	sc_setEditorFromFile(fname);
}

void CViewText::LoadFile(std::wstring fname)
{
	loaded_file_name = fname;
	ClearAll(inputed);
	Sleep(10);
	std::ifstream f(loaded_file_name);
	std::stringstream buffer;
	buffer << f.rdbuf();
	buffer.seekg(0, std::ios::end);
	const int size = buffer.tellg();
	send_editor(inputed, SCI_SETTEXT, size, reinterpret_cast<LPARAM>(buffer.str().c_str()));
	Sleep(10);
	f.close();
	if (!loaded_file_name.empty()) {
		loaded_message = fmt::format(L"Loaded file: {} size: {} bytes", loaded_file_name, size);
		HWND h = Utility::get_main();
		::PostMessageW(h, WM_USER + 500, (WPARAM)0, (LPARAM)loaded_message.c_str());
	}
	Sleep(10);

}

void CViewText::NewFile()
{
	loaded_file_name = L"";
	ClearAll(inputed);
}

void CViewText::SaveFile() {

	if (loaded_file_name.empty()) {
		return;
	}

	saving_message = fmt::format(L"Saving file: {}", loaded_file_name);
	HWND h = Utility::get_main();
	::PostMessageW(h, WM_USER + 500, (WPARAM)0, (LPARAM)saving_message.c_str());
	Sleep(10);

	const auto l = send_editor(inputed, SCI_GETLENGTH) + 1;
	if (l == 0) {
		return;
	}

	auto* text_data = new(std::nothrow) char[l];
	if (text_data == nullptr) {
		return;
	}
	memset(text_data, 0, l);
	send_editor(inputed, SCI_GETTEXT, l, reinterpret_cast<LPARAM>(text_data));
	DWORD bytesWritten;

	HANDLE hFile;
	hFile = CreateFile(loaded_file_name.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	WriteFile(hFile, text_data, l, &bytesWritten, NULL);
	Sleep(10);
	CloseHandle(hFile);
	Sleep(10);
	saved_message = fmt::format(L"Saved file: {} size: {} bytes.", loaded_file_name,(int)bytesWritten);
	::PostMessageW(h, WM_USER + 500, (WPARAM)0, (LPARAM)saved_message.c_str());
	Sleep(10);
	delete[] text_data;
 

}

void CViewText::SaveFileAs(std::wstring fname)
{
	loaded_file_name = fname;
	const auto l = send_editor(inputed, SCI_GETLENGTH) + 1;

	if (l == 0) {
		return;
	}

	saving_message = fmt::format(L"Saving file AS: {}", loaded_file_name);
	HWND h = Utility::get_main();
	::PostMessageW(h, WM_USER + 500, (WPARAM)0, (LPARAM)saving_message.c_str());
	Sleep(10);

	auto* text_data = new(std::nothrow) char[l];
	if (text_data == nullptr) {
		return;
	}
	memset(text_data, 0, l);
	send_editor(inputed, SCI_GETTEXT, l, reinterpret_cast<LPARAM>(text_data));
	DWORD bytesWritten;

	HANDLE hFile;
	hFile = CreateFile(loaded_file_name.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	WriteFile(hFile, text_data, l, &bytesWritten, NULL);
	Sleep(100);
	CloseHandle(hFile);

	saved_message = fmt::format(L"Saved file AS: {} size: {} bytes.", loaded_file_name, (int)bytesWritten);
	::PostMessageW(h, WM_USER + 500, (WPARAM)0, (LPARAM)saved_message.c_str());
	Sleep(10);

	delete[] text_data;

 
}


void CViewText::transcriptln(char* s)
{
	appendTranscriptNL(s);
}



void CViewText::TakeSnapShot()
{

	snap_shot_len = send_editor(inputed, SCI_GETLENGTH) + 1;
	if (snap_shot_len == 0) {
		return;
	}
 
	if (snap_shot != nullptr) delete[] snap_shot;

	snap_shot = new(std::nothrow) char[snap_shot_len];
	if (snap_shot == nullptr) {
		return;
	}
	memset(snap_shot, 0, snap_shot_len);
	send_editor(inputed, SCI_GETTEXT, snap_shot_len, reinterpret_cast<LPARAM>(snap_shot));
	 
	response_snap_shot_len = send_editor(response, SCI_GETLENGTH) + 1;
	if (response_snap_shot_len == 0) {
		return;
	}
 
	if (response_snap_shot != nullptr) delete[] response_snap_shot;

	response_snap_shot = new(std::nothrow) char[response_snap_shot_len];
	if (response_snap_shot == nullptr) {
		return;
	}
	memset(response_snap_shot, 0, response_snap_shot_len);
	send_editor(response, SCI_GETTEXT, response_snap_shot_len, reinterpret_cast<LPARAM>(response_snap_shot));


	transcript_snap_shot_len = send_editor(transcript, SCI_GETLENGTH) + 1;
	if (transcript_snap_shot_len == 0) {
		return;
	}
	if (transcript_snap_shot != nullptr) delete[] transcript_snap_shot;

	transcript_snap_shot = new(std::nothrow) char[transcript_snap_shot_len];
	if (transcript_snap_shot == nullptr) {
		return;
	}
	memset(transcript_snap_shot, 0, transcript_snap_shot_len);
	send_editor(transcript, SCI_GETTEXT, transcript_snap_shot_len, reinterpret_cast<LPARAM>(transcript_snap_shot));


}

void CViewText::RestoreSnapShot()
{

 
}

std::string finding;
int find_pos = 0;
int search_flags;

void CViewText::ResetSearch() {
	find_pos = 0;
	finding = "";
}
void CViewText::SetSearchFlags(int flags) {
	search_flags = flags;
	send_editor(inputed, SCI_SETSEARCHFLAGS,flags);
}

void CViewText::Search(std::wstring s)
{
	int pos = 0;
	int flags = SCFIND_MATCHCASE | SCFIND_WHOLEWORD;
	send_editor(inputed, SCI_TARGETWHOLEDOCUMENT);
	std::string s1 = Utility::ws_2s(s);
	if (s1.compare(finding)!=0){
		finding = s1;
		find_pos = 0;
	}
	if ((s1.compare(finding) == 0) && (find_pos > 0)) {
		send_editor(inputed, SCI_SETTARGETSTART, find_pos);
	}
	pos = send_editor(inputed, SCI_SEARCHINTARGET, s1.length(), (LPARAM)s1.c_str());
	auto line = send_editor(inputed, SCI_LINEFROMPOSITION, pos);
	auto len = send_editor(inputed, SCI_GETLENGTH);
	send_editor(inputed, SCI_GOTOLINE, line);
	send_editor(inputed, SCI_SETSELECTIONSTART, pos);
	send_editor(inputed, SCI_SETSELECTIONEND, pos+s1.length());
	find_pos = pos + s1.length() + 1;
}


CDockText::CDockText()
{
	SetView(m_View);
	SetBarWidth(3);
}

void CViewResponseText::PreCreate(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("Scintilla");
}

void CViewResponseText::OnInitialUpdate()
{
	const auto h = this->GetHwnd();
	initialize_response_editor(h);
	if (response_snap_shot == nullptr) return;
	send_editor(h, SCI_SETTEXT, response_snap_shot_len, reinterpret_cast<LPARAM>(response_snap_shot));
	response_snap_shot_len = 0;


}

void CViewTranscriptText::PreCreate(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("Scintilla");
}

void CViewTranscriptText::OnInitialUpdate()
{
	const auto h = this->GetHwnd();
	initialize_transcript_editor(h);
	if (transcript_snap_shot == nullptr) return;
	send_editor(h, SCI_SETTEXT, transcript_snap_shot_len, reinterpret_cast<LPARAM>(transcript_snap_shot));
	transcript_snap_shot_len = 0;
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
	SetBarWidth(3);
}
 
CDockTranscriptText::CDockTranscriptText()
{
	// Set the view window to our edit control
	CDocker::SetView(m_View);

	// Set the width of the splitter bar
	SetBarWidth(3);
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