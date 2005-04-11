/***********************************************************************
 ***        THIS FILE IS GENERATED AUTOMATICALLY. DON'T EDIT!        ***
 ***********************************************************************/
/*
 * XML DRI client-side driver configuration
 * Copyright (C) 2003 Felix Kuehling
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * FELIX KUEHLING, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */
/**
 * \file t_options.h
 * \brief Templates of common options
 * \author Felix Kuehling
 *
 * This file defines macros for common options that can be used to
 * construct driConfigOptions in the drivers. This file is only a
 * template containing English descriptions for options wrapped in
 * gettext(). xgettext can be used to extract translatable
 * strings. These strings can then be translated by anyone familiar
 * with GNU gettext. gen_xmlpool.py takes this template and fills in
 * all the translations. The result (options.h) is included by
 * xmlpool.h which in turn can be included by drivers.
 *
 * The macros used to describe otions in this file are defined in
 * ../xmlpool.h.
 */

/* This is needed for xgettext to extract translatable strings.
 * gen_xmlpool.py will discard this line. */
/* #include <libintl.h>
 * commented out by gen_xmlpool.py */

/*
 * predefined option sections and options with multi-lingual descriptions
 */

/** \brief Debugging options */
#define DRI_CONF_SECTION_DEBUG \
DRI_CONF_SECTION_BEGIN \
	DRI_CONF_DESC(en,"Debugging") \
	DRI_CONF_DESC(de,"Fehlersuche")

#define DRI_CONF_NO_RAST(def) \
DRI_CONF_OPT_BEGIN(no_rast,bool,def) \
        DRI_CONF_DESC(en,"Disable 3D acceleration") \
        DRI_CONF_DESC(de,"3D-Beschleunigung abschalten") \
DRI_CONF_OPT_END

#define DRI_CONF_PERFORMANCE_BOXES(def) \
DRI_CONF_OPT_BEGIN(performance_boxes,bool,def) \
        DRI_CONF_DESC(en,"Show performance boxes") \
        DRI_CONF_DESC(de,"Zeige Performanceboxen") \
DRI_CONF_OPT_END


/** \brief Texture-related options */
#define DRI_CONF_SECTION_QUALITY \
DRI_CONF_SECTION_BEGIN \
	DRI_CONF_DESC(en,"Image Quality") \
	DRI_CONF_DESC(de,"Bildqualität")

#define DRI_CONF_TEXTURE_DEPTH_FB       0
#define DRI_CONF_TEXTURE_DEPTH_32       1
#define DRI_CONF_TEXTURE_DEPTH_16       2
#define DRI_CONF_TEXTURE_DEPTH_FORCE_16 3
#define DRI_CONF_TEXTURE_DEPTH(def) \
DRI_CONF_OPT_BEGIN_V(texture_depth,enum,def,"0:3") \
	DRI_CONF_DESC_BEGIN(en,"Texture color depth") \
                DRI_CONF_ENUM(0,"Prefer frame buffer color depth") \
                DRI_CONF_ENUM(1,"Prefer 32 bits per texel") \
                DRI_CONF_ENUM(2,"Prefer 16 bits per texel") \
                DRI_CONF_ENUM(3,"Force 16 bits per texel") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Texturfarbtiefe") \
                DRI_CONF_ENUM(0,"Bevorzuge Farbtiefe des Framebuffers") \
                DRI_CONF_ENUM(1,"Bevorzuge 32 bits pro Texel") \
                DRI_CONF_ENUM(2,"Bevorzuge 16 bits pro Texel") \
                DRI_CONF_ENUM(3,"Erzwinge 16 bits pro Texel") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_DEF_MAX_ANISOTROPY(def,range) \
DRI_CONF_OPT_BEGIN_V(def_max_anisotropy,float,def,range) \
        DRI_CONF_DESC(en,"Initial maximum value for anisotropic texture filtering") \
        DRI_CONF_DESC(de,"Initialer Maximalwert für anisotropische Texturfilterung") \
DRI_CONF_OPT_END

