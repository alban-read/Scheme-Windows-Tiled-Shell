# Scheme-Windows-Tiled-Shell

This is a windows workspace for Chez Scheme.

## Reason

Provides a simple alternative to a terminal; creates a workspace type interface; prevents text rushing off the screen; places results and output into different panes from the script source.

- Uses tiled panes rather than overlapping windows
  - with several standard arrangements that rearrange the panes in a useful way.
- Provides a graphical output pane; for drawings and animations.
- I use versions of this application to write apps that script dozens of C libraries; Chez Scheme is very good at that.

## Fun things

You can work through the online Scheme programming book; executing the examples as you go.

Provides a simple tiled user interface for Chez Scheme on Windows 10.

#### Selfie

<img src='https://github.com/alban-read/Scheme-Windows-Tiled-Shell/blob/master/docs/selfie.PNG'> 

- I hope that programming can be fun again in a fast interactive workspace.
- Create animations using direct 2D accelerated graphics.

#### Docs

- The Wiki here https://github.com/alban-read/Scheme-Windows-Tiled-Shell/wiki
- The Wiki is meant to be used from the documentation pane of the application.

----

### Requirements

In order to work - this application requires Windows 10; the new Webview2 and the visual C++ 2019 run time libraries.

If your run this app; and the documentation panes are blank; just use the installer from Microsoft for the new WebView2 here:-

https://developer.microsoft.com/en-us/microsoft-edge/webview2/

If running the app does nothing at all; it does not open; you probably do not have the latest C runtime installed; which you can get from Microsoft here:-

[Visual C Redistributable](https://support.microsoft.com/en-gb/help/2977003/the-latest-supported-visual-c-downloads)



### Components used



## Chez Scheme

The famous fast and reliable Scheme interpreter and compiler.

https://github.com/cisco/ChezScheme

### Win32++

Win32++ is used for the windows; dockers etc.

[http://win32-framework.sourceforge.net](http://win32-framework.sourceforge.net/)

### Scintilla

Scintilla editor is used for the text/editor views. http://www.scintilla.org/

**These are all open source components**

*Any (few) modifications are in the source code of the application.*

See the license in the documentation.



