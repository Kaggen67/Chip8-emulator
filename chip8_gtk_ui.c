#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "chip8.h"

/*
#define SCALE 8
#define WIDTH (64 * SCALE)
#define HEIGHT (32 * SCALE)
*/
#define SCALE 8
#define WIDTH (128 * SCALE)
#define HEIGHT (64 * SCALE)
#define FPS 15
#define FRAME_TIME (1000 / FPS)
#define CPU_FREQ 200
#define CPU_CPULSE_TIME (1000 / CPU_FREQ)
#define XO_COLOR_DEPTH 4

//typedef unsigned char UBYTE;

typedef struct {
	char name[20];
	UBYTE bg_color[3];
	UBYTE fg_color[3];
} Chip8Colors;

typedef struct {
	char name[20];
	unsigned int colors[XO_COLOR_DEPTH];
} XOChip8Colors;

UBYTE *fg_color;
UBYTE *bg_color;
static unsigned int *active_colortable;

static Chip8Colors chip8colors[] = {{"Green Phosphor", {10,10,10},{30, 240, 30}},
		{"C64 Blue", {30, 30, 130}, {150, 150, 244}},
		{"Ugly as f**k", {245, 245, 90}, {240, 50, 50}},
		{"Reverse Ugly", {240, 50, 50},{245, 245, 90}},
		{"Black & White",{250,250,250},{10,10,10}},
		{"White & Black",{10,10,10},{250,250,250}}
};

static XOChip8Colors xocolors[] = {
	{"XO Original",	{0x996600, 0xFFCC00, 0xFF6600, 0x662200}},
	{"Pea Soup", {0x0F380F, 0x306230, 0x8BAC0F, 0x9BBC0F}},
	{"Green Phosphor", {0x0A0A0A, 0x18F018, 0x389638, 0x2A522A}},
	{"C64 Blue", {0x282882, 0x9696F4, 0x5656B5, 0x30308E}},
	{"Ugly as f**k", {0xF5F55A, 0xF03232, 0xD02222, 0xA01010}},
	{"Reverse Ugly", {0xF03232, 0xF5F55A, 0xA01010, 0xD02222}},
	{"Black & White",{0xFAFAFA, 0x0A0A0A, 0xC0C0C0, 0x808080}},
	{"White & Black",{0x0A0A0A, 0xFAFAFA, 0x808080, 0xC0C0C0}}
};

static gchar tmp_buf[128];

static unsigned char character[] = {
	0b10000001,
	0b01000010,
	0b00100100,
	0b00011000,
	0b00011000,
	0b00100100,
	0b01000010,
	0b10000001
};

// Variables from chip8
//static unsigned int *chip8_vmem;	// Pointer to chip8 videomemory/buffer.
static UBYTE *chip8_vmem;
static int chip8_width;		// Width in pixels of chip8 displaymemory.
static int chip8_height;		// Height in pixels of chip8 displaymemory.

static int scale;			// Scale of videobuffer.

static int cpu_iter;			// CPU iterations/interrupt.

static GtkWidget *window;		// Main GTK window.
static GdkPixbuf *pixbuf;		// Pixbuffer is used to display chip8 video output
static GtkWidget *img;			// Image to put pixbuffer in
static guchar *pixeldata;		// Data where we store pixels, 3 bytes/pixel (R,G,B)
static GtkWidget *brun;			// RUN Toggle button. Need to access it globaly.

static guint draw_timer;		// GTK Timeout. How often should we draw on screen
static guint cpu_clock;		// GTK Timeout. How often should we emulate chip8 CPU
static guint info_timer;		// GTK Timeout. How often should we update info, like instructions/sec

static void copy_chip8_vmem();

extern int chip8_parse_opcode();
extern int chip8_get_width();
extern int chip8_get_height();
//extern unsigned int *chip8_get_vbuf();
extern UBYTE *chip8_get_vbuf();
extern int chip8_init();
extern int chip8_load_rom(char*);
extern void chip8_key_pressed(int);
extern void chip8_key_released(int);
extern void chip8_reset();
extern unsigned int chip8_get_pc();
extern int chip8_get_instructions_per_sec();
extern UBYTE *chip8_get_program_memory();
extern unsigned short chip8_status_get_flags();
extern void chip8_status_set_flag(unsigned short);
extern void chip8_status_clear_flag(unsigned short);


