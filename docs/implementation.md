## Implementation

 [Index](welcome.html)  



#### Scheme 

Supports functional and imperative programming and allows for efficient recursive algorithms.

In 1975 Gerald J. Sussman and Guy L. Steele Jr. began experimenting; this led to the publication during 1976–1978 of a series of papers describing a new dialect of Lisp called Scheme.

In 1998, Sussman and Steele remarked that the minimalism of Scheme was not a conscious design goal, but rather the unintended outcome of the design process. "We were actually trying to build something complicated and discovered, serendipitously, that we had accidentally designed something that met all our goals but was much simpler than we had intended....we realized that the lambda calculus—a small, simple formalism—could serve as the core of a powerful and expressive programming language."

Very modern features of scheme are lexical scoping; first class functions; closures; and continuations.

#### Chez Scheme 

Chez Scheme supports all standard features of Scheme, including first-class procedures, proper treatment of tail calls, continuations, user-defined records, libraries, exceptions, and hygienic macro expansion.

Chez Scheme also includes extensive support for interfacing with C and other languages, support for multiple threads possibly running on multiple cores, non-blocking I/O, and many other features.

The Chez Scheme implementation consists of a compiler, run-time system, and programming environment. Although an interpreter is available, all code is compiled by default. Source code is compiled on-the-fly when loaded from a source file or entered via the shell. A source file can also be precompiled into a stored binary form and automatically recompiled when its dependencies change. Whether compiling on the fly or precompiling, the compiler produces optimized machine code, with some optimization across separately compiled library boundaries. The compiler can also be directed to perform whole-program compilation, which does full cross-library optimization and also reduces a program and the libraries upon which it depends to a single binary.

The run-time system interfaces with the operating system and supports, among other things, binary and textual (Unicode) I/O, automatic storage management (dynamic memory allocation and generational garbage collection), library management, and exception handling. By default, the compiler is included in the run-time system, allowing programs to be generated and compiled at run time, and storage for dynamically compiled code, just like any other dynamically allocated storage, is automatically reclaimed by the garbage collector.

