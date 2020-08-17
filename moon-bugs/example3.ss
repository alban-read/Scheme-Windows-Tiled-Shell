;; example 3 - enemy list - render mode 2
;; 

(define h-x 400.0)

(define load-all-sprites 
 (lambda ()
   (load-sprites "images/bg1.jpg" 0)	
   (load-sprites "images/h1.png" 1)	
   (load-sprites "images/laser-18.png" 18)	
  
   (load-sprites "images/D-14.png" 20) 
   (load-sprites "images/D-16.png" 21) 
   (load-sprites "images/C-14.png" 22) 
   (load-sprites "images/D-01.png" 23)  ))

(define clear-scene 
  (lambda ()
    (add-clear-image 0.0 0.0 0.0 1.0)
	(add-draw-sprite 0 0.0 10.0 )))
	
	
(define new-enemy 
  (lambda (x y xn yn s)
   (list (list (list x y) (list xn yn) s))))
   
(define enemies '())

(define enemy-wave-one
 (lambda ()

 (for y from -840.0 to -3800.0 step -840.0 
    (set! enemies 
		(append enemies 
			(new-enemy 300.0 y 0.0 0.8 23))) )

  (for y from 0.0 to -3850.0 step -124.0 

	  (set! enemies 
		(append enemies 
			(new-enemy 10.0 y 0.0 0.8 20)))
	   (set! enemies 
		(append enemies 
			(new-enemy 110.0 y 0.0 0.9 21))) 
	   (set! enemies 
		(append enemies 
			(new-enemy 210.0 y 0.0 0.8 22)))

	  (set! enemies 
		(append enemies 
			(new-enemy 450.0 y 0.0 0.8 22)))
	   (set! enemies 
		(append enemies 
			(new-enemy 550.0 y 0.0 0.9 21))) 
	   (set! enemies 
		(append enemies 
			(new-enemy 650.0 y 0.0 0.8 20)))  )))
   

(define draw-invaders
  (lambda ()
    (map (lambda (e)
           (apply
             add-scaled-rotated-sprite
             (list (caddr e) (caar e) (cadar e) 0.0 0.45)))
         enemies)))
		 
		 
(define move-enemy 
 (lambda (e)
	(let* 	([xy (car e)]
	         [xn (cadr e)]
		     [newxy (map (lambda (x y) (+ x y)) xy xn)]
             [s (caddr e)])
	(list newxy xn s))))
	
(define move-enemies 
	(lambda () 
	  (set! enemies  
		(map (lambda (e) 
		  (move-enemy e)) enemies))))

(define draw-hero 
  (lambda (x) 
    (add-scaled-rotated-sprite 1 x 460.0 0.0 0.65)))

(define check-hx 
 (lambda () 
  (when (< h-x 1.0) (set! h-x 1.0))
  (when (> h-x 713.0) (set! h-x 713.0))))
 

(define draw-scene 
 (lambda ( hx )
   (clear-scene)
   (draw-invaders)
   (draw-hero hx)))


(define check-keys
  (lambda (keys)
    (when (and (< (cdr (assq 'recent keys)) 50)
               (cdr (assq 'left keys)))
      (set! h-x (- h-x 4.0)))
    (when (and (< (cdr (assq 'recent keys)) 50)
               (cdr (assq 'right keys)))
      (set! h-x (+ h-x 4.0)))))

(load-all-sprites)
(enemy-wave-one)

(define game-step 
  (lambda ()
   (check-keys (graphics-keys))
   (check-hx)
   (move-enemies)
 
   (draw-scene h-x)))


(set-every-function 1000 16 2 
		(lambda ()
		  (game-step)(gc)))
 

