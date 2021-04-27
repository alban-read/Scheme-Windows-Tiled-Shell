// added this to schsig.c 
// traps escape and turns it into the keyboard interrupt.
// this is then caught in base.ss and turned into an error.


//  add this

#ifdef WIN32

// this intercepts scheme when escape is pressed

HANDLE escape_key_thread;

void DisplayEscapeMessageBox()
{
  int msgboxID = MessageBoxA(
      NULL,
      "Send control C to scheme?",
      "Escape Pressed.",
      MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1 | MB_SYSTEMMODAL);

  switch (msgboxID)
  {
  case IDYES:

    tc_mutex_acquire()

        ptr ls;
    for (ls = S_threads; ls != Snil; ls = Scdr(ls))
    {
      ptr tc = THREADTC(Scar(ls));
      KEYBOARDINTERRUPTPENDING(tc) = Strue;
      SOMETHINGPENDING(tc) = Strue;
    }

    tc_mutex_release()

        break;
  case IDNO:
    break;
  }
}

DWORD WINAPI escape_watcher(LPVOID x)
{
  while (TRUE)
  {
    Sleep(125);
    if (GetAsyncKeyState(VK_ESCAPE) != 0)
    {
      Sleep(400); // detect a long key press on escape ..
      if (GetAsyncKeyState(VK_ESCAPE) != 0)
        DisplayEscapeMessageBox();
    }
  }
}
#endif

//  ...

// arrange for thread to be started by the windows init_signal_handler below.
 static void init_signal_handlers()
  {
    SetConsoleCtrlHandler(handle_signal, TRUE);
    HANDLE escape_key_thread = CreateThread(
        0,
        0,
        escape_watcher,
        0,
        0,
        0);
  }
  
  //  ...