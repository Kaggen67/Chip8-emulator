#include "chip8.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

static GtkWidget *menubar, *comp_menu, *comp_menu_title;
static GtkWidget *comp_menu_shift, *comp_menu_i, *comp_menu_vf;
static GtkWidget *comp_menu_clip_sprites, *comp_menu_jump0, *comp_menu_xo;

extern void chip8_set_compatibility(int);
extern void chip8_clear_compatibility(int);
extern unsigned short chip8_get_compatibility_flags();
extern void chip8_set_compatibility_flags(unsigned short);


void chip8_menu_update_comp_menu(unsigned short compf) {

	int state;

	state = (compf & (1 << COMPAT_SHIFT_QUIRK)) ? TRUE:FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(comp_menu_shift), state);

	state = (compf & (1 << COMPAT_I_QUIRK)) ? TRUE:FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(comp_menu_i), state);

	state = (compf & (1 << COMPAT_VF_QUIRK)) ? TRUE:FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(comp_menu_vf), state);

	state = (compf & (1 << COMPAT_CLIP_SPRITES_QUIRK)) ? TRUE:FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(comp_menu_clip_sprites), state);

	state = (compf & (1 << COMPAT_JUMP0_QUIRK)) ? TRUE:FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(comp_menu_jump0), state);

	state = (compf & (1 << COMPAT_ENABLE_XO)) ? TRUE:FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(comp_menu_xo), state);

	printf("Flags %04X\n", compf);

}


gboolean handle_compat_menu(GtkWidget *w, gpointer f) {

	unsigned short compat;

	if((int)f == COMPAT_ENABLE_XO) {
		if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
			compat = (1 << COMPAT_ENABLE_XO);
			chip8_set_compatibility_flags(compat);
		} else {
			compat = (1 << COMPAT_SHIFT_QUIRK) | (1 <<COMPAT_I_QUIRK);
			chip8_set_compatibility_flags(compat);
		}
		chip8_menu_update_comp_menu(compat);
		return TRUE;
	}

	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
		chip8_set_compatibility((int) f);
	} else {
		chip8_clear_compatibility((int) f);
	}

	return TRUE;
}



GtkWidget *chip8_menu_init() {

	unsigned short compf;

	compf = chip8_get_compatibility_flags();

	menubar = gtk_menu_bar_new();
	comp_menu = gtk_menu_new();

	comp_menu_title = gtk_menu_item_new_with_label("Compatibility");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(comp_menu_title), comp_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), comp_menu_title);

	comp_menu_shift = gtk_check_menu_item_new_with_label("Shift quirk");
	g_signal_connect(G_OBJECT(comp_menu_shift), "toggled", G_CALLBACK(handle_compat_menu), (gpointer) COMPAT_SHIFT_QUIRK);


	comp_menu_i = gtk_check_menu_item_new_with_label("Load/Store I quirk");
	g_signal_connect(G_OBJECT(comp_menu_i), "toggled", G_CALLBACK(handle_compat_menu), (gpointer) COMPAT_I_QUIRK);

	comp_menu_vf = gtk_check_menu_item_new_with_label("Result after VF write");
	g_signal_connect(G_OBJECT(comp_menu_vf), "toggled", G_CALLBACK(handle_compat_menu), (gpointer) COMPAT_VF_QUIRK);

	comp_menu_clip_sprites = gtk_check_menu_item_new_with_label("Clip sprites");
	g_signal_connect(G_OBJECT(comp_menu_clip_sprites), "toggled", G_CALLBACK(handle_compat_menu), (gpointer) COMPAT_CLIP_SPRITES_QUIRK);

	comp_menu_jump0 = gtk_check_menu_item_new_with_label("jump0 quirk");
	g_signal_connect(G_OBJECT(comp_menu_jump0), "toggled", G_CALLBACK(handle_compat_menu), (gpointer) COMPAT_JUMP0_QUIRK);

	comp_menu_xo = gtk_check_menu_item_new_with_label("Enable XO");
	g_signal_connect(G_OBJECT(comp_menu_xo), "toggled", G_CALLBACK(handle_compat_menu), (gpointer) COMPAT_ENABLE_XO);

	chip8_menu_update_comp_menu(compf);

	gtk_menu_shell_append(GTK_MENU_SHELL(comp_menu), comp_menu_shift);
	gtk_menu_shell_append(GTK_MENU_SHELL(comp_menu), comp_menu_i);
	gtk_menu_shell_append(GTK_MENU_SHELL(comp_menu), comp_menu_vf);
	gtk_menu_shell_append(GTK_MENU_SHELL(comp_menu), comp_menu_clip_sprites);
	gtk_menu_shell_append(GTK_MENU_SHELL(comp_menu), comp_menu_jump0);
	gtk_menu_shell_append(GTK_MENU_SHELL(comp_menu), comp_menu_xo);

	return menubar;

}

