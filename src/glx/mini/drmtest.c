#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "xf86drm.h"

char *pciBusID = "PCI:1:0:0";
#define DRM_PAGE_SIZE 4096
void *pSAREA;


static int client( void )
{
   int fd, ret, err;
   drmContext clientContext;

   fprintf(stderr, "Opening client drm\n");

   fd = drmOpen(NULL,pciBusID);
   if (fd < 0) {
      fprintf(stderr, "failed to open DRM: %s\n", strerror(-fd));
      return 1;
   }


   fprintf(stderr, "Create server context\n");
   if ((err = drmCreateContext(fd, &clientContext)) != 0) {
      fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
      return 0;
   }


   fprintf(stderr, "DRM_LOCK( %d %p %d )\n", fd, pSAREA, clientContext);
   DRM_LOCK(fd, pSAREA, clientContext, 0);
   fprintf(stderr, "locked\n");
   DRM_UNLOCK(fd, pSAREA, clientContext);
   fprintf(stderr, "DRM_UNLOCK finished\n");


   fprintf(stderr, "Closing client drm: %d\n", fd);
   ret = drmClose(fd);
   fprintf(stderr, "done %d\n", ret);

   return ret;
}

int main( int argc, char *argv[] )
{
   char *drmModuleName = "radeon";
   int drmFD;
   int err;
   int SAREASize;
   drmHandle hSAREA;
   drmContext serverContext;

   /* Note that drmOpen will try to load the kernel module, if needed. */
   drmFD = drmOpen(drmModuleName, NULL );
   if (drmFD < 0) {
      /* failed to open DRM */
      fprintf(stderr, "[drm] drmOpen failed\n");
      return 0;
   }


   if ((err = drmSetBusid(drmFD, pciBusID)) < 0) {
      drmClose(drmFD);
      fprintf(stderr, "[drm] drmSetBusid failed (%d, %s), %s\n",
	      drmFD, pciBusID, strerror(-err));
      return 0;
   }

   
   SAREASize = DRM_PAGE_SIZE;
   
   if (drmAddMap( drmFD,
		  0,
		  SAREASize,
		  DRM_SHM,
		  DRM_CONTAINS_LOCK,
		  &hSAREA) < 0)
   {
      drmClose(drmFD);
      fprintf(stderr, "[drm] drmAddMap failed\n");
      return 0;
   }

   fprintf(stderr, "[drm] added %d byte SAREA at 0x%08lx\n",
	   SAREASize, hSAREA);

   if (drmMap( drmFD,
	       hSAREA,
	       SAREASize,
	       (drmAddressPtr)(&pSAREA)) < 0)
   {
      drmClose(drmFD);
      fprintf(stderr, "[drm] drmMap failed\n");
      return 0;
   }

   memset(pSAREA, 0, SAREASize);
   fprintf(stderr, "[drm] mapped SAREA 0x%08lx to %p, size %d\n",
	   hSAREA, pSAREA, SAREASize);

   fprintf(stderr, "Create server context\n");
   if ((err = drmCreateContext(drmFD, &serverContext)) != 0) {
      fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
      return 0;
   }


   fprintf(stderr, "DRM_LOCK( %d %p %d )\n", drmFD, pSAREA, serverContext);
   DRM_LOCK(drmFD, pSAREA, serverContext, 0);
   fprintf(stderr, "locked\n");
   DRM_UNLOCK(drmFD, pSAREA, serverContext);
   fprintf(stderr, "DRM_UNLOCK finished\n");


   client();


   fprintf(stderr, "DRM_LOCK( %d %p %d )\n", drmFD, pSAREA, serverContext);
   DRM_LOCK(drmFD, pSAREA, serverContext, 0);
   fprintf(stderr, "locked\n");
   DRM_UNLOCK(drmFD, pSAREA, serverContext);
   fprintf(stderr, "DRM_UNLOCK finished\n");


   drmUnmap(pSAREA, SAREASize);
   fprintf(stderr, "[drm] unmapped SAREA 0x%08lx from %p, size %d\n",
	   hSAREA, pSAREA, SAREASize);
   pSAREA = 0;

   fprintf(stderr, "%s: Closing DRM fd\n", __FUNCTION__);
   (void)drmClose(drmFD);

   return 0;
}



