#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>


typedef unsigned char UBYTE;

static GtkTreeIter iter;
static GtkCellRenderer *renderer;
static GtkTreeModel *model;
static GtkWidget *treeview;
static GtkWidget *vreg_entries[16];
static GtkWidget *vreg_labels[16];
static GtkWidget *sp_label, *i_label, *pc_label, *opc_label;
static GtkWidget *sp_entry, *i_entry, *pc_entry, *opc_entry;
static GtkWidget *v_table;
static GtkWidget *v_frame;
static GtkWidget *vbox, *hbox, *hseparator;
static GtkWidget *entry_from, *entry_to;
static GtkWidget *mem_button, *mem_window, *mem_treeview;
static gboolean mem_window_up;

static UBYTE V_old[16];

static char hexc[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

/*
static unsigned int PC_old;
static unsigned int SP_old;
static unsigned int I_old;
*/

static unsigned int list_addr_start;
static unsigned int list_addr_stop;
static unsigned int program_size;

GdkColor bg_color_changed = {0x0, 0xF000, 0x2000, 0x2000}; // = {0.8, 0.8, 0.2, 0.0};
GdkColor bg_color_normal = {0x0, 0xD000, 0xD000, 0xD000}; // = {0.8, 0.8, 0.2, 0.0};

extern UBYTE *chip8_get_V_regs();
extern unsigned int chip8_get_I();
extern unsigned int chip8_get_SP();
extern unsigned int chip8_get_pc();
extern unsigned short chip8_get_opcode();
extern UBYTE *chip8_get_mem_ptr();

enum {
	ADDR=0,
	OPCODE,
	MNEMONIC,
	N_COLUMNS
};


void int2hex(char *s, int numchars, int val) {
	if(numchars == 4) {
		val &= 0xFFFF;
		s[0] = hexc[(val >> 12) & 0x0F];
		s[1] = hexc[(val >> 8) & 0x0F];
		s[2] = hexc[(val >> 4) & 0x0F];
		s[3] = hexc[val & 0x0F];
		s[4] = (char)0;
	} else {
		val &= 0xFF;
		s[0] = hexc[(val >> 4) & 0x0F];
		s[1] = hexc[val & 0x0F];
		s[2] = (char)0;
	}
}

void mem_window_close() {
	printf("Closing mem window!\n");
}

GtkListStore *mem_treeview_load(int start, int stop) {
	int i,k;
	UBYTE  *mem;
	char s[32];
	char s2[32];
	char c;
	GtkListStore *store;
	GtkTreeIter iter;

	mem = chip8_get_mem_ptr();

	if(start < 0) start = 0;
	if(stop > 65536) stop = 65536;

	store = gtk_list_store_new(18, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING);

	for(i = start; i < stop; i += 16) {
		int2hex(s, 4, i);
		//sprintf(s, "%04X", i);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, s, -1);
		for(k = 0; k < 16; k++) {
			c = mem[i+k];
			if(c >31 && c < 127) {
				s2[k] = c;
			} else {
				s2[k] = '.';
			}
			int2hex(s, 2, c);
			//sprintf(s, "%02X", mem[i+k]);
			gtk_list_store_set(store, &iter, k+1, s);
		}
		s2[k] = 0;
		gtk_list_store_set(store, &iter, k+1, s2);
	}
	return store;
}

void mem_window_open() {
	GtkWidget *vbox, *scrollwindow;
	GtkCellRenderer *rend, *rend2;
	GtkTreeViewColumn *col;
	GtkTreeModel *mem_model;
	PangoFontDescription *pango_font_desc;
	char s[8];
	int i;

	mem_model = GTK_TREE_MODEL(mem_treeview_load(0, 65536));
	//mem_model = GTK_TREE_MODEL(mem_treeview_load(0, 4096));

	mem_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(mem_model));

	// Set font for treeview
	//pango_font_desc = pango_font_description_new();
	//pango_font_description_set_family(pango_font_desc, "monospace: 24");
	pango_font_desc = pango_font_description_from_string("monospace 14");
	//pango_font_description_set_absolute_size(pango_font_desc, (gint)24);
	//gtk_widget_modify_font(textview, pango_font_desc);
	gtk_widget_modify_font(mem_treeview, pango_font_desc);

	g_object_unref(G_OBJECT(mem_model));

	rend = gtk_cell_renderer_text_new();
	rend2 = gtk_cell_renderer_text_new();
	g_object_set(rend2, "editable", TRUE, NULL);
	g_object_set(G_OBJECT(rend), "foreground", "blue", NULL);

	col = gtk_tree_view_column_new_with_attributes("Addr", rend, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mem_treeview), col);
	for(i = 0; i < 16; i++) {
		sprintf(s, "%02X", i);
		//rend = gtk_cell_renderer_text_new();
		col = gtk_tree_view_column_new_with_attributes(s, rend2, "text", i+1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(mem_treeview), col);
	}
	col = gtk_tree_view_column_new_with_attributes("Chars", rend2, "text", i+1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mem_treeview), col);

	mem_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new(FALSE, 4);
	scrollwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwindow), GTK_SHADOW_IN);
	gtk_container_set_border_width(GTK_CONTAINER(scrollwindow), 8);

	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(mem_treeview), GTK_TREE_VIEW_GRID_LINES_VERTICAL);
	gtk_container_add(GTK_CONTAINER(scrollwindow), mem_treeview);
	gtk_box_pack_start(GTK_BOX(vbox), scrollwindow, 1, 1, 4);
	gtk_container_add(GTK_CONTAINER(mem_window), GTK_WIDGET(vbox));

	g_signal_connect(mem_window, "destroy", G_CALLBACK(mem_window_close), NULL);
	gtk_window_set_default_size(GTK_WINDOW(mem_window), 800, 380);
	gtk_widget_show_all(mem_window);
}


