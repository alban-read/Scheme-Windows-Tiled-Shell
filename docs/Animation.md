## Animation

  [Index](welcome.html)    [Timers](Timers.html) [Textures](Images and Textures.html)

The general idea with computer animation is you show the viewer the latest frame and while they are looking at that, you draw the next one; behind the scenes.

There are three surfaces used by the image viewer; these are the display surface; the active surface and the screen surface. 

The active surface is used by all of the drawing commands; the display surface is only used to display the results.

When displaying the image view; a screen surface is also used; this is filled with a grid pattern first and then the display surface is drawn onto it; finally the combined image is then copied into the window.

 To show a picture on screen you need to swap in the surface you have been drawing on; the command is called show.

| (show 0) | used for animation; where every scene is completely redrawn. |
| -------- | ------------------------------------------------------------ |
| (show 1) | copies the current active surface so that a picture builds incrementally. |

### Moving circles 

```Scheme
  
;; example just draws circles.
 
(define circlecount 1000)

;; make a new circle
(define newcircle
  (lambda ()
    (list
      (list (random 800.0) (random 600.0))
      (list (- 5.0 (random 10.0)) (- 5.0 (random 10.0)))
      (list (random 1.0) (random 1.0) (random 1.0)))))

;; make n new circles
(define newcircles 
  (lambda (n) 
	(let ([l '()])
	  (dotimes n 
		(set! l (append l (list (newcircle))))) l)))

;; keep a list of circles
(define circles 
  (newcircles circlecount))

;; unjam any circle that is stuck	
(define unstickv 
 (lambda (v) 
	(list (if (= (car v) 0) (- 5.0 (random 10.0)) (car v))
		  (if (= (cadr v) 0) (- 5.0 (random 10.0)) (cadr v))))) 

(define count-offscreen
 (lambda ()
	(let ([count 0])
	 (for e in circles 
	  (when 
		(or 
		 (> (caar e) 800.0) 
		 (< (caar e) 0.0)
		 (> (cadar e) 600.0) 
		 (< (cadar e) 0.0))
			(set! count (+ count 1))))  count ))) 
			
(define few-circles
	(lambda ()
	 (>= (count-offscreen) (- circlecount 20))))



;; move all circles
(define move-circles
 (lambda (c)
 (list (map + (car c)(cadr c)) (unstickv (cadr c)) (caddr c))))

;; draw a circle
(define drawcirc
 (lambda (c) 
	(apply fill-colour (append (caddr c) (list 0.5)))
	(apply line-colour (append (caddr c) (list 1.0)))
    (apply fill-ellipse (append (car c) (list 50.0 50.0)))
	(apply draw-ellipse (append (car c) (list 50.0 50.0)))))


(define show-status
 (lambda ()
	(fill-colour 1.0 0.5 0.9 1.0)
	(font "Calibri" 32.0)
	(write-text 5.0 5.0 
		(string-append "Circles:"
		 (number->string
			(- circlecount (count-offscreen)))))))	
			 

;; perform one step
(define circle-step
 (lambda ()
	(fill-colour 0.0 0.0 0.0 1.0)
	(fill-rect 0.0 0.0 800.0 600.0)
	(map drawcirc circles)
	(when (few-circles) 
		(set! circles 
			(newcircles circlecount)))
	(set! circles (map move-circles circles))))
 
(circle-step) (show 0)
 
```

 By repeating 

```
(circle-step) (show 0)
```

The circles are animated.

## Automating the step function

In the code above we see that each step of the animation (each frame created) is performed by circle-step.

You can run the sequence one step at a time; single stepping it; using shift-return.

```Scheme
;; add more circles
(define circles 
  (newcircles 700))

;; single step
(circle-step)

;; a lot of steps
(dotimes 1000 
 (circle-step) (show 0))

```

This loop runs enough steps to display part of the animation.

It is normal for an animation or a game; to have a game loop.

The problem with running the steps in a loop is the timing of each frame will vary.

You can see this; when there are fewer circles the animation speeds up and they rush faster off the display.

The loop also prevents any other Scheme functions from running; killing off our interactivity.

For this reason there is a repeating timer that can be used schedule the step function automatically.

It also swaps the surfaces; *you must not use show inside the step function*; the step function always ends with an automatic show command.  

```Scheme
;; wait 1000ms run every 60ms; swap and clear.
(set-every-function 1000 60 0 
		(lambda ()
		  (circle-step)(gc)))
```

Note I added a call to **gc** garbage collect; otherwise garbage will rapidly build up from calling the function thousands of times.

The timer auto rotates the active and display images;  the Scheme function should longer swap them.

When running the circle animation in the viewer; you will probably run out of circles; however you can just recreate them and the animation will continue.

```Scheme
(define circles 
  (newcircles 700))
```

As  a rule the step function needs to be barebones; well-tested; simple and fast.

It is used to only run the animation; it should check for keys; update animation state.

if there is an error; it may well crash; ***the escape key will not work.***

The step function needs to complete quickly (it has to be fast to draw many frames in a second.)

### Stopping 

Setting the times to 0 causes the timer to be stopped :-

```
(set-every-function 0 0 0 '())
```

It is easier to type

```
(stop-every)
```

Assuming the step function does finish quickly; there is plenty of time left over to run other commands in-between; as above we could add more circles when we ran out.

Only one scheme command runs at a time; commands from the evaluator have to sneak into the spare time available between steps



 <script>
    function evaluate_selected_expression() {
        var text = window.getSelection().toString();
        text = text.replace(/[\u{0080}-\u{FFFF}]/gu, "");
        console.log(text);
        window.chrome.webview.postMessage('::eval:' + text);
    }
    document.addEventListener('keydown', function (event) {
        if (event.code == 'Enter' && (event.shiftKey)) {
            evaluate_selected_expression(); 
        }
    });
</script>

