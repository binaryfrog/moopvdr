#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "main.h"
#include "separator.h"
#include "recordings.h"

GtkWidget *window_main;
GtkWidget *notebook_main;
GtkWidget *label_header;

static void on_recordings_clicked(GtkWidget *button)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_main), 1);
	gtk_label_set_text(GTK_LABEL(label_header), "<span weight='bold' font_desc='20'>Moop VDR - Recordings</span>");
	gtk_label_set_use_markup(GTK_LABEL(label_header), TRUE);
}

int main(int argc, char **argv)
{
	GtkWidget *alignment;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *separator;
	GtkWidget *page;
	GtkWidget *button;

	gtk_init(&argc, &argv);

#ifdef RC_FILE
	gtk_rc_parse(RC_FILE);
#endif

	window_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window_main), WIDTH, HEIGHT);
	gtk_widget_set_size_request(window_main, WIDTH, HEIGHT);
	g_signal_connect(G_OBJECT(window_main), "delete-event",
			G_CALLBACK(gtk_main_quit), NULL);

	alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
			HEIGHT*UNDERSCAN_Y, HEIGHT*UNDERSCAN_Y,
			WIDTH*UNDERSCAN_X, WIDTH*UNDERSCAN_X);
	gtk_container_add(GTK_CONTAINER(window_main), alignment);
	gtk_widget_show(alignment);

	vbox = gtk_vbox_new(FALSE, 16);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);
	gtk_widget_show(vbox);

	alignment = gtk_alignment_new(0, 0.5, 0.0, 1.0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
	label_header = label = gtk_label_new(
			"<span weight='bold' font_desc='20'>Moop VDR</span>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(alignment), label);
	gtk_widget_show(label);
	gtk_widget_show(alignment);
	
	separator = separator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
	gtk_widget_show(separator);

	notebook_main = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook_main), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook_main), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), notebook_main, TRUE, TRUE, 0);
	gtk_widget_show(notebook_main);

	/*
	 * Main page
	 */

	page = gtk_table_new(0, 0, TRUE);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_main), page, NULL);
	gtk_widget_show(page);

	button = gtk_button_new_with_label("Recordings");
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
	gtk_table_attach(GTK_TABLE(page), button, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(on_recordings_clicked), NULL);
	gtk_widget_show(button);

	/*
	 * Recordings page
	 */

	page = create_recordings_page();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_main), page, NULL);
	gtk_widget_show(page);

	gtk_widget_show(window_main);

	gtk_main();

	return 0;
}

