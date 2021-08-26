#include <time.h>
#include <stdlib.h>
#include <stdio.h>

//#include "chip8.h"

#define TIME_FRAME 1000


main(int argc, char **argv) {
	FILE *fh;
	int i, res;
	int tmr;


	tmr = TIME_FRAME;


	if(argc < 2) {
		printf("Yoc need a chip8 file!\n");
	} else {
		printf("Emulating file %s\n", argv[1]);
	}



	chip8_init();
	if(!chip8_load_rom(argv[1])) {
		printf("Load error!\n");
		exit(2);
	}

	
	while(1) {
		chip8_parse_opcode();
		tmr--;
		if(tmr == 0) {
			tmr = TIME_FRAME;
			chip8_debug_print_videomemory();
			usleep(25000);
		}
	}

	for(i = 0; i < 100040; i++) {
		chip8_parse_opcode();
//		chip8_debug_print_videomemory();
	//	sleep(1);
	}
	
	chip8_debug_print_videomemory();
	//res = (*opcodes[0])(10,20);

}
