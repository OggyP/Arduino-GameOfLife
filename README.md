# Arduino-GameOfLife
Arduino Conway's Game of Life with an Arduino UNO, MAX7219 and an 8x8 Dot Matrix Display. Video - https://youtu.be/qY5oLLSfSIA

# Controls
This program will run Conway's Game of Life on an Arudino. It will start up with a randomised board and run the simulation. After 150 generations the board will be rerandomised. 

There are four buttons and a potentiometer used for controls.
### Main Button (Pin 5)
If it is not paused then it is used to enable / disable the 150 generation randomisations of the board. If the simulation is paused it will then invert the state of the cell.

### Play / Pause Button (Pin 6)
This will pause / play the game. Doing this will also disable the randomisations. If it is paused it will allow editing of the board.

### Cursor Movement Buttons (Pin 7 and 8)
When the game is paused pressing this will change the location of the cursor.

### Speed Control Potentiometer (Pin 0)
The reading from this will be the delay at the end of each generation. This is done to slow down the generation speed and make it viewable.
