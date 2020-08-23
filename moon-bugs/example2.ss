;; example 2 - enemy list

(define h-x 400.0)

(define load-all-sprites 
 (lambda ()
   (load-sprites "images/bg1.jpg" 0)	
   (load-sprites "images/h1.png" 1)	
   (load-sprites "images/D-17.png" 20)))

(define clear-scene 
  (lambda ()
    (clear-image 0.0 0.0 0.0 1.0)
    (draw-sprite 0 0.0 10.0)))
	
	
(define new-enemy 
  (lambda (x y xn yn s)
   (list (list (list x y) (list xn yn) s))))
   
(define enemies '())

(define enemy-wave-one
 (lambda ()

  (for y from 0.0 to -1850.0 step -350.0 

	  (set! enemies 
		(append enemies 
			(new-enemy 10.0 y 0.0 0.8 20)))
	   (set! enemies 
		(append enemies 
			(new-enemy 110.0 y 0.0 1.0 20))) 
	   (set! enemies 
		(append enemies 
			(new-enemy 210.0 y 0.0 0.8 20)))

	  (set! enemies 
		(append enemies 
			(new-enemy 400.0 y 0.0 0.8 20)))
	   (set! enemies 
		(append enemies 
			(new-enemy 500.0 y 0.0 1.0 20))) 
	   (set! enemies 
		(append enemies 
			(new-enemy 600.0 y 0.0 0.8 20)))
)))
   

(define draw-invaders
  (lambda ()
    (map (lambda (e)
           (apply
             draw-scaled-rotated-sprite
             (list (caddr e) (caar e) (cadar e) 0.0 0.65)))
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
    (draw-sprite 1 x 460.0)))

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
	  
(image-size 800 600)
(load-all-sprites)
(enemy-wave-one)

(define game-step 
  (lambda ()
   (check-keys (graphics-keys))
   (check-hx)
   (move-enemies)
   (draw-scene h-x)))


(set-every-function 1000 33 0 
		(lambda ()
		  (game-step)(gc)))
		  
		  
		  