## demo python payload

/!\ PHOTOSENSITIVITY WARNING /!\

due to captouch malfunctions this payload can produce rapidly flashing lights. please refrain from using it if this could cause issues for you. this is a driver issue and is currently being addressed.

### available states:

MENU: the screen shows the text "SELECT :3" and a volume bar. the MENU can be entered at any point by pressing right shoulder button down. volume can be adjusted with right shoulder button L/R in any state. two bottom petals are lit up by LEDs in different colors. pressing each of them loads one of two available INSTRUMENTS:

INSTRUMENT 1, CHORD ORGAN: all LEDs are lit up in the same color. each top petal plays a different note. each bottom petal changes global LED color and changes the notes for each top petal. the note selection is intended to provide 5 different chords accessible with the bottom petals which can be manually appregiated with the top petals in a crude omnichord-esque fashion. the screen is entirely black.

INSTRUMENT 2, MELODY PLAYER: the 3 most down-pointing petal LEDs are lit in pink. these 3 petals select between 3 different octaves. depending on octave selection, the remaining petals are lit in blue (low octave), cyan (mid octave) and green (high octave). these "playing petals" provide tones in a major scale. it is a single oscillator system, so pressing more than one "playing petal" results in the oscillator jumping between pitches rapidly, providing a HIDDEN NOISE MODE. the screen is entirely black. author's note: this one didn't turn out well at all and could use a bunch more love. we find it valuable to provide people with a simple instrument to try to play their favorite melodies on, but this ain't it yet.

INSTRUMENT 3, CAP TOUCH DEBUG: shows 10 dots corresponding to the 10 petals. each gets bigger if the captouch threshold is exceeded (i.e., a touch is registered) and moves with the current position on the petal (1d for 2-segment petals, 2d for 3-segment petals). note that on p3/p4 the inner pad of the bottom petal opposed to the usb c port is currently not registered for technical reasons.

### captouch calibration:

the captouch controller can be recalibrated by pressing the left shoulder button while in the menu. the screen shows the text "CAL" while in calibration. the screen background is teal for 0.5s to give the user time to remove their hands, then turns plum for the actual calibration. calibration is intended to get a baseline for pads that are not being touched at the moment, it is recommeded to lie the badge flat on the able while recalibrating. calibration is also performed at startup. 
