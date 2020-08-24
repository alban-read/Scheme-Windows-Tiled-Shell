;; example 3 - enemy list - render mode 2

(define frames 0)
(define h-x 400.0)
(define h-xn 0.0 )
(define enemies '())
(define missiles '())
(define hits '())


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
   
(define enemy-wave-one
 (lambda ()

 (for y from -840.0 to -3800.0 step -840.0 
    (set! enemies 
		(append enemies 
			(new-enemy 330.0 y 0.0 0.8 23))) )

  (for y from 0.0 to -3850.0 step -124.0 

	  (set! enemies 
		(append enemies 
			(new-enemy 20.0 y 0.0 0.8 20)))
	   (set! enemies 
		(append enemies 
			(new-enemy 120.0 y 0.0 0.9 21))) 
	   (set! enemies 
		(append enemies 
			(new-enemy 220.0 y 0.0 0.8 22)))

	  (set! enemies 
		(append enemies 
			(new-enemy 530.0 y 0.0 0.8 22)))
	   (set! enemies 
		(append enemies 
			(new-enemy 630.0 y 0.0 0.9 21))) 
	   (set! enemies 
		(append enemies 
			(new-enemy 730.0 y 0.0 0.8 20)))  )))
   
(define new-missile
  (lambda (x y)
   (list (list x y))))

(define fire-missiles 
  (lambda ()
	 (set! missiles
		(append missiles (new-missile (+ h-x 05.0) 510.0)))
	 (set! missiles
		(append missiles (new-missile (+ h-x 35.0) 510.0)))))

(define draw-missiles
 (lambda ()
  (map 
	(lambda (m)
		(add-draw-sprite 18 (car m) (cadr m))) 
			missiles)))

(define clean-missiles
  (lambda ()
    (set! missiles
      (filter (lambda (a) (< -100 (cadr a))) missiles))))

(define move-missiles 
  (lambda ()
	(set! missiles 
		 (map 
		  (lambda (m) 
	       (list (car m) (- (cadr m) 10.0)))
								missiles)   )))
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

(define clean-enemies
  (lambda ()
    (set! enemies
      (filter (lambda (a) (> 1200.0 (caar a))) enemies ))))

(define collisions
  (lambda ()
    (when (and (> (length missiles) 0) (> (length enemies)))
      (set! hits
        (filter
          (lambda (h) (not (null? h)))
          (map (lambda (m)
                 (filter
                   (lambda (e)
                     (and (> (+ (car m) 40.0) (caar e))
                          (< (- (car m) 40.0) (caar e))
                          (< (- (cadr m) 40.0) (cadar e))
                          (> (+ (cadr m) 40.0) (cadar e))))
                   enemies))
               missiles))))))

 (define draw-hits
  (lambda ()
    (when (> (length hits) 0)
      (add-line-colour 1.0 0.0 1.0 0.4)
	  (add-pen-width 2.5)
      (map (lambda (e)
             (when (and (not (null? e))
                        (not (null? (caaar e)))
                        (not (null? (cadaar e))))
               (add-draw-ellipse
                   (+ 20 (caaar e)) (+ 20(cadaar e)) 40.0 40.0)))
           hits))))


(define draw-hero 
  (lambda (x) 
    (add-scaled-rotated-sprite 1 x 510.0 0.0 0.65)))

(define move-hero 
  (lambda ()
	(set! h-x (+ h-x h-xn))))

(define check-hx 
 (lambda () 
  (when (< h-x 1.0) (set! h-x 1.0))
  (when (> h-x 713.0) (set! h-x 713.0))
  (when (> h-xn 0.0) (set! h-xn (- h-xn 0.2)))
  (when (< h-xn 0.0) (set! h-xn (+ h-xn 0.2))
  (when (< (abs h-xn) 0.4) (set! h-xn 0.0)))))
 
(define draw-scene 
	(lambda ( hx )
	   (clear-scene)
	   (draw-invaders)
	   (draw-missiles)
	   (draw-hero hx)
       (draw-hits)))

(define check-keys
  
 (lambda (keys)

    (when (and (< (cdr (assq 'recent keys)) 50)
			   (cdr (assq 'ctrl keys)) 
			   (<  (length missiles) 8))
                  (fire-missiles))

    (when (and (< (cdr (assq 'recent keys)) 50)
               (cdr (assq 'left keys)))
						(set! h-xn (- h-xn 2.0)))

    (when (and (< (cdr (assq 'recent keys)) 50)
               (cdr (assq 'right keys)))
						(set! h-xn (+ h-xn 2.0)))))

(load-all-sprites)
(enemy-wave-one)

(define game-step 
    (lambda ()
	   (set! frames (+ frames 1))
	   (check-keys (graphics-keys))
	   (check-hx)
	   (move-enemies) 
	   (move-missiles)
	   (move-hero)     
       (collisions)
	   (clean-missiles)
	   (clean-enemies)
	   (draw-scene h-x)))

(keyboard-delay 100)

(set-every-function 1000 16 2 
		(lambda ()
		  (safely game-step)))

 
 
 
 