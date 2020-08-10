 
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