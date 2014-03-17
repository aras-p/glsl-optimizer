#ifndef _PCI_ID_DRIVER_MAP_H_
#define _PCI_ID_DRIVER_MAP_H_

#include <stddef.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef __IS_LOADER
#  error "Only include from loader.c"
#endif

static const int i915_chip_ids[] = {
#define CHIPSET(chip, desc, name) chip,
#include "pci_ids/i915_pci_ids.h"
#undef CHIPSET
};

static const int i965_chip_ids[] = {
#define CHIPSET(chip, family, name) chip,
#include "pci_ids/i965_pci_ids.h"
#undef CHIPSET
};

static const int r100_chip_ids[] = {
#define CHIPSET(chip, name, family) chip,
#include "pci_ids/radeon_pci_ids.h"
#undef CHIPSET
};

static const int r200_chip_ids[] = {
#define CHIPSET(chip, name, family) chip,
#include "pci_ids/r200_pci_ids.h"
#undef CHIPSET
};

static const int r300_chip_ids[] = {
#define CHIPSET(chip, name, family) chip,
#include "pci_ids/r300_pci_ids.h"
#undef CHIPSET
};

static const int r600_chip_ids[] = {
#define CHIPSET(chip, name, family) chip,
#include "pci_ids/r600_pci_ids.h"
#undef CHIPSET
};

static const int radeonsi_chip_ids[] = {
#define CHIPSET(chip, name, family) chip,
#include "pci_ids/radeonsi_pci_ids.h"
#undef CHIPSET
};

static const int vmwgfx_chip_ids[] = {
#define CHIPSET(chip, name, family) chip,
#include "pci_ids/vmwgfx_pci_ids.h"
#undef CHIPSET
};

int is_nouveau_vieux(int fd);

static const struct {
   int vendor_id;
   const char *driver;
   const int *chip_ids;
   int num_chips_ids;
   unsigned driver_types;
   int (*predicate)(int fd);
} driver_map[] = {
   { 0x8086, "i915", i915_chip_ids, ARRAY_SIZE(i915_chip_ids), _LOADER_DRI | _LOADER_GALLIUM },
   { 0x8086, "i965", i965_chip_ids, ARRAY_SIZE(i965_chip_ids), _LOADER_DRI | _LOADER_GALLIUM },
   { 0x1002, "radeon", r100_chip_ids, ARRAY_SIZE(r100_chip_ids), _LOADER_DRI },
   { 0x1002, "r200", r200_chip_ids, ARRAY_SIZE(r200_chip_ids), _LOADER_DRI },
   { 0x1002, "r300", r300_chip_ids, ARRAY_SIZE(r300_chip_ids), _LOADER_GALLIUM },
   { 0x1002, "r600", r600_chip_ids, ARRAY_SIZE(r600_chip_ids), _LOADER_GALLIUM },
   { 0x1002, "radeonsi", radeonsi_chip_ids, ARRAY_SIZE(radeonsi_chip_ids), _LOADER_GALLIUM},
   { 0x10de, "nouveau_vieux", NULL, -1,  _LOADER_DRI, is_nouveau_vieux },
   { 0x10de, "nouveau", NULL, -1,  _LOADER_GALLIUM },
   { 0x15ad, "vmwgfx", vmwgfx_chip_ids, ARRAY_SIZE(vmwgfx_chip_ids), _LOADER_GALLIUM },
   { 0x0000, NULL, NULL, 0 },
};

#endif /* _PCI_ID_DRIVER_MAP_H_ */
