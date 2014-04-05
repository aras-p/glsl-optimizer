/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include "dri_util.h"
#include <dlfcn.h>
#include "main/macros.h"

/* We need GNU extensions to dlfcn.h in order to provide backward
 * compatibility for the older DRI driver loader mechanism. (dladdr,
 * Dl_info, and RTLD_DEFAULT)
 */
#if defined(RTLD_DEFAULT) && defined(HAVE_DLADDR)

#define MEGADRIVER_STUB_MAX_EXTENSIONS 10
#define LIB_PATH_SUFFIX "_dri.so"
#define LIB_PATH_SUFFIX_LENGTH (sizeof(LIB_PATH_SUFFIX)-1)

/* This is the table of extensions that the loader will dlsym() for.
 *
 * Initially it is empty for the megadriver stub, but the library
 * constructor may initialize it based on the name of the library that
 * is being loaded.
 */
PUBLIC const __DRIextension *
__driDriverExtensions[MEGADRIVER_STUB_MAX_EXTENSIONS] = {
   NULL
};

/**
 * This is a constructor function for the megadriver dynamic library.
 *
 * When the driver is dlopen'ed, this function will run. It will
 * search for the name of the foo_dri.so file that was opened using
 * the dladdr function.
 *
 * After finding foo's name, it will call __driDriverGetExtensions_foo
 * and use the return to update __driDriverExtensions to enable
 * compatibility with older DRI driver loaders.
 */
__attribute__((constructor)) static void
megadriver_stub_init(void)
{
   Dl_info info;
   char *driver_name;
   size_t name_len;
   char *get_extensions_name;
   const __DRIextension **(*get_extensions)(void);
   const __DRIextension **extensions;
   int i;

   /* Call dladdr on __driDriverExtensions. We are really
    * interested in the returned info.dli_fname so we can
    * figure out the path name of the library being loaded.
    */
   i = dladdr((void*) __driDriverExtensions, &info);
   if (i == 0)
      return;

   /* Search for the last '/' character in the path. */
   driver_name = strrchr(info.dli_fname, '/');
   if (driver_name != NULL) {
      /* Skip '/' character */
      driver_name++;
   } else {
      /* Try using the start of the path */
      driver_name = (char*) info.dli_fname;
   }

   /* Make sure the path ends with _dri.so */
   name_len = strlen(driver_name);
   i = name_len - LIB_PATH_SUFFIX_LENGTH;
   if (i < 0 || strcmp(driver_name + i, LIB_PATH_SUFFIX) != 0)
      return;

   /* Duplicate the string so we can modify it.
    * So far we've been using info.dli_fname.
    */
   driver_name = strdup(driver_name);
   if (!driver_name)
      return;

   /* The path ends with _dri.so. Chop this part of the
    * string off. Then we'll have the driver's final name.
    */
   driver_name[i] = '\0';

   i = asprintf(&get_extensions_name, "%s_%s",
                __DRI_DRIVER_GET_EXTENSIONS, driver_name);
   free(driver_name);
   if (i == -1)
      return;

   /* dlsym to get the driver's get extensions function. We
    * don't have the dlopen handle, so we have to use
    * RTLD_DEFAULT. It seems unlikely that the symbol will
    * be found in another library, but this isn't optimal.
    */
   get_extensions = dlsym(RTLD_DEFAULT, get_extensions_name);
   free(get_extensions_name);
   if (!get_extensions)
      return;

   /* Use the newer DRI loader entrypoint to find extensions.
    * We will then expose these extensions via the older
    * __driDriverExtensions symbol.
    */
   extensions = get_extensions();

   /* Copy the extensions into the __driDriverExtensions array
    * we declared.
    */
   for (i = 0; i < ARRAY_SIZE(__driDriverExtensions); i++) {
      __driDriverExtensions[i] = extensions[i];
      if (extensions[i] == NULL)
         break;
   }

   /* If the driver had more extensions than we reserved, then
    * bail out.
    */
   if (i == ARRAY_SIZE(__driDriverExtensions)) {
      __driDriverExtensions[0] = NULL;
      fprintf(stderr, "Megadriver stub did not reserve enough extension "
              "slots.\n");
      return;
   }
}

#endif /* RTLD_DEFAULT && HAVE_DLADDR */

static const
__DRIconfig **stub_error_init_screen(__DRIscreen *psp)
{
   fprintf(stderr, "An updated DRI driver loader (libGL.so or X Server) is "
           "required for this Mesa driver.\n");
   return NULL;
}

/**
 * This is a stub driDriverAPI that is referenced by dri_util.c but should
 * never be used.
 */
const struct __DriverAPIRec driDriverAPI = {
   .InitScreen = stub_error_init_screen,
};
