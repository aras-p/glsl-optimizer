#include "xorg_tracker.h"

#include <xf86xv.h>
#include <X11/extensions/Xv.h>
#include <fourcc.h>

/*XXX get these from pipe's texture limits */
#define IMAGE_MAX_WIDTH		2048
#define IMAGE_MAX_HEIGHT	2048

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvBrightness, xvContrast;

#define NUM_TEXTURED_ATTRIBUTES 2
static XF86AttributeRec TexturedAttributes[NUM_TEXTURED_ATTRIBUTES] = {
   {XvSettable | XvGettable, -128, 127, "XV_BRIGHTNESS"},
   {XvSettable | XvGettable, 0, 255, "XV_CONTRAST"}
};

#define NUM_FORMATS 3
static XF86VideoFormatRec Formats[NUM_FORMATS] = {
   {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

static XF86VideoEncodingRec DummyEncoding[1] = {
   {
      0,
      "XV_IMAGE",
      IMAGE_MAX_WIDTH, IMAGE_MAX_HEIGHT,
      {1, 1}
   }
};

#define NUM_IMAGES 2
static XF86ImageRec Images[NUM_IMAGES] = {
   XVIMAGE_UYVY,
   XVIMAGE_YUY2,
};

struct xorg_xv_port_priv {
   RegionRec clip;

   int brightness;
   int contrast;
};


static void
stop_video(ScrnInfoPtr pScrn, pointer data, Bool shutdown)
{
   struct xorg_xv_port_priv *priv = (struct xorg_xv_port_priv *)data;

   REGION_EMPTY(pScrn->pScreen, &priv->clip);
}

static int
set_port_attribute(ScrnInfoPtr pScrn,
                   Atom attribute, INT32 value, pointer data)
{
   struct xorg_xv_port_priv *priv = (struct xorg_xv_port_priv *)data;

   if (attribute == xvBrightness) {
      if ((value < -128) || (value > 127))
         return BadValue;
      priv->brightness = value;
   } else if (attribute == xvContrast) {
      if ((value < 0) || (value > 255))
         return BadValue;
      priv->contrast = value;
   } else
      return BadMatch;

   return Success;
}

static int
get_port_attribute(ScrnInfoPtr pScrn,
                   Atom attribute, INT32 * value, pointer data)
{
   struct xorg_xv_port_priv *priv = (struct xorg_xv_port_priv *)data;

   if (attribute == xvBrightness)
      *value = priv->brightness;
   else if (attribute == xvContrast)
      *value = priv->contrast;
   else
      return BadMatch;

   return Success;
}

static void
query_best_size(ScrnInfoPtr pScrn,
                Bool motion,
                short vid_w, short vid_h,
                short drw_w, short drw_h,
                unsigned int *p_w, unsigned int *p_h, pointer data)
{
   if (vid_w > (drw_w << 1))
      drw_w = vid_w >> 1;
   if (vid_h > (drw_h << 1))
      drw_h = vid_h >> 1;

   *p_w = drw_w;
   *p_h = drw_h;
}

static int
put_image(ScrnInfoPtr pScrn,
          short src_x, short src_y,
          short drw_x, short drw_y,
          short src_w, short src_h,
          short drw_w, short drw_h,
          int id, unsigned char *buf,
          short width, short height,
          Bool sync, RegionPtr clipBoxes, pointer data,
          DrawablePtr pDraw)
{
   return 0;
}

static int
query_image_attributes(ScrnInfoPtr pScrn,
                       int id,
                       unsigned short *w, unsigned short *h,
                       int *pitches, int *offsets)
{
   int size;

   if (*w > IMAGE_MAX_WIDTH)
      *w = IMAGE_MAX_WIDTH;
   if (*h > IMAGE_MAX_HEIGHT)
      *h = IMAGE_MAX_HEIGHT;

   *w = (*w + 1) & ~1;
   if (offsets)
      offsets[0] = 0;

   switch (id) {
   case FOURCC_UYVY:
   case FOURCC_YUY2:
   default:
      size = *w << 1;
      if (pitches)
	 pitches[0] = size;
      size *= *h;
      break;
   }

   return size;
}

static struct xorg_xv_port_priv *
port_priv_create(ScreenPtr pScreen)
{
   /*ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];*/
   /*modesettingPtr ms = modesettingPTR(pScrn);*/
   struct xorg_xv_port_priv *priv = NULL;

   priv = calloc(1, sizeof(struct xorg_xv_port_priv));

   if (!priv)
      return NULL;

   REGION_NULL(pScreen, &priv->clip);

   return priv;
}

static XF86VideoAdaptorPtr
xorg_setup_textured_adapter(ScreenPtr pScreen)
{
   /*ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];*/
   /*modesettingPtr ms = modesettingPTR(pScrn);*/
   XF86VideoAdaptorPtr adapt;
   XF86AttributePtr attrs;
   DevUnion *dev_unions;
   int nports = 16, i;
   int nattributes;

   nattributes = NUM_TEXTURED_ATTRIBUTES;

   adapt = calloc(1, sizeof(XF86VideoAdaptorRec));
   dev_unions = calloc(nports, sizeof(DevUnion));
   attrs = calloc(nattributes, sizeof(XF86AttributeRec));
   if (adapt == NULL || dev_unions == NULL || attrs == NULL) {
      free(adapt);
      free(dev_unions);
      free(attrs);
      return NULL;
   }

   adapt->type = XvWindowMask | XvInputMask | XvImageMask;
   adapt->flags = 0;
   adapt->name = "Gallium3D Textured Video";
   adapt->nEncodings = 1;
   adapt->pEncodings = DummyEncoding;
   adapt->nFormats = NUM_FORMATS;
   adapt->pFormats = Formats;
   adapt->nPorts = 0;
   adapt->pPortPrivates = dev_unions;
   adapt->nAttributes = nattributes;
   adapt->pAttributes = attrs;
   memcpy(attrs, TexturedAttributes, nattributes * sizeof(XF86AttributeRec));
   adapt->nImages = NUM_IMAGES;
   adapt->pImages = Images;
   adapt->PutVideo = NULL;
   adapt->PutStill = NULL;
   adapt->GetVideo = NULL;
   adapt->GetStill = NULL;
   adapt->StopVideo = stop_video;
   adapt->SetPortAttribute = set_port_attribute;
   adapt->GetPortAttribute = get_port_attribute;
   adapt->QueryBestSize = query_best_size;
   adapt->PutImage = put_image;
   adapt->QueryImageAttributes = query_image_attributes;

   for (i = 0; i < nports; i++) {
      struct xorg_xv_port_priv *priv =
         port_priv_create(pScreen);

      adapt->pPortPrivates[i].ptr = (pointer) (priv);
      adapt->nPorts++;
   }

   return adapt;
}

void
xorg_init_video(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   /*modesettingPtr ms = modesettingPTR(pScrn);*/
   XF86VideoAdaptorPtr *adaptors, *new_adaptors = NULL;
   XF86VideoAdaptorPtr textured_adapter;
   int num_adaptors;

   num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
   new_adaptors = malloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr *));
   if (new_adaptors == NULL)
      return;

   memcpy(new_adaptors, adaptors, num_adaptors * sizeof(XF86VideoAdaptorPtr));
   adaptors = new_adaptors;

   /* Add the adaptors supported by our hardware.  First, set up the atoms
    * that will be used by both output adaptors.
    */
   xvBrightness = MAKE_ATOM("XV_BRIGHTNESS");
   xvContrast = MAKE_ATOM("XV_CONTRAST");

   textured_adapter = xorg_setup_textured_adapter(pScreen);

   debug_assert(textured_adapter);

   if (textured_adapter) {
      adaptors[num_adaptors++] = textured_adapter;
   }

   if (num_adaptors) {
      xf86XVScreenInit(pScreen, adaptors, num_adaptors);
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                 "Disabling Xv because no adaptors could be initialized.\n");
   }

   free(adaptors);
}
