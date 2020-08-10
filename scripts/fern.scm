
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
    (dotimes 100 
      (transform) (draw-point))))

 
(fill-colour 0.0 0.0 0.0 1.0)
(fill-rect 0.0 0.0 400.0 600.0)
(set-every-function 1000 33 1 
		(lambda ()
		  (fern)(gc)))