#define DRI_CONF_NO_NEG_LOD_BIAS(def) \
DRI_CONF_OPT_BEGIN(no_neg_lod_bias,bool,def) \
        DRI_CONF_DESC(en,"Forbid negative texture LOD bias") \
        DRI_CONF_DESC(de,"Verbiete negative Textur-Detailgradverschiebung") \
DRI_CONF_OPT_END

#define DRI_CONF_FORCE_S3TC_ENABLE(def) \
DRI_CONF_OPT_BEGIN(force_s3tc_enable,bool,def) \
        DRI_CONF_DESC(en,"Enable S3TC texture compression even if software support is not available") \
        DRI_CONF_DESC(de,"Aktiviere S3TC Texturkomprimierung auch wenn die nötige Softwareunterstützung fehlt") \
DRI_CONF_OPT_END

#define DRI_CONF_COLOR_REDUCTION_ROUND 0
#define DRI_CONF_COLOR_REDUCTION_DITHER 1
#define DRI_CONF_COLOR_REDUCTION(def) \
DRI_CONF_OPT_BEGIN_V(color_reduction,enum,def,"0:1") \
        DRI_CONF_DESC_BEGIN(en,"Initial color reduction method") \
                DRI_CONF_ENUM(0,"Round colors") \
                DRI_CONF_ENUM(1,"Dither colors") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Initiale Farbreduktionsmethode") \
                DRI_CONF_ENUM(0,"Farben runden") \
                DRI_CONF_ENUM(1,"Farben rastern") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_ROUND_TRUNC 0
#define DRI_CONF_ROUND_ROUND 1
#define DRI_CONF_ROUND_MODE(def) \
DRI_CONF_OPT_BEGIN_V(round_mode,enum,def,"0:1") \
	DRI_CONF_DESC_BEGIN(en,"Color rounding method") \
                DRI_CONF_ENUM(0,"Round color components downward") \
                DRI_CONF_ENUM(1,"Round to nearest color") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Farbrundungsmethode") \
                DRI_CONF_ENUM(0,"Farbkomponenten abrunden") \
                DRI_CONF_ENUM(1,"Zur ähnlichsten Farbe runden") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_DITHER_XERRORDIFF 0
#define DRI_CONF_DITHER_XERRORDIFFRESET 1
#define DRI_CONF_DITHER_ORDERED 2
#define DRI_CONF_DITHER_MODE(def) \
DRI_CONF_OPT_BEGIN_V(dither_mode,enum,def,"0:2") \
	DRI_CONF_DESC_BEGIN(en,"Color dithering method") \
                DRI_CONF_ENUM(0,"Horizontal error diffusion") \
                DRI_CONF_ENUM(1,"Horizontal error diffusion, reset error at line start") \
                DRI_CONF_ENUM(2,"Ordered 2D color dithering") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Farbrasterungsmethode") \
                DRI_CONF_ENUM(0,"Horizontale Fehlerstreuung") \
                DRI_CONF_ENUM(1,"Horizontale Fehlerstreuung, Fehler am Zeilenanfang zurücksetzen") \
                DRI_CONF_ENUM(2,"Geordnete 2D Farbrasterung") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_FLOAT_DEPTH(def) \
DRI_CONF_OPT_BEGIN(float_depth,bool,def) \
        DRI_CONF_DESC(en,"Floating point depth buffer") \
        DRI_CONF_DESC(de,"Fließkomma z-Puffer") \
DRI_CONF_OPT_END

/** \brief Performance-related options */
#define DRI_CONF_SECTION_PERFORMANCE \
DRI_CONF_SECTION_BEGIN \
        DRI_CONF_DESC(en,"Performance") \
        DRI_CONF_DESC(de,"Leistung")

