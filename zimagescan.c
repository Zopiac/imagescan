#include <gtk/gtk.h>
#include <dirent.h>  // dynamic image loading dep
#include <stdlib.h>
#include <string.h>  
#include <unistd.h>  // getcwd() dep

#define CYCLE_INTERVAL 100 // ms delay for scrubbing

char **image_files = NULL;
char *image_directory = NULL;
int image_count = 0;
int current_image = 0;
gboolean cycling_forward = FALSE;
gboolean cycling_backward = FALSE;

GtkWidget *image_widget;
GtkWidget *image_label;  // Label for filename & sequence display
GtkWidget *window;
int window_width = 1024, window_height = 600;

// Function for sorting filenames alphanumerically
int compare_filename(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// **Dynamic Image Loading**
void load_images(const char *directory) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    image_files = NULL;
    image_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".png")) {
            image_files = realloc(image_files, (image_count + 1) * sizeof(char *));
            image_files[image_count] = malloc(strlen(directory) + strlen(entry->d_name) + 2);
            sprintf(image_files[image_count], "%s/%s", directory, entry->d_name);
            image_count++;
        }
    }
    closedir(dir);

    // Sort images alphabetically by filename
    qsort(image_files, image_count, sizeof(char *), compare_filename);
}

// **Free Allocated Memory**
void free_images() {
    for (int i = 0; i < image_count; i++) free(image_files[i]);
    free(image_files);
}

// **Image Scaling**
GdkPixbuf *scale_image(const char *filepath) {
    GdkPixbuf *original = gdk_pixbuf_new_from_file(filepath, NULL);
    if (!original) return NULL;

    int img_width = gdk_pixbuf_get_width(original);
    int img_height = gdk_pixbuf_get_height(original);

    int win_width, win_height;
    gtk_window_get_size(GTK_WINDOW(window), &win_width, &win_height);

    int new_width = win_width * 0.8;
    int new_height = win_height * 0.8;

    double scale_factor = (double)new_width / img_width < (double)new_height / img_height ?
                          (double)new_width / img_width : (double)new_height / img_height;

    GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(original, img_width * scale_factor, img_height * scale_factor, GDK_INTERP_BILINEAR);
    g_object_unref(original);
    return scaled_pixbuf;
}

// **Update Image**
void update_image() {
    if (image_count > 0) {
        // Extract filename from full path
        const char *filename = strrchr(image_files[current_image], '/');
        filename = filename ? filename + 1 : image_files[current_image];  // Remove directory path

        // Update image display
        GdkPixbuf *scaled_pixbuf = scale_image(image_files[current_image]);
        if (scaled_pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), scaled_pixbuf);
            g_object_unref(scaled_pixbuf);
        }

        // Update label text with filename & sequence number
        char label_text[256];
        snprintf(label_text, sizeof(label_text), "%s (%d/%d)", filename, current_image + 1, image_count);
        gtk_label_set_text(GTK_LABEL(image_label), label_text);
    }
}

// Navigation Functions
void first_image(GtkWidget *widget, gpointer data) { current_image = 0; update_image(); }
void last_image(GtkWidget *widget, gpointer data) { current_image = image_count - 1; update_image(); }
void prev_image(GtkWidget *widget, gpointer data) { current_image = (current_image - 1 + image_count) % image_count; update_image(); }
void next_image(GtkWidget *widget, gpointer data) { current_image = (current_image + 1) % image_count; update_image(); }

// Cycling Functions
gboolean cycle_backward(gpointer data) { if (cycling_backward) { prev_image(NULL, NULL); return TRUE; } return FALSE; }
void start_cycle_backward(GtkWidget *widget, GdkEventButton *event, gpointer data) { cycling_backward = TRUE; g_timeout_add(CYCLE_INTERVAL, cycle_backward, NULL); }
void stop_cycle_backward(GtkWidget *widget, GdkEventButton *event, gpointer data) { cycling_backward = FALSE; }

gboolean cycle_forward(gpointer data) { if (cycling_forward) { next_image(NULL, NULL); return TRUE; } return FALSE; }
void start_cycle_forward(GtkWidget *widget, GdkEventButton *event, gpointer data) { cycling_forward = TRUE; g_timeout_add(CYCLE_INTERVAL, cycle_forward, NULL); }
void stop_cycle_forward(GtkWidget *widget, GdkEventButton *event, gpointer data) { cycling_forward = FALSE; }

// Refresh Image List
void refresh_images(GtkWidget *widget, gpointer data) {
    free_images();  // Clear old images
    image_files = NULL;
    image_count = 0;
    load_images(image_directory);  // Use stored directory path
    current_image = 0;
    update_image();
}

// Handle Key Presses
gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    switch (event->keyval) {
        case GDK_KEY_q: gtk_main_quit(); break;
        case GDK_KEY_Left: prev_image(NULL, NULL); break;
        case GDK_KEY_Right: next_image(NULL, NULL); break;
        case GDK_KEY_Home: first_image(NULL, NULL); break;
        case GDK_KEY_End: last_image(NULL, NULL); break;
        case GDK_KEY_BackSpace: refresh_images(NULL, NULL); break;
    }
    return FALSE;
}

