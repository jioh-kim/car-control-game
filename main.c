#include "imagearrays.c"

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define KEY_BASE              0xFF200050
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030
#define MPCORE_PRIV_TIMER	0xfffec600

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

//subroutine for the interrupts
void PS2_ISR();
void PRIV_TIMER_ISR();
void config_PS2();
void set_A9_IRQ_stack();
void enable_A9_interrupts();
void disable_A9_interrupts();
void config_GIC();
void config_interrupt(int N, int CPU_target);
void config_PRIV_TIMER();

//subroutines
void plot_pixel(int x, int y, short int line_color);
void swap(int *a, int *b);
void drawBackground();
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
void wait_for_vsync();
void drawLanes(int dy, int draw, int lane);
void draw_car(int xStart, int yStart, int arr[60][40]);
void moveCar(int leftOrRight);
void clearCar(int xStart);
void write_char(int x, int y, char c);
void draw_moving_car(int xStart, int dy, int clear, int arr[60][40], int carNumber);
void setup();
void clearChar();
void initialScreen();
void draw();
void drawBoom();

// global variable
volatile int pixel_buffer_start;
//Array of cars for random 
int* car_options[] = {purpleCarArray, yellowCarArray, policeCarArray, blueCarArray, greenCarArray};
//Moving car choice index
int carTypeOne = 0;
int carTypeTwo = 0;
int carTypeThree = 0;
//Moving car position index
int carStartPosOne = 0;
int carStartPosTwo = 0;
int carStartPosThree = 0;
//Don't draw
int dontDrawTwo = 0;
int dontDrawThree = 0;
//whether the crash has occured *sets to 1 when crashed
int crash = 0;
//global variables for positions of the lanes
int firstLineX = 130;
int secondLineX = 190;
//Array of Car positions for X coordinates
int car_positions[] = {82, 143, 205}; // x coordinate for each left, middle, right
//current position of our car
int current_position = 1;
//Keep track of each position
int laneStripes[] = {0, 49, 98, 147, 196};
//Keep track of the y position of the moving car
int movingCarDy[] = {0, 0, 0};
//Keeps track of the score
int scoreMultiplier = 0;
//Keeps track of the time
int time_value = 0;
//Keeps track of the time of the crash
int timeOfCrash = 0;
int scoreAtCrash = 0;
// global variable to see if user starts the game or not
int start = 0;

int main(void){
	setup();
	clearChar();
	initialScreen();
	draw();
}

void initialScreen(){
	while(1){
		for(int x = 0; x < 320; x++){
			for(int y = 0; y < 240; y++){
				plot_pixel(x, y, startPage[y][x]);
			}
		}
		
		if(start == 1){
			break;
		}
	}
}

// code for subroutines (not shown)
void drawBackground(){
	for(int x = 0; x < 320; x++){
        for(int y = 0; y < 240; y++){
            plot_pixel(x, y, mainRoadArray[y][x]);
        }
    }
}

void moveCar(int leftOrRight){ // left = 0, right = 1
    if(leftOrRight == 0){ // move left
        if (current_position == 1){
            clearCar(car_positions[1]);
            draw_car(car_positions[0], 160, ourCarArray);
            current_position = 0;
            return;
        }else if (current_position == 2){
            clearCar(car_positions[2]);
            draw_car(car_positions[1], 160, ourCarArray);
            current_position = 1;
            return;
        } else {
            return;
        }
    }

    if(leftOrRight == 1){ // move right
        if(current_position == 0){
            clearCar(car_positions[0]);
            draw_car(car_positions[1], 160, ourCarArray);
            current_position = 1;
            return;
        }else if (current_position == 1){
            clearCar(car_positions[1]);
            draw_car(car_positions[2], 160, ourCarArray);
            current_position = 2;
            return;
        } else {
            return;
        }
    }
}

//put boom sign when it crashes
void drawBoom(){
	for(int x = 0; x < 320; x++){
        for(int y = 0; y < 240; y++){
            plot_pixel(x, y, boom[y][x]);
        }
    }
	
	//delay for 5 seconds
	while(time_value-timeOfCrash<5){
		fflush(stdout);
	}
	
	for(int x = 0; x < 320; x++){
        for(int y = 0; y < 240; y++){
            plot_pixel(x, y, gameOver[y][x]);
        }
    }
}