#define DRI_CONF_TCL_SW 0
#define DRI_CONF_TCL_PIPELINED 1
#define DRI_CONF_TCL_VTXFMT 2
#define DRI_CONF_TCL_CODEGEN 3
#define DRI_CONF_TCL_MODE(def) \
DRI_CONF_OPT_BEGIN_V(tcl_mode,enum,def,"0:3") \
        DRI_CONF_DESC_BEGIN(en,"TCL mode (Transformation, Clipping, Lighting)") \
                DRI_CONF_ENUM(0,"Use software TCL pipeline") \
                DRI_CONF_ENUM(1,"Use hardware TCL as first TCL pipeline stage") \
                DRI_CONF_ENUM(2,"Bypass the TCL pipeline") \
                DRI_CONF_ENUM(3,"Bypass the TCL pipeline with state-based machine code generated on-the-fly") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"TCL-Modus (Transformation, Clipping, Licht)") \
                DRI_CONF_ENUM(0,"Benutze die Software-TCL-Pipeline") \
                DRI_CONF_ENUM(1,"Benutze Hardware TCL als erste Stufe der TCL-Pipeline") \
                DRI_CONF_ENUM(2,"Umgehe die TCL-Pipeline") \
                DRI_CONF_ENUM(3,"Umgehe die TCL-Pipeline mit zur Laufzeit erzeugtem, zustandsbasiertem Maschinencode") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_FTHROTTLE_BUSY 0
#define DRI_CONF_FTHROTTLE_USLEEPS 1
#define DRI_CONF_FTHROTTLE_IRQS 2
#define DRI_CONF_FTHROTTLE_MODE(def) \
DRI_CONF_OPT_BEGIN_V(fthrottle_mode,enum,def,"0:2") \
        DRI_CONF_DESC_BEGIN(en,"Method to limit rendering latency") \
                DRI_CONF_ENUM(0,"Busy waiting for the graphics hardware") \
                DRI_CONF_ENUM(1,"Sleep for brief intervals while waiting for the graphics hardware") \
                DRI_CONF_ENUM(2,"Let the graphics hardware emit a software interrupt and sleep") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Methode zur Begrenzung der Bildverzögerung") \
                DRI_CONF_ENUM(0,"Aktives Warten auf die Grafikhardware") \
                DRI_CONF_ENUM(1,"Kurze Schlafintervalle beim Warten auf die Grafikhardware") \
                DRI_CONF_ENUM(2,"Die Grafikhardware eine Softwareunterbrechnung erzeugen lassen und schlafen") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3
#define DRI_CONF_VBLANK_MODE(def) \
DRI_CONF_OPT_BEGIN_V(vblank_mode,enum,def,"0:3") \
        DRI_CONF_DESC_BEGIN(en,"Synchronization with vertical refresh (swap intervals)") \
                DRI_CONF_ENUM(0,"Never synchronize with vertical refresh, ignore application's choice") \
                DRI_CONF_ENUM(1,"Initial swap interval 0, obey application's choice") \
                DRI_CONF_ENUM(2,"Initial swap interval 1, obey application's choice") \
                DRI_CONF_ENUM(3,"Always synchronize with vertical refresh, application chooses the minimum swap interval") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Synchronisation mit der vertikalen Bildwiederholung") \
                DRI_CONF_ENUM(0,"Niemals mit der Bildwiederholung synchronisieren, Anweisungen der Anwendung ignorieren") \
                DRI_CONF_ENUM(1,"Initiales Bildinterval 0, Anweisungen der Anwendung gehorchen") \
                DRI_CONF_ENUM(2,"Initiales Bildinterval 1, Anweisungen der Anwendung gehorchen") \
                DRI_CONF_ENUM(3,"Immer mit der Bildwiederholung synchronisieren, Anwendung wählt das minmale Bildinterval") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_HYPERZ_DISABLED 0
