## Release Notes

 [Index](welcome.html)  



August 2020 minimal Direct2D "Leading Edge" edition.

- Improved handling of escape key; putting safety above FPS.
- Added sound support.
- This version has been updated with the latest Win32++ library; and targets Windows 10 only.
- This version use the Windows 10 Edge/Chromium Web View 2
  - This does require you to have an very up to date version of Windows and Edge.
- This version uses Direct2D for hardware (GPU) accelerated graphics.
- Enhanced the graphics image pane.
- Added timers for animations.
- Tidied up a bit.
- Ripped out reams of library code; including a giant DLL glue library full of things only I am interested in.
- Redirected more functions that expect a terminal (tracing, timing, statistics.)
- Use one persistent scheme thread and a queue; instead of re-creating the thread.
- Added escape key termination DLL side.

Latest version of Cisco Chez Scheme is 9.5.3

- This app has been tested and built on Windows 10 versions *from the future*.
- Using computer languages from the past.