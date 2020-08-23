#include "stdafx.h"
#include "Engine.h"
#include "scheme.h"
#include "ContainerApp.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <deque>
#include "Utility.h"
#include "Sound.h"
#include "Text.h"
#include "WebViewer.h"


#pragma comment(lib, "Shlwapi.lib")

#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)

ptr every(int delay, int period, int mode, ptr p);
ptr after(int delay, ptr p);

HANDLE script_thread = nullptr;
HANDLE g_script_mutex = nullptr;
HANDLE g_commands_mutex = nullptr;

HANDLE every_timer_queue = nullptr;
HANDLE h_every_timer = nullptr;
HANDLE after_timer_queue = nullptr;
ptr every_function;

std::wstring ws_status;

void setStatusBarText(int n, const char* s) {
	HWND h = Utility::get_main();
	auto ns = std::wstring(Utility::s2_ws(s));
	if (ws_status != ns) {
		ws_status = ns;
		if (h != nullptr)
			PostMessageW(h, WM_USER + 500, (WPARAM)n, (LPARAM)ws_status.c_str());
	}
}

void invalidate() {
	HWND h = Utility::get_main();
	PostMessageW(h, WM_USER + 501, (WPARAM)0, (LPARAM)0);
}

extern "C" __declspec(dllexport) ptr EscapeKeyPressed()
{
	if (GetAsyncKeyState(VK_ESCAPE) != 0)
	{
		return Strue;
	}
	return Sfalse;
}


void load_script_ifexists(const char* script_relative)
{
	auto dest = Utility::GetFullPathFor(Utility::widen(script_relative).c_str());

	if (!(INVALID_FILE_ATTRIBUTES == GetFileAttributes(dest.c_str()) &&
		GetLastError() == ERROR_FILE_NOT_FOUND))
	{
		CALL1("load", Sstring(Utility::ws_2s(dest).c_str()));
	}
}


void register_boot_file(const char* boot_file, bool& already_loaded)
{
 
	auto dest = Utility::GetFullPathFor(Utility::widen(boot_file).c_str());
	if (!already_loaded &&
		!(INVALID_FILE_ATTRIBUTES == GetFileAttributes(dest.c_str()) &&
			GetLastError() == ERROR_FILE_NOT_FOUND))
	{
		already_loaded = true;
		Sregister_boot_file( Utility::ws_2s(dest).c_str());
	}
}

static void custom_init()
{
}

void abnormal_exit()
{
	MessageBox(nullptr,
		L"Fatal error",
		L"Sorry to say; the App has died.",
		MB_OK | MB_ICONERROR);
	exit(1);
}

// we are running ahead of the GUI opening
DWORD WINAPI execstartup(LPVOID cmd)
{
	try
	{
 
		Sscheme_init(abnormal_exit);
		bool register_petite = false;
		bool register_cs = false;
		register_boot_file("\\boot\\petite.boot", register_petite);
		register_boot_file("petite.boot", register_petite);
		register_boot_file("\\boot\\scheme.boot", register_cs);
		register_boot_file("scheme.boot", register_cs);


		if (!register_cs && !register_petite)
		{
			MessageBox(nullptr,
				L"The BOOT FILES could not be loaded.",
				L"Looking for petite.boot and cs.boot",
				MB_OK | MB_ICONERROR);
			exit(1);
		}


		Sbuild_heap("DSCHEMEWSP2020.exe", custom_init);
		Sforeign_symbol("EscapeKeyPressed", static_cast<ptr>(EscapeKeyPressed));

		CD2DView::AddCommands();

		CViewText::Start();

		WebViewer::Start();

		Sforeign_symbol("every", static_cast<ptr>(every));
		Sforeign_symbol("after", static_cast<ptr>(after));

		// load scripts
		load_script_ifexists("\\scripts\\base.ss");
		load_script_ifexists("\\scripts\\init.ss");
		load_script_ifexists("\\scripts\\env.ss");

		CALL1("suppress-greeting", Strue);
		CALL1("waiter-prompt-string", Sstring(""));
		load_script_ifexists("\\scripts\\browser.ss");
		load_script_ifexists("\\scripts\\appstart.ss");

	}
	catch (const CException& e)
	{
		MessageBox(nullptr, e.GetText(), AtoT(e.what()), MB_ICONERROR);
		return 0;
	}
	return 0;
}
// runs on initial update
void post_gui_load_script()
{
	load_script_ifexists("\\scripts\\initialupdate.ss");
}