//clear our Car by drawing the grey background over the car
void clearCar(int xStart){ 
	for(int y=160; y<220; y++){
		for(int x=0; x<40; x++){
			plot_pixel(x+xStart, y, 0x8C50);
		}
	}
}

void drawLanes(int dy, int draw, int lane){
	int colour = draw?0x8C50:0xFFFF;
	int y=0;
	int offset = 25;
	
	for(int x = firstLineX; x < firstLineX + 5; x++){
		for(y = dy; (y < dy+offset); y++){
			plot_pixel(x,y%240, colour);
			
		}
	}

	for(int x = secondLineX; x < secondLineX + 5; x++){
		for(y = dy; (y < dy+offset); y++){
			plot_pixel(x,y%240, colour);
		}
	}
}

void draw_car(int xStart, int yStart, int arr[60][40]){
	for(int y=0; y<60; y++){
		for(int x=0; x<40; x++){
			plot_pixel(x+xStart, y+yStart, arr[y][x]);
		}
	}
}

void draw_moving_car(int xStart, int dy, int clear, int arr[60][40], int carNumber){
	int y;
    if(crash == 0){
        for(y=0; y<60 && (y+dy)<240; y++){
            for(int x=0; x<40; x++){
                if (clear==1){
                    plot_pixel(x+xStart, (y+dy), 0x8C50); //clears with grey
                } else {
                    plot_pixel(x+xStart, (y+dy), arr[y][x]);
                }
            }
        }
        if ((y+dy)>240){
            if (carNumber==1){
                movingCarDy[0] = 0;
                carTypeOne = rand()%5;
                carStartPosOne = rand()%3;
            } else if (carNumber==2){
                movingCarDy[1] = 0;
                carTypeTwo = rand()%5;
                carStartPosTwo = rand()%3;
            } 
            
            if ((carStartPosOne == carStartPosTwo) && ((movingCarDy[0] <= 60) && (movingCarDy[1] <= 60))) {
                if (carStartPosOne == 0){
                    carStartPosTwo = 2;
                } else if (carStartPosOne == 1){
                    carStartPosTwo = 0;
                } else {
                    carStartPosTwo = 0;
                }   
            }
        }
    }	
}


void draw(){
	//initialize variables
	int speed = 0;
	carStartPosOne = rand()%3; // x position for the first car
	carTypeOne = rand()%5; // pick the first car 
	
	carStartPosTwo = rand()%3; // x position for the second car
	carTypeTwo = rand()%5; // pick the second car 
	
	if ((carStartPosOne == carStartPosTwo) && ((movingCarDy[0] <= 60) && (movingCarDy[1] <= 60))) {
		if (carStartPosOne == 0){
			carStartPosTwo = 2;
		} else if (carStartPosOne == 1){
			carStartPosTwo = 0;
		} else {
			carStartPosTwo = 0;
		}
	}

	drawBackground();
	draw_car(car_positions[1], 160, ourCarArray);
	
	while(1){
		fflush(stdout);
		
		//Clear car
		draw_moving_car(car_positions[carStartPosOne], movingCarDy[0], 1, car_options[carTypeOne], 1);
		
		//Clear second car
		if (time_value>15){
			draw_moving_car(car_positions[carStartPosTwo], movingCarDy[1], 1, car_options[carTypeTwo], 2);
		}
		
		//draw lanes
		drawLanes(laneStripes[0], 1, 1); //0x8C50
			
		//increment the lines
		speed = 1;
		laneStripes[0] = laneStripes[0] + speed;
		
		if (time_value<=10){
			speed = 3;
			scoreMultiplier = 2;
		} else if (time_value<30){
			speed = 4;
			scoreMultiplier = 3;
		} else if (time_value<60) {
			speed = 5;
			scoreMultiplier = 4;
		} else if (time_value<90){
			speed = 6;
			scoreMultiplier = 5;
		} else if (time_value<120) {
			speed = 7;
			scoreMultiplier = 6;
		} else {
			speed = 8;
			scoreMultiplier = 7;
		}
		
		movingCarDy[0] = movingCarDy[0] + speed;
		
		drawLanes(laneStripes[0], 0, 1); //0xFFFF
		
		//Draw second car
		if (time_value>15) {
			movingCarDy[1] = movingCarDy[1] + speed;
			draw_moving_car(car_positions[carStartPosTwo], movingCarDy[1], 0, car_options[carTypeTwo], 2);
		}
		
		//Draw car
		draw_moving_car(car_positions[carStartPosOne], movingCarDy[0], 0, car_options[carTypeOne], 1);
		 
		
		//check for collision
		if((movingCarDy[0] >= 100) && (movingCarDy[0] <= 220)){
			if(current_position == carStartPosOne){
				crash = 1;
				timeOfCrash = time_value;
				scoreAtCrash = time_value*scoreMultiplier;
				drawBoom();
				break;
			}
		}
		
		if((movingCarDy[1] >= 100) && (movingCarDy[1] <= 220)){
			if(current_position == carStartPosTwo){
				crash = 1;
				timeOfCrash = time_value;
				scoreAtCrash = time_value*scoreMultiplier;
				drawBoom();
				break;
			} 
		}
		
		//draw lanes
		drawLanes(laneStripes[1], 1, 2); //0x8C50
			
		//increment the lines
		speed = 1;
		laneStripes[1] = laneStripes[1] + speed;
		drawLanes(laneStripes[1], 0, 2); //0xFFFF
		
		//draw lanes
		drawLanes(laneStripes[2], 1, 3); //0x8C50
			
		//increment the lines
		speed = 1;
		laneStripes[2] = laneStripes[2] + speed;
		drawLanes(laneStripes[2], 0, 3); //0xFFFF
		
		//draw lanes
		drawLanes(laneStripes[3], 1, 4); //0x8C50
			
		//increment the lines
		speed = 1;
		laneStripes[3] = laneStripes[3] + speed;
		drawLanes(laneStripes[3], 0, 4); //0xFFFF
		
		//draw lanes
		drawLanes(laneStripes[4], 1, 5); //0x8C50
			
		//increment the lines
		speed = 1;
		laneStripes[4] = laneStripes[4] + speed;
		drawLanes(laneStripes[4], 0, 5); //0xFFFF
		
		wait_for_vsync();
	}
}

