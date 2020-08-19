;; base script
;; loads first; contains essentials only.


;; The debugger is too terminal oriented.
(debug-on-exception #f)

;; when escape is pressed ctrl/c is 
;; simulated; that fires this handler
;; which uses the timer to raise
;; escape pressed shortly; when it is safe to do so.
;; (see added-to-schsig.c)

(keyboard-interrupt-handler
  (lambda ()
    (define escape-pressed!
      (lambda ()
        (raise
          (condition
            (make-error)
            (make-message-condition "Escape pressed")))))
    (transcript0 "**ctrl/c**")
    (newline-transcript)
    (timer-interrupt-handler
      (lambda () (set-timer 0) (escape-pressed!)))
    (set-timer 20)))
 

(break-handler
  (lambda ()
	(transcript0 "**reset on break**")
	(newline-transcript)))


(define gc
  (lambda () (collect) (collect) (collect) (collect)))
  
(define >0 (lambda (x) (> x 0)))
 
 
 
(define escape-pressed?
  (lambda () ((foreign-procedure "EscapeKeyPressed" () ptr))))

(define-syntax dotimes
  (syntax-rules ()
    [(_ n body ...)
     (let loop ([i n])
       (when (< 0 i)
         body
         ...
         (loop (- i 1))))]))

(define-syntax while
  (syntax-rules ()
    ((while condition body ...)
     (let loop ()
       (if condition
           (begin
             body ...
             (loop))
           #f)))))

(define-syntax for
  (syntax-rules (for in to step)
    [(for i in elements body ...)
     (for-each (lambda (i) body ...) elements)]
    [(for i from start to end step is body ...)
     (let ([condition (lambda (i)
                        (cond
                          [(< is 0) (< i end)]
                          [(> is 0) (> i end)]
                          [else #f]))])
       (do ([i start (+ i is)]) ((condition i) (void)) body ...))]
    [(for i from start to end body ...)
     (do ([i start (+ i 1)]) ((> i end) (void)) body ...)]))

(define-syntax try
  (syntax-rules (catch)
    [(_ body (catch catcher))
     (call-with-current-continuation
       (lambda (exit)
         (with-exception-handler
           (lambda (condition) (catcher condition) (exit condition))
           (lambda () body))))]))

 
(define transcript0
  (lambda (x)
    ((foreign-procedure "append_transcript" (string) void) x)))


  

(define get-input-ed
  (lambda () ((foreign-procedure "getInputed" () ptr))))

(define set-input-ed
  (lambda (s)
    ((foreign-procedure "setInputed" (string) void) s)))

(define printf-actual printf)

(define printf-print-transcript
  (lambda (x o)
    (transcript0
      (with-output-to-string (lambda () (printf-actual x o))))))
	  
(define printf 
 (case-lambda
    [(p x o)
     (unless (and (output-port? p) (textual-port? p))
       (errorf 'display "~s is not a textual output port" p))
     (printf-actual p x o)]
    [(x o) (printf-print-transcript x o)]))
	
(define pretty-actual pretty-print)

(define pretty-print-transcript
  (lambda (x)
    (transcript0
      (with-output-to-string (lambda () (pretty-actual x))))))

(define pretty-print  
 (case-lambda
    [(o p)
     (unless (and (output-port? p) (textual-port? p))
       (errorf 'display "~s is not a textual output port" p))
     (pretty-actual o p)]
    [(o) (pretty-print-transcript o)]))


(define display-statistics-actual display-statistics) 

(define display-statistics-transcript
  (lambda ()
    (transcript0
      (with-output-to-string (lambda () (display-statistics-actual))))))
	  
(define display-statistics
  (case-lambda
    [(p)
     (unless (and (output-port? p) (textual-port? p))
       (errorf 'display "~s is not a textual output port" p))
     (display-statistics-actual p)]
    [() (display-statistics-transcript)]))
	  
	  
(define display-port display)

(define newline-port newline)

(define display-transcript
  (lambda (x)
    (transcript0
      (with-output-to-string (lambda () (display-port x))))))

(define newline-transcript (lambda () (display-transcript #\newline)))

(define apropos-print apropos)
	
(define apropos
	(lambda (x) 
		 (transcript0
			(with-output-to-string (lambda () (apropos-print x))))))
	

(define display
  (case-lambda
    [(x p)
     (unless (and (output-port? p) (textual-port? p))
       (errorf 'display "~s is not a textual output port" p))
     (display-port x p)]
    [(x) (display-transcript x)]))

(define display-string (lambda (x) (display x)))

(define newline
  (case-lambda
    [(p)
     (unless (and (output-port? p) (textual-port? p))
       (errorf 'display "~s is not a textual output port" p))
     (newline-port p)]
    [() (newline-transcript)]))

(define blank
  (lambda ()
    ((foreign-procedure "clear_transcript" () void))))

(define (println . args)
  (apply transcript0 args)
  (transcript0 "\r\n"))

 
(define evalrespond
  (lambda (x)
    ((foreign-procedure "eval_respond" (string) void) x)))
 

(define eval->string
  (lambda (x)
    (define os (open-output-string))
    (define op (open-output-string))
    (trace-output-port op)
    (console-output-port op)
    (console-error-port op)
    (enable-interrupts)
    (try (begin
           (let* ([is (open-input-string x)])
             (let ([expr (read is)])
               (while
                 (not (eq? #!eof expr))
                 (try (begin (write (eval expr) os) (gc))
                      (catch
                        (lambda (c)
                          (println
                            (call-with-string-output-port
                              (lambda (p) (display-condition c p)))))))
                 (newline os)
                 (set! expr (read is)))))
           (evalrespond (get-output-string os))
           (transcript0 (get-output-string op)))
         (catch
           (lambda (c)
             (println
               (call-with-string-output-port
                 (lambda (p) (display-condition c p)))))))))
 
(define eval->text
  (lambda (x)
    (define os (open-output-string))
	(trace-output-port os)
	(console-output-port os)
	(console-error-port os)
    (try (begin
           (let* ([is (open-input-string x)])
             (let ([expr (read is)])
               (while
                 (not (eq? #!eof expr))
                 (try (begin (write (eval expr) os))
                      (catch (transcript0 (string-append "\r\n error: "))))
                 (newline os)
                 (set! expr (read is)))))
           (transcript0 (get-output-string os)))
         (catch (transcript0 (string-append "\r\n error: "))))))

 

;;; used to format text

(define eval->pretty
  (lambda (x)
    (define os (open-output-string))
    (try (begin
           (let* ([is (open-input-string x)])
             (let ([expr (read is)])
               (while
                 (not (eq? #!eof expr))
                 (try (begin (pretty-actual expr os))
                      (catch
                        (lambda (c)
                          (println
                            (call-with-string-output-port
                              (lambda (p) (display-condition c p)))))))
                 (newline os)
                 (set! expr (read is)))))
           (display (get-output-string os)))
         (catch
           (lambda (c)
             (println
               (call-with-string-output-port
                 (lambda (p) (display-condition c p)))))))))
				 
(define format-scite
  (lambda () (eval->pretty (get-input-ed))))
				 
 
(define every
  (lambda (d p m f)
    ((foreign-procedure "every" (int int int ptr) ptr) d p m f)))
		
 
;; delay, period, mode (mode=0,1)  f
;; runs every_step after d ms; every m ms.
(define set-every-function
  (lambda (d p m f)
    ((foreign-procedure "every" (int int int ptr) ptr) d p m f)))
		

	
;; run p after n 
(define after 
	(lambda (d n) 
		  ((foreign-procedure "after" (int ptr) ptr) d n)))

		
(define stop-every
	(lambda()
	 (set-every-function 0 0 0 '())))		
	
 
;; direct 2d

(define identity
 (lambda ()
   ((foreign-procedure "d2d_matrix_identity"
    () ptr))))
 
(define rotate
 (lambda (a x y)
   ((foreign-procedure "d2d_matrix_rotate"
    (float float string) ptr) a x y)))

(define show
 (lambda ( n)
   ((foreign-procedure "d2d_show"
    (int) ptr) n)))
	
 
(define release
 (lambda ()
   ((foreign-procedure "d2d_release"
    () ptr) )))

(define render
 (lambda (x y)
   ((foreign-procedure "d2d_render"
    (float float) ptr) x y)))

(define font
 (lambda (face size)
   ((foreign-procedure "d2d_set_font"
    (string float) ptr) face size)))

(define image-size
 (lambda (w h)
   ((foreign-procedure "d2d_image_size"
    (int int) ptr) w h)))
	
(define pen-width
 (lambda (w)
   ((foreign-procedure "d2d_set_stroke_width"
    (float) ptr) w)))	
	
(define add-pen-width
 (lambda (w)
   ((foreign-procedure "add_pen_width"
    (float) ptr) w)))		
	
(define set-pen-width
 (lambda (w)
   ((foreign-procedure "d2d_set_stroke_width"
    (float) ptr) w)))
	
(define write-text
 (lambda (x y s)
   ((foreign-procedure "d2d_write_text"
    (float float string) ptr) x y s)))
	
	
(define add-write-text
 (lambda (x y s)
   ((foreign-procedure "add_write_text"
    (float float string) ptr) x y s)))

(define batch-identity
 (lambda ()
   ((foreign-procedure "d2d_zmatrix_identity"
    () ptr))))

(define batch-skew
 (lambda (x y x1 y1)
   ((foreign-procedure "d2d_zmatrix_skew"
    (float float float float) ptr) x y x1 y1)))

(define batch-translate
 (lambda (x y)
   ((foreign-procedure "d2d_zmatrix_translate"
    (float float) ptr) x y)))
	
(define batch-trans-rot
 (lambda (a x y x1 y1 )
   ((foreign-procedure "d2d_zmatrix_transrot"
    (float float float float float) ptr) a x y x1 y1)))

(define batch-rot-trans
 (lambda (a x y x1 y1 )
   ((foreign-procedure "d2d_zmatrix_rotrans"
    (float float float float float) ptr) a x y x1 y1)))

(define batch-write-text
 (lambda (x y s)
   ((foreign-procedure "d2d_zwrite_text"
    (float float string) ptr) x y s)))
	
(define draw-line
 (lambda (x y x1 y1)
   ((foreign-procedure "d2d_line"
    (float float float float) ptr) x y x1 y1)))
	
(define add-draw-line
 (lambda (x y x1 y1)
   ((foreign-procedure "add_draw_line"
    (float float float float) ptr) x y x1 y1)))	
	
(define batch-draw-line
 (lambda (x y x1 y1)
   ((foreign-procedure "d2d_zline"
    (float float float float) ptr) x y x1 y1)))


(define draw-rect
 (lambda (x y w h)
   ((foreign-procedure "d2d_rectangle"
    (float float float float) ptr) x y w h)))
	
(define add-draw-rect
 (lambda (x y w h)
   ((foreign-procedure "add_draw_rect"
    (float float float float) ptr) x y w h)))	

(define batch-draw-rect
 (lambda (x y w h)
   ((foreign-procedure "d2d_zrectangle"
    (float float float float) ptr) x y w h)))

(define draw-ellipse
 (lambda (x y w h)
   ((foreign-procedure "d2d_ellipse"
    (float float float float) ptr) x y w h)))

	
(define add-draw-ellipse
 (lambda (x y w h)
   ((foreign-procedure "add_ellipse"
    (float float float float) ptr) x y w h)))
	
(define batch-draw-ellipse
 (lambda (x y w h)
   ((foreign-procedure "d2d_zellipse"
    (float float float float) ptr) x y w h)))


(define fill-ellipse
 (lambda (x y w h)
   ((foreign-procedure "d2d_fill_ellipse"
    (float float float float) ptr) x y w h)))
	

(define add-fill-ellipse
 (lambda (x y w h)
   ((foreign-procedure "add_fill_ellipse"
    (float float float float) ptr) x y w h)))
	
(define batch-fill-ellipse
 (lambda (x y w h)
   ((foreign-procedure "d2d_zfill_ellipse"
    (float float float float) ptr) x y w h)))

(define fill-rect
 (lambda (x y w h)
   ((foreign-procedure "d2d_fill_rectangle"
    (float float float float) ptr) x y w h)))
	
(define add-fill-rect
 (lambda (x y w h)
   ((foreign-procedure "add_fill_rect"
    (float float float float) ptr) x y w h)))
	
(define batch-fill-rect
 (lambda (x y w h)
   ((foreign-procedure "d2d_zfill_rectangle"
    (float float float float) ptr) x y w h)))

 (define fill-colour
 (lambda (r g b a)
   ((foreign-procedure "d2d_fill_color"
    (float float float float) ptr) r g b a)))
	
 (define add-fill-colour
 (lambda (r g b a)
   ((foreign-procedure "add_fill_colour"
    (float float float float) ptr) r g b a)))
	
(define clear-image
 (lambda (r g b a)
   ((foreign-procedure "d2d_clear"
    (float float float float) ptr) r g b a)))	
	
(define add-clear-image
 (lambda (r g b a)
   ((foreign-procedure "add_clear_image"
    (float float float float) ptr) r g b a)))	
	
(define batch-clear-active
 (lambda (r g b a)
   ((foreign-procedure "d2d_zclear"
    (float float float float) ptr) r g b a)))	
	
(define line-colour
 (lambda (r g b a)
   ((foreign-procedure "d2d_color"
    (float float float float) ptr) r g b a)))
	
(define add-line-colour
 (lambda (r g b a)
   ((foreign-procedure "add_line_colour"
    (float float float float) ptr) r g b a)))
		
(define free-sprites
 (lambda ()
   ((foreign-procedure "d2d_FreeAllSprites"
    () ptr))))
 			
			
 (define free-sprite
 (lambda (n)
   ((foreign-procedure "d2d_FreeSpriteInBank"
    (int) ptr) n)))
	
(define make-sprite
 (lambda (n w h f)
   ((foreign-procedure "d2d_MakeSpriteInBank"
    (int int int ptr) ptr) n w h f)))			
			
(define load-sprites
 (lambda (s n)
   ((foreign-procedure "d2d_load_sprites"
    (string int) ptr) s n)))

;;  (int n, float sx, float sy, float sw, float sh, 
;;	float dx, float dy, float dw, float dh)
(define draw-sprite
 (lambda (n dx dy)
   ((foreign-procedure "d2d_render_sprite"
    (int float float  ) 
		ptr) n dx dy)))	
		
(define batch-draw-sprite
 (lambda (n dx dy)
   ((foreign-procedure "d2d_zrender_sprite"
    (int float float  ) 
		ptr) n dx dy)))	
		
(define set-draw-sprite
 (lambda (c n dx dy)
   ((foreign-procedure "set_draw_sprite"
    (int int float float  ) 
		ptr) c n dx dy)))	
		
(define add-draw-sprite
 (lambda (n dx dy)
   ((foreign-procedure "add_draw_sprite"
    (int float float  ) 
		ptr) n dx dy)))	
		
	
(define draw-scaled-rotated-sprite
 (lambda (n dx dy s a)
   ((foreign-procedure "d2d_render_sprite_rotscale"
    (int float float float float  ) 
		ptr) n dx dy s a)))	
		
(define add-scaled-rotated-sprite
 (lambda (n dx dy s a)
   ((foreign-procedure "add_scaled_rotated_sprite"
    (int float float float float  ) 
		ptr) n dx dy s a)))	
	
(define batch-draw-scaled-rotated-sprite
 (lambda (n dx dy s a)
   ((foreign-procedure "d2d_zrender_sprite_rotscale"
    (int float float float float  ) 
		ptr) n dx dy s a)))	
	
(define render-sprite
 (lambda (n dx dy dh dw
            sx sy sh sw scale)
   ((foreign-procedure "d2d_render_sprite_sheet"
    (int float float float float 
		 float float float float
		 float) 
		ptr) n dx dy dh dw
			sx sy sh sw scale)))		
			
			
(define add-render-sprite
 (lambda (n dx dy dh dw
            sx sy sh sw scale)
   ((foreign-procedure "add_render_sprite_sheet"
    (int float float float float 
		 float float float float
		 float) 
		ptr) n dx dy dh dw
			sx sy sh sw scale)))	
			
(define batch-render-sprite
 (lambda (n dx dy dh dw
            sx sy sh sw scale)
   ((foreign-procedure "d2d_zrender_sprite_sheet"
    (int float float float float 
		 float float float float
		 float) 
		ptr) n dx dy dh dw
			sx sy sh sw scale)))	
			
(define render-sprite-scale-rot
 (lambda (n dx dy dh dw
            sx sy sh sw scale rot x2 y2)
   ((foreign-procedure "d2d_render_sprite_sheet_rot_scale"
    (int float float float float 
		 float float float float
		 float float float float) 
		ptr) n dx dy dh dw
			sx sy sh sw scale rot x2 y2)))	
				
(define batch-render-sprite-scale-rot
 (lambda (n dx dy dh dw
            sx sy sh sw scale rot x2 y2)
   ((foreign-procedure "d2d_zrender_sprite_sheet_rot_scale"
    (int float float float float 
		 float float float float
		 float float float float) 
		ptr) n dx dy dh dw
			sx sy sh sw scale rot x2 y2)))	
			
(define draw-batch 
  (lambda (f)
   ((foreign-procedure "d2d_draw_func"
    (ptr) ptr) f)))
		

;; used to track keys in graphics window
(define graphics-keys
 (lambda ()
   ((foreign-procedure "graphics_keys"
    () ptr))))
	
(define keyboard-delay
  (lambda (n)
   ((foreign-procedure "keyboard_debounce"
    (int) ptr) n)))	
	
;; sound from the xbox audio api here on windows.	
(define load-sound
 (lambda (s n)
   ((foreign-procedure "load_sound"
    (string int) ptr) s n)))

(define play-sound
  (lambda (n)
   ((foreign-procedure "play_sound"
    (int) ptr) n)))

 