/* 
 * Seahorse
 * 
 * Copyright (C) 2008 Stefan Walter
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *  
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include "config.h"

#include <glib/gi18n.h>

#include "seahorse-gpgme-dialogs.h"
#include "seahorse-gpgme-key.h"
#include "seahorse-gpgme-uid.h"
#include "seahorse-pgp.h"
#include "seahorse-pgp-commands.h"
#include "seahorse-pgp-dialogs.h"

#include "common/seahorse-registry.h"

#include "seahorse-object.h"
#include "seahorse-util.h"

enum {
	PROP_0
};

struct _SeahorsePgpCommandsPrivate {
	GtkActionGroup* command_actions;
};

G_DEFINE_TYPE (SeahorsePgpCommands, seahorse_pgp_commands, SEAHORSE_TYPE_COMMANDS);

static const char* UI_DEFINITION = ""\
"<ui>"\
"	<menubar>"\
"		<menu name='File' action='file-menu'> "\
"			<placeholder name='FileCommands'>"\
"				<menuitem action='key-sign'/>"\
"			</placeholder>"\
"		</menu>"\
"	</menubar>"\
"	<toolbar name='MainToolbar'>"\
"		<placeholder name='ToolItems'>"\
"			<toolitem action='key-sign'/>"\
"		</placeholder>"\
"	</toolbar>"\
"	<popup name='KeyPopup'>"\
"		<menuitem action='key-sign'/>"\
"	</popup>"\
"</ui>";

static SeahorseObjectPredicate actions_predicate = { 0 };
static SeahorseObjectPredicate commands_predicate = { 0 };

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static void 
on_key_sign (GtkAction* action, SeahorsePgpCommands* self) 
{
	SeahorseView *view;
	GtkWindow *window;
	GList *keys;

	g_return_if_fail (SEAHORSE_IS_PGP_COMMANDS (self));
	g_return_if_fail (GTK_IS_ACTION (action));
	
	view = seahorse_commands_get_view (SEAHORSE_COMMANDS (self));
	keys = seahorse_view_get_selected_matching (view, &actions_predicate);
	
	if (keys == NULL)
		return;

	/* Indicate what we're actually going to operate on */
	seahorse_view_set_selected (view, keys->data);

	window = seahorse_commands_get_window (SEAHORSE_COMMANDS (self));
	
	if (G_TYPE_FROM_INSTANCE (keys->data) == SEAHORSE_TYPE_GPGME_KEY) {
		seahorse_gpgme_sign_prompt (SEAHORSE_GPGME_KEY (keys->data), window);
	} else if (G_TYPE_FROM_INSTANCE (keys->data) == SEAHORSE_TYPE_GPGME_UID) {
		seahorse_gpgme_sign_prompt_uid (SEAHORSE_GPGME_UID (keys->data), window);
	}
}

static const GtkActionEntry COMMAND_ENTRIES[] = {
	{ "key-sign", GTK_STOCK_INDEX, N_("_Sign Key..."), "", N_("Sign public key"), G_CALLBACK (on_key_sign) }
};


/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
seahorse_pgp_commands_show_properties (SeahorseCommands* base, SeahorseObject* obj) 
{
	g_return_if_fail (SEAHORSE_IS_OBJECT (obj));
	
	if (G_TYPE_FROM_INSTANCE (G_OBJECT (obj)) == SEAHORSE_TYPE_PGP_UID || 
	    G_TYPE_FROM_INSTANCE (G_OBJECT (obj)) == SEAHORSE_TYPE_GPGME_UID)
		obj = seahorse_object_get_parent (obj);

	g_return_if_fail (G_TYPE_FROM_INSTANCE (G_OBJECT (obj)) == SEAHORSE_TYPE_PGP_KEY || 
	                  G_TYPE_FROM_INSTANCE (G_OBJECT (obj)) == SEAHORSE_TYPE_GPGME_KEY);
	seahorse_pgp_key_properties_show (SEAHORSE_PGP_KEY (obj), seahorse_commands_get_window (base));
}

