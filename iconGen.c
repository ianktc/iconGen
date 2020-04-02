#include <stdbool.h>
#include <math.h>
#include <time.h>

// defined variables
#define DISP_WIDTH 320
#define DISP_HEIGHT 240
#define UNIT 20
#define ITERATIONS 3
#define BORDERS UNIT

// global variables
volatile int pixel_buffer_start; 

//function def
void clear_screen();
void plot_pixel(int x, int y, short line_color);
void wait_for_vsync();
int getNeighbours(int arr[DISP_WIDTH][DISP_HEIGHT], int x, int y);
void randomInit(int arr[DISP_WIDTH][DISP_HEIGHT]);
void setOutline(int arr[(DISP_WIDTH/2)/UNIT][(DISP_HEIGHT)/UNIT]);
void clear_array(int tempArr[DISP_WIDTH/2/UNIT][(DISP_HEIGHT)/UNIT]);

int main(void){
    
    //set to pixel buffer
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	
	//set to switch address
	volatile int * SW_ptr         = 0xFF200040;     // SW slide switch address
	
    int currGen[DISP_WIDTH][DISP_HEIGHT] = {0};
    int prevGen[DISP_WIDTH][DISP_HEIGHT] = {0};
    int displayPrev1[DISP_WIDTH][DISP_HEIGHT] = {0};
    int displayPrev2[DISP_WIDTH][DISP_HEIGHT] = {0};
    int newArr[DISP_WIDTH][DISP_HEIGHT] = {0};
	int tempArr[DISP_WIDTH/2/UNIT][(DISP_HEIGHT)/UNIT] = {0};
	
    bool buf1 = true;
	bool init = false;
	bool plotted = false;
	int switchVal = 0;	
	int iterations = 0;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
    
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
	
   	/* Intializes random number generator */
	srand(time(NULL));
	        
    while(1){
		
		//check switches
		switchVal = *(SW_ptr);
		
		//hard reset
		if(switchVal > 0){
			randomInit(prevGen);
			clear_screen();
			clear_array(newArr);
		}
			
		//game of life
		while(switchVal > 0){

			switchVal = *(SW_ptr);

			//Iterate through previous generation to determine new generation.
			for (int i = 0; i < DISP_WIDTH; i++) {
				for (int j = 0; j < DISP_HEIGHT; j++) {

					int neighbours = getNeighbours(prevGen, i, j);

					//rules of life
					
					/*
					
					//2 or 3 neighbours, survive, everything else dies
					if(neighbours == 3){
						currGen[i][j] = 1;
					} else if (neighbours == 2 && prevGen[i][j] == 1){
						currGen[i][j] = 1;
					} else{
						currGen[i][j] = 0;
					}
					
					*/
					
					if(prevGen[i][j] == 1){
					if(neighbours == 2 || neighbours == 3) currGen[i][j] = 1;
					} else if(neighbours <= 1) currGen[i][j] = 1;
					else currGen[i][j] = 0;

					
					
					//blackout old generation and plot new generation
					if(buf1) {
						if(displayPrev1[i][j] == 1){
							plot_pixel(i,j,0x0);
						}
					} else {
						if(displayPrev2[i][j] == 1){
							plot_pixel(i,j,0x0);
						}
					}
					
					//plot in white for life
					if(currGen[i][j] == 1){
						plot_pixel(i,j,0xFFFF);
					}
				}
			}

			//copy over current to prev
			for (int i = 0; i < DISP_WIDTH; i++) {
				for(int j = 0; j < DISP_HEIGHT; j++){
					if(buf1){
						displayPrev1[i][j] = currGen[i][j];
					} else{
						displayPrev2[i][j] = currGen[i][j];
					}
					prevGen[i][j] = currGen[i][j];
				}
			}

			//pick any 5x8 rectangle
			int x1 = rand()% (DISP_WIDTH - (DISP_WIDTH/2)/UNIT - 3);
			int x2 = x1 + (DISP_WIDTH/2)/UNIT - 1 -(BORDERS/UNIT);
			int y1 = rand()% (DISP_HEIGHT - (DISP_HEIGHT)/UNIT - 4);
			int y2 = y1 + (DISP_HEIGHT)/UNIT - 4 - 2*(BORDERS/UNIT);

			//fill temp with the chosen 5x8 rectangle (contained in 8x12)
			memset(tempArr, 0, sizeof(tempArr));	
			for(int i = x1; i < x2; ++i){
				for(int j = y1; j < y2; ++j){
					tempArr[i - x1 + 3][j - y1 + 3] = prevGen[i][j];
				}
			}

			//add outline to 8x12 array
			setOutline(tempArr);

			buf1 = !buf1;

			wait_for_vsync(); // swap front and back buffers on VGA vertical sync
			pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

			init = true;
		}
		
		
			
		//plot sprite
		while (switchVal == 0 && init) {
			
			clear_screen();
			
			switchVal = *(SW_ptr);
			
			if(switchVal > 0 && plotted) {
				clear_array(tempArr);
				clear_screen();
			} 
			
			else {
			
				//enlarge
				for(int i = 0; i < (DISP_WIDTH/2)/UNIT; ++i){
					for(int j = 0; j < (DISP_HEIGHT)/UNIT; ++j){

						for(int k = 0; k < UNIT; ++k){
							for(int l = 0; l < UNIT; ++l){
								newArr[(i*UNIT)+k][(j*UNIT)+l] = tempArr[i][j];
							}
						}

					}
				}

				//reflect
				for(int i = 0; i < DISP_WIDTH/2; ++i){
					for(int j = 0; j < DISP_HEIGHT; ++j){
						newArr[(DISP_WIDTH - 1) - i][j] = newArr[i][j];        
					}
				}

				//redraw
				for(int i = 0; i < DISP_WIDTH; i++){
					for(int j = 0; j < DISP_HEIGHT; j++){
						if(newArr[i][j] == 1) plot_pixel(i, j, 0x039F);			//light blue
						else if(newArr[i][j] == 2) plot_pixel(i, j, 0x1212);	//dark blue
						else plot_pixel(i,j, 0xffff);							//white
					}
				}
				
				plotted = true;
				
			}

			/* now, swap the front/back buffers, to set the front buffer location */
			wait_for_vsync();

			/* initialize a pointer to the pixel buffer, used by drawing functions */
			pixel_buffer_start = *(pixel_ctrl_ptr + 1);
		}
			
	}

}
//draw in neighbours
void setOutline(int arr[(DISP_WIDTH/2)/UNIT][(DISP_HEIGHT)/UNIT]){

    for(int k = 0; k < (DISP_WIDTH)/UNIT; ++k){
        for(int l = 0; l < (DISP_HEIGHT)/UNIT; ++l){

            if(arr[k][l] == 1){
                
                for(int i = -1; i <= 1 ; i++){
                    for(int j = -1; j <= 1; j++){
                        
						if(abs(i) + abs(j) == 2) continue;
						
                        int xparam = k + i;
                        int yparam = l + j;

                        //ignore unit if beyond bound
                        if(xparam < 0) continue;
                        if(xparam >= (DISP_WIDTH)/UNIT) continue;
                        if(yparam < 0) continue;
                        if(yparam >= (DISP_HEIGHT)/UNIT) continue;

                        if(arr[xparam][yparam] == 0) arr[xparam][yparam] = 2;
                    }
                }
            
            }
        }
    }
    
}

                        
//get neighbour count
int getNeighbours(int arr[DISP_WIDTH][DISP_HEIGHT], int x, int y){
    
    int result = 0;
    
    for(int i = -1; i <= 1 ; i++){
        for(int j = -1; j <= 1; j++){
            
            int xparam = x + i;
            int yparam = y + j;
            
            //catch off screen instances (wrap around)    
            if(x + i < 0) xparam = DISP_WIDTH-1;
            if(x + i == DISP_WIDTH) xparam = 0;
            if(y + j < 0) yparam = DISP_HEIGHT-1;
            if(y + j == DISP_HEIGHT)yparam = 0;
        
            //tally neighbour count
            result += arr[xparam][yparam];
        }
    }
    
    //subtract the pixel itself
    result -= arr[x][y];
    
    return result;
}

//init
void randomInit(int arr[DISP_WIDTH][DISP_HEIGHT]){
    for (int i = 0; i < DISP_WIDTH; ++i){
        for (int j = 0; j < DISP_HEIGHT; ++j){
            arr[i][j] = rand()%2;
        }
    }
}

//pixel plot
void plot_pixel(int x, int y, short line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//clear screen
void clear_screen(){
    for(int y = 0; y < DISP_HEIGHT; y++){
        for(int x = 0; x < DISP_WIDTH ; x++){
            plot_pixel(x, y, 0x0);
        }
    }
}
			
//blackout array
void clear_array(int tempArr[DISP_WIDTH/2/UNIT][(DISP_HEIGHT)/UNIT]){
    for(int i = 0; i < DISP_WIDTH/2/UNIT; i++){
        for(int j = 0; j < (DISP_HEIGHT)/UNIT; j++){
            tempArr[i][j] = 0x0;
        }
    }
}

//wait for status bit
void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    register int status;
    (*pixel_ctrl_ptr) = 1;
    status = *(pixel_ctrl_ptr + 3);
    while((status & 0x1) != 0){
        status = *(pixel_ctrl_ptr + 3);
    }
}
