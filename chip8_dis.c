// #include <time.h>
// #include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chip8.h"

// #define STACK_SIZE 16
// #define STACK_MASK 0x0F
#define MEMORY_SIZE 4096

extern unsigned short chip8_get_compatibility_flags();

extern UBYTE *mem;			// Memory
					// Stops playing at zero.

static unsigned short compat_flags;	// Compatibility flags


void chip8_disassemble(int pc, unsigned short opcode, char *disstr) {
	UBYTE c, x, y, n, nn;
	unsigned short nnn, tmp;


	compat_flags = chip8_get_compatibility_flags();

	c = (opcode >> 12) & 0x0F;
	x = (opcode >> 8) & 0x0F;
	y = (opcode >> 4) & 0x0F;
	n = opcode & 0x0F;
	nn = opcode & 0xFF;
	nnn = opcode & 0x0FFF;

	// SCROLU n : XO-Chip command. Scroll up n pixels.
	if((opcode & 0xFFF0) == 0x00D0) {
		sprintf(disstr, "SCROLU %d\n", n);
	// CLS : Clear display
	} else if(opcode == 0x00E0) {
		sprintf(disstr,"CLS");
	// RET : Return from subroutine
	} else if(opcode == 0x00EE) {
		sprintf(disstr, "RET");
	// LORES : Superchip command
	} else if(opcode == 0x00FE) {
		sprintf(disstr, "LORES 64x32");
	// HIRES : Superchip_command
	} else if(opcode == 0x00FF) {
		sprintf(disstr, "HIRES 128x64");
	// SCROLD n : Superchip command. Scroll down n lines (n = 0-15).
	} else if((opcode & 0xFFF0) == 0x00C0) {
		sprintf(disstr, "SCROLD %02d", n);
	// SCROLR : Superchip command. Scroll right by 4 pixels.
	}else if(opcode == 0x00FB) {
		sprintf(disstr, "SCROLR 4");
	// SCROLL : Superchip command. Scroll left by 4 pixels.
	} else if(opcode == 0x00FC) {
		sprintf(disstr, "SCROLL 4");
	// JP nnn : Jump to adress nnn
	} else if(c == 0x01) {
		sprintf(disstr, "JUMP 0x%03X", nnn);
		if(nnn & 0x01) strcat(disstr, "    <- Jump to uneven adress");
	// CALL nnn : Call subroutine at adress nnn
	} else if(c == 0x02) {
		sprintf(disstr, "CALL 0x%03X", nnn);
		if(nnn & 0x01) strcat(disstr, "    <- Call to uneven adress");
	// SE Vx, nn : Skip next instruction if Vx == nn
	} else if(c == 0x03) {
		sprintf(disstr, "SE V%X, 0x%02X", x, nn);
	// SNE Vx, nn : Skip next instruction if Vx != nn
	} else if(c == 0x04) {
		sprintf(disstr, "SNE V%X, 0x%02X", x, nn);

	} else if(c == 0x05) {
		// SE Vx, Vy : Skip next instruction if Vx == Vy
		if(n == 0x00) {
			sprintf(disstr, "SE V%X, V%X", x, y);
		// SAVE I,Vx-Vy : XO-Chip command. Save Vx - Vy to I. I is unchanged.
		} else if(n == 0x02) {
			sprintf(disstr, "SAVE I, V%1X-V%1X", x, y);
		// LOAD Vx-Vy,I : XO-Chip command. Load Vx-Vy from I. I is unchanged.
		} else if(n == 0x03) {
			sprintf(disstr, "LOAD V%1X-V%1X, I", x, y);
		}
	// LD Vx, nn : Set Vx = nn
	} else if(c == 0x06) {
		sprintf(disstr, "LD V%X, 0x%02X", x, nn);
	// ADD Vx, nn : Set Vx = Vx + nn
	} else if(c == 0x07) {
		sprintf(disstr, "ADD V%X, 0x%02X", x, nn);

	// Group instructions with c = 0x08 together for optimization
	} else if(c == 0x08) {
		// LD Vx, Vy : Set Vx = Vy
		if(n == 0x0) {
			sprintf(disstr, "LD V%X, V%X", x, y);
		// OR Vx, Vy : Set Vx = Vx OR Vy
		} else if(n == 0x01) {
			sprintf(disstr, "OR V%X, V%X", x, y);
		// AND Vx, Vy : Set Vx = Vx AND Vy
		} else if(n == 0x02) {
			sprintf(disstr, "AND V%X, V%X", x, y);
		// XOR Vx, Vy : Set Vx = Vx XOR Vy
		} else if(n == 0x03) {
			sprintf(disstr, "XOR V%X, V%X", x, y);
		// ADD Vx, Vy : Set Vx = Vx + Vy, set VF = carry
		} else if(n == 0x04) {
			sprintf(disstr, "ADDC V%X, V%X", x, y);
		// SUB Vx, Vy : Set Vx = Vx - Vy, set VF = NOT borrow
		} else if(n == 0x05) {
			sprintf(disstr, "SUB V%X, V%X", x, y);
		// SHR Vx : Set Vx = Vx >> 1 and shift out least significant bit to VF
		} else if(n == 0x06) {
			if(compat_flags & (1 << COMPAT_SHIFT_QUIRK)) {
				sprintf(disstr, "SHR V%X", x);
			} else {
				sprintf(disstr, "SHR V%X, V%X", x, y);
			}
		// SUBN Vx, Vy : Set Vx = Vy - Vx, set VF = NOT borrow (if Vy > Vx, VF = 1)
		} else if(n == 0x07) {
			sprintf(disstr, "SUBN V%X, V%X", x, y);
		// SHL Vx : Set Vx = Vx << 1 and shift out most significant bit to VF
		} else if(n == 0x0E) {
			if(compat_flags & (1 << COMPAT_SHIFT_QUIRK)) {
				sprintf(disstr, "SHL V%X", x);
			} else {
				sprintf(disstr, "SHL V%X, V%X", x, y);
			}
		}
	// SNE Vx, Vy : Skip next instruction if Vx != Vy
	} else if(c == 0x09 && n == 0x00) {
		sprintf(disstr, "SNE V%X, V%X", x, y);
	// LD I, nnn : Set I = nnn
	} else if(c == 0x0A) {
		sprintf(disstr, "LD I, 0x%03X", nnn);
	// JP V0, nnn : Jump to location nnn + V0
	} else if(c == 0x0B) {
		sprintf(disstr, "JP V0, 0x%03X", nnn);
	// RND Vx, nn : Set Vx = random byte & nn
	} else if(c == 0x0C) {
		sprintf(disstr, "RND V%X, 0x%02X", x, nn);
	// DXYN, DRW Vx, Vy, n,	Draws sprite at coordinate x,y. n = height, width = 8 pixels.
	//                      Sprite data is fetched from memory location I.
	} else if(c == 0x0D) {
		if(n) {
			sprintf(disstr, "DRW V%X, V%X, 0x%1X", x, y, n);
		} else {
			// Superchip command. Draw 16x16 size sprite.
			sprintf(disstr, "DRW16 V%X, V%X, 16", x, y);
		}
	// SKP Vx : Skip next instruction if key with the value of Vx IS pressed
	} else if(c == 0x0E && nn ==0x9E) {
		sprintf(disstr, "SKP V%X", x);
	// SKNP Vx : Skip next instruction if key with the value of Vx is NOT pressed
	} else if(c == 0x0E && nn == 0xA1) {
		sprintf(disstr, "SKNP V%X", x);
	// Group instruction with c == 0x0F
	} else if(c == 0x0F) {
		// AUDIO : XO-Chip command.
		if(nnn == 0x002) {
			sprintf(disstr, "AUDIO");
		// LD I,#16bit : XO-Chip command. Load I with 16 bit immediate value
		} else if(nnn == 0x0000) {
			tmp = (mem[pc+2] << 8) | mem[pc+3];
			sprintf(disstr, "LD I,#%04X", tmp);
		// PLANE x : XO-Chip command. Set drawing plane.
		} else if(nn == 0x01) {
			sprintf(disstr, "PLANE %d", x);
		// LD Vx, DT : Set Vx = delay_timer value
		} else if(nn == 0x07) {
			sprintf(disstr, "LD V%X, DelayTimer", x);
		// LD Vx, K : Wait for keypress, store the value of key in Vx
		} else if(nn == 0x0A) {
			sprintf(disstr, "LD V%X, Keypressed", x);
		// LD DT, Vx : Set delay timer = Vx
		} else if(nn == 0x15) {
			sprintf(disstr, "LD DelayTimer, V%X", x);
		// LD ST, Vx : Set sound timer 0 Vx
		} else if(nn == 0x18) {
			sprintf(disstr, "LD SoundTimer, V%X", x);
		// ADD I, Vx : Set I = I + Vx
		} else if(nn == 0x1E) {
			sprintf(disstr, "ADD I, V%X", x);
		// LD F, Vx : Set I = location of sprite for hexdigit Vx
		// Since we have loaded the charset to 0x00 - 0x50
		// and each character is 5 bytes, we multiply Vx with 5
		// and store result in I.
		} else if(nn == 0x29) {
			sprintf(disstr, "LD F, V%X", x);
		// LD B, Vx : Store BCD representation of Vx in memory locations I, I+1, I+2
		} else if(nn == 0x33) {
			sprintf(disstr, "LD B, V%X", x);
		// LD [I], Vx : Store registers V0 through Vx in memory starting at location I
		} else if(nn == 0x55) {
			sprintf(disstr, "LD [I], V0-V%X", x);
		// LD Vx, [I] : Read registers V0 through Vx from memory starting at location I
		} else if(nn == 0x65) {
			sprintf(disstr, "LD V0-V%X, [I]", x);
		// LD I, HEX16 : Superchip command. Load I with pointer to hexdigits.
		} else if(nn == 0x30) {
			sprintf(disstr, "LD I, HEX16[V%X]", x);
		// LD [R], Vx : Superchip command. Store registers V0 - VX in R memory.
		} else if(nn == 0x75) {
			sprintf(disstr, "LD [R], V0-V%X", x);
		// LD Vx, [R] : Superchip command. Load register V0 - VX from R memory.
		} else if(nn == 0x85) {
			sprintf(disstr, "LD V0-V%X, [R]", x);
		}
	} else {
		sprintf(disstr, "D.W 0x%04X", opcode);
	}
}