gboolean callback_mem_window(GtkWidget *w, gpointer p) {
	if(mem_window_up) {
		mem_window_up = 0;
		gtk_widget_destroy(mem_window);
	} else {
		mem_window_up = 1;
		mem_window_open();
	}

	printf("Klicked mem_button\n");
	return TRUE;
}

void mem_update() {
}


void make_register_widget(GtkWidget **label, GtkWidget **mentry, char *ltext, unsigned int reg) {
	char s[8];
	*label = gtk_label_new(ltext);
	*mentry = gtk_entry_new();
	sprintf(s,"%04X", reg & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(*mentry), s);
	gtk_entry_set_width_chars(GTK_ENTRY(*mentry), 5);
}

GtkWidget *chip8_debug_init_regs() {
	int i, row;
	char s[8];
	UBYTE *v;

	mem_window_up = FALSE;

	v = chip8_get_V_regs();

	vbox = GTK_WIDGET(gtk_vbox_new(FALSE, 4));
	hbox = GTK_WIDGET(gtk_hbox_new(FALSE, 8));

	mem_button = gtk_button_new_with_label("Memory");

	make_register_widget(&sp_label, &sp_entry, "SP", chip8_get_SP());
	make_register_widget(&pc_label, &pc_entry, "PC", chip8_get_pc());
	make_register_widget(&i_label, &i_entry, "I-reg", chip8_get_I());
	make_register_widget(&opc_label, &opc_entry, "Opcode", chip8_get_opcode());

	gtk_box_pack_start(GTK_BOX(hbox), pc_label,0,0,4);
	gtk_box_pack_start(GTK_BOX(hbox), pc_entry,0,0,4);

	gtk_box_pack_start(GTK_BOX(hbox), i_label,0,0,4);
	gtk_box_pack_start(GTK_BOX(hbox), i_entry,0,0,4);

	gtk_box_pack_start(GTK_BOX(hbox), sp_label,0,0,4);
	gtk_box_pack_start(GTK_BOX(hbox), sp_entry,0,0,4);

	gtk_box_pack_start(GTK_BOX(hbox), opc_label,0,0,4);
	gtk_box_pack_start(GTK_BOX(hbox), opc_entry,0,0,4);

	gtk_box_pack_start(GTK_BOX(hbox), mem_button,0,0,4);


	v_table = gtk_table_new(2, 16, TRUE);
	v_frame = gtk_frame_new("Registers");
	gtk_frame_set_shadow_type(GTK_FRAME(v_frame), GTK_SHADOW_IN);
	hseparator = gtk_hseparator_new();

	row = 0;

	for(i = 0; i < 16; i++) {
		sprintf(s, "V%1X", i);
		vreg_labels[i] = gtk_label_new(s);
		gtk_table_attach(GTK_TABLE(v_table), vreg_labels[i],
				i, i+1, row, row+1,
				GTK_SHRINK,
				GTK_SHRINK,
				4, 4);
	}

	row++;

	for(i = 0; i < 16; i++) {
		vreg_entries[i] = gtk_entry_new();
		sprintf(s, "%02X", v[i]);
		V_old[i] = v[i];
		gtk_entry_set_text(GTK_ENTRY(vreg_entries[i]), s);
		gtk_table_attach(GTK_TABLE(v_table), vreg_entries[i],
				i, i+1, row, row+1,
				GTK_SHRINK,
				GTK_SHRINK,
				4, 4);
		gtk_entry_set_width_chars(GTK_ENTRY(vreg_entries[i]), 2);
	}

	gtk_box_pack_start(GTK_BOX(vbox), v_table, 0,0,4);

	gtk_box_pack_start(GTK_BOX(vbox), hseparator, 1, 1, 2);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, 0, 0, 4);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

	gtk_container_add(GTK_CONTAINER(v_frame), vbox);