// Window Resize Handling
void on_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer data) { window_width = allocation->width; window_height = allocation->height; update_image(); }

// Main Function
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    char cwd[1024];  // Buffer to hold current working directory
    if (argc < 2) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            image_directory = strdup(cwd);  // Use current directory
        } else {
            perror("Failed to get current directory");
            return 1;
        }
    } else {
        image_directory = strdup(argv[1]);  // Use provided directory
    }

    load_images(image_directory);
    if (image_count == 0) {
        fprintf(stderr, "No images found in directory: %s\n", image_directory);
        return 1;
    }
    // Create Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ZImageScan");
    gtk_window_set_default_size(GTK_WINDOW(window), window_width, window_height);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "size-allocate", G_CALLBACK(on_resize), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(key_press), NULL);

    // Set Background Color
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_set_name(frame, "background");
    gtk_container_add(GTK_CONTAINER(window), frame);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, "#background { background-color: #000; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(frame);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Layout
    // Create main vertical layout to stack image and label
    GtkWidget *vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox_main);

    // Create horizontal layout for buttons & image
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox, TRUE, TRUE, 0);

    // Left & Right Button Panels
    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), right_vbox, FALSE, FALSE, 0);


    // Image Display
    image_widget = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(hbox), image_widget, TRUE, TRUE, 0);

    // Image Display (wrapped in event box to allow clicking to refresh) -- broken
    //GtkWidget *image_event_box = gtk_event_box_new();
    //gtk_container_add(GTK_CONTAINER(image_event_box), image_widget);
    //g_signal_connect(image_event_box, "button-press-event", G_CALLBACK(refresh_images), NULL);
    //gtk_box_pack_start(GTK_BOX(hbox), image_event_box, TRUE, TRUE, 0);

    // Create Label for Filename & Sequence
    image_label = gtk_label_new("");
    gtk_box_pack_end(GTK_BOX(vbox_main), image_label, FALSE, FALSE, 0);


    // Create buttons with labels
    GtkWidget *buttons[] = {
        gtk_button_new_with_label("|<"),
        gtk_button_new_with_label("<"),
        gtk_button_new_with_label("<<<"),
        gtk_button_new_with_label(">|"),
        gtk_button_new_with_label(">"),
        gtk_button_new_with_label(">>>"),
        gtk_button_new_with_label("Refresh Images"),
        gtk_button_new_with_label("Quit")
    };

    // Connect each button to its function
    g_signal_connect(buttons[0], "clicked", G_CALLBACK(first_image), NULL);
    g_signal_connect(buttons[1], "clicked", G_CALLBACK(prev_image), NULL);
    g_signal_connect(buttons[2], "button-press-event", G_CALLBACK(start_cycle_backward), NULL);
    g_signal_connect(buttons[2], "button-release-event", G_CALLBACK(stop_cycle_backward), NULL);
    g_signal_connect(buttons[3], "clicked", G_CALLBACK(last_image), NULL);
    g_signal_connect(buttons[4], "clicked", G_CALLBACK(next_image), NULL);
    g_signal_connect(buttons[5], "button-press-event", G_CALLBACK(start_cycle_forward), NULL);
    g_signal_connect(buttons[5], "button-release-event", G_CALLBACK(stop_cycle_forward), NULL);
    g_signal_connect(buttons[6], "clicked", G_CALLBACK(refresh_images), NULL);  // Refresh button action
    g_signal_connect(buttons[7], "clicked", G_CALLBACK(gtk_main_quit), NULL);  // Quit button

    // Add buttons to left and right panels
    gtk_box_pack_start(GTK_BOX(left_vbox), buttons[7], TRUE, TRUE, 0);  // Quit button
    for (int i = 0; i < 3; i++) { gtk_box_pack_start(GTK_BOX(left_vbox), buttons[i], TRUE, TRUE, 0); }  // Left side navigation
    gtk_box_pack_start(GTK_BOX(right_vbox), buttons[6], TRUE, TRUE, 0);  // Refresh button
    for (int i = 3; i < 6; i++) { gtk_box_pack_start(GTK_BOX(right_vbox), buttons[i], TRUE, TRUE, 0); } // Right side navigation
    for (int i = 0; i < 6; i++) { gtk_widget_set_size_request(buttons[i], 100, 50); } //nav button size
    for (int i = 6; i < 8; i++) { gtk_widget_set_size_request(buttons[i], 100, 10); }

    //gtk_box_pack_start(GTK_BOX(left_vbox), buttons[6], TRUE, TRUE, 0);  // Refresh button on the left side -- ruins symmetry; find another solution
 
    update_image();
    gtk_widget_show_all(window);
    gtk_main();
    free_images();
    return 0;
}
