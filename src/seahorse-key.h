/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
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

#ifndef __SEAHORSE_KEY_H__
#define __SEAHORSE_KEY_H__

#include <gtk/gtk.h>
#include <gpgme.h>

#include "seahorse-validity.h"

#define SEAHORSE_TYPE_KEY		(seahorse_key_get_type ())
#define SEAHORSE_KEY(obj)		(GTK_CHECK_CAST ((obj), SEAHORSE_TYPE_KEY, SeahorseKey))
#define SEAHORSE_KEY_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_KEY, SeahorseKeyClass))
#define SEAHORSE_IS_KEY(obj)		(GTK_CHECK_TYPE ((obj), SEAHORSE_TYPE_KEY))
#define SEAHORSE_IS_KEY_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_KEY))
#define SEAHORSE_KEY_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), SEAHORSE_TYPE_KEY, SeahorseKeyClass))

/* Possible key changes */
typedef enum {
	/* Owner trust has changed */
	SKEY_CHANGE_TRUST,
	/* Expiration date has changed */
	SKEY_CHANGE_EXPIRE,
	/* Disabled state has changed */
	SKEY_CHANGE_DISABLE,
	/* Passphrase has changed */
	SKEY_CHANGE_PASS,
	/* User IDs have changed */
	SKEY_CHANGE_UIDS,
	/* Subkeys have changed */
	SKEY_CHANGE_SUBKEYS,
	/* Key signatures have changed */
	SKEY_CHANGE_SIGN,
	/* Revoker has changed */
	SKEY_CHANGE_REVOKER
} SeahorseKeyChange;

typedef struct _SeahorseKey SeahorseKey;
typedef struct _SeahorseKeyClass SeahorseKeyClass;
	
struct _SeahorseKey
{
	GtkObject		parent;
	
	/*< public >*/
	GpgmeKey		key;
};

struct _SeahorseKeyClass
{
	GtkObjectClass		parent_class;
	
	/* Signal emitted when one of the key's attributes has changed */
	void 			(* changed)		(SeahorseKey		*skey,
							 SeahorseKeyChange	change);
};

SeahorseKey*		seahorse_key_new		(GpgmeKey		key);

void			seahorse_key_destroy		(SeahorseKey		*skey);

void			seahorse_key_changed		(SeahorseKey		*skey,
							 SeahorseKeyChange	change);

const gint		seahorse_key_get_num_uids	(const SeahorseKey	*skey);

const gint		seahorse_key_get_num_subkeys	(const SeahorseKey	*skey);

const gchar*		seahorse_key_get_keyid		(const SeahorseKey	*skey,
							 const guint		index);

const gchar*		seahorse_key_get_userid		(const SeahorseKey	*skey,
							 const guint		index);

gboolean		seahorse_key_is_valid		(const SeahorseKey	*skey);

gboolean		seahorse_key_can_encrypt	(const SeahorseKey	*skey);

const SeahorseValidity	seahorse_key_get_validity	(const SeahorseKey	*skey);

const SeahorseValidity	seahorse_key_get_trust		(const SeahorseKey	*skey);

#endif /* __SEAHORSE_KEY_H__ */
