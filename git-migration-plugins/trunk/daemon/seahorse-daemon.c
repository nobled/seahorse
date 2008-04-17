/*
 * Seahorse
 *
 * Copyright (C) 2005 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <sys/signal.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>

#include <gnome.h>

#include "config.h"
#include "seahorse-daemon.h"
#include "seahorse-secure-memory.h"
#include "seahorse-gconf.h"
#include "seahorse-gtkstock.h"
#include "seahorse-context.h"
#include "seahorse-unix-signal.h"

static gboolean daemon_no_daemonize = FALSE;
static gboolean daemon_running = FALSE;
static gboolean daemon_quit = FALSE;

static const gchar *daemon_icons[] = {
    SEAHORSE_ICON_SHARING,
    NULL
};

static const GOptionEntry options[] = {
    { "no-daemonize", 'd', 0, G_OPTION_ARG_NONE, &daemon_no_daemonize, 
        N_("Do not run seahorse-daemon as a daemon"), NULL },
    { NULL }
};

static void
daemonize ()
{
    /* 
     * We can't use the normal daemon call, because we have
     * special things to do in the parent after forking 
     */

    pid_t pid;
    int i;

    if (!daemon_no_daemonize) {
        switch ((pid = fork ())) {
        case -1:
            err (1, _("couldn't fork process"));
            break;

        /* The child */
        case 0:
            if (setsid () == -1)
                err (1, _("couldn't create new process group"));

            /* Close std descriptors */
            for (i = 0; i <= 2; i++)
                close (i);
            
            /* Open stdin, stdout and stderr. GPGME doesn't work without this */
            open ("/dev/null", O_RDONLY, 0666);
            open ("/dev/null", O_WRONLY, 0666);
            open ("/dev/null", O_WRONLY, 0666);

            chdir ("/tmp");
            return; /* Child process returns */
        };
    }

    /* Not daemonizing */
    else {
        pid = getpid ();
    }

    /* The parent process or not daemonizing ... */
    if (!daemon_no_daemonize)
        exit (0);
}

static void
unix_signal (int signal)
{
    daemon_quit = TRUE;
    if (daemon_running)
        gtk_main_quit ();
}

static void
log_handler (const gchar *log_domain, GLogLevelFlags log_level, 
             const gchar *message, gpointer user_data)
{
    int level;

    /* Note that crit and err are the other way around in syslog */
        
    switch (G_LOG_LEVEL_MASK & log_level) {
    case G_LOG_LEVEL_ERROR:
        level = LOG_CRIT;
        break;
    case G_LOG_LEVEL_CRITICAL:
        level = LOG_ERR;
        break;
    case G_LOG_LEVEL_WARNING:
        level = LOG_WARNING;
        break;
    case G_LOG_LEVEL_MESSAGE:
        level = LOG_NOTICE;
        break;
    case G_LOG_LEVEL_INFO:
        level = LOG_INFO;
        break;
    case G_LOG_LEVEL_DEBUG:
        level = LOG_DEBUG;
        break;
    default:
        level = LOG_ERR;
        break;
    }
    
    /* Log to syslog first */
    if (log_domain)
        syslog (level, "%s: %s", log_domain, message);
    else
        syslog (level, "%s", message);
 
    /* And then to default handler for aborting and stuff like that */
    g_log_default_handler (log_domain, log_level, message, user_data); 
}

static void
prepare_logging ()
{
    GLogLevelFlags flags = G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR | 
                G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | 
                G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO;
                
    openlog ("seahorse-daemon", LOG_PID, LOG_AUTH);
    
    g_log_set_handler (NULL, flags, log_handler, NULL);
    g_log_set_handler ("Glib", flags, log_handler, NULL);
    g_log_set_handler ("Gtk", flags, log_handler, NULL);
    g_log_set_handler ("Gnome", flags, log_handler, NULL);
}

static void
client_die ()
{
    gtk_main_quit ();
}

int main(int argc, char* argv[])
{
    SeahorseOperation *op;
    GnomeClient *client = NULL;
    GOptionContext *octx = NULL;

    seahorse_secure_memory_init ();
    
    octx = g_option_context_new ("");
    g_option_context_add_main_entries (octx, options, GETTEXT_PACKAGE);

    gnome_program_init ("seahorse-daemon", VERSION, LIBGNOMEUI_MODULE, argc, argv,
                    GNOME_PARAM_GOPTION_CONTEXT, octx,
                    GNOME_PARAM_HUMAN_READABLE_NAME, _("Encryption Daemon (Seahorse)"),
                    GNOME_PARAM_APP_DATADIR, DATA_DIR, NULL);

    /* 
     * All functions after this point have to print messages 
     * nicely and not just called exit() 
     */
    daemonize ();

    /* Handle some signals */
    seahorse_unix_signal_register (SIGINT, unix_signal);
    seahorse_unix_signal_register (SIGTERM, unix_signal);

    /* Force gconf to reconnect after daemonizing */
    if (!daemon_no_daemonize)
        seahorse_gconf_disconnect ();    
        
    client = gnome_master_client();
    g_signal_connect(client, "die", G_CALLBACK(client_die), NULL);
    
    /* We log to the syslog */
    prepare_logging ();

    /* Insert Icons into Stock */
    seahorse_gtkstock_init ();
    seahorse_gtkstock_add_icons (daemon_icons);
    
    /* Make the default SeahorseContext */
    seahorse_context_new (SEAHORSE_CONTEXT_APP | SEAHORSE_CONTEXT_DAEMON, 0);
    op = seahorse_context_load_local_keys (SCTX_APP ());
    g_object_unref (op);
    
    /* Initialize the various daemon components */
    seahorse_dbus_server_init ();

#ifdef WITH_SHARING
    seahorse_sharing_init ();
#endif

    /* Sometimes we've already gotten a quit signal */
    if(!daemon_quit) {
        daemon_running = TRUE;
        gtk_main ();
        g_message ("left gtk_main\n");
    }

    /* And now clean them all up */
#ifdef WITH_SHARING
    seahorse_sharing_cleanup ();
#endif
    
    seahorse_dbus_server_cleanup ();

    seahorse_context_destroy (SCTX_APP ());

    return 0;
}