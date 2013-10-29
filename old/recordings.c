#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include "recordings.h"
#include "main.h"
#include "separator.h"
#include "mplayer.h"
#include "misc.h"

static void set_property_float(GObject *gobj, const char *prop, float f)
{
	GValue *gval = g_new0(GValue, 1);
	g_value_init(gval, G_TYPE_FLOAT);
	g_value_set_float(gval, f);
	g_object_set_property(gobj, prop, gval);
	g_value_unset(gval);
	g_free(gval);
}

static void on_tree_view_realize(GtkWidget *);
static void on_tree_view_row_activated(GtkTreeView *,
		GtkTreePath *, GtkTreeViewColumn *);
static void on_tree_view_selection_changed(GtkTreeSelection *);
static GtkTreeStore *build_tree();
static void find_recordings(const char *);
static struct Recording *new_recording(const char *, const char *);
static gint recording_cmp(gconstpointer a, gconstpointer b);

struct Recording
{
	char *dir;		/* full directory path of recording */
	double dir_size;	/* size of directory in MB */
	char *info_path;	/* full path to info.vdr */
	time_t time;		/* unix time taken from dir name */
	char *title;		/* title taken from info.vdr */
	char *description;	/* description take from info.vdr */
};

struct Group
{
	GSList *recordings;
	time_t newest;
};

GList *recordings = NULL;
GtkTreeStore *store;
GtkTreeModel *sorted_store;

enum
{
	TITLE_COLUMN,
	TIME_STR_COLUMN,
	TIME_COLUMN,
	REC_PTR_COLUMN,
	N_COLUMNS
};

GtkWidget *create_recordings_page()
{
	GtkWidget *vbox;
	GtkWidget *scroll;
	GtkWidget *tree;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkWidget *separator;
	GtkWidget *alignment;
	GtkWidget *label;
	GtkTreeSelection *selection;
	GValue *gval;

	vbox = gtk_vbox_new(FALSE, 16);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_widget_show(scroll);

	store = build_tree();
	sorted_store = gtk_tree_model_sort_new_with_model(
			GTK_TREE_MODEL(store));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sorted_store),
			TIME_COLUMN, GTK_SORT_DESCENDING);

	tree = gtk_tree_view_new_with_model(sorted_store);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_attributes(column, cell,
			"text", TITLE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, cell, FALSE);
	gtk_tree_view_column_set_expand(column, FALSE);
	gtk_tree_view_column_set_attributes(column, cell,
			"text", TIME_STR_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	g_signal_connect(G_OBJECT(selection), "changed",
			G_CALLBACK(on_tree_view_selection_changed), NULL);

	gtk_container_add(GTK_CONTAINER(scroll), tree);
	g_signal_connect_after(G_OBJECT(tree), "realize",
			G_CALLBACK(on_tree_view_realize), NULL);
	g_signal_connect(G_OBJECT(tree), "row-activated",
			G_CALLBACK(on_tree_view_row_activated), NULL);
	gtk_widget_show(tree);

	return vbox;
}

static void on_tree_view_realize(GtkWidget *tree)
{
//	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(tree));
}

static void on_tree_view_row_activated(GtkTreeView *tree,
		GtkTreePath *path, GtkTreeViewColumn *column)
{
	GtkTreeIter iter;
	GValue *val = g_new0(GValue, 1);
	struct Recording *rec;

	gtk_tree_model_get_iter(sorted_store, &iter, path);
	gtk_tree_model_get_value(sorted_store, &iter, REC_PTR_COLUMN, val);
	if((rec = g_value_get_pointer(val)))
		g_debug("would have played %s", rec->dir);
//		run_mplayer(rec->dir, "001.vdr");

	g_value_unset(val);
	g_free(val);
}

static void on_tree_view_selection_changed(GtkTreeSelection *selection)
{
	GtkTreeIter iter, child_iter;
	GValue *val = g_new0(GValue, 1);
	struct Recording *rec;

	if(gtk_tree_selection_get_selected(selection, NULL, &iter))
	{
		gtk_tree_model_get_value(sorted_store, &iter,
				REC_PTR_COLUMN, val);
		if((rec = g_value_get_pointer(val)))
		{
			gtk_tree_model_sort_convert_iter_to_child_iter(
					GTK_TREE_MODEL_SORT(sorted_store),
					&child_iter, &iter);
		}
		g_value_unset(val);
	}
	
	g_free(val);
}

