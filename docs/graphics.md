## Graphics 

 [Index](welcome.html)  



#### <a name="image-pane">The image pane</a>

Graphics commands can be used to draw lines and shapes that can be displayed in the image pane.

There are a couple of examples in the scripts folder; you can run them to do some drawing.  

Also you can try out the examples in the documentation.

#### <a name="graphics-functions">Graphics functions</a>

These functions work on an 'offscreen' image in memory; the image pane is just a way to display them.

You can also create drawings and save them to a file; without showing them in the image pane.

Windows includes a graphics library (Direct2D)  that provides these drawing functions.

####  Showing the image pane

- If you have set Docking to Browser Layout;  you can select the image pane to the right.
- If not using the documentation: Set Docking to Image Layout.
- You can drag and drop an example from the scripts folder to the Evaluator view; such as drawtree.ss which will draw a tree shape.
- Or select a function from this browser documentation pane and shift-run it.

#### Warning

- On  high DPI screens; the image pane becomes very large.

 

------



## Graphics

##### A fractal fern

You can select and execute (shift-return) this script to create a fern-like image.

The image is drawn slowly using a timer for animation.

```Scheme

(image-size 300 500)

;; fractal fern 
(define x 150.0)
(define y 200.0)
(define x0 0.0)
(define y0 0.0)
;;

(define set-pixel
 (lambda (x y )
    (fill-colour 0.4 1.0 0.4 1.0)
	(fill-rect x y (+ x 0.5) (+ y 0.5))))

(define transform
  (lambda ()
    (set! x0 x)
    (set! y0 y)
    (let ([r (random 100)])
      (cond
        [(< r 1) (begin (set! x 0.0) (set! y (* 0.16 y0)))]
        [(< r 86)
         (begin
           (set! x (+ (* 0.85 x0) (* 0.04 y0)))
           (set! y (+ (* -0.04 x0) (* 0.85 y0) 1.6)))]
        [(< r 93)
         (begin
           (set! x (+ (* 0.2 x0) (* -0.26 y0)))
           (set! y (+ (* 0.23 x0) (* 0.22 y0) 1.6)))]
        [else
         (begin
           (set! x (+ (* -0.15 x0) (* 0.28 y0)))
           (set! y (+ (* 0.26 x0) (* 0.24 y0) 0.44)))]))))
;;
(define draw-point
  (lambda ()
    (set-pixel (+ (* x 50) 120) (- 500 (- (* y 50) 10)))))
 
(define fern
  (lambda () 
    (dotimes 1000 
      (transform) (draw-point))))

 
(clear-image 0.0 0.0 0.0 1.0)

;; draws the fern as a step function.
(set-every-function 1000 33 1 
		(lambda ()
		  (fern)(gc)))


```

  You can stop the display with

```Scheme
;; stop the fern with
(stop-every)
```

Which stops the function in the timer.

## Graphics 2D commands

- For 2D graphics Direct 2D provides smooth anti-aliased lines; using graphics acceleration.
- The app has a pane that displays an image.
- All graphics operations draw onto the active surface which is an **off-screen bitmap**.

------

There is a single active surface that all these commands draw on.

------

*Set the docking mode to image layout in the shell.*

#### Create  an image

An image always has a background paper colour or gradient.

Set the paper colour to an r,g,b,a (red,green,blue,alpha) value; then create a blank image

```scheme
 
(image-size 800 600) 
(show 0)

```

## Colours

- Colours are set for pens and for brushes.
- The colours are set using red, green, blue, and alpha values (0.0 to 1.0)
- Alpha is used to blend.

------

### Drawing  

### Lines

#### Line drawing

A line has a colour and a pen width

```scheme

(clear-image 0.0 0.0 0.0 1.0)
(line-colour 0.0 1.0 0.0 1.0)  
(pen-width 1.2)	 
(draw-line 10.0 10.0 200.0 200.0) ;; draw a line
(show 1)

```

#### Shape Drawing

The shape commands can draw a shape; using a line colour and pen width.

#### Rectangle drawing

```scheme
(clear-image 0.0 0.0 0.0 1.0)
(line-colour 0.8 0.4 0.8 1.0)   ;; purple
(set-pen-width 3.5)	   			;; thick line	
(draw-rect 10.0 10.0 40.0 40.0) ;; x,y, width height
(show 1)
```

#### Ellipse Drawing

```scheme
(line-colour 0.8 0.4 0.8 1.0)  
(set-pen-width 0.5) ;; narrow line
(draw-ellipse 80.0 80.0 50.0 50.0) ;; x,y, width, height
(show 1)
```

------

## Filled shapes

The shape commands can fill a shape; using a fill colour.

#### Ellipse Filling

```scheme
(fill-colour 0.8 0.4 0.8 1.0)  
(fill-ellipse 80.0 80.0 50.0 50.0) ;; x,y, width, height
(show 1)
```

