 
;; example just draws circles on sprites; in render mode 2
 
(define circlecount 1000)


 ;; make some circle sprites
(define make-sprites
 (lambda ()
  (for s from 1 to 1000
   (make-sprite s 80 80 
	(lambda ()
		(pen-width 2.5)
		(fill-colour 
			(random 1.0)(random 1.0)(random 1.0) 0.9)
		(line-colour 
			1.0 1.0 1.0 0.6)
		(batch-fill-ellipse 40.0 40.0 38.0 38.0)
		(batch-draw-ellipse 40.0 40.0 38.0 38.0))))))
		



(define newcircle
  (lambda ()
    (list
      (list (+ 200 (random 300.0)) (+ 100 (random 200.0)))
      (list (- 5.0 (random 10.0)) (- 5.0 (random 10.0)))
      (list (+ 1 (random 5000))))))


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
	(apply add-draw-sprite  (list (caaddr c) (caar c) (cadar c)))))

(define show-status
 (lambda ()
	(add-fill-colour 1.0 0.5 0.9 1.0)
	(add-write-text 5.0 5.0 
		(string-append "Circles:"
		 (number->string
			(- circlecount (count-offscreen)))))))	
			 

;; perform one step
(define circle-step
 (lambda ()
 (try
   (begin
	(when (few-circles) 
		(set! circles 
			(newcircles circlecount)))
	(set! circles (map move-circles circles))
	(add-clear-image 0.2 0.2 0.2 0.5) 
	(map drawcirc circles)
	(show-status))
 (catch (stop-every)))))


 
(image-size 800 600)
(make-sprites)		
(font "Calibri" 34.0)
 
(set-every-function 1000 33 2
		(lambda ()
		  (safely circle-step)))