#include <unistd.h>
#include <stdio.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
#include "address_map_niosv.h"


typedef uint16_t pixel_t;

volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

void delay( int N )
{
	for( int i=0; i<N; i++ ) 
		*pVGA; // read volatile memory location to waste time
}


/* STARTER CODE BELOW. FEEL FREE TO DELETE IT AND START OVER */

void drawPixel( int y, int x, pixel_t colour )
{
	*(pVGA + (y<<YSHIFT) + x ) = colour;
}

pixel_t makePixel( uint8_t r8, uint8_t g8, uint8_t b8 )
{
	// inputs: 8b of each: red, green, blue
	const uint16_t r5 = (r8 & 0xf8)>>3; // keep 5b red
	const uint16_t g6 = (g8 & 0xfc)>>2; // keep 6b green
	const uint16_t b5 = (b8 & 0xf8)>>3; // keep 5b blue
	return (pixel_t)( (r5<<11) | (g6<<5) | b5 );
}

void rect( int y1, int y2, int x1, int x2, pixel_t c )
{
	for( int y=y1; y<y2; y++ )
		for( int x=x1; x<x2; x++ )
			drawPixel( y, x, c );
}

pixel_t getPixel(int y, int x)
{
    if (x < 0 || x >= MAX_X || y < 0 || y >= MAX_Y)
        return wht; // outside screen = wall
    return *(pVGA + (y << YSHIFT) + x);
}

/* DIRECTIONS */
#define DIR_UP      0
#define DIR_RIGHT   1
#define DIR_DOWN    2
#define DIR_LEFT    3

void dirDelta(int dir, int *dx, int *dy)
{
    *dx = 0;
    *dy = 0;
    if (dir == DIR_UP)       { *dy = -1; }
    else if (dir == DIR_RIGHT) { *dx =  1; }
    else if (dir == DIR_DOWN)  { *dy =  1; }
    else if (dir == DIR_LEFT)  { *dx = -1; }
}

/* Check if a direction is safe 1 AND 2 pixels ahead from (x,y) */
int directionIsSafe(int dir, int x, int y)
{
    int dx, dy;
    dirDelta(dir, &dx, &dy);

    for (int step = 1; step <= 2; step++) {
        int nx = x + step * dx;
        int ny = y + step * dy;
        if (getPixel(ny, nx) != blk) {
            return 0; // blocked
        }
    }
    return 1; // both 1 and 2 ahead are black
}

