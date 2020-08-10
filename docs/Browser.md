 [Index](welcome.html)  

# Browser pane

This pane is used for documentation and help files.

This pane can contain code snippets like this

```Scheme

(define factorial
  (lambda (n)
 	(cond ((< n 0) #f)
          ((<= n 1) 1)
          (else (* n (factorial (- n 1)))))))

(factorial 10)

```



You can select and execute these using shift-return.

This pane can be useful for many things; such as displaying a help file; interacting with the programming scheme online book; or displaying some reports.  

This view is intended for documentation; although it could be adapted into an entire user interface.

The modern WebView2 is used; which is wonderful; except you currently need the very latest software to run it.

**Just for information:**

The script startup.js is loaded into every document that loads.

This script provides the shift return execution function; which sends an ::eval:: message to Scheme; allowing examples from the documentation to be run interactively.

It would be possible to also open a message channel from scheme to edge; that is not yet implemented here; since I am not all that interested in writing applications in JavaScript.

You do need to be on the Edge Beta channel for this app; for the time being; I assume that when the webview is mature; this will not be necessary; note that the edge beta channel is the safer of the early release channels.