//	gtk_container_add_with_properties(GTK_CONTAINER(v_frame), vbox,
//			"border-width", 8, NULL);

	//gtk_widget_show_all(hbox);

	return v_frame;
}

void chip8_debug_get_vregs() {
	int i;
	UBYTE *v;
	char s[8];

	v = chip8_get_V_regs();

	for(i = 0; i < 16; i++) {
		sprintf(s, "%02X", v[i]);
		gtk_entry_set_text(GTK_ENTRY(vreg_entries[i]), s);
		if(v[i] != V_old[i]) {
			/*
			gtk_widget_override_background_color(vreg_entries[i],
					GTK_STATE_FLAG_NORMAL,
					&color);
			*/
			gtk_widget_modify_bg(vreg_entries[i], GTK_STATE_NORMAL, &bg_color_changed);
			V_old[i] = v[i];
		} else {
			gtk_widget_modify_bg(vreg_entries[i], GTK_STATE_NORMAL, &bg_color_normal);
		}
	}
	sprintf(s, "%04X", chip8_get_pc() & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(pc_entry), s);
	sprintf(s, "%04X", chip8_get_I() & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(i_entry), s);
	sprintf(s, "%04X", chip8_get_SP() & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(sp_entry), s);
	sprintf(s, "%04X", chip8_get_opcode() & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(opc_entry), s);
}
/*
chip8_debug_add_data(unsigned short addr, unsigned short opcode, char *dis) {
	
}
*/

extern void chip8_disassemble(int, unsigned short, char*);
extern unsigned int chip8_get_programsize();

void init_listview(UBYTE *chip8_mem, int start, int stop) {
	int i, pc;
	unsigned short opcode;
	char str_addr[16];
	char str_opcode[16];
	char str_mnemonic[64];

	pc = start & 0xFFFF;
	stop &= 0xFFFF;

	// If fprogram is the size of uneven bytes, add 1 because
	// opcodes are always two bytes.
	if(stop & 1) stop++;

	// Read in program_size/2 words in treeview
	for(i = 0; i < (stop - start)/2; pc+=2, i++) {
		opcode = (chip8_mem[pc] * 0x100 + chip8_mem[pc+1]) & 0xFFFF;
		sprintf(str_addr, "%04X", pc);
		sprintf(str_opcode, "%04X", opcode);
		chip8_disassemble(pc, opcode, str_mnemonic);
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			ADDR, str_addr, OPCODE, str_opcode, MNEMONIC, str_mnemonic, -1);

	}
}

void chip8_debug_view_range(UBYTE *chip8_mem, unsigned int start, unsigned int stop) {

	char s[8];

	sprintf(s, "%04X", list_addr_start & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(entry_from), s);
	sprintf(s, "%04X", list_addr_stop & 0xFFFF);
	gtk_entry_set_text(GTK_ENTRY(entry_to), s);

	if(model != 0) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		gtk_list_store_clear(GTK_LIST_STORE(model));
	}
	model = GTK_TREE_MODEL(gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
	//model = GTK_TREE_MODEL(gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));

	init_listview(chip8_mem, start, stop);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);

	g_object_unref(model);

}

