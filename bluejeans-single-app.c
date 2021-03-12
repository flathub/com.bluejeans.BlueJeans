/*
 * Copyright (c) 2021 Bastien Nocera <hadess@hadess.net>
 *
 * This program is not free software, all rights reserved.
 *
 */

#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static GPid helper_pid = 0;

static void
main_app_exited (GPid pid,
		 gint status,
		 gpointer user_data)
{
  GApplication *app = user_data;

  g_debug ("Main app (PID %d) exited", pid);
  g_application_release (app);
}

static gboolean
launch (char        **args,
	GSpawnFlags   flags,
	GPid         *pid,
	GError      **error)
{
  g_autofree char *cmdline = NULL;
  cmdline = g_strjoinv (" ", args);

  if (g_getenv ("BJ_DEBUG") != NULL) {
    g_message ("“Launching” %s", cmdline);
    return TRUE;
  }

  g_debug ("Launching %s", cmdline);
  return g_spawn_async (NULL,
			args,
			NULL,
			flags,
			NULL,
			NULL,
			pid,
			error);
}

static void
activate (GApplication *app)
{
  g_autofree char *logs_dir = NULL;
  const char *helper_args[] = { "/app/extra/resources/BluejeansHelper", NULL };
  g_autoptr(GError) error = NULL;
  const char *main_app_args[] = { "/app/extra/bluejeans-v2", NULL };
  GPid main_app_pid = 0;

  g_debug ("Bluejeans activated\n");

  /* Create logs directory */
  logs_dir = g_build_filename (g_get_home_dir(), ".config", "bluejeans-v2", "logs", NULL);
  g_mkdir_with_parents (logs_dir, 0700);

  /* Launch BluejeansHelper */
  if (!launch ((char **) helper_args, G_SPAWN_DEFAULT, &helper_pid, &error)) {
    g_warning ("Failed to launch BluejeansHelper: %s",
               error->message);
    return;
  }

  /* Launch main app */
  if (!launch ((char **) main_app_args, G_SPAWN_DO_NOT_REAP_CHILD, &main_app_pid, &error)) {
    g_warning ("Failed to launch bluejeans-v2: %s",
               error->message);
    return;
  }

  if (!g_getenv ("BJ_DEBUG"))
    g_child_watch_add (main_app_pid, (GChildWatchFunc) main_app_exited, app);

  g_application_hold (app);
}

static void
open (GApplication  *app,
      GFile        **files,
      gint           n_files,
      const gchar   *hint)
{
  GPtrArray *array = NULL;
  guint i;
  g_autoptr(GError) error = NULL;
  GPid main_app_pid = 0;

  g_debug ("Bluejeans open called");

  array = g_ptr_array_new_with_free_func (g_free);
  g_ptr_array_add (array, g_strdup ("/app/extra/bluejeans-v2"));
  for (i = 0; i < n_files; i++)
    g_ptr_array_add (array, g_file_get_uri (files[i]));
  g_ptr_array_add (array, NULL);

  /* Note that launching a new instance, actually closes the original
   * instance and leaves us with the "updated" second instance,
   * so be careful to continue watching the instance we're interested in */
  g_application_hold (app);

  g_debug ("Launching bluejeans-v2 with URIs");
  if (!launch ((char **) array->pdata, G_SPAWN_DO_NOT_REAP_CHILD, &main_app_pid, &error)) {
    g_warning ("Failed to launch bluejeans-v2 with URI: %s",
               error->message);
    return;
  }
  g_ptr_array_free (array, TRUE);

  if (!g_getenv ("BJ_DEBUG"))
    g_child_watch_add (main_app_pid, (GChildWatchFunc) main_app_exited, app);
}

int
main (int argc, char **argv)
{
  GApplication *app;
  g_autoptr(GError) error = NULL;
  int status;

#if DEBUG > 0
  g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);
#endif

  app = g_application_new ("com.bluejeans.BlueJeans",
                           G_APPLICATION_HANDLES_OPEN);
  if (!g_application_register (app, NULL, &error)) {
    g_warning ("Failed to register com.bluejeans.BlueJeans on D-Bus: %s",
    	       error->message);
    return 1;
  }

  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "open", G_CALLBACK (open), NULL);
  g_application_set_inactivity_timeout (app, 1000);

  status = g_application_run (app, argc, argv);

  if (helper_pid > 0)
    kill (helper_pid, SIGKILL);

  g_object_unref (app);

  return status;
}