The R6RS core of the Chez Scheme language is described in [The Scheme Programming Language](http://www.scheme.com/tspl4/), which also includes an introduction to Scheme and a set of example programs. Chez Scheme's additional language, run-time system, and programming environment features are described in the [Chez Scheme User's Guide](http://cisco.github.io/ChezScheme/csug9.5/csug.html). The latter includes a shared index and a shared summary of forms, with links where appropriate to the former, so it is often the best starting point.

[contents](#contents)

------

### <a name="implementation">Implementation of this program</a>

**Keeping up to date**

- This is the most modern 'bleeding edge' version of the application at the time of writing.
  - That does mean you need to be using the Edge Browser from the BETA channel; despite the name this is the 2nd most conservative and safe channel.
- The version of scheme (kernel and boot files) used; can be unmodified; the app will work with the DLLS from the Chez Scheme windows binary installer.
  - The version included has had two functions added to the kernel to translate the escape key into a control/c interrupt; and that is the only change
-  vcpkg a Microsoft c++ package manager is used for other library code.
-  nupkg is used for operating system code; direct 2D; edge browser; whatever helpers these all need.
- There are solutions for the C++ projects that run in Visual Studio 2017-2019; they assume vcpkg exists .



##### App

The workspace app is written in C++ using the Win32cpp (Win32++)  library.

This adds human oriented interactive I/O to the Scheme engine; which otherwise uses a terminal.

The App provides views as arrangeable panes or tiles for text, graphics, documentation views.

And connects to Scheme

From the C++ worlds perspective

- The app is a workspace that allows you to write and run Scheme code.

In the real world I tend to link in DLLs from other C libraries and create scheme libraries.

When doing that you can use Scheme either as a way of automating C++ functions; or you can regard the C++ app as a number of functions that act as extensions  to Scheme.

A program is either a Script in Scheme or a C++ application with Scheme scripting; either view works.

##### Composition of the app

This App uses Win32cpp a Win32 library (it is 64 bit; the 32 is the name of the Windows API); this App includes only view related things; like the scintilla text editor; the Direct2D graphics pane; and the windows browser pane.

Chez Scheme designed for the terminal; is composed of an application; a shared library kernel; and boot files (containing compiled scheme code and data.)

This application replaces the windows console exe; with a windows GUI exe; keeping the shared library kernel and boot files; to run the Scheme compiling interpreter.

Some challenges are that the kernel and boot file code; still think they are running in a terminal; so various streams need to be redirected; keyboard interrupts are awkward; interactive debugger and expeditor are not supported (yet) and so on.

##### binding to engine

The scheme engine is linked in directly and called on a thread; functions have been added to scheme in the extension libraries; so that the app has functions in scheme to talk to.

- The app calls into scheme functions.

As each scheme expression is invoked; a call is made to scheme on a scheme thread.

- Only a single scheme thread is active. 
- The Scheme script from the evaluation pane is sent to the Scheme thread.
- The App has several of its own threads; so you can carry on editing while scheme runs the latest expression.
- A queue of expressions may build up for the scheme thread.

##### Garbage collection in the App

Scheme is a a garbage collected language; lots of little items get created all the time; and they are automatically deleted when no longer needed. Chez Scheme has the interesting and famous big bag of pages garbage collector.

When using the App; the Apps threads are always active while the scheme engine is probably doing nothing a lot of the time; it is not running all the time in an idle loop; as it does when it runs in a terminal.

Periodically the App asks scheme (when it is idle) to wake up and do some garbage collection; this call actually triggers the heuristics in the Scheme engine; that triggers collection after N function calls. 

This is because garbage can build up whenever a Scheme function runs; and while scheme is not constantly running to tidy up after itself. 

This means in the App the memory usage can be very `bursty`; it is smoother in the terminal; as the collector is running more frequently in the terminal; and less frequently in the App.

None the less as you run the GUI you will see the memory is fairly aggressively repossessed over time.

This also the reason why the CPU usage of the app seldom drops to 0; it often has (a very small) CPU overhead; as it wakes up the Scheme engine periodically.

Note that whenever a large scheme program is running; it is also collecting garbage; this applies especially to small functions that create a large number of objects.

There is a thread in the App that asks Scheme to tidy up periodically. 

[contents](#contents)

------

 

------

#### Working in the Workspace 

The application displays a workspace;  with a fixed number of tiled panes.

You can move these; and change their size; they never overlap; this is intentional.

Typically these panes contain text; there is also browser pane; meant for documentation.

There is an image pane; that displays graphics; It is like a scratch pane; for when you are working on a graphical function.

The concept here is not to create an IDE or even a replace an editor; the concept is to create an interactive workspace-like alternative to a terminal. On Mac OS classic there were workspaces and *programmers workbenches* long before there were terminals in OSX; and *I preferred those.*

Unlike a terminal - in a workspace results are captured rather than flying past the screen; there is a separate pane for storing messages from functions; distinct from a pane for results from functions and so on.

You can do things like select *parts of a function* and evaluate them; using  shift return.

There is one scheme interpreter in the application; not one per thread; when you evaluate an expression that creates a variable or defines a new function; or load a whole script; that is added to your Scheme environment; the next time you evaluate an expression it is still there waiting for you; so your program can be gradually; interactively written and tested step by step.

------

##### Implementation of the App

This is a 64bit; Win32 application; based on the very nice Win32cpp library.

- Win32cpp is a MIT licensed open source library; and has only been slightly changed. 
- It is a light weight way to write Windows applications in C++.
- The library provides the tiled docking container.

The application contains docks within tiled views; I think of these as panes in a tiled workspace; you can move them around; and change their size; they do not overlap; this is intentional.

The Scintilla text editor component is used for the text views; Scintilla is a programmable text editor for embedding in applications; as a pure text editor; it should be quick.

This workspace is bound to the Scheme engine 

As the application starts; it initializes scheme; and loads some scripts; extends scheme with any extra DLL and library code and so on. 

Scheme is part of this application; albeit embedded in a DLL.

In the scheme library there is a function the GUI uses to evaluate an expression.

- This function is an interpreter; it evaluates each expression sent over from the expression evaluator window.
- This evaluation happens each time in a new thread; so it does not block the windows UI for the App.

[contents](#contents)

------

#### Recompile this application

Some of the functions in this code; were fairly old C code; and some use quite new modern C++ functions; it contains parts written years apart; in different styles; and merged together; so not ideal; I am working to modernize it; using the latest production versions of C/C++ available. 

Obviously writing C++ is not my thing; I like Scheme.

None the less it is not hard to compile this App; using the visual studio solution. 

It can be harder to link in all the dependencies; the good news is that I choose only library code that is available in vcpkg or that is now part of the base OS from Nuget.

You need to install vpckg; download and compile a couple of libraries; (re2, scilexer, fmt) and visual studio will find it.

To compile scheme; you need scheme; all the scheme parts; can be taken directly from the Windows binary Chez Scheme installer; I am using the **non threaded x64 version**; the Scheme parts can be completely unmodified; however the version shipped has a change to catch the escape key.

#### Supporting more threads

I would like to support the threaded version of Scheme; which was tagged as experimental for windows; when this project started.

I have written some multi-threaded windows applications before; and the combination of windows, threads and C/C++ is not safe. Especially when it comes to *not crashing and losing all of your work*; which is a feature I value even more highly than speed.

Obviously we can run more than one process; the scheme.exe file can run scripts; so spawning multiple processes is very possible; running multiple scheme engines in their own threads is also interesting to explore; although it looks difficult.

 

------

#### The ecological imperative for using C++ and Lisp

Stop killing the planet by running dog slow interpreters on cloud computing super clusters.

![](environmentalism.png)



Or if you are in business 'stop wasting money.'



**Performance**

Cisco Chez Scheme is the products of decades of work; and has a fast optimizing nano-pass-compiler; that converts a scheme library to fast machine code.

The best speed comes from the use of compiled scheme library code; see the user guide; linked from the help page.

Even scheme that is just evaluated at the top level is very fast.

<https://github.com/cisco/ChezScheme>

Code defined in a scheme library; is faster than code in the top-level; although all code is compiled; the top level is not a static environment so it is slower.

 You can use the scheme time function to time your functions.





