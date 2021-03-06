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

#include "seahorse-pkcs11-certificate.h"
#include "seahorse-pkcs11-helpers.h"

#include "seahorse-object.h"
#include "seahorse-pkcs11.h"
#include "seahorse-util.h"
#include "seahorse-validity.h"

#include <gcr/gcr-certificate.h>

#include <pkcs11.h>
#include <pkcs11g.h>

#include <glib/gi18n-lib.h>

static const gulong REQUIRED_ATTRS[] = {
	CKA_VALUE,
	CKA_ID,
	CKA_TRUSTED,
	CKA_END_DATE
};

enum {
	PROP_0,
	PROP_FINGERPRINT,
	PROP_VALIDITY,
	PROP_VALIDITY_STR,
	PROP_TRUST,
	PROP_TRUST_STR,
	PROP_EXPIRES,
	PROP_EXPIRES_STR
};

struct _SeahorsePkcs11CertificatePrivate {
	GP11Attribute der_value;
};

static void seahorse_pkcs11_certificate_iface (GcrCertificateIface *iface);

G_DEFINE_TYPE_WITH_CODE (SeahorsePkcs11Certificate, seahorse_pkcs11_certificate, SEAHORSE_PKCS11_TYPE_OBJECT, 
                         G_IMPLEMENT_INTERFACE (GCR_TYPE_CERTIFICATE, seahorse_pkcs11_certificate_iface));

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static gchar* 
calc_display_id (SeahorsePkcs11Certificate* self) 
{
	gsize len;
	gchar *id, *ret;
	
	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), NULL);
	
	id = seahorse_pkcs11_certificate_get_fingerprint (self);
	g_return_val_if_fail (id, NULL);
	
	len = strlen (id);
	if (len <= 8)
		return id;

	ret = g_strndup (id + (len - 8), 8);
	g_free (id);
	return ret;
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
seahorse_pkcs11_certificate_realize (SeahorseObject *obj)
{
	SeahorsePkcs11Certificate *self = SEAHORSE_PKCS11_CERTIFICATE (obj);
	gchar *identifier = NULL;
	guint flags;

	SEAHORSE_OBJECT_CLASS (seahorse_pkcs11_certificate_parent_class)->realize (obj);

	seahorse_pkcs11_object_require_attributes (SEAHORSE_PKCS11_OBJECT (self), 
	                                           REQUIRED_ATTRS, G_N_ELEMENTS (REQUIRED_ATTRS));
	
	if (!seahorse_object_get_label (obj))
		g_object_set (self, "label", _("Certificate"), NULL);
	
	flags = seahorse_object_get_flags (obj);

	/* TODO: Expiry, revoked, disabled etc... */
	if (seahorse_pkcs11_certificate_get_trust (self) >= SEAHORSE_VALIDITY_MARGINAL)
		flags |= SEAHORSE_FLAG_TRUSTED;

	identifier = calc_display_id (self);

	g_object_set (self,
		      "location", SEAHORSE_LOCATION_LOCAL,
		      "usage", SEAHORSE_USAGE_PUBLIC_KEY,
		      "flags", flags,
		      "identifier", identifier,
		      NULL);

	g_free (identifier);
}

static GObject* 
seahorse_pkcs11_certificate_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	SeahorsePkcs11Certificate *self = SEAHORSE_PKCS11_CERTIFICATE (G_OBJECT_CLASS (seahorse_pkcs11_certificate_parent_class)->constructor(type, n_props, props));
	
	g_return_val_if_fail (self, NULL);	

	return G_OBJECT (self);
}

static void
seahorse_pkcs11_certificate_init (SeahorsePkcs11Certificate *self)
{
	self->pv = (G_TYPE_INSTANCE_GET_PRIVATE (self, SEAHORSE_PKCS11_TYPE_CERTIFICATE, SeahorsePkcs11CertificatePrivate));
	gp11_attribute_init_invalid (&self->pv->der_value, CKA_VALUE);
}

