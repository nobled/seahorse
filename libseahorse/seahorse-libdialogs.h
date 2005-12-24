/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004-2005 Nate Nielsen
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
 * Various UI elements and dialogs used in libseahorse.
 */
 
#ifndef __SEAHORSE_LIBDIALOGS_H__
#define __SEAHORSE_LIBDIALOGS_H__

#include <glib.h>
#include <gpgme.h>

#include "seahorse-context.h"
#include "seahorse-widget.h"
#include "seahorse-pgp-key.h"

#define SEAHORSE_PASS_BAD    0x00000001
#define SEAHORSE_PASS_NEW    0x01000000

GtkDialog*      seahorse_passphrase_prompt_show     (const gchar *title, 
                                                     const gchar *description, 
                                                     const gchar *prompt, 
                                                     const gchar *errmsg);
                                                     
const gchar*    seahorse_passphrase_prompt_get      (GtkDialog *dialog);

gpgme_error_t   seahorse_passphrase_get             (gconstpointer dummy,
                                                     const gchar *passphrase_hint, 
                                                     const char* passphrase_info, 
                                                     int prev_bad, int fd);

GList*          seahorse_recipients_get             (SeahorsePGPKey **signkey);

SeahorsePGPKey* seahorse_signer_get                 (void);

void            seahorse_signatures_notify          (const gchar* data, 
                                                     gpgme_verify_result_t status);

void            seahorse_import_notify              (guint keys);

void            seahorse_notification_display       (const gchar *summary, 
                                                     const gchar* body, 
                                                     gboolean urgent, 
                                                     const gchar *icon);
  
gboolean        seahorse_notification_have          (void);
                                               
#endif /* __SEAHORSE_LIBDIALOGS_H__ */
