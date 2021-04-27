## Things needed to update the scheme engine for this project.

- A Windows C++ compiler.
  - get Visual Studio Community
     - from https://visualstudio.microsoft.com/vs/community/
   

A good way to install C++ libraries is to use vcpkg

There are many usful libraries under vcpkg that you might like to integrate into Scheme later.

- git clone https://github.com/microsoft/vcpkg
   - .\vcpkg\bootstrap-vcpkg.bat
   - .\vcpkg\vcpkg integrate install
   - .\vcpkg\vcpkg install scintilla:x64-windows
   - .\vcpkg\vcpkg install fmt:x64-windows


### Unmodified chez

Grab the binary code (lib and dll) from the chez installer

This binary code works in every respect *except it does not trap the windows escape key*

To just use the scheme from the installer:-

- You need to copy the .lib and DLL from the binary installation into the VCPKG tree.
- You need to copy the DLL to the project release folder. 
- The project will then build and run; however scripts can not be stopped if they run away; 


### Building Chez from source on windows.

- The app essentially works with unmodified binary files; provided by the installer.
- However you might want to modify scheme; if so you need to build it from source.


Building Chez on Windows is based on text here

https://github.com/cisco/ChezScheme/blob/main/BUILDING

Unfortunately it means that we need to install linux on windows to build scheme.


Since this experience also uses a terminal I recommend you install the new windows terminal from the App Store.
 

### Install WSL

Follow the tedious seven step process

https://docs.microsoft.com/en-us/windows/wsl/install-win10

This requires admin access; a reboot and installs an alien operating system onto your pc.

- wsl 1 is a very clever subsystem for windows; that emulates a Linux kernel on NT.
- wsl 2 is just a Linux virtual machine with some extra integration; it is faster at linux file access.

wsl --set-default-version [1 or 2] (I have tested with 1.)

### Fetch Ubuuntu from the store.

- You will need to select Ubuntu from the store to follow along.
- Once installed you will have ubuntu as an app; that starts a bash shell.
- You can select it easilly from the new windows terminal.

- On installation Ubuntu will unpack 
- Create a user id (mine is alban) and password; and store them in your secrets manager; 
- or write them in your notepad etc; you will need them later.

Start the ubuntu shell (I keep projects under c:\projects)

In the terminal

### change directory
cd /mnt/c/projects

### we do need the make utility

sudo apt install make

### get the source code for Chez from github

sudo git clone https://github.com/cisco/ChezScheme.git

# change the owner of the files to your own linux user (mine was alban)

cd Chez*

sudo chown -R alban .

### configure for windows

env OS=Windows_NT ./configure

### build using windows tools

env OS=Windows_NT make


I get some errors at the end of the build.

However ensure the necessary files (.dll, .lib, .boot) have been created here

C:\projects\ChezScheme\a6nt\bin\a6nt

and here

C:\projects\ChezScheme\a6nt\boot\a6nt

### Recap

We have used Linux to build a windows exe with visual C++

The exe runs here from the bash shell

/mnt/c/projects/ChezScheme/a6nt/bin$ ./scheme

or here from cmd

c:\projects\ChezScheme\a6nt\bin\a6nt>scheme


The whole point though is we want to modify scheme; to be interruptable in a GUI window.

We need to edit a file and recompile again.

For example add this minimal code; or the more expansive code in this repo.


```C++
DWORD WINAPI escape_watcher(LPVOID x)
{
    while (TRUE) {

        Sleep(1);
        if (GetAsyncKeyState(VK_ESCAPE) != 0)
        {
            tc_mutex_acquire()
                ptr ls;
            for (ls = S_threads; ls != Snil; ls = Scdr(ls)) {
                ptr tc = THREADTC(Scar(ls));
                KEYBOARDINTERRUPTPENDING(tc) = Strue;
            }
            tc_mutex_release()
        }
        Sleep(125);

    }
}

// .. update the windows init_signal_handlers code ..
static void init_signal_handlers() {

    HANDLE escape_key_thread = CreateThread(
        0,
        0,
        escape_watcher,
        0,
        0,
        0);
}

```

to schsig.c the chez signal handler; and make sure you are editing the a6nt\c files. 

Edit the init_signal_handler to run do_init_signal_handlers

When run this starts a thread that watches for the escape key; it also needs some script support.

Note there is scheme support for this in the script base.ss

```Scheme

;; when escape is pressed ctrl/c is 
;; simulated; that fires this handler
;; which uses the timer to raise
;; escape pressed shortly; when it is safe to do so.
;; (see code added-to-schsig.c)

(keyboard-interrupt-handler
  (lambda ()
    (define escape-pressed!
      (lambda ()
        (raise
          (condition
            (make-error)
            (make-message-condition "Escape pressed")))))
    (transcript0 "**ctrl/c**")
    (newline-transcript)
    (timer-interrupt-handler
      (lambda () (set-timer 0) (escape-pressed!)))
    (set-timer 20)))


```
This script (in theory) allows Scheme to watch for the escape key in its interrupt handler.

Interrupting a running thread mid-flow is generally fatal; which is why we go to all this trouble to let
scheme interrupt itself when safe to do so.


### Rebuild Chez scheme

Close all your terminals; exit scheme; and reopen ubuntu.


Now recompile the modified scheme; repeating the make step.

cd /mnt/c/projects/ChezScheme

chown -R alban .

env OS=Windows_NT make clean

env OS=Windows_NT make

### Check the dates are updated in here:-

C:\projects\ChezScheme\a6nt\bin\a6nt

C:\projects\ChezScheme\a6nt\boot\a6nt

We need to copy .lib and .dll files to the places they can be used.

On my system my projects look in vcpkg so I copy dlls to here

C:\projects\vcpkg\installed\x64-windows\bin

and copy .libs to here

C:\projects\vcpkg\installed\x64-windows\lib




The DLL for the scheme engine needs to be deployed e.g. added to release folder

C:\projects\Shell2020\applications\TiledShell\ProjectFiles\x64\Release\boot

The dll file from 

C:\projects\ChezScheme\a6nt\bin\a6nt

Needs to also go in the release folder

C:\projects\Shell2020\applications\TiledShell\ProjectFiles\x64\Release\

The .boot files from here 

C:\projects\ChezScheme\a6nt\boot\a6nt

Need to be copied to your release, debug and final distribution.


You only need to do this when a new major version of scheme is released on github and only if you modify the files.

Otherwise just copy the same files from the installed scheme.


After putting scheme into all the right folders:

Just use the project file to rebuild the shell app.

The shell.exe file created should actually work; it will tend to explode quickly if anything is wrong.