std::deque<std::string> commands;
// script execution; 
bool cancelling = false;

void eval_text(const char* cmd)
{
	WaitForSingleObject(g_commands_mutex, INFINITE);
	commands.emplace_back(cmd);
	ReleaseMutex(g_commands_mutex);
}

void safe_cancel_commands()
{
	cancelling = true;
	WaitForSingleObject(g_commands_mutex, INFINITE);
	while (!commands.empty())
	{
		commands.pop_front();
	}
	commands.shrink_to_fit();
	ReleaseMutex(g_commands_mutex);
	Sleep(250);
}

void init_commands();

DWORD WINAPI  process_commands(LPVOID x)
{
	execstartup((LPVOID)L"");
	Sound::Start();
	int ticks = 0;
	int pending_commands = 0;

	while (true) {

		if (GetAsyncKeyState(VK_ESCAPE) != 0)
		{
			while (!commands.empty())
			{
				commands.pop_front();
			}
			commands.shrink_to_fit();

			if (every_timer_queue != nullptr && h_every_timer != nullptr) {
				DeleteTimerQueueTimer(every_timer_queue, h_every_timer, NULL);
				every_timer_queue = nullptr; h_every_timer = nullptr;
			}
			CD2DView::Stop();
			ReleaseMutex(g_commands_mutex);
			ReleaseMutex(g_script_mutex);
			setStatusBarText(0, "Escape pressed.");
			invalidate();
			Sleep(1000);
		}
 
		WaitForSingleObject(g_script_mutex, INFINITE);
		if (commands.empty()) {
			setStatusBarText(0, "Ready.");
		}
		ReleaseMutex(g_script_mutex);
		while (commands.empty())
		{
			Sleep(25);
		}
		WaitForSingleObject(g_commands_mutex, INFINITE);
		std::string eval;

		if (!commands.empty()) {
			eval = commands.front();
			commands.pop_front();
			pending_commands = commands.size();
			if (pending_commands > 0) {
				setStatusBarText(0, fmt::format("Busy {0}", pending_commands).c_str());
			}
		}
		ReleaseMutex(g_commands_mutex);

		WaitForSingleObject(g_script_mutex, INFINITE);
		try {

			if (pending_commands == 0) {
				setStatusBarText(0, "Ready");
			}
			if (eval.c_str()[0] == 33) {
				CALL0(&eval.c_str()[1]);
			}
			else {
				CALL1("eval->string", Sstring_utf8(eval.c_str(), -1));
			}
			 
		}
		catch (...) {
			ReleaseMutex(g_script_mutex);
		}
		// run some gc
		ticks++;
		if (ticks % 10 == 0) {
			CALL0("gc");
		}
		ReleaseMutex(g_script_mutex);
		::Sleep(20);
	}
}


void init_commands() {

	// script exec background thread
	static auto cmd_thread = CreateThread(
		nullptr,
		0,
		process_commands,
		nullptr,
		0,
		nullptr);
}

void cancel_commands()
{
	cancelling = true;
	WaitForSingleObject(g_commands_mutex, INFINITE);
	while (!commands.empty())
	{
		commands.pop_front();
	}
	commands.shrink_to_fit();
	ReleaseMutex(g_commands_mutex);
	Sleep(250);
}


void stop_every() {
	if (every_timer_queue != nullptr && h_every_timer != nullptr) {
		DeleteTimerQueueTimer(every_timer_queue, h_every_timer, NULL);
		every_timer_queue = nullptr; h_every_timer = nullptr;
	}
}

ptr run_func(ptr f) {
	Slock_object(f);
	ptr result = Snil;
	WaitForSingleObject(g_script_mutex, INFINITE);
	try {
		if (Sprocedurep(f)) {
			result=Scall0(f);
			Sunlock_object(f);
		}
	}
	catch (const CException& e)
	{
		Sunlock_object(f);
		ReleaseMutex(g_script_mutex);
		return result;
	}
	ReleaseMutex(g_script_mutex);
	return result;
}

 

// no mutex, we are called from running scheme
ptr run_naked_func(ptr f) {
	Slock_object(f);
	ptr result = Snil;
	try {
		if (Sprocedurep(f)) {
			result = Scall0(f);
			Sunlock_object(f);
		}
	}
	catch (const CException& e)
	{
		Sunlock_object(f);
		return result;
	}
	return result;
}