#define DRI_CONF_HYPERZ_ENABLED 1
#define DRI_CONF_HYPERZ(def) \
DRI_CONF_OPT_BEGIN(hyperz,bool,def) \
        DRI_CONF_DESC(en,"Use HyperZ to boost performance") \
        DRI_CONF_DESC(de,"HyperZ zur Leistungssteigerung verwenden") \
DRI_CONF_OPT_END

#define DRI_CONF_MAX_TEXTURE_UNITS(def,min,max) \
DRI_CONF_OPT_BEGIN_V(texture_units,int,def, # min ":" # max ) \
        DRI_CONF_DESC(en,"Number of texture units used") \
        DRI_CONF_DESC(de,"Anzahl der benutzten Textureinheiten") \
DRI_CONF_OPT_END

#define DRI_CONF_TEXTURE_LEVEL_HACK(def) \
DRI_CONF_OPT_BEGIN(texture_level_hack,bool,def) \
        DRI_CONF_DESC(en,"Enable hack to allow larger textures with texture compression on radeon/r200") \
        DRI_CONF_DESC(de,"Hack aktivieren, der mit Texturkompression auf radeon/r200 größere Texturen erlaubt") \
DRI_CONF_OPT_END

#define DRI_CONF_TEXTURE_BLEND_QUALITY(def,range) \
DRI_CONF_OPT_BEGIN_V(texture_blend_quality,float,def,range) \
	DRI_CONF_DESC(en,"Texture filtering quality vs. speed, AKA “brilinear” texture filtering") \
	DRI_CONF_DESC(de,"Texturfilterqualität versus -geschwindigkeit, auch bekannt als „brilineare“ Texturfilterung") \
DRI_CONF_OPT_END

#define DRI_CONF_TEXTURE_HEAPS_ALL 0
#define DRI_CONF_TEXTURE_HEAPS_CARD 1
#define DRI_CONF_TEXTURE_HEAPS_GART 2
#define DRI_CONF_TEXTURE_HEAPS(def) \
DRI_CONF_OPT_BEGIN_V(texture_heaps,enum,def,"0:2") \
	DRI_CONF_DESC_BEGIN(en,"Used types of texture memory") \
		DRI_CONF_ENUM(0,"All available memory") \
		DRI_CONF_ENUM(1,"Only card memory (if available)") \
		DRI_CONF_ENUM(2,"Only GART (AGP/PCIE) memory (if available)") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Benutzte Arten von Texturspeicher") \
		DRI_CONF_ENUM(0,"Aller verfügbarer Speicher") \
		DRI_CONF_ENUM(1,"Nur Grafikspeicher (falls verfügbar)") \
		DRI_CONF_ENUM(2,"Nur GART-Speicher (AGP/PCIE) (falls verfügbar)") \
	DRI_CONF_DESC_END \
DRI_CONF_OPT_END

/* Options for features that are not done in hardware by the driver (like GL_ARB_vertex_program
   On cards where there is no documentation (r200) or on rasterization-only hardware). */
#define DRI_CONF_SECTION_SOFTWARE \
DRI_CONF_SECTION_BEGIN \
        DRI_CONF_DESC(en,"Features that are not hardware-accelerated") \
        DRI_CONF_DESC(de,"Funktionalität, die nicht hardwarebeschleunigt ist")

#define DRI_CONF_ARB_VERTEX_PROGRAM(def) \
DRI_CONF_OPT_BEGIN(arb_vertex_program,bool,def) \
        DRI_CONF_DESC(en,"Enable extension GL_ARB_vertex_program") \
        DRI_CONF_DESC(de,"Erweiterung GL_ARB_vertex_program aktivieren") \
DRI_CONF_OPT_END

#define DRI_CONF_NV_VERTEX_PROGRAM(def) \
DRI_CONF_OPT_BEGIN(nv_vertex_program,bool,def) \
        DRI_CONF_DESC(en,"Enable extension GL_NV_vertex_program") \
        DRI_CONF_DESC(de,"Erweiterung GL_NV_vertex_program aktivieren") \
DRI_CONF_OPT_END
