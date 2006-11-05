/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "nouveau_context.h"
#include "nouveau_screen.h"
#include "nouveau_object.h"

static nouveauScreenPtr nouveauCreateScreen(__DRIscreenPrivate *sPriv)
{
	nouveauScreenPtr screen;
	NOUVEAUDRIPtr dri_priv=(NOUVEAUDRIPtr)sPriv->pDevPriv;

	screen->card=nouveau_card_lookup(dri_priv->device_id);
}

static GLboolean nouveauInitDriver(__DRIscreenPrivate *sPriv)
{
	sPriv->private = (void *) nouveauCreateScreen( sPriv );
	if ( !sPriv->private ) {
		nouveauDestroyScreen( sPriv );
		return GL_FALSE;
	}

	return GL_TRUE;
}

