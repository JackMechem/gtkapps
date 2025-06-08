#include <gio/gio.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <string.h>

static gboolean on_key_pressed(GtkEventControllerKey *ctrl, guint keyval,
							   guint keycode, GdkModifierType mods,
							   gpointer user_data) {
	GApplication *app = G_APPLICATION(user_data);
	if (keyval == GDK_KEY_Escape) {
		g_application_quit(app);
		return TRUE;
	}
	if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
		GtkListBox *list = g_object_get_data(G_OBJECT(app), "app-list");
		if (list) {
			GtkListBoxRow *row = gtk_list_box_get_selected_row(list);
			if (row) {
				GAppInfo *info = g_object_get_data(G_OBJECT(row), "appinfo");
				if (info) {
					GError *err = NULL;
					GAppLaunchContext *ctx = g_app_launch_context_new();
					if (!g_app_info_launch(info, NULL, ctx, &err)) {
						g_printerr("Launch failed: %s\n",
								   err ? err->message : "(unknown)");
						g_clear_error(&err);
					}
					g_object_unref(ctx);
				}
			}
		}
		g_application_quit(app);
		return TRUE;
	}
	return FALSE;
}

static void on_map(GtkWidget *widget, gpointer unused) {
	GtkWindow *win = GTK_WINDOW(widget);
	GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(widget));
	GdkDisplay *disp = gdk_surface_get_display(surface);
	GdkMonitor *mon = gdk_display_get_monitor_at_surface(disp, surface);
	if (mon) {
		GdkRectangle geo;
		gdk_monitor_get_geometry(mon, &geo);
		int W, H;
		gtk_window_get_default_size(win, &W, &H);
		gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_LEFT,
							 (geo.width - W) / 2);
		gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_TOP,
							 (geo.height - H) / 2);
	}
}

static gboolean filter_func(GtkListBoxRow *row, gpointer user_data) {
	GtkSearchEntry *search = GTK_SEARCH_ENTRY(user_data);
	const char *text = gtk_editable_get_text(GTK_EDITABLE(search));
	const char *name = g_object_get_data(G_OBJECT(row), "app-name");
	char *ln = g_utf8_strdown(name, -1);
	char *lt = g_utf8_strdown(text, -1);
	gboolean keep = (*lt == '\0') || (strstr(ln, lt) != NULL);
	g_free(ln);
	g_free(lt);
	return keep;
}

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
	GtkListBox *list = GTK_LIST_BOX(user_data);
	gtk_list_box_invalidate_filter(list);
	for (guint i = 0;; i++) {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index(list, i);
		if (!row)
			break;
		if (filter_func(row, entry)) {
			gtk_list_box_select_row(list, row);
			break;
		}
	}
}

static void on_search_entry_activate(GtkSearchEntry *entry,
									 gpointer user_data) {
	GApplication *app = G_APPLICATION(user_data);
	GtkListBox *list = g_object_get_data(G_OBJECT(app), "app-list");
	if (list) {
		for (guint i = 0;; i++) {
			GtkListBoxRow *row = gtk_list_box_get_row_at_index(list, i);
			if (!row)
				break;
			if (filter_func(row, entry)) {
				GAppInfo *info = g_object_get_data(G_OBJECT(row), "appinfo");
				if (info) {
					GError *err = NULL;
					GAppLaunchContext *ctx = g_app_launch_context_new();
					if (!g_app_info_launch(info, NULL, ctx, &err)) {
						g_printerr("Failed to launch \"%s\": %s\n",
								   g_app_info_get_name(info),
								   err ? err->message : "(unknown)");
						g_clear_error(&err);
					}
					g_object_unref(ctx);
				}
				break;
			}
		}
	}
	g_application_quit(app);
}