static void
seahorse_pkcs11_certificate_notify (GObject *obj, GParamSpec *pspec)
{
	/* When the base class pkcs11-attributes changes, it can affects lots */
	if (g_str_equal (pspec->name, "pkcs11-attributes")) {
		g_object_notify (obj, "fingerprint");
		g_object_notify (obj, "validity");
		g_object_notify (obj, "validity-str");
		g_object_notify (obj, "trust");
		g_object_notify (obj, "trust-str");
		g_object_notify (obj, "expires");
		g_object_notify (obj, "expires-str");
	}
	
	if (G_OBJECT_CLASS (seahorse_pkcs11_certificate_parent_class)->notify)
		G_OBJECT_CLASS (seahorse_pkcs11_certificate_parent_class)->notify (obj, pspec);
}


static void
seahorse_pkcs11_certificate_finalize (GObject *obj)
{
	SeahorsePkcs11Certificate *self = SEAHORSE_PKCS11_CERTIFICATE (obj);

	gp11_attribute_clear (&self->pv->der_value);
	
	G_OBJECT_CLASS (seahorse_pkcs11_certificate_parent_class)->finalize (obj);
}

static void
seahorse_pkcs11_certificate_get_property (GObject *obj, guint prop_id, GValue *value, 
                                          GParamSpec *pspec)
{
	SeahorsePkcs11Certificate *self = SEAHORSE_PKCS11_CERTIFICATE (obj);
	
	switch (prop_id) {
	case PROP_FINGERPRINT:
		g_value_take_string (value, seahorse_pkcs11_certificate_get_fingerprint (self));
		break;
	case PROP_VALIDITY:
		g_value_set_uint (value, seahorse_pkcs11_certificate_get_validity (self));
		break;
	case PROP_VALIDITY_STR:
		g_value_set_string (value, seahorse_pkcs11_certificate_get_validity_str (self));
		break;
	case PROP_TRUST:
		g_value_set_uint (value, seahorse_pkcs11_certificate_get_trust (self));
		break;
	case PROP_TRUST_STR:
		g_value_set_string (value, seahorse_pkcs11_certificate_get_trust_str (self));
		break;
	case PROP_EXPIRES:
		g_value_set_ulong (value, seahorse_pkcs11_certificate_get_expires (self));
		break;
	case PROP_EXPIRES_STR:
		g_value_take_string (value, seahorse_pkcs11_certificate_get_expires_str (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
seahorse_pkcs11_certificate_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                          GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
seahorse_pkcs11_certificate_class_init (SeahorsePkcs11CertificateClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	SeahorseObjectClass *seahorse_class = SEAHORSE_OBJECT_CLASS (klass);
	
	seahorse_pkcs11_certificate_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SeahorsePkcs11CertificatePrivate));

	gobject_class->constructor = seahorse_pkcs11_certificate_constructor;
	gobject_class->finalize = seahorse_pkcs11_certificate_finalize;
	gobject_class->set_property = seahorse_pkcs11_certificate_set_property;
	gobject_class->get_property = seahorse_pkcs11_certificate_get_property;
	gobject_class->notify = seahorse_pkcs11_certificate_notify;

	seahorse_class->realize = seahorse_pkcs11_certificate_realize;
	
	g_object_class_install_property (gobject_class, PROP_FINGERPRINT, 
	         g_param_spec_string ("fingerprint", "fingerprint", "fingerprint", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_VALIDITY, 
	         g_param_spec_uint ("validity", "validity", "validity", 0, G_MAXUINT, 0U, 
	                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_VALIDITY_STR, 
	         g_param_spec_string ("validity-str", "validity-str", "validity-str", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_TRUST, 
	         g_param_spec_uint ("trust", "trust", "trust", 0, G_MAXUINT, 0U, 
	                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_TRUST_STR, 
	         g_param_spec_string ("trust-str", "trust-str", "trust-str", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_EXPIRES, 
	         g_param_spec_ulong ("expires", "expires", "expires", 0, G_MAXULONG, 0UL, 
	                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_EXPIRES_STR, 
	         g_param_spec_string ("expires-str", "expires-str", "expires-str", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
}

const guchar*
seahorse_pkcs11_certificate_get_der_data (GcrCertificate *cert, gsize *n_length)
{
	SeahorsePkcs11Object *obj = SEAHORSE_PKCS11_OBJECT (cert);
	SeahorsePkcs11Certificate *self = SEAHORSE_PKCS11_CERTIFICATE (cert);
	GP11Attributes *attrs;
	GP11Attribute *attr;
	
	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (cert), NULL);
	
	if (!seahorse_pkcs11_object_require_attributes (obj, REQUIRED_ATTRS, 
	                                                G_N_ELEMENTS (REQUIRED_ATTRS)))
		return NULL;
	
	/* TODO: This whole copying and possibly freeing memory in use is risky */

	if (gp11_attribute_is_invalid (&self->pv->der_value)) {
		attrs = seahorse_pkcs11_object_get_pkcs11_attributes (obj);
		g_return_val_if_fail (attrs, NULL);
		attr = gp11_attributes_find (attrs, CKA_VALUE);
		g_return_val_if_fail (attr, NULL);
		gp11_attribute_clear (&self->pv->der_value);
		gp11_attribute_init_copy (&self->pv->der_value, attr);
	}
	
	g_return_val_if_fail (!gp11_attribute_is_invalid (&self->pv->der_value), NULL);
	*n_length = self->pv->der_value.length;
	return self->pv->der_value.value;
}

static void 
seahorse_pkcs11_certificate_iface (GcrCertificateIface *iface)
{
	iface->get_der_data = (gpointer)seahorse_pkcs11_certificate_get_der_data;
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

SeahorsePkcs11Certificate*
seahorse_pkcs11_certificate_new (GP11Object* object)
{
	return g_object_new (SEAHORSE_PKCS11_TYPE_CERTIFICATE, 
	                     "pkcs11-object", object, NULL);
}

gchar* 
seahorse_pkcs11_certificate_get_fingerprint (SeahorsePkcs11Certificate* self) 
{
	GP11Attribute* attr;
	
	/* TODO: We should be using the fingerprint off the DER */
	attr = seahorse_pkcs11_object_require_attribute (SEAHORSE_PKCS11_OBJECT (self), CKA_ID);
	if (attr == NULL)
		return g_strdup ("");

	return seahorse_util_hex_encode (attr->value, attr->length);
}

guint 
seahorse_pkcs11_certificate_get_validity (SeahorsePkcs11Certificate* self) 
{
	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), 0U);
	
	/* TODO: We need to implement proper validity checking */
	return SEAHORSE_VALIDITY_UNKNOWN;
}

const char* 
seahorse_pkcs11_certificate_get_validity_str (SeahorsePkcs11Certificate* self) 
{
	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), NULL);
	return seahorse_validity_get_string (seahorse_pkcs11_certificate_get_validity (self));
}

guint 
seahorse_pkcs11_certificate_get_trust (SeahorsePkcs11Certificate* self) 
{
	GP11Attribute *attr;

	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), 0);

	attr = seahorse_pkcs11_object_require_attribute (SEAHORSE_PKCS11_OBJECT (self), CKA_TRUSTED);
	if (attr && gp11_attribute_get_boolean (attr))
		return SEAHORSE_VALIDITY_FULL;

	return SEAHORSE_VALIDITY_UNKNOWN;
}

const char* 
seahorse_pkcs11_certificate_get_trust_str (SeahorsePkcs11Certificate* self) 
{
	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), NULL);
	return seahorse_validity_get_string (seahorse_pkcs11_certificate_get_trust (self));
}


gulong
seahorse_pkcs11_certificate_get_expires (SeahorsePkcs11Certificate* self) 
{
	GP11Attribute *attr;
	GDate date = { 0 };
	struct tm time = { 0 };

	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), 0);
	
	attr = seahorse_pkcs11_object_require_attribute (SEAHORSE_PKCS11_OBJECT (self), CKA_END_DATE);
	if (attr == NULL)
		return 0;
	
	gp11_attribute_get_date (attr, &date);
	g_date_to_struct_tm (&date, &time);
	return (gulong)(mktime (&time));
}

char* 
seahorse_pkcs11_certificate_get_expires_str (SeahorsePkcs11Certificate* self) 
{
	gulong expiry;
	
	g_return_val_if_fail (SEAHORSE_PKCS11_IS_CERTIFICATE (self), NULL);
	
	/* TODO: When expired return Expired */
	expiry = seahorse_pkcs11_certificate_get_expires (self);
	if (expiry == 0)
		return g_strdup ("");
	return seahorse_util_get_date_string (expiry);
}