static GtkTreeStore *build_tree()
{
	GtkTreeStore *store;
	GtkTreeIter tree_iter, *tree_parent = NULL;
	GList *rec_iter;
	char *prev_title;

	find_recordings(RECORDINGS_DIR);
	store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING,G_TYPE_STRING,
			G_TYPE_UINT, G_TYPE_POINTER);

	for(rec_iter = recordings; rec_iter; rec_iter = rec_iter->next)
	{
		struct Recording *rec = rec_iter->data;
		struct tm tm_now, tm_rec;
		time_t now;
		char date_str[32];

		if(prev_title && strcmp(rec->title, prev_title) == 0)
		{
			if(!tree_parent)
				tree_parent = gtk_tree_iter_copy(&tree_iter);
		} else if(tree_parent)
		{
			gtk_tree_iter_free(tree_parent);
			tree_parent = NULL;
		}

		now = time(NULL);
		localtime_r(&now, &tm_now);
		localtime_r(&rec->time, &tm_rec);

		strftime(date_str, sizeof(date_str), "%Y-%m-%d",
				&tm_rec);

		if(tm_rec.tm_year == tm_now.tm_year)
		{
			int diff = tm_now.tm_yday - tm_rec.tm_yday;

			if(diff == 0)
				strftime(date_str, sizeof(date_str),
						"Today", &tm_rec);
			else if(diff == 1)
				strftime(date_str, sizeof(date_str),
						"Yesterday", &tm_rec);
			else if(diff < 6)
				strftime(date_str, sizeof(date_str),
						"%A", &tm_rec);
		}

		gtk_tree_store_append(store, &tree_iter, tree_parent);
		gtk_tree_store_set(store, &tree_iter,
				TITLE_COLUMN, rec->title,
				TIME_STR_COLUMN, date_str,
				TIME_COLUMN, rec->time,
				REC_PTR_COLUMN, rec,
				-1);

		prev_title = rec->title;
	}

	return store;
}

static void find_recordings(const char *dir_path)
{
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	GSList *subdirs = NULL;
	struct Recording *rec = NULL;
	double dir_size = 0;

	if((dir = opendir(dir_path)) == NULL)
		return;

	while((ent = readdir(dir)) != NULL)
	{
		char *path = join_paths(dir_path, ent->d_name);

		/* skip hidden files and files we can't stat */
		if(ent->d_name[0] == '.' || stat(path, &statbuf) != 0)
		{
			/* skip hidden files */
			g_free(path);
			continue;
		}

		if(S_ISDIR(statbuf.st_mode))
		{
			/* it's a sub-directory; save it for later */
			subdirs = g_slist_append(subdirs, path);
			continue;
		} else if(!S_ISREG(statbuf.st_mode))
		{
			/* skip non-regular files */
			g_free(path);
			continue;
		}

		if(strcmp(ent->d_name, "info.vdr") == 0)
		{
			/* definitely a recording */
			rec = new_recording(dir_path, path);
		}

		dir_size += statbuf.st_size / 1000000.0;

		g_free(path);
	}

	closedir(dir);

	if(rec)
	{
		char *tmp = pretty_size(dir_size);
		rec->dir_size = dir_size;
		g_debug("%s size = %s", rec->title, tmp);
		g_free(tmp);
	}

	while(subdirs)
	{
		char *path = subdirs->data;
		find_recordings(path);
		g_free(path);
		subdirs = g_slist_delete_link(subdirs, subdirs);
	}
}

static struct Recording *new_recording(const char *dir, const char *info_path)
{
	struct Recording *rec;
	gchar **path_bits;
	gchar *info;
	int i;

	rec = g_malloc0(sizeof(struct Recording));
	rec->dir = g_strdup(dir);
	rec->info_path = strdup(info_path);

	path_bits = g_strsplit(dir, "/", 0);
	for(i = 0; path_bits[i] && path_bits[i + 1]; i++);
	
	if(path_bits[i])
	{
		/* last element; date and time */
		struct tm tm;
		memset(&tm, 0, sizeof(struct tm));
		sscanf(path_bits[i], "%u-%u-%u.%u.%u.%*u.%*u.rec",
				&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
				&tm.tm_hour, &tm.tm_min);
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		rec->time = mktime(&tm);
	}

	g_strfreev(path_bits);

	if(g_file_get_contents(info_path, &info, NULL, NULL))
	{
		char *beg, *end;

		for(beg = end = info;; beg = ++end)
		{
			while(*end && *end != '\n')
				end++;

			if(!*end)
				break;

			if(end - beg < 3)
				continue;

			*end = '\0';

			switch(*beg)
			{
				case 'T':
					rec->title = g_strdup(beg + 2);
					break;
				case 'D':
					rec->description =
						g_strdup(beg + 2);
					break;
			}
		}

		g_free(info);
	}

	recordings = g_list_insert_sorted(recordings, rec, recording_cmp);

	return rec;
}

static gint recording_cmp(gconstpointer a, gconstpointer b)
{
	const struct Recording *rec_a = a;
	const struct Recording *rec_b = b;
	int r = strcmp(rec_a->title, rec_b->title);

	return (r == 0 ? (rec_a->time == rec_b->time ? 0
				: (rec_a->time > rec_b->time ? -1 : 1))
			: r);
}

