#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
//#include <gdk/gdkkeysyms.h>

typedef struct {
	int ID;
	char *keylabel;
} KeyLabel;

/*
 *  CHIP8 Keypad layout
 *
 * 	---------
 * 	|1 2 3 C|
 * 	|4 5 6 D|
 * 	|7 8 9 E|
 * 	|A 0 B F|
 * 	---------
 *
 */

static GtkWidget *window;
static GtkWidget *buttons[16];
static GtkWidget *table;
static GtkWidget *button_toggle_keypad;

static gboolean toggle_flag;

static KeyLabel key_labels[16] = {{1, "1"},{2, "2"},{3, "3"},{0x0C, "C"},
			{4, "4"},{5, "5"},{6, "6"},{0x0D, "D"},
			{7, "7"},{8, "8"},{9, "9"},{0x0E, "E"},
			{0x0A, "A"},{0, "0"},{0x0B, "B"},{0x0F, "F"}};

extern void chip8_key_pressed(int);
extern void chip8_key_released(int);

gboolean button_toggle(GtkWidget *w, gpointer keylabel) {
	KeyLabel *kl = (KeyLabel *) keylabel;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
		chip8_key_pressed(kl->ID);
		printf("Index: %d, Label: %s, ACTIVE\n", kl->ID, kl->keylabel);
	} else {
		chip8_key_released(kl->ID);
		printf("Index: %d, Label: %s, DEACTIVE\n", kl->ID, kl->keylabel);
	}
	return TRUE;
}

gboolean window_close(GtkWidget *w) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_toggle_keypad), FALSE);
	return TRUE;
}

GtkWidget *chip8_keypad_open() {
	int i, j, k;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "CHIP8 KeyPad");
	gtk_window_set_default_size(GTK_WINDOW(window), 240, 320);
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);

	table = gtk_table_new(4, 4, TRUE);

	k = 0;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			buttons[k] = gtk_toggle_button_new_with_label(key_labels[k].keylabel);
			gtk_table_attach(GTK_TABLE(table),
					buttons[k],
					j, j+1,
					i, i+1,
					GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
					2, 2);
			g_signal_connect(buttons[k], "toggled", G_CALLBACK(button_toggle), (gpointer) &key_labels[k]);
			k++;
		}
	}

	gtk_container_add(GTK_CONTAINER(window), table);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(window_close), NULL);

	gtk_widget_show_all(window);

	return window;
}

void chip8_keypad_close() {
	gtk_widget_destroy(window);
}

gboolean toggle_keypad(GtkWidget *w) {
	if(!toggle_flag) {
		chip8_keypad_open();
		toggle_flag = TRUE;
	} else {
		chip8_keypad_close();
		toggle_flag = FALSE;
	}
	return TRUE;
}

GtkWidget *chip8_keypad_init() {
	toggle_flag = 0;
	button_toggle_keypad = gtk_toggle_button_new_with_label("Keypad");
	g_signal_connect(button_toggle_keypad, "toggled", G_CALLBACK(toggle_keypad), NULL);

	return button_toggle_keypad;
}