int main(){

	
	printf( "start\n" );

    volatile int *KEY_ptr = (int *) KEY_BASE;
    volatile int *HEX_ptr = (int *) HEX3_HEX0_BASE;
	
    /* HEX DISPLAY */
    int HEX_MAP[10] = {
        0x40, // 0
        0x79, // 1
        0x24, // 2
        0x30, // 3
        0x19, // 4
        0x12, // 5
        0x02, // 6
        0x78, // 7
        0x00, // 8
        0x10  // 9
    };

    int humanScore = 0;
    int robotScore = 0;

    /* Show 0–0 at the start */
    int hex_value = (HEX_MAP[robotScore] << 8) | HEX_MAP[humanScore];
    *HEX_ptr = hex_value;


    const pixel_t HUMAN_COLOUR = blu;
    const pixel_t ROBOT_COLOUR = red;

    /* STARTING POSITIONS */
    const int HUMAN_START_X = MAX_X/3;
    const int ROBOT_START_X = 2*MAX_X/3;
    const int START_Y = MAX_Y/2;

    while (humanScore < 9 && robotScore < 9){
        
        /* CLEAR SCREEN, SET BORDERS*/
        
        rect(0, MAX_Y, 0, MAX_X, blk); // clear screen to black
        rect(0, 1, 0, MAX_X, wht);     // draw top white line for border
        rect(MAX_Y-1, MAX_Y, 0, MAX_X, wht); // draw bottom white line for border
        rect(0, MAX_Y, 0, 1, wht);     // draw left white line for border
        rect(0, MAX_Y, MAX_X-1, MAX_X, wht); // draw right white line for border

        /* DRAW HUMAN AND ROBOT */

        int human_x = HUMAN_START_X;
        int human_y = START_Y;
        int human_dir = DIR_RIGHT;
        int human_alive = 1;
        
        int robot_x = ROBOT_START_X;
        int robot_y = START_Y;
        int robot_dir = DIR_LEFT;
        int robot_alive = 1;

        drawPixel(human_y, human_x, HUMAN_COLOUR);
        drawPixel(robot_y, robot_x, ROBOT_COLOUR);

        uint32_t prev_keys = 0;

        while(human_alive && robot_alive){
            /*HUMAN INPUT*/

            uint32_t raw  = ~(*KEY_ptr) & 0x3;  // KEY0, KEY1
            uint32_t edge = raw & ~prev_keys;
            prev_keys = raw;

            if(edge & 0x1) human_dir = (human_dir + 3) % 4; // turn left
            if(edge & 0x2) human_dir = (human_dir + 1) % 4; // turn right
            
            /*ROBOT AI*/

            int forward_safe = directionIsSafe(robot_dir, robot_x, robot_y);
            if (!forward_safe) {
                int left_dir  = (robot_dir + 3) % 4;
                int right_dir = (robot_dir + 1) % 4;

                if (directionIsSafe(left_dir, robot_x, robot_y)) {
                    robot_dir = left_dir;
                } else if (directionIsSafe(right_dir, robot_x, robot_y)) {
                    robot_dir = right_dir;
                }
                // else: no safe direction, keep going straight and probably die
            }

            // NEXT POSITIONS
            int new_hx = human_x, new_hy = human_y;

            if(human_dir == DIR_UP) new_hy--;
            else if(human_dir == DIR_RIGHT) new_hx++;
            else if(human_dir == DIR_DOWN) new_hy++;
            else if(human_dir == DIR_LEFT) new_hx--;

            int new_rx = robot_x, new_ry = robot_y;

            if(robot_dir == DIR_UP) new_ry--;
            else if(robot_dir == DIR_RIGHT) new_rx++;
            else if(robot_dir == DIR_DOWN) new_ry++;
            else if(robot_dir == DIR_LEFT) new_rx--;
            
            /* HEAD ON COLLISIONS */
            int head_on = (new_hx == new_rx) && (new_hy == new_ry);

            int cross_over = (new_hx == robot_x && new_hy == robot_y) &&
                             (new_rx == human_x && new_ry == human_y);

            if(head_on || cross_over){
                human_alive = 0;
                robot_alive = 0;
                break;
            }
            
            /* HUMAN MOVE & COLLISION */
            pixel_t h_target = getPixel(new_hy, new_hx);
            if (h_target != blk){
                human_alive = 0;
            }
            else{
                human_x = new_hx;
                human_y = new_hy;
                drawPixel(human_y, human_x, HUMAN_COLOUR);
            }

            /* ROBOT MOVE & COLLISION */
             pixel_t r_target = getPixel(new_ry, new_rx);
            if(r_target != blk){
                robot_alive = 0;
            }
            else{
                robot_x = new_rx;
                robot_y = new_ry;
                drawPixel(robot_y, robot_x, ROBOT_COLOUR);
            }
		

            delay(200000);
        }

        // SCORE UPDATE
        if(human_alive && !robot_alive){
            humanScore++;
        }

        else if(robot_alive && !human_alive){
            robotScore++;
        }
        
        hex_value = (HEX_MAP[robotScore] << 8) | HEX_MAP[humanScore];
        *HEX_ptr = hex_value;

        delay(5000000);
    }

    // GAME OVER

    if(humanScore == 9){
        rect(0, MAX_Y, 0, MAX_X, HUMAN_COLOUR);
    }

    else{
        rect(0, MAX_Y, 0, MAX_X, ROBOT_COLOUR);
    }
    
    while(1);
}