void chip8_debug_view_reload(UBYTE* chip8_mem) {
	program_size = chip8_get_programsize();

	list_addr_start = 0x200;
	list_addr_stop = 0x200 + program_size -2;

	chip8_debug_view_range(chip8_mem, list_addr_start, list_addr_stop);
}

gboolean treeview_change_range(GtkWidget *w, gpointer *p) {
	char s[8];
	unsigned int from, to;
	UBYTE *chip8_mem;

	strncpy(s, gtk_entry_get_text(GTK_ENTRY(entry_from)), 7);
	from = strtol(s, 0, 16);
	strncpy(s, gtk_entry_get_text(GTK_ENTRY(entry_to)), 7);
	to = strtol(s, 0, 16);

	printf("From: %04X, To: %04X\n", from, to);

	if(from > 0xFFFF || to > 0xFFFF) {
		printf("From/To addresses out of bounds!\n");
		return TRUE;
	} else if (from >= to) {
		printf("From/To address range reversed!\n");
		return TRUE;
	}

	chip8_mem = chip8_get_mem_ptr();
	list_addr_start = from;
	list_addr_stop = to;

	chip8_debug_view_range(chip8_mem, list_addr_start, list_addr_stop);

	return TRUE;
}


GtkWidget *chip8_debug_view_init(UBYTE *chip8_mem) {
	unsigned int pc;
	GtkWidget *frame, *scrollwindow;
	GtkWidget *vbox, *hbox, *label, *blist;

	model = 0;

	pc = 0x200;

	program_size = chip8_get_programsize();

	list_addr_start = pc;
	list_addr_stop = pc + program_size -2;

	// VBOX containing all dissassembly widgets.
	vbox = gtk_vbox_new(FALSE, 4);

	// List assembly from address - to adress widgets.
	hbox = gtk_hbox_new(FALSE, 4);
	label = gtk_label_new("Adress From - To");
	entry_from = gtk_entry_new();
	entry_to = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_from), 5);
	gtk_entry_set_width_chars(GTK_ENTRY(entry_to), 5);
	blist = gtk_button_new_with_label("List");

	gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);

	gtk_box_pack_start(GTK_BOX(hbox), label, 0, 0, 2);
	gtk_box_pack_start(GTK_BOX(hbox), entry_from, 0, 0, 2);
	gtk_box_pack_start(GTK_BOX(hbox), entry_to, 0, 0, 2);
	gtk_box_pack_start(GTK_BOX(hbox), blist, 1, 1, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, 0, 0, 2);

	// TreeView for disassembly listing.
	treeview = gtk_tree_view_new();
	frame = gtk_frame_new("Disassembly");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	scrollwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwindow), GTK_SHADOW_IN);
	gtk_container_set_border_width(GTK_CONTAINER(scrollwindow), 8);

	/*
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Brk Pt",
			renderer,
			"icon", 3, NULL);
	*/

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Address",
			renderer,
			"text", ADDR, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Opcode",
			renderer,
			"text", OPCODE, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Mnemonics",
			renderer,
			"text", MNEMONIC, NULL);


	chip8_debug_view_range(chip8_mem, list_addr_start, list_addr_stop);

	gtk_container_add(GTK_CONTAINER(scrollwindow), treeview);
	gtk_box_pack_start(GTK_BOX(vbox), scrollwindow, 1, 1, 4);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeview), GTK_TREE_VIEW_GRID_LINES_BOTH);

	g_signal_connect(blist, "clicked", G_CALLBACK(treeview_change_range), NULL);
	g_signal_connect(mem_button, "clicked", G_CALLBACK(callback_mem_window), NULL);

	return frame;

}

void chip8_debug_view_focus(unsigned int PC) {
	int index;

	if(PC < list_addr_start || PC > list_addr_stop) {
		printf("PC: %04X is out of range of treeview\n", PC);
		return;
	}
	//index = (PC - 0x200) / 2;
	index = (PC - list_addr_start) / 2;
	GtkTreePath *mpath = gtk_tree_path_new_from_indices(index,-1);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), mpath, NULL, FALSE);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), mpath, NULL, TRUE, 0.5, 0.0);
}