extern void chip8_disassemble(int, unsigned short, char*);
extern void chip8_debug_view_reload(UBYTE *);
extern void chip8_debug_view_focus(unsigned int);

extern GtkWidget *chip8_keypad_init();

extern GtkWidget *chip8_debug_view_init(UBYTE*);
extern GtkWidget *chip8_debug_init_regs();
extern void chip8_debug_get_vregs();

extern GtkWidget *chip8_menu_init();

static char rom_filename[256];
static int rom_bytesize;

#define DISASSEMBLY_LINE_SIZE 64	// Max number of chars on each line of disassembly.
static char *disassembly_text;			// Pointer to disassembly text buffer.

static gboolean emulator_running;	// Flag if emulator is running or not.

static gulong keyh1;			// Signal handler for key press
static gulong keyh2;			// Signal handler for key release

//gboolean draw(GtkWidget *, cairo_t *);

static void hello(GtkWidget *widget, gpointer data) {
	g_print("Quitting!\n");
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
	g_print("Delete event occurred\n");

	return FALSE;
}

static void destroy(GtkWidget *widget, gpointer data) {
	gtk_main_quit();
}


gboolean draw() {

	if(emulator_running && (chip8_status_get_flags() & (1 << F_DRAW_RDY))) {
		copy_chip8_vmem();
	}
	gtk_image_set_from_pixbuf(img, pixbuf); // Note: You seem to only need this command to update buffer on screen
	//gtk_widget_queue_draw(img);			// No need for this line anymore (worked in gtk+-2.0)?
	chip8_status_clear_flag(F_DRAW_RDY);
	return TRUE;
}


// Plot a single scaled pixel with color at x, y
void plot_pixel(UBYTE *vmem, int x, int y, UBYTE *color_ptr) {
	int xx, yy;
	int row, col;

	if(x >= chip8_width || x < 0) {
		return;
	}
	if(y >= chip8_height || y <0) {
		return;
	}

	x *= scale;
	y *= scale;

	for(yy = 0; yy < scale; yy++) {
		row = ((y + yy) * chip8_width * scale) * 3;
		for(xx = 0; xx < scale; xx++) {
			col = (x + xx) * 3;
			vmem[row + col] = color_ptr[0];
			vmem[row + col + 1] = color_ptr[1];
			vmem[row + col + 2] = color_ptr[2];
		}
	}
}

// Plot 8-bit character pattern
void plot_character(guchar *vmem, int x, int y) {
	int xx, yy;
	UBYTE *color_ptr;

	for(yy = 0; yy < 8; yy++) {
		for(xx = 0; xx < 8; xx++) {
			if((character[yy] << xx) & 0x80) {
				color_ptr = fg_color;
			} else {
				color_ptr = bg_color;
			}
			plot_pixel(vmem, (x+xx) & 63, (y+yy) & 31, color_ptr);
		}
	}
}


// Copy pixels from chip8 videobuffer to pixelbuffer for GTK.
void copy_chip8_vmem() {
	int x, y, xx, yy;
	int w, h;
	int pindex, c_scale;
	unsigned int *color_ptr;
	UBYTE color;

	pindex = 0;

	w = chip8_get_width();
	h = chip8_get_height();

	if(w > 64) {
		c_scale = SCALE >> 1;
	} else {
		c_scale = SCALE;
	}

	color_ptr = active_colortable;

	for(y = 0; y < h; y++) {
		for(yy = 0; yy < c_scale; yy++) {
			for(x = 0; x < w; x++) {
				color = chip8_vmem[y * w + x] & 0x03;
				for(xx = 0; xx < c_scale; xx++) {
					pixeldata[pindex++] = (color_ptr[color] >> 16) & 0xFF;
					pixeldata[pindex++] = (color_ptr[color] >> 8) & 0xFF;
					pixeldata[pindex++] = color_ptr[color] & 0xFF;
				}

				/*
				if(chip8_vmem[y * w + x]) {
					color_ptr = fg_color;
				} else {
					color_ptr = bg_color;
				}
				for(xx = 0; xx < c_scale; xx++) {
					pixeldata[pindex++] = color_ptr[0];
					pixeldata[pindex++] = color_ptr[1];
					pixeldata[pindex++] = color_ptr[2];
				}
				*/
			}
		}
	}
}