static void activate(GtkApplication *app, gpointer unused) {
	const int W = 600, H = 400;
	GtkWindow *win = GTK_WINDOW(gtk_application_window_new(app));
	gtk_window_set_title(win, "Overlay Launcher");
	gtk_window_set_default_size(win, W, H);
	gtk_layer_init_for_window(win);
	gtk_layer_set_layer(win, GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_keyboard_mode(win, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
	gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
	gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
	gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
	gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
	g_signal_connect(win, "map", G_CALLBACK(on_map), NULL);

	GtkEventController *kc = gtk_event_controller_key_new();
	g_signal_connect(kc, "key-pressed", G_CALLBACK(on_key_pressed), app);
	gtk_widget_add_controller(GTK_WIDGET(win), kc);

	GtkCssProvider *css = gtk_css_provider_new();
	char *css_path =
		g_build_filename(g_get_user_config_dir(), "gtkapps", "style.css", NULL);
	gtk_css_provider_load_from_path(css, css_path);
	g_free(css_path);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
		GTK_STYLE_PROVIDER_PRIORITY_USER);
	g_object_unref(css);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkSearchEntry *search = GTK_SEARCH_ENTRY(gtk_search_entry_new());
	GtkWidget *scrolled = gtk_scrolled_window_new();
	GtkListBox *list = GTK_LIST_BOX(gtk_list_box_new());

	gtk_list_box_set_selection_mode(list, GTK_SELECTION_SINGLE);
	gtk_widget_set_can_focus(GTK_WIDGET(list), TRUE);

	gtk_widget_set_hexpand(GTK_WIDGET(search), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(list), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
								  GTK_WIDGET(list));

	g_object_set_data(G_OBJECT(app), "app-list", list);

	gtk_list_box_set_filter_func(list, filter_func, search, NULL);
	g_signal_connect(search, "changed", G_CALLBACK(on_search_changed), list);
	g_signal_connect(search, "activate", G_CALLBACK(on_search_entry_activate),
					 app);

	gtk_widget_add_css_class(GTK_WIDGET(win), "window");
	gtk_widget_add_css_class(GTK_WIDGET(win), "overlay-launcher");
	gtk_widget_add_css_class(vbox, "vbox");
	gtk_widget_add_css_class(GTK_WIDGET(search), "search-entry");
	gtk_widget_add_css_class(scrolled, "scrolled-window");
	gtk_widget_add_css_class(GTK_WIDGET(list), "list-box");

	GtkListBoxRow *row;
	GList *apps = g_app_info_get_all();
	for (GList *l = apps; l; l = l->next) {
		GAppInfo *info = G_APP_INFO(l->data);
		const char *name = g_app_info_get_display_name(info);
		GIcon *icon = g_app_info_get_icon(info);

		GtkWidget *img =
			icon && G_IS_OBJECT(icon)
				? gtk_image_new_from_gicon(icon)
				: gtk_image_new_from_icon_name("application-x-executable");
		if (icon)
			g_object_unref(icon);
		gtk_image_set_pixel_size(GTK_IMAGE(img), 32);

		GtkWidget *lbl = gtk_label_new(name);
		gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
		gtk_box_append(GTK_BOX(hbox), img);
		gtk_box_append(GTK_BOX(hbox), lbl);

		row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
		gtk_widget_add_css_class(GTK_WIDGET(row), "list-box-row");
		gtk_widget_add_css_class(hbox, "hbox");
		gtk_widget_add_css_class(img, "app-image");
		gtk_widget_add_css_class(lbl, "app-label");

		g_object_set_data_full(G_OBJECT(row), "app-name", g_strdup(name),
							   g_free);
		g_object_set_data_full(G_OBJECT(row), "appinfo", g_object_ref(info),
							   (GDestroyNotify)g_object_unref);

		gtk_list_box_append(list, GTK_WIDGET(row));
		gtk_list_box_row_set_child(row, hbox);
		g_object_unref(info);
	}
	g_list_free(apps);

	gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(search));
	gtk_box_append(GTK_BOX(vbox), scrolled);
	gtk_window_set_child(win, vbox);
	gtk_window_present(win);
}

int main(int argc, char **argv) {
	GtkApplication *app = gtk_application_new("com.example.OverlayLauncher",
											  G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}
