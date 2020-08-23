;; example 1 - move hero with arrow keys.

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

(define draw-invaders 
 (lambda (x y)
   (for x-pos from x to 700.0 step 88.0
	(draw-scaled-rotated-sprite
		20 x-pos y 0.0 0.65))))

(define draw-hero 
  (lambda (x) 
    (draw-sprite 1 x 460.0)))

(define draw-scene 
 (lambda (hx ix iy )
   (clear-scene)
   (draw-invaders ix iy)
   (draw-hero hx)))

(define check-keys
  (lambda (keys)
    (when (and (< (cdr (assq 'recent keys)) 50)
               (cdr (assq 'left keys)))
      (set! h-x (- h-x 1.0)))
    (when (and (< (cdr (assq 'recent keys)) 50)
               (cdr (assq 'right keys)))
      (set! h-x (+ h-x 1.0)))))

(image-size 800 600)
(load-all-sprites)

(define game-step 
  (lambda ()
   (check-keys (graphics-keys))
   (draw-scene h-x 100.0 20.0))) 

(game-step)(show 0)
