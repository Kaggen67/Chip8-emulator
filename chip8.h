/*
 *
 *
 * // CHIP8 character set.
 *
 *
 *
 */

#ifndef CHIP8_H

#define CHIP8_H
typedef unsigned char UBYTE;
typedef unsigned int UINT32;

// Messages for callback to GTK or UserInterface code.
// This is for splitting emulator code from GUI code
// but still allows emulator control some GUI behaviour.
enum {
	MSG_COMPAT_FLAG_CHANGE = 0,
	MSG_SUPERCHIP_EXIT,
	MSG_UNKNOWN_OPCODE
};

// CHIP8 Quirks
// Theese quirks are different implementations between old
// and newer chip8 emulators, like XO-Chip and Superchip.
enum {
	COMPAT_SHIFT_QUIRK = 0,
	COMPAT_I_QUIRK,
	COMPAT_VF_QUIRK,
	COMPAT_CLIP_SPRITES_QUIRK,
	COMPAT_JUMP0_QUIRK,
	COMPAT_ENABLE_XO
};

// CHIP8 Status Flags
//
enum {
	F_DRAW_RDY = 0		// Set when drawing a sprite is done to speed up rendering.
};

#endif