GdkPixbuf* init_pixbuf(int width, int height) {
	GdkPixbuf *pb;
	int x, y;

	// Allocate packed pixeldata 8-bit per sample.
	// Assuming 3 samples per pixel as following: R,G,B,R,G,B,R,G,B....
//	pixeldata = (guchar *)malloc((width * height) * 3);
	pixeldata = (guchar *)malloc((WIDTH * HEIGHT) * 3);
	if(pixeldata == NULL) {
		printf("Couldn't allocate pixel-memory!\n");
		return NULL;
	}

	pb = gdk_pixbuf_new_from_data(pixeldata, GDK_COLORSPACE_RGB,
					FALSE,
					8,
					width,
					height,
					width * 3,
					NULL,
					NULL);
	if(pb == NULL) {
		printf("Failed to allocate pixbuf!\n");
		return NULL;
	}

	for(y=0; y < chip8_height; y += 8) {
		for(x = 0; x < chip8_width; x += 8) {
			plot_character(pixeldata, x, y);
		}
	}
	return pb;
}

gboolean key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	switch(event->keyval) {
		case GDK_KEY_1:
			chip8_key_pressed(0x01);
			break;
		case GDK_KEY_2:
			chip8_key_pressed(0x02);
			break;
		case GDK_KEY_3:
			chip8_key_pressed(0x03);
			break;
		case GDK_KEY_4:
			chip8_key_pressed(0x0C);
			break;
		case GDK_KEY_q:
			chip8_key_pressed(0x04);
			break;
		case GDK_KEY_w:
			chip8_key_pressed(0x05);
			break;
		case GDK_KEY_e:
			chip8_key_pressed(0x06);
			break;
		case GDK_KEY_r:
			chip8_key_pressed(0x0D);
			break;
		case GDK_KEY_a:
			chip8_key_pressed(0x07);
			break;
		case GDK_KEY_s:
			chip8_key_pressed(0x08);
			break;
		case GDK_KEY_d:
			chip8_key_pressed(0x09);
			break;
		case GDK_KEY_f:
			chip8_key_pressed(0x0E);
			break;
		case GDK_KEY_z:
			chip8_key_pressed(0x0A);
			break;
		case GDK_KEY_x:
			chip8_key_pressed(0x00);
			break;
		case GDK_KEY_c:
			chip8_key_pressed(0x0B);
			break;
		case GDK_KEY_v:
			chip8_key_pressed(0x0F);
			break;
	}
	return TRUE;
}

gboolean key_released(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	switch(event->keyval) {
		case GDK_KEY_1:
			chip8_key_released(0x01);
			break;
		case GDK_KEY_2:
			chip8_key_released(0x02);
			break;
		case GDK_KEY_3:
			chip8_key_released(0x03);
			break;
		case GDK_KEY_4:
			chip8_key_released(0x0C);
			break;
		case GDK_KEY_q:
			chip8_key_released(0x04);
			break;
		case GDK_KEY_w:
			chip8_key_released(0x05);
			break;
		case GDK_KEY_e:
			chip8_key_released(0x06);
			break;
		case GDK_KEY_r:
			chip8_key_released(0x0D);
			break;
		case GDK_KEY_a:
			chip8_key_released(0x07);
			break;
		case GDK_KEY_s:
			chip8_key_released(0x08);
			break;
		case GDK_KEY_d:
			chip8_key_released(0x09);
			break;
		case GDK_KEY_f:
			chip8_key_released(0x0E);
			break;
		case GDK_KEY_z:
			chip8_key_released(0x0A);
			break;
		case GDK_KEY_x:
			chip8_key_released(0x00);
			break;
		case GDK_KEY_c:
			chip8_key_released(0x0B);
			break;
		case GDK_KEY_v:
			chip8_key_released(0x0F);
			break;
	}
	return TRUE;
}

gboolean reset_chip8() {
	chip8_reset();
	chip8_load_rom(rom_filename);
	emulator_running = FALSE;
	return TRUE;
}

gboolean change_colors(GtkWidget *widget) {
	int i;

	i = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

	fg_color = chip8colors[i].fg_color;
	bg_color = chip8colors[i].bg_color;
	active_colortable = xocolors[i].colors;

	return TRUE;
}

gboolean update_info(gpointer widget) {
	int ips;

	ips = chip8_get_instructions_per_sec();

	sprintf(tmp_buf, "IPS: %05d", ips);

	gtk_label_set_text(GTK_LABEL(widget), tmp_buf);


	return TRUE;
}

