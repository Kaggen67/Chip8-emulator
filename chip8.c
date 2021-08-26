#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "chip8.h"

#define STACK_SIZE 16
#define STACK_MASK 0x0F
#define MEMORY_SIZE 65536
#define WIDTH 64			// CHIP8 resolution WIDTH
#define HEIGHT 32			// CHIP8 resolution HEIGHT
#define S_WIDTH 128			// Superchip resolution WIDTH
#define S_HEIGHT 64			// Superchip resolution HEIGHT
#define FRAMETIME (1000000 / 60)	// Frametime is the time between each frame in usec.

//#define PRINT_CODE
//#define COMPATIBILITY_SHIFT		// Compatibility mode for shift instruction

UBYTE charset[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,	// 0
	0x20, 0x60, 0x20, 0x20, 0x70,	// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,	// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,	// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,	// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,	// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,	// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,	// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,	// 8
	0xF0, 0x90, 0xF0, 0x10, 0x10,	// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,	// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,	// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,	// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,	// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,	// E
	0xF0, 0x80, 0xF0, 0x80, 0x80	// F
};

/*
 * 
 * Parsing file: trimmed_charset16.bmp
 *
 * BMP header: BM
 * Bits/pixel 1
 * Width: 128, Height: 10
 *
 */
UBYTE charset16[] = {
	0x7E,0x7E,0x66,0x66,0x66,0x66,0x66,0x66,0x7E,0x7E,
	0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x7E,
	0x7E,0x7E,0x06,0x06,0x7E,0x7E,0x60,0x60,0x7E,0x7E,
	0x7E,0x7E,0x06,0x06,0x3E,0x3E,0x06,0x06,0x7E,0x7E,
	0x66,0x66,0x66,0x66,0x7E,0x7E,0x06,0x06,0x06,0x06,
	0x7E,0x7E,0x60,0x60,0x7E,0x7E,0x06,0x06,0x7E,0x7E,
	0x7E,0x7E,0x60,0x60,0x7E,0x7E,0x66,0x66,0x7E,0x7E,
	0x7E,0x7E,0x06,0x06,0x0E,0x1C,0x18,0x18,0x18,0x18,
	0x7E,0x7E,0x66,0x66,0x7E,0x7E,0x66,0x66,0x7E,0x7E,
	0x7E,0x7E,0x66,0x66,0x7E,0x7E,0x06,0x06,0x7E,0x7E,
	0x3C,0x7E,0x66,0x66,0x7E,0x7E,0x66,0x66,0x66,0x66,
	0x7C,0x7E,0x66,0x66,0x7C,0x7E,0x66,0x66,0x7E,0x7C,
	0x3C,0x7E,0x66,0x60,0x60,0x60,0x60,0x66,0x7E,0x3C,
	0x7C,0x7E,0x66,0x66,0x66,0x66,0x66,0x66,0x7E,0x7C,
	0x7E,0x7E,0x60,0x60,0x7C,0x7C,0x60,0x60,0x7E,0x7E,
	0x7E,0x7E,0x60,0x60,0x7C,0x7C,0x60,0x60,0x60,0x60
};

static int tcount;
static int instruction_count;
static int instructions_per_sec;
static unsigned int PC;			// Program Counter
static unsigned int I;				// Indirect Register
static unsigned int SP;			// Stack Pointer
static unsigned int last_address;
static unsigned short last_opcode;
unsigned int current_address;
unsigned short current_opcode;
static unsigned short cwidth, cheight;		// Current Width and Height.
static int keys[16];				// Keypad.
static unsigned short Stack[STACK_SIZE];	// Stack
UBYTE *mem;					// Memory
static UBYTE V[16];				// V registers
static UBYTE R[8];				// R registers. For Superchip HP-48 compatibility.
static int ch16;					// Index to charset for Superchip.
static UBYTE plane;				// XO-Chip active drawing plane.
//static UINT32 vbuf[HEIGHT * WIDTH + WIDTH];	// Converted videomemory for display on 32 bit platform.
//UBYTE vbuf[S_HEIGHT * S_WIDTH + S_WIDTH];	// Always allocate max screen size.
UBYTE *vbuf;					// Always allocate max screen size.
static UBYTE delay_timer;			// Delay Timer. Counts down at 60Hz until it reach zero.
static UBYTE sound_timer;			// Sound Timer. Counts down at 60Hz to zero while playing a sound.
							// Stops playing at zero.
static unsigned int program_size;		// Size of program loaded.

static clock_t time_start;			// Used to time frametime.