------

#### Rectangle filling

```scheme
(clear-image 0.0 0.0 0.0 1.0)
(set-pen-width 3.5)	   			;; thick line	
(fill-rect 10.0 10.0 40.0 40.0) ;; x,y, width height
(show 1)
```

 

## Examples

### Koch

```Scheme
(define koch
  (lambda (x1 y1 x2 y2 i)
    (let* ([pi  3.1415926535]
          [a (/ pi 3.0)]
          [x3 (/ (+ (* x1 2.0) x2) 3.0)]
          [y3 (/ (+ (* y1 2.0) y2) 3.0)]
          [x4 (/ (+ x1 (* x2 2.0)) 3.0)]
          [y4 (/ (+ y1 (* y2 2.0)) 3.0)]
          [x5 (+ x3 (* (- x4 x3) (cos a))
                    (* (- y4 y3) (sin a)))]
          [y5 (- y3 (* (- x4 x3) (sin a))
                    (* (- y4 y3) (cos a)))])
      
      (if ( > i 0) 
          (begin 
		   (let ([i (- i 1)])
           (koch x1 y1 x3 y3 i)
           (koch x3 y3 x5 y5 i)
           (koch x5 y5 x4 y4 i)
           (koch x4 y4 x2 y2 i)))
      	  (begin 
            (draw-line x1 y1 x3 y3)
            (draw-line x3 y3 x5 y5)
            (draw-line x5 y5 x4 y4)
            (draw-line x4 y4 x2 y2))))))
          
(clear-image 0.0 0.0 0.0 1.0)
(pen-width 1.4)	 
(for i from 1 to 4
 (line-colour (random 1.0) (random 1.0) (random 1.0) 1.0)
 (koch 200.0 200.0 400.0 400.0 i)
 (show 0))
```

Draws a Koch fractal line

#### Tree

```scheme
;; draw tree

(define draw-a-line 
  (lambda (x y x1 y1 s) 
    ;; redisplay every line drawn
    (show 1) 
    (set-pen-width (exact->inexact s))
    (draw-line 
     (round x)
     (round y)
     (round x1)
     (round y1))))

(define draw-tree
 (lambda ()
    (define *scale* 10) 
    (define *split* 20)  
 
    (define degrees->radians 
     (lambda (d)
        (let [ (pi 3.1415926535897932384626433832795028841)]
                (* d pi 1/180))))
 
    (define (create-tree x1 y1 angle depth)
      (if (zero? depth)
        '()
        (let ((x2 (+ x1 (* (cos (degrees->radians angle)) depth *scale*)))
              (y2 (- y1 (* (sin (degrees->radians angle)) depth *scale*))))
          (append (list (map truncate (list x1 y1 x2 y2 depth)))
                  (create-tree x2 y2 (- angle *split*) (- depth 1))
                  (create-tree x2 y2 (+ angle *split*) (- depth 1))))))
 
    (define tree (create-tree 320.0 500.0 90.0 9.0))
 
 
 
    (define get-line 
      (lambda (x) 
        (apply draw-a-line x)))
 
    (line-colour 1.0 0.5 0.5 1.0)
    (clear-image 0.0 0.0 0.0 1.0) 
    (pen-width 1.5)	 
    (map get-line tree)
    (show 1)))
 

(draw-tree)

```

Seems slow; this is intentional as every line is displayed when it is added.

```Scheme
 (clear-image 0.0 0.0 0.0 1.0) 
 (show 0)
 
 (define draw-a-line 
        (lambda (x y x1 y1 s) 
          ;; do not redisplay every line drawn
          ;; (show 1) 
          (set-pen-width (exact->inexact s))
          (draw-line 
            (round x)
            (round y)
            (round x1)
            (round y1))))
 
(draw-tree) 
```

Draws the tree at once.





------

### Random Lines

```Scheme

(define random-lines
  (lambda (n)
    (dotimes n   
		 (pen-width 
			(+ 2.0 (random 6)))
		 (line-colour 
			(random 1.0) (random 1.0) (random 1.0) 1.0)
		  (draw-line
		  (random 800.0)
		  (random 600.0)
		  (random 800.0)
		  (random 600.0))) (show 1)))
;;
(clear-image 0.0 0.0 0.0 1.0)

(random-lines 300)(show 1)
```

###  Circles

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
 (try
   (begin
	(fill-colour 0.0 0.0 0.0 1.0)
	(fill-rect 0.0 0.0 800.0 600.0)
	(map drawcirc circles)
	(when (few-circles) 
		(set! circles 
			(newcircles circlecount)))
	(set! circles (map move-circles circles)))
 (catch (stop-every)))))
 
 
 
(set-every-function 1000 33 1 
		(lambda ()
		  (circle-step)(gc)))

```



 