void clearChar(){
	int x = 3;
	char* clearSpace = "                                      ";
	while (*clearSpace) {
		write_char(x, 2, *clearSpace);
		x++;
		clearSpace++;		
	}
	clearSpace = "                                      ";
	x = 324;
	while (*clearSpace) {
		write_char(x, 0, *clearSpace);
		x++;
		clearSpace++;
	}
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void wait_for_vsync(){
    volatile int* pixel_ctrl_ptr = (int *)0xFF203020;
    register int status;

    *pixel_ctrl_ptr = 1; //start the synchronization preocess

	status = *(pixel_ctrl_ptr + 3);
	while((status & 0x01) != 0){ // pulled I/O
		status = *(pixel_ctrl_ptr + 3); // wait for s to become 0
	}
}

void draw_line(int x0, int y0, int x1, int y1, short int line_color){
	int is_steep = 0;
	is_steep = abs(y1-y0) > abs(x1-x0);
	
	if(is_steep){ // swap x0 and y0, swap x1, y1
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	
	if(x0>x1){
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
	
	int deltaX = x1 - x0;
	int deltaY = abs(y1 - y0);
	int error = -(deltaX / 2);
	int y = y0;
	int y_step = 0; 
	if (y0 < y1){
		y_step = 1;
	} else {
		y_step = -1;
	}
	
	for(int i = x0; i <= x1; i++){
		if(is_steep){
			plot_pixel(y, i, line_color);
		} else {
			plot_pixel(i, y, line_color);
		}
		error = error + deltaY;
		if(error >= 0){
			y = y + y_step;
			error = error - deltaX;
		}
	}
}

void swap(int *a, int *b){
	int temp = *a;
	*a = *b;
	*b = temp;
}

//http://www-ug.eecg.utoronto.ca/desl/nios_devices_SoC/dev_ps2
void PS2_ISR(){ 
	unsigned char byte1 = 0;
	unsigned char byte2 = 0;
	
	volatile int *PS2_ptr = (int *)0xFF200100; 
    int PS2_data, RVALID, RAVAIL;
    PS2_data = *(PS2_ptr);         // read the Data register in the PS/2 port
    RVALID = (PS2_data & 0x8000);  // extract the RVALID field
	RAVAIL = PS2_data & 0xFFFF0000;
	
	int interruptReg;
	interruptReg = *(PS2_ptr + 1);
	*(PS2_ptr + 1) = interruptReg;
	
	if(RAVAIL == 0 && RVALID) {
		fflush(stdout);
		byte1 = PS2_data & 0xFF;
		PS2_data = *(PS2_ptr);
		RVALID = PS2_data & 0x8000;
		RAVAIL = PS2_data & 0xFFFF0000;
		
		if (byte1 == (char)0xF0) {
			if(RVALID && RAVAIL == 0){
			byte2 = PS2_data & 0xFF;
				
				if ((byte2 == (char)0x23) && (start == 1) && (crash !=1)) { // "D key" to move right
					moveCar(1);
					return;
				}
				if ((byte2 == (char)0x1C) && (start == 1) && (crash !=1)) { // "A key" to move left
					moveCar(0);
					return;
				}
				if (( byte2 == (char)0x29) || ( byte2 == (char)0x5A)){
					// 0x29 == space, 0x5A == space
					start = 1;
					return;
				}
			}
		}
	}
}

void PRIV_TIMER_ISR(){

	volatile int * timer_ptr = (int *) MPCORE_PRIV_TIMER;
	*(timer_ptr+3) = 1;
	if ((start == 1) && (crash!=1)){
		int x = 3;
		char* clearSpace = "                                      ";
		while (*clearSpace) {
			write_char(x, 2, *clearSpace);
			x++;
			clearSpace++;
		}
		//char * number_str = " ";
		char number_str[20];

		sprintf(number_str, "%d", time_value);
		//printf("%d \n", crash);
		//itoa(amount, number_str, 10);

		//Display time
		char *timeDisplayed = "Time: ";
		char *scoreDisplayed = "Score: ";
		//strcat(timeDisplayed, number_str);
		x = 3;

		while (*timeDisplayed) {
			write_char(x, 2, *timeDisplayed);
			x++;
			timeDisplayed++;
		}

		int i = 0;
		while (number_str[i]) {
			write_char(x, 2, number_str[i]);
			x++;
			i++;
		}

		clearSpace = "                                      ";
		x = 324;
		while (*clearSpace) {
			write_char(x, 0, *clearSpace);
			x++;
			clearSpace++;
		}

		x= 324;
		while (*scoreDisplayed) {
			write_char(x, 0, *scoreDisplayed);
			x++;
			scoreDisplayed++;
		}

		char score_value_str[20];

		sprintf(score_value_str, "%d", time_value*scoreMultiplier);

		i=0;
		while (score_value_str[i]) {
			write_char(x, 0, score_value_str[i]);
			x++;
			i++;
		}
		time_value++;
	} else if (crash==1){
		int x = 3;
		char* clearSpace = "                                      ";
		while (*clearSpace) {
			write_char(x, 2, *clearSpace);
			x++;
			clearSpace++;
		}
		//char * number_str = " ";
		char number_str[20];

		sprintf(number_str, "%d", timeOfCrash);
		//printf("%d \n", crash);
		//itoa(amount, number_str, 10);

		//Display time
		char *timeDisplayed = "Time: ";
		char *scoreDisplayed = "Score: ";
		//strcat(timeDisplayed, number_str);
		x = 3;

		while (*timeDisplayed) {
			write_char(x, 2, *timeDisplayed);
			x++;
			timeDisplayed++;
		}

		int i = 0;
		while (number_str[i]) {
			write_char(x, 2, number_str[i]);
			x++;
			i++;
		}

		clearSpace = "                                      ";
		x = 324;
		while (*clearSpace) {
			write_char(x, 0, *clearSpace);
			x++;
			clearSpace++;
		}

		x= 324;
		while (*scoreDisplayed) {
			write_char(x, 0, *scoreDisplayed);
			x++;
			scoreDisplayed++;
		}

		char score_value_str[20];

		sprintf(score_value_str, "%d", scoreAtCrash);

		i=0;
		while (score_value_str[i]) {
			write_char(x, 0, score_value_str[i]);
			x++;
			i++;
		}
		time_value++;
	}
}

/* write a single character to the character buffer at x,y
 * x in [0,79], y in [0,59]
 */
void write_char(int x, int y, char c) {
  // VGA character buffer
  volatile char * character_buffer = (char *) (FPGA_CHAR_BASE + (y<<7) + x);
  *character_buffer = c;
}

/* setup the PS/2 interrupts in the FPGA */
void config_PS2() {
	volatile int * PS2_ptr = (int *) 0xFF200100; // PS/2 base address
	*(PS2_ptr + 1) = 0x00000001; // set RE to 1 to enable interrupts
}

// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void) {
	// Read the ICCIAR from the CPU Interface in the GIC
	int interrupt_ID = *((int *)0xFFFEC10C);
	if (interrupt_ID == 79) // check if interrupt is from the KEYs
	PS2_ISR();
	else if (interrupt_ID == 29)
	PRIV_TIMER_ISR();
	else
	while (1); // if unexpected, then stay here
	// Write to the End of Interrupt Register (ICCEOIR)
	*((int *)0xFFFEC110) = interrupt_ID;
}

// Define the remaining exception handlers
void __attribute__((interrupt)) __cs3_reset(void) {
	while (1);
}

void __attribute__((interrupt)) __cs3_isr_undef(void) {
	while (1);
}

void __attribute__((interrupt)) __cs3_isr_swi(void) {
	while (1);
}

void __attribute__((interrupt)) __cs3_isr_pabort(void) {
	while (1);
}

void __attribute__((interrupt)) __cs3_isr_dabort(void) {
	while (1);
}

void __attribute__((interrupt)) __cs3_isr_fiq(void) {
	while (1);
}

//Initialize the banked stack pointer register for IRQ mode
void set_A9_IRQ_stack(void) {
	int stack, mode;
	stack = 0xFFFFFFFF - 7; // top of A9 onchip memory, aligned to 8 bytes
	/* change processor to IRQ mode with interrupts disabled */
	mode = 0b11010010;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r"(stack));
	/* go back to SVC mode before executing subroutine return! */
	mode = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
}

/*
* Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void) {
	int status = 0b01010011;
	asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}

// Turn off interrupts in the ARM processor
void disable_A9_interrupts(void) {
	int status = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}

/*
* Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC() {
	config_interrupt (79, 1); // configure the FPGA KEYs interrupt (73)
	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all
	// priorities
	config_interrupt (29, 1);
	*((int *) 0xFFFEC104) = 0xFFFF;
	// Set CPU Interface Control Register (ICCICR). Enable signaling of
	// interrupts
	*((int *) 0xFFFEC100) = 1;
	// Configure the Distributor Control Register (ICDDCR) to send pending
	// interrupts to CPUs
	*((int *) 0xFFFED000) = 1;
}

void config_PRIV_TIMER(){
	volatile int * timer_ptr = (int *) MPCORE_PRIV_TIMER;
	*(timer_ptr+2) = 7;
	*timer_ptr = 200000000;
}

/*
* Configure Set Enable Registers (ICDISERn) and Interrupt Processor Target
* Registers (ICDIPTRn). The default (reset) values are used for other registers
* in the GIC.
*/
void config_interrupt(int N, int CPU_target) {
	int reg_offset, index, value, address;
	/* Configure the Interrupt Set-Enable Registers (ICDISERn).
	* reg_offset = (integer_div(N / 32) * 4
	* value = 1 << (N mod 32) */
	reg_offset = (N >> 3) & 0xFFFFFFFC;
	index = N & 0x1F;
	value = 0x1 << index;
	address = 0xFFFED100 + reg_offset;
	/* Now that we know the register address and value, set the appropriate bit */
	*(int *)address |= value;

	/* Configure the Interrupt Processor Targets Register (ICDIPTRn)
	* reg_offset = integer_div(N / 4) * 4
	* index = N mod 4 */
	reg_offset = (N & 0xFFFFFFFC);
	index = N & 0x3;
	address = 0xFFFED800 + reg_offset + index;
	/* Now that we know the register address and value, write to (only) the
	* appropriate byte */
	*(char *)address = (char)CPU_target;
}

void setup(){
	srand(time(0));
	disable_A9_interrupts();
	set_A9_IRQ_stack();// initialize the stack pointer for IRQ mode
	config_GIC();// configure the general interrupt controller
	config_PS2();
	config_PRIV_TIMER();
	enable_A9_interrupts();// enable interrupts
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    pixel_buffer_start = *pixel_ctrl_ptr;
}