static unsigned short compat_flags;		// Compatibility flags

static unsigned short chip8_status_flags;	// Status flags

//static UBYTE plane;				// Actitive bitplane when drawing sprites/scrolling

void chip8_debug_print_videomemory();

int gettimer();

unsigned short chip8_status_get_flags();
void chip8_status_set_flag(unsigned short);
void chip8_status_clear_flag(unsigned short);

// Clear videomemory.
void chip8_cls(UBYTE all) {
	int i, len;

	// If all is NOT NULL, clear all videomemory as in superchip mode (128x64).
	if(all) {
		len = S_WIDTH * S_HEIGHT;
	} else {
		len = cwidth * cheight;
	}

	for(i = 0; i < len; i++) {
		vbuf[i] = 0x00;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

// XO-CHIP function for multicolor (multibitplane) sprites.
// Function for drawing a sprite to videomemory while checking for collision.
// Returns 1 if a collision occurred. else returns 0;

int chip8_draw_sprite_XO(UBYTE x, UBYTE y, UBYTE n) {
	int rp, r, c, i, collide;
	UBYTE sprb, msk;
	UBYTE layers;
	i = I;
	int starttime, stoptime;

	starttime = gettimer();

	collide = 0;
	layers = plane;

	for(msk = 0x01; layers != 0; layers >>= 1, msk <<= 1) {
		if(!(layers & 0x01)) {
			continue;
		}
		for(r = y; r < y+n; r++) {
			rp = (r & (cheight -1)) * cwidth;
			sprb = mem[i++];
			for(c = 0; c < 8; c++) {
				if((sprb << c) & 0x80) {
					if(vbuf[rp + ((x+c) & (cwidth -1))] & msk) {
						collide = 1;
					}
					vbuf[rp + ((x+c) & (cwidth -1))] ^= msk;
				}
			}
		}
	}
	stoptime = gettimer();

	chip8_status_set_flag(F_DRAW_RDY);
	// printf("Drawsprite planes %d, time %d us\n", plane, (stoptime - starttime));
	return collide;
}

int chip8_draw_sprite16_XO(UBYTE x, UBYTE y, UBYTE n) {
	int rp, r, c, i, collide;
	unsigned short sprb, msk;
	UBYTE layers;
	i = I;

	collide = 0;
	layers = plane;

	x &= 127;
	y &= 63;

	for(msk = 0x01; layers != 0; layers >>= 1, msk <<= 1) {
		if(!(layers & 0x01)) {
			continue;
		}
		for(r = y; r < y + 16; r++) {
			rp = (r & (cheight -1)) * cwidth;
			sprb = (mem[i] << 8) + mem[i+1];
			i += 2;
			for(c = 0; c < 16; c++) {
				if((sprb << c) & 0x8000) {
					if(vbuf[rp + ((x+c) & (cwidth -1))] & msk) {
						collide = 1;
					}
					vbuf[rp + ((x+c) & (cwidth -1))] ^= msk;
				}
			}
		}
	}
	chip8_status_set_flag(F_DRAW_RDY);
	return collide;
}



// Function for drawing a sprite to videomemory while checking for collision.
// Returns 1 if a collision occurred. else returns 0;

int chip8_draw_sprite(UBYTE x, UBYTE y, UBYTE n) {
	int rp, r, c, i, collide;
	UBYTE sprb;
	i = I;

	collide = 0;

	for(r = y; r < y+n; r++) {
		rp = (r & (cheight -1)) * cwidth;
		sprb = mem[i++];
		for(c = 0; c < 8; c++) {
			if((sprb << c) & 0x80) {
				if(vbuf[rp + ((x+c) & (cwidth -1))]) {
					collide = 1;
				}
				vbuf[rp + ((x+c) & (cwidth -1))] ^= 0x01;
			}
		}
	}
	chip8_status_set_flag(F_DRAW_RDY);
	return collide;
}


int chip8_draw_sprite16(UBYTE x, UBYTE y, UBYTE n) {
	int rp, r, c, i, collide;
	unsigned short sprb;
	i = I;

	collide = 0;

	x &= 127;
	y &= 63;

	for(r = y; r < y + 16; r++) {
		rp = (r & (cheight -1)) * cwidth;
		sprb = (mem[i] << 8) + mem[i+1];
		i += 2;
		for(c = 0; c < 16; c++) {
			if((sprb << c) & 0x8000) {
				if(vbuf[rp + ((x+c) & (cwidth -1))]) {
					collide = 1;
				}
				vbuf[rp + ((x+c) & (cwidth -1))] ^= 0x01;
			}
		}
	}
	chip8_status_set_flag(F_DRAW_RDY);
	return collide;
}


// Handle statusflags
// F_DRAW_RDY...
unsigned short chip8_status_get_flags() {
	return chip8_status_flags;
}


void chip8_status_set_flag(unsigned short f) {
	chip8_status_flags |= (1 << f);
}

void chip8_status_clear_flag(unsigned short f) {
	chip8_status_flags &= ~(1 << f);
}


UBYTE *chip8_get_V_regs() {
	return V;
}


unsigned int chip8_get_I() {
	return I;
}


unsigned int chip8_get_SP() {
	return SP;
}

unsigned short chip8_get_opcode() {
	return (mem[PC] << 8) | mem[PC+1];
}

UBYTE *chip8_get_mem_ptr() {
	return mem;
}


// Register keys pressed and released.
void chip8_key_pressed(int key) {
	key &= 0x0F;
	keys[key] = 1;
}


void chip8_key_released(int key) {
	key &= 0x0F;
	keys[key] = 0;
}



// Need this to keep track of timers.
int gettimer() {
	
	struct timespec tp;
	long int res;

	if(clock_gettime(CLOCK_MONOTONIC, &tp)) {
		return 0;
	}

	res = (tp.tv_sec * 1000000 + (tp.tv_nsec / 1000));

	//printf("tv_sec: %d, tv_nsec: %d\n", (int)tp.tv_sec, (int)tp.tv_nsec);

	//printf("clock_gettime: %ld\n", res);

	return (int)res;

}

unsigned int chip8_get_programsize() {
	return program_size;
}

void chip8_set_compatibility(int f) {
	compat_flags |= 1 << f;
}

void chip8_clear_compatibility(int f) {
	compat_flags &= ~(1 << f);
}

unsigned short chip8_get_compatibility_flags() {
	return compat_flags;
}

void chip8_set_compatibility_flags(unsigned short cmp) {
	compat_flags = cmp;
}


// XO-CHIP Multicolor scroll
void scroll_down_XO(int p) {
	UBYTE layers;
	int i,y;

	layers = plane;

	y = (cheight - p) * cwidth -1;
	i = cheight * cwidth -1;

	for(; y >= 0; y--, i--) {
		vbuf[i] = (vbuf[i] & ~layers) | (vbuf[y] & layers);
	}
	for(; i >= 0; i--) {
		vbuf[i] &= ~layers;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

void scroll_up_XO(int p) {
	UBYTE layers;
	int i,y;

	layers = plane;

	y = p * cwidth;
	i = 0;

	for(; y < cwidth * cheight -1; y++, i++) {
		vbuf[i] = (vbuf[i] & ~layers) | (vbuf[y] & layers);
	}
	for(; i < cwidth * cheight -1; i++) {
		vbuf[i] &= ~layers;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

void scroll_right4_XO() {
	UBYTE layers;
	int x,y;

	layers = plane;

	y = cwidth * cheight -5;
	x = cwidth * cheight -1;

	for(; y >= 0; y--, x--) {
		vbuf[y] = (vbuf[y] & ~layers) | (vbuf[x] & layers);
	}

	for(x = 0, y = 0; y < cheight; y++, x += cwidth) {
		vbuf[x] &= ~layers;
		vbuf[x+1] &= ~layers;
		vbuf[x+2] &= ~layers;
		vbuf[x+3] &= ~layers;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

void scroll_left4_XO() {
	UBYTE layers;
	int x,y;

	layers = plane;

	for(y = 0, x = 4; x < cwidth * cheight; y++, x++) {
		vbuf[y] = (vbuf[y] & ~layers) | (vbuf[x] & layers);
	}

	for(y = 0, x = cwidth -4; y < cheight; y++, x += cwidth) {
		vbuf[x] &= ~layers;
		vbuf[x+1] &= ~layers;
		vbuf[x+2] &= ~layers;
		vbuf[x+3] &= ~layers;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}


// SUPERCHIP scroll.
void scroll_down(int p) {
	int i,y;

	y = (cheight - p) * cwidth -1;
	i = cheight * cwidth -1;

	for(; y >= 0; y--, i--) {
		vbuf[i] = vbuf[y];
	}
	for(; i >= 0; i--) {
		vbuf[i] = 0;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

void scroll_right4() {
	int x,y;

	y = cwidth * cheight -5;
	x = cwidth * cheight -1;

	for(; y >= 0; y--, x--) {
		vbuf[y] = vbuf[x];
	}

	for(x = 0, y = 0; y < cheight; y++, x += cwidth) {
		vbuf[x] = 0x00;
		vbuf[x+1] = 0x00;
		vbuf[x+2] = 0x00;
		vbuf[x+3] = 0x00;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

void scroll_left4() {
	int x,y;

	for(y = 0, x = 4; x < cwidth * cheight; y++, x++) {
		vbuf[y] = vbuf[x];
	}

	for(y = 0, x = cwidth -4; y < cheight; y++, x += cwidth) {
		vbuf[x] = 0x00;
		vbuf[x+1] = 0x00;
		vbuf[x+2] = 0x00;
		vbuf[x+3] = 0x00;
	}
	chip8_status_set_flag(F_DRAW_RDY);
}

void chip8_reset() {
	int i, k;

	// Set default width and height.
	cwidth = WIDTH;
	cheight = HEIGHT;

	// Clear status flags
	chip8_status_flags = 0;

	// Clear screen
	chip8_cls(1);

	//init drawingplane.
	plane = 1;

	// Init registers.
	PC = 0x200;
	SP = -1;
	delay_timer = 0;
	sound_timer = 0;
	tcount = 0;
	instruction_count = 0;
	instructions_per_sec = 0;
	for(i = 0; i < 16; i++) {
		V[i] = 0;
		keys[i] = 0;
	}

	// Init charset.
	for(i = 0; i < sizeof(charset); i++) {
		mem[i] = charset[i];
	}
	// Init charset 16.
	ch16 = i;
	for(k = 0; k < sizeof(charset16); k++) {
		mem[i + k] = charset16[k];
	}

	// Start counting frametime.
	time_start = gettimer();
}


void chip8_init() {
	vbuf = malloc(S_HEIGHT * S_WIDTH + S_WIDTH);
	mem = malloc(MEMORY_SIZE);
	compat_flags = (1 << COMPAT_SHIFT_QUIRK) | (1 << COMPAT_I_QUIRK);
	chip8_reset();
}

// Load a chip8 ROM/Porgram file.
int chip8_load_rom(char* filename) {
	FILE *fh;

	fh = fopen(filename, "r");

	if(!fh) {
		printf("Couldn't open file %s\n", filename);
		return 0;
	}

	program_size = fread(&mem[0x200],1, (MEMORY_SIZE - 0x200), fh);
	if(!program_size) {
		printf("Couldn't read file %s\n", filename);
		fclose(fh);
		return 0;
	}
	fclose(fh);
	return program_size;
}


// Method to get chip8 video resolution WIDTH
int chip8_get_width() {
	return cwidth;
}

// Method to get chip8 video resolution HEIGHT
int chip8_get_height() {
	return cheight;
}

// Method to get pointer to chip8 videomemory/videobuffer.
//UINT32 *chip8_get_vbuf() {
UBYTE *chip8_get_vbuf() {
	return vbuf;
}

unsigned int chip8_get_pc() {
	return (PC & 0x0FFF);
}

int chip8_get_instructions_per_sec() {
	return instructions_per_sec;
}

UBYTE *chip8_get_program_memory() {
	return mem;
}

// Print the videomemory to console for debugging purposes.
// Originally 64x32 characters.
void chip8_debug_print_videomemory() {
	int i, lc, len;

	len = WIDTH * HEIGHT;
	lc = 0;

	printf("################################################################\n");
	for(i = 0; i < len; i++) {
		if(vbuf[i]) {
			// Plot white pixel.
			putchar('#');
		} else {
			// Plot black pixel.
			putchar(' ');
		}
		lc++;
		if(lc == WIDTH) {
			lc = 0;
			printf("\n");
		}
	}
	printf("################################################################\n");
}


int chip8_execute_opcode() {
	int i;
	unsigned short opcode;
	UBYTE c, x, y, n, nn;
	unsigned short nnn;
	unsigned int tmp;

	PC = PC & 0x0FFF;

	opcode = mem[PC] * 0x100 + mem[PC+1];
	c = (opcode >> 12) & 0x0F;
	x = (opcode >> 8) & 0x0F;
	y = (opcode >> 4) & 0x0F;
	n = opcode & 0x0F;
	nn = opcode & 0xFF;
	nnn = opcode & 0x0FFF;

	// Print current adress and opcode in hex
	//
#ifdef PRINT_CODE
	printf("0x%04X  0x%04X\t", PC, opcode);
#endif

	// SCROLU n : XO-Chip command.
	if(0x00D0 == (opcode & 0xFFF0) && (compat_flags & (1 << COMPAT_ENABLE_XO))) {
		scroll_up_XO(n);
		PC += 2;
#ifdef PRINT_CODE
		printf("SCROLU %d\n", n);
#endif

	// CLS : Clear display
	} else if(opcode == 0x00E0) {
		chip8_cls(0);
		PC += 2;
#ifdef PRINT_CODE
		printf("CLS\n");
#endif
	// RET : Return from subroutine
	} else if(opcode == 0x00EE) {
		PC = Stack[SP];
		SP = (SP - 1);
#ifdef PRINT_CODE
		printf("RET\n");
#endif
	// LORES : Superchip command. Set resolution to 64 x 32.
	} else if(opcode == 0x00FE) {
		cwidth = WIDTH;
		cheight = HEIGHT;
		PC += 2;


#ifdef PRINT_CODE
		printf("LORES\n");
#endif
	// HIRES : Superchip command. Set hight resolution to 128 x 64.
	} else if(opcode == 0x00FF) {
		cwidth = S_WIDTH;
		cheight = S_HEIGHT;
		PC += 2;


#ifdef PRINT_CODE
		printf("HIRES\n");
#endif
	// SCROLD n : Superchip command. Scroll down by 0 - 15 pixels.
	} else if((opcode & 0xFFF0) == 0x00C0) {
		if(compat_flags & (1 << COMPAT_ENABLE_XO)) {
			scroll_down_XO(n & 0x0F);
		} else {
			scroll_down(n & 0x0F);
		}
		PC += 2;


#ifdef PRINT_CODE
		printf("SCROLD %1X\n", n);
#endif
	// SCROLR : Superchip command. Scroll right by 4 pixels.
	} else if(opcode == 0x00FB) {
		if(compat_flags & (1 << COMPAT_ENABLE_XO)) {
			scroll_right4_XO();
		} else {
			scroll_right4();
		}
		PC += 2;


#ifdef PRINT_CODE
		printf("SCROLR\n");
#endif
	// SCROLL : Superchip command. Scroll left by 4 pixels.
	} else if(opcode == 0x00FC) {
		if(compat_flags & (1 << COMPAT_ENABLE_XO)) {
			scroll_left4_XO();
		} else {
			scroll_left4();
		}
		PC += 2;


#ifdef PRINT_CODE
		printf("SCROLL\n");
#endif
	// JP nnn : Jump to adress nnn
	} else if(c == 0x01) {
		PC = nnn;
#ifdef PRINT_CODE
		printf("JUMP 0x%03X\n", nnn);
#endif
	// CALL nnn : Call subroutine at adress nnn
	} else if(c == 0x02) {
		Stack[++SP] = (unsigned short)((PC+2) & 0x0FFF);
		SP &= STACK_MASK;
		PC = nnn;
#ifdef PRINT_CODE
		printf("CALL 0x%03X\n", nnn);
#endif
	// SE Vx, nn : Skip next instruction if Vx == nn
	} else if(c == 0x03) {
		if(V[x] == nn) {
			PC += 2;
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("SE V%X, 0x%02X\n", x, nn);
#endif
	// SNE Vx, nn : Skip next instruction if Vx != nn
	} else if(c == 0x04) {
		if(V[x] != nn) {
			PC += 2;
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("SNE V%X, 0x%02X\n", x, nn);
#endif
	} else if(c == 0x05) {

		// SE Vx, Vy : Skip next instruction if Vx == Vy
		if(n == 0x00) {
			if(V[x] == V[y]) {
				PC += 2;
			}
			PC += 2;
#ifdef PRINT_CODE
		printf("SE V%X, V%X\n", x, y);
#endif

		// SAVE I,Vx-Vy : XO-Chip command.
		} else if(n == 0x02 && (compat_flags & (1 << COMPAT_ENABLE_XO))) {
			tmp = abs(x - y);
			if(x < y) {
				for(i = 0; i <= tmp; i++) {
					mem[I+i] = V[x+i];
				}
			} else {
				for(i = 0; i <= tmp; i++) {
					mem[I+i] = V[x-i];
				}
			}
			PC += 2;
#ifdef PRINT_CODE
			printf("SAVE I,V%1X-V%1X\n", x, y);
#endif

		// LOAD Vx-Vy, I : XO-Chip command.
		} else if(n == 0x03 && (compat_flags & (1 << COMPAT_ENABLE_XO))) {
			tmp = abs(x - y);
			if(x < y) {
				for(i = 0; i <=tmp; i++) {
					V[x+i] = mem[I+i];
				}
			} else {
				for(i = 0; i<= tmp; i++) {
					V[x-i] = mem[I+i];
				}
			}
			PC += 2;
#ifdef PRINT_CODE
			printf("LOAD V%1X - V%1X\n", x, y);
#endif
		}

	// LD Vx, nn : Set Vx = nn
	} else if(c == 0x06) {
		V[x] = nn;
		PC += 2;
#ifdef PRINT_CODE
		printf("LD V%X, 0x%02X\n", x, nn);
#endif
	// ADD Vx, nn : Set Vx = Vx + nn
	} else if(c == 0x07) {
		V[x] = (V[x] + nn) & 0xFF;
		PC += 2;
#ifdef PRINT_CODE
		printf("ADD V%X, 0x%02X\n", x, nn);
#endif

	// Group instructions with c = 0x08 together for optimization
	} else if(c == 0x08) {
		// LD Vx, Vy : Set Vx = Vy
		if(n == 0x0) {
			V[x] = V[y];
			PC += 2;
#ifdef PRINT_CODE
			printf("LD V%X, V%X\n", x, y);
#endif
		// OR Vx, Vy : Set Vx = Vx OR Vy
		} else if(n == 0x01) {
			V[x] |= V[y];
			PC += 2;
#ifdef PRINT_CODE
			printf("OR V%X, V%X\n", x, y);
#endif
		// AND Vx, Vy : Set Vx = Vx AND Vy
		} else if(n == 0x02) {
			V[x] &= V[y];
			PC += 2;
#ifdef PRINT_CODE
			printf("AND V%X, V%X\n", x, y);
#endif
		// XOR Vx, Vy : Set Vx = Vx XOR Vy
		} else if(n == 0x03) {
			V[x] = (V[x] ^ V[y]) & 0xFF;
			PC += 2;
#ifdef PRINT_CODE
			printf("XOR V%X, V%X\n", x, y);
#endif
		// ADD Vx, Vy : Set Vx = Vx + Vy, set VF = carry
		} else if(n == 0x04) {
			tmp = V[x] + V[y];
			V[0xF] = 0x00;
			if(tmp > 255) {
				V[0xF] = 0x01;
			}
			V[x] = (UBYTE)(tmp & 0xFF);
			PC += 2;
#ifdef PRINT_CODE
			printf("ADDC V%X, V%X\n", x, y);
#endif
		// SUB Vx, Vy : Set Vx = Vx - Vy, set VF = NOT borrow
		} else if(n == 0x05) {
			V[0xF] = 0x00;
			if(V[x] >= V[y]) {
				V[0xF] = 0x01;
			}
			V[x] = (V[x] - V[y]) & 0xFF;
			PC += 2;
#ifdef PRINT_CODE
			printf("SUB V%X, V%X\n", x, y);
#endif
		// Compatibility: SHR Vx : Set Vx = Vx >> 1 and shift out least significant bit to VF
		// SHR Vx, Vy : Set Vx = Vy = Vy >> 1, VF = least significant bit
		} else if(n == 0x06) {
			V[0xF] = 0x00;
			if(compat_flags & (1 << COMPAT_SHIFT_QUIRK)) {
				if(V[x] & 0x01) {
					V[0xF] = 0x01;
				}
				V[x] = (V[x] >> 1) & 0xFF;
			} else {
				if(V[y] & 0x01) {
					V[0xF] = 0x01;
				}
				V[x] = (V[y] >> 1) & 0xFF;
			}
			PC += 2;
#ifdef PRINT_CODE
			printf("SHR V%X\n", x);
#endif
		// SUBN Vx, Vy : Set Vx = Vy - Vx, set VF = NOT borrow (if Vy >= Vx, VF = 1)
		} else if(n == 0x07) {
			V[0xF] = 0x00;
			if(V[y] >= V[x]) {
				V[0xF] = 0x01;
			}
			V[x] = (V[y] - V[x]) & 0xFF;
			PC += 2;
#ifdef PRINT_CODE
			printf("SUBN V%X, V%X\n", x, y);
#endif
		// Compatibility: SHL Vx : Set Vx = Vx << 1 and shift out most significant bit to VF
		// SHL Vx, Vy : Set Vx = Vy = Vy << 1, VF = most significan bit
		} else if(n == 0x0E) {
			V[0xF] = 0x00;
			if(compat_flags & (1 << COMPAT_SHIFT_QUIRK)) {
				if(V[x] & 0x80) {
					V[0xF] = 0x01;
				}
				V[x] = (V[x] << 1) & 0xFF;
			} else {
				if(V[y] & 0x80) {
					V[0xF] = 0x01;
				}
				V[x] = (V[y] << 1) & 0xFF;
			}
			PC += 2;
#ifdef PRINT_CODE
			printf("SHL V%X\n", x);
#endif
		}
	// SNE Vx, Vy : Skip next instruction if Vx != Vy
	} else if(c == 0x09 && n == 0x00) {
		if(V[x] != V[y]) {
			PC += 2;
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("SNE V%X, V%X\n", x, y);
#endif
	// LD I, nnn : Set I = nnn
	} else if(c == 0x0A) {
		I = nnn & 0x0FFF;
		PC += 2;
#ifdef PRINT_CODE
		printf("LD I, 0x%03X\n", nnn);
#endif
	// JP V0, nnn : Jump to location nnn + V0
	} else if(c == 0x0B) {
		if(compat_flags & (1 << COMPAT_JUMP0_QUIRK)) {
			PC = nnn + V[x];
		} else {
			PC = nnn + V[0];
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("JUMP V0, 0x%03X\n", nnn);
#endif
	// RND Vx, nn : Set Vx = random byte & nn
	} else if(c == 0x0C) {
		V[x] = (unsigned char)(255 * (rand() / (RAND_MAX + 1.0))) & nn;
		PC += 2;
#ifdef PRINT_CODE
		printf("RND V%X, 0x%02X\n", x, nn);
#endif
	// DXYN, DRW Vx, Vy, n,	Draws sprite at coordinate x,y. n = height, width = 8 pixels.
	//                      Sprite data is fetched from memory location I.
	} else if(c == 0x0D) {
		if(n) {
			V[0xF] = (UBYTE)chip8_draw_sprite_XO(V[x], V[y], n);
		} else {
			// Superchip command. Draw 16x16 size sprite.
			V[0xF] = (UBYTE)chip8_draw_sprite16_XO(V[x], V[y], n);
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("DRW V%X, V%X, 0x%1X\n", x, y, n);
#endif
	// SKP Vx : Skip next instruction if key with the value of Vx IS pressed
	} else if(c == 0x0E && nn ==0x9E) {
		if(keys[V[x]]) {
			PC += 2;
			//keys[V[x]] = 0;
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("SKP V%X\n", x);
#endif
	// SKNP Vx : Skip next instruction if key with the value of Vx is NOT pressed
	} else if(c == 0x0E && nn == 0xA1) {
		if(!keys[V[x]]) {
			PC += 2;
		}
		PC += 2;
#ifdef PRINT_CODE
		printf("SKNP V%X\n", x);
#endif
	// Group instruction with c == 0x0F
	} else if(c == 0x0F) {
		// AUDIO XO-Chip command.
		if(nnn == 0x002 && (compat_flags & (1 << COMPAT_ENABLE_XO))) {
			PC += 2;

		// LD I,#16bit : XO-Chip command. Load 16 bit immediate value to I.
		} else if(nnn == 0x0000 && (compat_flags & (1 << COMPAT_ENABLE_XO))) {
			PC += 2;
			I = ((mem[PC] << 8) | (mem[PC+1] & 0xFF)) & 0xFFFF;
			PC += 2;
#ifdef PRINT_CODE
			printf("LD I, #0x%04X\n", I);
#endif

		// PLANE x : XO-Chip command.
		} else if(nn == 0x01 && (compat_flags & (1 << COMPAT_ENABLE_XO))) {
			plane = x;
			PC += 2;
#ifdef PRINT_CODE
			printf("PLANE %d\n", n);
#endif

		// LD Vx, DT : Set Vx = delay_timer value
		} else if(nn == 0x07) {
			V[x] = delay_timer;
			PC += 2;
#ifdef PRINT_CODE
			printf("LD V%X, DelayTimer\n", x);
#endif
		// LD Vx, K : Wait for keypress, store the value of key in Vx
		} else if(nn == 0x0A) {
			for(i = 0; i < 0x0F; i++) {
				if(keys[i]) {
					PC += 2;
					V[x] = i;
				}
			}
#ifdef PRINT_CODE
			printf("LD V%X, Keypressed\n", x);
#endif
		// LD DT, Vx : Set delay timer = Vx
		} else if(nn == 0x15) {
			delay_timer = V[x];
			PC += 2;
#ifdef PRINT_CODE
			printf("LD DelayTimer, V%X\n", x);
#endif
		// LD ST, Vx : Set sound timer 0 Vx
		} else if(nn == 0x18) {
			sound_timer = V[x];
			PC += 2;
#ifdef PRINT_CODE
			printf("LD SoundTimer, V%X\n", x);
#endif
		// ADD I, Vx : Set I = I + Vx
		} else if(nn == 0x1E) {
			I = (I + V[x]) & 0xFFFF;
			PC += 2;
#ifdef PRINT_CODE
			printf("ADD I, V%X\n", x);
#endif
		// LD F, Vx : Set I = location of sprite for hexdigit Vx
		// Since we have loaded the charset to 0x00 - 0x50
		// and each character is 5 bytes, we multiply Vx with 5
		// and store result in I.
		} else if(nn == 0x29) {
			I = (V[x] * 5) & 0xFFFF;
			PC += 2;
#ifdef PRINT_CODE
			printf("LD F, V%X\n", x);
#endif
		// LD B, Vx : Store BCD representation of Vx in memory locations I, I+1, I+2
		} else if(nn == 0x33) {
			mem[I] = (unsigned char) (V[x] / 100);
			tmp = V[x] - (mem[I] * 100);
			mem[I+1] = tmp / 10;
			mem[I+2] = tmp % 10;
			PC += 2;
#ifdef PRINT_CODE
			printf("LD B, V%X\n", x);
#endif
		// LD [I], Vx : Store registers V0 through Vx in memory starting at location I
		// ?? Should I update?
		} else if(nn == 0x55) {
#ifdef PRINT_CODE
			printf("LD [I=0x%03X], V%X\n", I, x);
#endif
			for(i = 0; i <= x; i++) {
				if(compat_flags & (1 << COMPAT_I_QUIRK)) {
					mem[I+i] = V[i];
				} else {
					mem[I++] = V[i];
				}
			}
			PC += 2;
		// LD Vx, [I] : Read registers V0 through Vx from memory starting at location I
		// ?? Should I update?
		} else if(nn == 0x65) {
#ifdef PRINT_CODE
			printf("LD V%X, [I=0x%03X]\n", x, I);
#endif
			for(i = 0; i <= x; i++) {
				if(compat_flags & (1 << COMPAT_I_QUIRK)) {
					V[i] = mem[I+i];
				} else {
					V[i] = mem[I++];
				}
			}
			PC += 2;

		// LD I, HEX16 : Superchip command. Load I with pointer to Superchip
		// hexdigits, which is 16 x 16 pixels.
		} else if(nn == 0x30) {
			I = ch16 + V[x] * 10;  // TODO fix offset!
			PC += 2;


		// LD [R], Vx : Superchip command. Stor registers V0 - VX in R memory
		// R memory = HP-48 calculator 64 bit memory. 64/8 = 8 so max 8 registers
		// can be stored. X need to be between 0 - 7.
		} else if(nn == 0x75) {
			tmp = x%7;
			for(i = 0; i <= tmp; i++) {
				R[i] = V[i];
			}
			PC += 2;


		// LD Vx, [R] : Superchip command Load registers V0 - VX from register.
		// X need to be between 0 - 7.
		} else if(nn == 0x85) {
			tmp = x%7;
			for(i = 0; i <= tmp; i++) {
				V[i] = R[i];
			}
			PC += 2;
		}
	} else {
		return 0;
	}
#ifdef PRINT_CODE
	sleep(1);
#endif
	return 1;
}



int chip8_parse_opcode() {

	// Check if we should count down the timers.
	if(gettimer() - time_start > FRAMETIME) {
		if(delay_timer > 0) {
			delay_timer--;
		}
		if(sound_timer > 0) {
			sound_timer--;
		}
		//printf("Delay: %d\n", gettimer() - time_start);
		time_start = gettimer();
		tcount++;
		if(tcount > 60) {
			instructions_per_sec = instruction_count;
			tcount = 0;
			instruction_count = 0;
		}
	}

	instruction_count++;

	current_address = PC & 0x0FFF;
	current_opcode = mem[PC] * 0x100 + mem[PC+1];
	if(!chip8_execute_opcode()) {
		printf("UNKNOWN OPCODE: PC = 0x%04X, OPCODE = 0x%04X\n", current_address, current_opcode);
		printf("PREVIOUS OPCODE: PC = 0x%04X, OPCODE = 0x%04X\n", last_address, last_opcode);
		return 0;
	}

	last_address = current_address;
	last_opcode = current_opcode;
	return 1;
}

