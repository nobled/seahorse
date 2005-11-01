/*
 * Seahorse
 *
 * Copyright (C) 2005 Nate Nielsen
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

/**
 * SeahorseSSHKey: Represents a SSH key 
 * 
 * - Derived from SeahorseKey
 * 
 * Properties:
 *   display-name: (gchar*) The display name for the key.
 *   simple-name: (gchar*) Shortened display name for the key (for use in files etc...).
 *   fingerprint: (gchar*) Displayable fingerprint for the key.
 *   validity: (SeahorseValidity) The key validity.
 *   trust: (SeahorseValidity) Trust for the key.
 *   expires: (gulong) Date this key expires or 0.
 */
 
#ifndef __SEAHORSE_SSH_KEY_H__
#define __SEAHORSE_SSH_KEY_H__

#include <gtk/gtk.h>

#include "seahorse-key.h"

/* The various algorithm types */
enum {
    SSH_ALGO_UNK,
    SSH_ALGO_RSA1,
    SSH_ALGO_RSA,
    SSH_ALGO_DSA
};

#define SKEY_SSH                         (g_quark_from_static_string ("openssh"))

#define SEAHORSE_TYPE_SSH_KEY            (seahorse_ssh_key_get_type ())
#define SEAHORSE_SSH_KEY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEAHORSE_TYPE_SSH_KEY, SeahorseSSHKey))
#define SEAHORSE_SSH_KEY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SEAHORSE_TYPE_SSH_KEY, SeahorseSSHKeyClass))
#define SEAHORSE_IS_SSH_KEY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SEAHORSE_TYPE_SSH_KEY))
#define SEAHORSE_IS_SSH_KEY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SEAHORSE_TYPE_SSH_KEY))
#define SEAHORSE_SSH_KEY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SEAHORSE_TYPE_SSH_KEY, SeahorseSSHKeyClass))

typedef struct _SeahorseSSHKey SeahorseSSHKey;
typedef struct _SeahorseSSHKeyData SeahorseSSHKeyData;
typedef struct _SeahorseSSHKeyClass SeahorseSSHKeyClass;
typedef struct _SeahorseSSHKeyPrivate SeahorseSSHKeyPrivate;

struct _SeahorseSSHKey {
    SeahorseKey	parent;

    /*< public >*/
    struct _SeahorseSSHKeyData *keydata;
        
    /*< private >*/
    struct _SeahorseSSHKeyPrivate *priv;
};

struct _SeahorseSSHKeyData {
    gchar *filename;
    gchar *filepub;
    gchar *comment;
    gchar *keyid;
    gchar *fingerprint;
    guint length;
    guint algo;
};

struct _SeahorseSSHKeyClass {
    SeahorseKeyClass            parent_class;
};

SeahorseSSHKey*         seahorse_ssh_key_new                  (SeahorseKeySource *sksrc, 
                                                               SeahorseSSHKeyData *data);

GType                   seahorse_ssh_key_get_type             (void);

const gchar*            seahorse_ssh_key_get_algo             (SeahorseSSHKey *skey);

guint                   seahorse_ssh_key_get_strength         (SeahorseSSHKey *skey);

const gchar*            seahorse_ssh_key_get_filename         (SeahorseSSHKey *skey,
                                                               gboolean private);

SeahorseSSHKeyData*     seahorse_ssh_key_data_read            (const gchar *filename);

gboolean                seahorse_ssh_key_data_is_valid        (SeahorseSSHKeyData *data);

void                    seahorse_ssh_key_data_free            (SeahorseSSHKeyData *data);

#endif /* __SEAHORSE_KEY_H__ */