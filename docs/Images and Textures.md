 [Index](welcome.html)  

# Images and Textures 

Sprites are the classic name of small images that can be moved around a screen.

A sprite is loaded from a file into a memory bank and then displayed at a position on the screen.

The image file formats understood are JPEG and PNG files.

Transparency is critical to sprites; in this case the alpha channel is used; where 0.0 is transparent.

### Load Sprites

```Scheme
(load-sprites "c:/images/A1.png" 10)	
(draw-sprite 10 100.0 100.0)
(show 1)	
```

Will load a sprite into sprite bank 10 and display the whole thing.

### Drawing Sprites

Sprites are drawn entirely (or partially from a sheet.)

```Scheme
 (draw-sprite 10 100.0 100.0)
```

Will draw all of the sprite in bank 10 on the screen at position 100.0,100.0

Direct2D can rotate and scale a sprite for us.

```Scheme
(draw-scaled-rotated-sprite 10 200.0 200.0 45.0 1.5)  
(show 1)
```

### Drawing from a sprite sheet

The render-sprite function displays a sprite picked out from a sprite sheet.

You do this by choosing which part of the sheet to draw; and where to draw it.

For this you need the destination and the source rectangles.

```Scheme
(render-sprite 
 	;; use sprite sheet in bank 12
 	12
 	;; draw at 100.0,100.0 size 32.0, 32.0
 	100.0 100.0 32.0 32.0 
    ;; from 
    130.0 130.0 128.0 128.0
 	;;
 	1.0)
```

A sprite sheet is a larger image; that contains many smaller images; typically but not required to be; in an orderly arrangement; imagine that you have an explosion to animate; and each frame is a 128 by 128 image. You can place them in single explosions.png file; on a 130 by 130 grid of rows and columns.

If the destination size is different to the source size; the image is scaled up or down.

### Keyboard 

You can read the keypresses made by "the player" in the image pane. 

Useful if you are writing a game.

The command is :-

```Scheme
(graphics-keys)
=> ((recent . 2703) (key . 0) (space . #f) (ctrl . #f) (down . #f) (up . #f) (right . #f) (left . #f))

```

Which returns an association list; the recent field is how long ago the key was pressed.

The other items are fairly self evident.

This can be used inside a step function; to control a game with the keyboard.



**Stand alone player**

Actually using a game in the image pane; is not ideal; although it is nice for writing a game.

Note that if you do find yourself writing an interesting animation or game; there is a single window 'player' app that runs most of these same commands and also includes sound playback.

https://github.com/alban-read/Scheme-Direct

So once completed; you could package and run your final script in a single player window; instead of this entire workspace.

<script>
    function evaluate_selected_expression() {
        var text = window.getSelection().toString();
        text = text.replace(/[\u{0080}-\u{FFFF}]/gu, "");
        console.log(text);
        window.chrome.webview.postMessage('::eval:' + text);
    }
    document.addEventListener('keydown', function (event) {
        if (event.code == 'Enter' && (event.shiftKey)) {
            evaluate_selected_expression(); 
        }
    });
</script>