static SeahorseOperation*
seahorse_pgp_commands_delete_objects (SeahorseCommands* base, GList* objects) 
{
	guint num;
	gint num_keys;
	gint num_identities;
	char* message;
	SeahorseObject *obj;
	GList* to_delete, *l;
	SeahorseOperation *op;
	guint length;

	num = g_list_length (objects);
	if (num == 0)
		return NULL;

	num_keys = 0;
	num_identities = 0;
	message = NULL;
	
	/* 
	 * Go through and validate all what we have to delete, 
	 * removing UIDs where the parent Key is also on the 
	 * chopping block.
	 */
	to_delete = NULL;
	
	for (l = objects; l; l = g_list_next (l)) {
		obj = SEAHORSE_OBJECT (l->data);
		if (G_OBJECT_TYPE (obj) == SEAHORSE_TYPE_GPGME_UID) {
			if (g_list_find (objects, seahorse_object_get_parent (obj)) == NULL) {
				to_delete = g_list_prepend (to_delete, obj);
				++num_identities;
			}
		} else if (G_OBJECT_TYPE (obj) == SEAHORSE_TYPE_GPGME_KEY) {
			to_delete = g_list_prepend (to_delete, obj);
			++num_keys;
		}
	}
	
	/* Figure out a good prompt message */
	length = g_list_length (to_delete);
	switch (length) {
	case 0:
		return NULL;
	case 1:
		message = g_strdup_printf (_ ("Are you sure you want to permanently delete %s?"), 
		                           seahorse_object_get_label (to_delete->data));
		break;
	default:
		if (num_keys > 0 && num_identities > 0) {
			message = g_strdup_printf (_("Are you sure you want to permanently delete %d keys and identities?"), length);
		} else if (num_keys > 0) {
			message = g_strdup_printf (_("Are you sure you want to permanently delete %d keys?"), length);
		} else if (num_identities > 0){
			message = g_strdup_printf (_("Are you sure you want to permanently delete %d identities?"), length);
		} else {
			g_assert_not_reached ();
		}
		break;
	}
	
	if (!seahorse_util_prompt_delete (message, GTK_WIDGET (seahorse_commands_get_window (base)))) {
		g_free (message);
		return NULL;

	}
	
	op = seahorse_source_delete_objects (to_delete);

	g_free (message);
	g_list_free (to_delete);
	
	return op;
}

static GObject* 
seahorse_pgp_commands_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (seahorse_pgp_commands_parent_class)->constructor (type, n_props, props);
	SeahorsePgpCommands *self = NULL;
	SeahorseCommands *base;
	SeahorseView *view;
	
	if (obj) {
		self = SEAHORSE_PGP_COMMANDS (obj);
		base = SEAHORSE_COMMANDS (self);
	
		view = seahorse_commands_get_view (base);
		g_return_val_if_fail (view, NULL);
		
		self->pv->command_actions = gtk_action_group_new ("pgp");
		gtk_action_group_set_translation_domain (self->pv->command_actions, GETTEXT_PACKAGE);
		gtk_action_group_add_actions (self->pv->command_actions, COMMAND_ENTRIES, 
		                              G_N_ELEMENTS (COMMAND_ENTRIES), self);
		
		seahorse_view_register_commands (view, &commands_predicate, base);
		seahorse_view_register_ui (view, &actions_predicate, UI_DEFINITION, self->pv->command_actions);
	}
	
	return obj;
}

static void
seahorse_pgp_commands_init (SeahorsePgpCommands *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, SEAHORSE_TYPE_PGP_COMMANDS, SeahorsePgpCommandsPrivate);

}

static void
seahorse_pgp_commands_dispose (GObject *obj)
{
	SeahorsePgpCommands *self = SEAHORSE_PGP_COMMANDS (obj);
    
	if (self->pv->command_actions)
		g_object_unref (self->pv->command_actions);
	self->pv->command_actions = NULL;
	
	G_OBJECT_CLASS (seahorse_pgp_commands_parent_class)->dispose (obj);
}

static void
seahorse_pgp_commands_finalize (GObject *obj)
{
	SeahorsePgpCommands *self = SEAHORSE_PGP_COMMANDS (obj);

	g_assert (!self->pv->command_actions);
	
	G_OBJECT_CLASS (seahorse_pgp_commands_parent_class)->finalize (obj);
}

static void
seahorse_pgp_commands_set_property (GObject *obj, guint prop_id, const GValue *value, 
                           GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
seahorse_pgp_commands_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
seahorse_pgp_commands_class_init (SeahorsePgpCommandsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	seahorse_pgp_commands_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SeahorsePgpCommandsPrivate));

	gobject_class->constructor = seahorse_pgp_commands_constructor;
	gobject_class->dispose = seahorse_pgp_commands_dispose;
	gobject_class->finalize = seahorse_pgp_commands_finalize;
	gobject_class->set_property = seahorse_pgp_commands_set_property;
	gobject_class->get_property = seahorse_pgp_commands_get_property;
    
	SEAHORSE_COMMANDS_CLASS (klass)->show_properties = seahorse_pgp_commands_show_properties;
	SEAHORSE_COMMANDS_CLASS (klass)->delete_objects = seahorse_pgp_commands_delete_objects;
	
	/* Setup predicate for matching entries for these commands */
	commands_predicate.tag = SEAHORSE_PGP_TYPE;
	actions_predicate.tag = SEAHORSE_PGP_TYPE;
	actions_predicate.location = SEAHORSE_LOCATION_LOCAL;

	/* Register this class as a commands */
	seahorse_registry_register_type (seahorse_registry_get (), SEAHORSE_TYPE_PGP_COMMANDS, 
	                                 SEAHORSE_PGP_TYPE_STR, "commands", NULL, NULL);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

SeahorsePgpCommands*
seahorse_pgp_commands_new (void)
{
	return g_object_new (SEAHORSE_TYPE_PGP_COMMANDS, NULL);
}