int swap_mode;
VOID CALLBACK run_every(PVOID lpParam, BOOLEAN TimerOrWaitFired) {

	if (GetAsyncKeyState(VK_ESCAPE) != 0)
	{
		while (!commands.empty())
		{
			commands.pop_front();
		}
		commands.shrink_to_fit();

		if (every_timer_queue != nullptr && h_every_timer != nullptr) {
			DeleteTimerQueueTimer(every_timer_queue, h_every_timer, NULL);
			every_timer_queue = nullptr; h_every_timer = nullptr;
		}
		CD2DView::Stop();
		ReleaseMutex(g_commands_mutex);
		ReleaseMutex(g_script_mutex);
		setStatusBarText(0, "Escape pressed.");
		invalidate();
		Sleep(1000);
		return;
	}
	
	
	CD2DView::ScanKeys();
	WaitForSingleObject(g_script_mutex, INFINITE);
	try {

		CD2DView::Step(lpParam);
	}
	catch (const CException& e)
	{
		ReleaseMutex(g_script_mutex);
		stop_every();
		return;
	}
	ReleaseMutex(g_script_mutex);
	CD2DView::Swap(swap_mode);
}

void start_every(int delay, int period, ptr p) {

	stop_every();

	if (nullptr == every_timer_queue)
	{
		every_timer_queue = CreateTimerQueue();

		try {
			CreateTimerQueueTimer(&h_every_timer, every_timer_queue,
				static_cast<WAITORTIMERCALLBACK>(run_every), p, delay, period,WT_EXECUTEINTIMERTHREAD);
		}
		catch (const CException& e)
		{
			// Display the exception and quit
			MessageBox(nullptr, e.GetText(), AtoT(e.what()), MB_ICONERROR);
			return;
		}
	}
}

// only one thing runs every.
ptr every(int delay, int period, int mode, ptr p)
{
	swap_mode = mode;
	if (delay == 0 || period == 0) {
		stop_every();
		Sunlock_object(every_function);
	}
	else {

		Slock_object(p);
		every_function = p;
		if (Sprocedurep(p)) {
			start_every(delay, period, p);
			return Strue;
		}
		else {
			return Sfalse;
		}
	}
	return Strue;
}



VOID CALLBACK run_after(PVOID lpParam, BOOLEAN TimerOrWaitFired) {

	WaitForSingleObject(g_script_mutex, INFINITE);
	try {

		if (Sprocedurep(lpParam)) {
			Scall0(lpParam);
			Sunlock_object(lpParam);
		}

	}
	catch (const CException& e)
	{
		ReleaseMutex(g_script_mutex);
		return;
	}
	ReleaseMutex(g_script_mutex);

}


void start_after(int delay, ptr p) {
	HANDLE h_after_timer = nullptr;
	Slock_object(p);
	try {

		// do not recreate queue
		if (after_timer_queue == nullptr) {
			after_timer_queue = CreateTimerQueue();
		}
		// add event to queue.
		CreateTimerQueueTimer(&h_after_timer, after_timer_queue,
			static_cast<WAITORTIMERCALLBACK>(run_after), p, delay, 0, WT_EXECUTEINTIMERTHREAD);
	}
	catch (const CException& e)
	{
		// Display the exception and quit
		MessageBox(nullptr, e.GetText(), AtoT(e.what()), MB_ICONERROR);
		return;
	}
}

ptr after(int delay, ptr p)
{
	if (Sprocedurep(p)) {
		start_after(delay, p);
		return Strue;
	}
	return Sfalse;
}

void Engine::Eval(char* cmd)
{
	eval_text(cmd);
}


ptr Engine::CALL2byName(char* f, ptr a1, ptr a2)
{
	WaitForSingleObject(g_script_mutex, INFINITE);
	ptr p = Stop_level_value(Sstring_to_symbol(f));
	if (!Sprocedurep(p)) {
		ReleaseMutex(g_script_mutex);
		return Snil;
	}
	ptr result = Scall2(p, a1, a2);
	ReleaseMutex(g_script_mutex);
	return result;
}

void Engine::Start() {

	if (g_script_mutex == nullptr) {
		g_script_mutex = CreateMutex(nullptr, FALSE, nullptr);
	}
	if (g_commands_mutex == nullptr) {
		g_commands_mutex = CreateMutex(nullptr, FALSE, nullptr);
	}
 
	init_commands();
}

bool Engine::Spin(const int turns)
{ 
	return true;
}


void Engine::Stop()
{
	cancel_commands();
	stop_every();

}

ptr Engine::Run(ptr f)
{
	return run_func(f);
}

ptr Engine::RunNaked(ptr f)
{
	return run_naked_func(f);
}