gboolean emulator_run(GtkWidget *widget) {
	if(emulator_running == TRUE) {
		emulator_running = FALSE;
		g_signal_handler_disconnect(window, keyh1);
		g_signal_handler_disconnect(window, keyh2);
		gtk_button_set_label(GTK_BUTTON(widget), "_Run");
		chip8_debug_view_focus(chip8_get_pc());
		chip8_debug_get_vregs();
	} else {
		emulator_running = TRUE;
		gtk_button_set_label(GTK_BUTTON(widget), "_Stop");
		keyh1 = g_signal_connect(window, "key_press_event", G_CALLBACK(key_pressed), NULL);
		keyh2 = g_signal_connect(window, "key_release_event", G_CALLBACK(key_released), NULL);

	}

	return TRUE;
}

gboolean cpu_exec() {
	int i;

	if(!emulator_running) {
		return TRUE;
	}
	for(i = 0; i < cpu_iter; i++) {
		if(!emulator_running) {
			break;
			return TRUE;
		}
		if(!chip8_parse_opcode()) {
			printf("TERMINATING EXECUTION OF CHIP8 CODE!\n");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(brun), FALSE);
			//emulator_running = FALSE;
		}
	}
	return TRUE;
}

gboolean emulator_step(GtkWidget *widget) {
	if(emulator_running) {
		return TRUE;
	}
	if(!chip8_parse_opcode()) {
		printf("TERMINATING EXECUTION OF CHIP8 CODE!\n");
		emulator_running = FALSE;
	}
	chip8_debug_view_focus(chip8_get_pc());
	copy_chip8_vmem();
	gtk_widget_queue_draw(img);
	chip8_debug_get_vregs();
	return TRUE;
}

gboolean cpu_iter_changed(GtkWidget *sbutton) {
	cpu_iter = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sbutton));
	return TRUE;
}

gboolean open_file(GtkWidget *widget, gpointer *run_button) {
	GtkWidget *file_dialog;
	GtkFileChooser *chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;
	char *filename;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(run_button), FALSE);

	file_dialog = gtk_file_chooser_dialog_new("Open CHIP8 ROM", GTK_WINDOW(window),
			action,
			"_Cancel", GTK_RESPONSE_CANCEL,
			"_Open", GTK_RESPONSE_ACCEPT,
			NULL);

	res = gtk_dialog_run(GTK_DIALOG(file_dialog));
	if(res == GTK_RESPONSE_ACCEPT) {
		chooser = GTK_FILE_CHOOSER(file_dialog);
		filename = gtk_file_chooser_get_filename(chooser);
		strncpy(rom_filename, filename, 256);
		printf("\nOpening file %s\n", rom_filename);
		chip8_reset();
		chip8_load_rom(rom_filename);
		chip8_debug_view_reload(chip8_get_program_memory());
	}

	gtk_widget_destroy(file_dialog);

	gtk_window_set_title(GTK_WINDOW(window), rom_filename);

	return TRUE;
}

void disassemble(UBYTE *prog_mem) {
	char dis_str[DISASSEMBLY_LINE_SIZE];
	int i, pc;
	unsigned short opcode;
	char *textbuf;

	textbuf = disassembly_text;
	// Start in program memory where programs are loaded.
	pc = 0x200;

	for(i = 0; i < rom_bytesize; pc+=2, i++) {
		opcode = (prog_mem[pc] * 0x100 + prog_mem[pc+1]) & 0xFFFF;
		sprintf(dis_str, "0x%03X  0x%04X  ", pc, opcode);
		textbuf = strcat(textbuf, dis_str);
		chip8_disassemble(pc, opcode, dis_str);
		textbuf = strncat(textbuf, dis_str, DISASSEMBLY_LINE_SIZE);
		strcat(textbuf, "\n");
	}
}


