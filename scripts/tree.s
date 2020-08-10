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
 
    (define draw-a-line 
        (lambda (x y x1 y1 s) 
          (show 1)
          (set-pen-width (exact->inexact s))
          (draw-line 
            (round x)
            (round y)
            (round x1)
            (round y1))))
 
    (define get-line 
      (lambda (x) 
        (apply draw-a-line x)))
 
    (line-colour 1.0 0.5 0.5 1.0)
    (clear-image 0.0 0.0 0.0 1.0) 
    (map get-line tree)
    (show 1)))
 

(draw-tree)