int main(int argc, char *argv[]) {
	GtkWidget *bquit, *bstep, *breset; //, *textview, *ips_entry;
	GtkWidget *colorchooser, *debug_treeview;
	GtkWidget *ips_label, *spin_cpu_iter_label, *spin_cpu_iter;
	GtkWidget *bopen;
	GtkWidget *debug_v_table;
	GtkWidget *bkeypad;
	GtkWidget *menubar;
	int i, sz;

	//GtkWidget *gtkbox1;
	GtkBox *gtkbox1, *gtkbox2, *gtkbox_info;
	PangoFontDescription *pango_font_desc;

	if(argc < 2) {
		printf("You need a chip8 file!\n");
		exit(1);
	} else {
		//rom_filename = argv[1];
		strncpy(rom_filename, argv[1], 256);
		printf("Emulating file %s\n", rom_filename);
	}

	chip8_init();
	rom_bytesize = chip8_load_rom(rom_filename);
	if(!rom_bytesize) {
		printf("Load error!\n");
		exit(2);
	}
	printf("Loaded %d bytes into chip8 ROM\n", rom_bytesize);

	/*
	disassembly_text = malloc(rom_bytesize * DISASSEMBLY_LINE_SIZE);
	if(!disassembly_text) {
		printf("Couldn't allocate memory for diassembly text buffer!\n");
		exit(3);
	}
	disassembly_text[0] = 0;	// NULL terminate string in empty buffer.
	printf("Disassembling ROM\n");
	disassemble(chip8_get_program_memory());
	printf("Disassembling ROM - DONE\n");
	*/

	gtk_init(&argc, &argv);

	// Init GUI variables from chip8.
	chip8_width = chip8_get_width();
	chip8_height = chip8_get_height();
	chip8_vmem = chip8_get_vbuf();

	// Init colors
	fg_color = chip8colors[0].fg_color;
	bg_color = chip8colors[0].bg_color;
	active_colortable = xocolors[0].colors;

	emulator_running = FALSE;

	scale = SCALE;
	
	pixbuf = init_pixbuf(chip8_width * scale, chip8_height * scale);

	// CPU Iterations/interrupt cpu_exec loop.
	cpu_iter = 2;

	if(pixbuf == NULL) {
		printf("Couldn't init pixbuf!\n");
		exit(3);
	}

	img = gtk_image_new_from_pixbuf(pixbuf);
	
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), -1, 840);

	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);

	g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 10);


	gtkbox1 = GTK_BOX(gtk_vbox_new(FALSE, 8));
	gtkbox2 = GTK_BOX(gtk_hbox_new(TRUE, 8));
	gtkbox_info = GTK_BOX(gtk_hbox_new(FALSE, 8));
	//gtkbox_last_instr = GTK_BOX(gtk_hbox_new(FALSE,8));

	// Instructions Per Sec info label
	ips_label = gtk_label_new("IPS: 00000");

	// Emulator speed widgets
	spin_cpu_iter_label = gtk_label_new("CPU iterations");
	spin_cpu_iter = gtk_spin_button_new_with_range(1,50,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_cpu_iter), cpu_iter);

	// Menubar
	menubar = chip8_menu_init();

	// KeyPad toggle button to turn keypad window on/off
	bkeypad = chip8_keypad_init();

	// Get all CHIP8 V-registers in a table widget.
	debug_v_table = chip8_debug_init_regs();

	// GtkEntry with Info on last instruction before stop or after step.
	//frame_last_instr = gtk_frame_new("Last instruction");
	//gtk_frame_set_shadow_type(GTK_FRAME(frame_last_instr), GTK_SHADOW_IN);
	
	bquit = gtk_button_new_with_label("Quit");

	//brun = gtk_button_new_with_label("Run");
	brun = gtk_toggle_button_new_with_mnemonic("_Run");
	bstep = gtk_button_new_with_label("Step");
	breset = gtk_button_new_with_label("Reset");
	bopen = gtk_button_new_with_mnemonic("_Open...");

	// TreeView for disassembly listing.
	debug_treeview = chip8_debug_view_init(chip8_get_program_memory());

	// Colorchooser ComboBox. Choose colortheme for the emulator.
	colorchooser = gtk_combo_box_text_new();
	//sz = sizeof(chip8colors)/sizeof(Chip8Colors);
	sz = sizeof(xocolors)/sizeof(XOChip8Colors);
	for(i = 0; i < sz; i++) {
		gtk_combo_box_text_append(GTK_COMBO_BOX(colorchooser), NULL, xocolors[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(colorchooser), 0);

	// Set font for treeview
	pango_font_desc = pango_font_description_new();
	pango_font_description_set_family(pango_font_desc, "monospace");
	//gtk_widget_modify_font(textview, pango_font_desc);
	gtk_widget_modify_font(debug_treeview, pango_font_desc);

	g_signal_connect(brun, "toggled", G_CALLBACK(emulator_run), NULL);
	g_signal_connect(bstep, "clicked", G_CALLBACK(emulator_step), NULL);
	g_signal_connect(breset, "clicked", G_CALLBACK(reset_chip8), NULL);
	g_signal_connect(bopen, "clicked", G_CALLBACK(open_file), (gpointer) brun);
	g_signal_connect(colorchooser, "changed", G_CALLBACK(change_colors), NULL);
	g_signal_connect(bquit, "clicked", G_CALLBACK(hello), NULL);

	g_signal_connect_swapped(bquit, "clicked", G_CALLBACK(gtk_widget_destroy), window);

	//g_signal_connect(window, "key_press_event", G_CALLBACK(key_pressed), NULL);
	//g_signal_connect(window, "key_release_event", G_CALLBACK(key_released), NULL);

	g_signal_connect(spin_cpu_iter, "value-changed", G_CALLBACK(cpu_iter_changed), NULL);

	gtk_box_pack_start(gtkbox2, brun, 0, 1, 4);
	gtk_box_pack_start(gtkbox2, bstep, 0, 1, 4);
	gtk_box_pack_start(gtkbox2, breset, 0, 1, 4);
	gtk_box_pack_start(gtkbox2, bopen, 0, 1, 4);

	gtk_box_pack_start(gtkbox_info, spin_cpu_iter_label, 0, 0, 4);
	gtk_box_pack_start(gtkbox_info, spin_cpu_iter, 0, 0, 4);
	gtk_box_pack_start(gtkbox_info, ips_label, 0, 0, 4);
	gtk_box_pack_start(gtkbox_info, colorchooser, 0, 0, 4);
	gtk_box_pack_start(gtkbox_info, bkeypad, 0, 0, 4);

	gtk_widget_show(brun);
	gtk_widget_show(bstep);
	gtk_widget_show(breset);
	gtk_widget_show(bopen);
	gtk_box_pack_start(gtkbox1, menubar, FALSE, FALSE, 4);
	gtk_box_pack_start(gtkbox1, img, 0, 0, 4);
	gtk_box_pack_start(gtkbox1, GTK_WIDGET(gtkbox_info), 0, 0, 4);
	gtk_box_pack_start(gtkbox1, debug_v_table, 0, 0, 4);
	//gtk_box_pack_start(gtkbox1, scrollwindow, 1,1,4);
	//gtk_box_pack_start(GTK_BOX(debug_frame), debug_scrollwindow, 1, 1, 4);
	//gtk_box_pack_start(gtkbox1, debug_scrollwindow, 1, 1, 4);
	gtk_box_pack_start(gtkbox1, debug_treeview, 1, 1, 4);
	gtk_box_pack_start(gtkbox1, (GtkWidget*) gtkbox2, 0, 0, 4);
	gtk_box_pack_start(gtkbox1, bquit, 0, 0, 4);

	gtk_container_add(GTK_CONTAINER(window), (GtkWidget*) gtkbox1);

	gtk_widget_show(img);

	gtk_widget_show_all(GTK_WIDGET(gtkbox_info));

	gtk_widget_show_all(debug_v_table);

	gtk_widget_show(GTK_WIDGET(ips_label));

//	gtk_widget_show(debug_treeview);

	//gtk_widget_show(GTK_WIDGET(debug_scrollwindow));

	gtk_widget_show_all(debug_treeview);

	gtk_widget_show(colorchooser);

	gtk_widget_show((GtkWidget*)gtkbox2);

	gtk_widget_show(bquit);

	gtk_widget_show_all(menubar);

	gtk_widget_show(GTK_WIDGET(gtkbox1));

	gtk_widget_show(window);

	draw_timer = g_timeout_add(FRAME_TIME, draw, NULL);
	cpu_clock = g_timeout_add(CPU_CPULSE_TIME, cpu_exec, NULL);
	info_timer = g_timeout_add_seconds(2, update_info, ips_label);

	gtk_main();
	/*
	while(gtk_main_iteration_do(TRUE)) {
	}
	*/
	free(disassembly_text);
	printf("QUITTING!\n");
	/*
//	g_source_remove(draw_timer);
//	gtk_main();
	*/
	return 0;
}

