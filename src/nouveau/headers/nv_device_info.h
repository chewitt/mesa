#ifndef NV_DEVINFO_H
#define NV_DEVINFO_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "util/macros.h"

#include "nvidia/g_nv_name_legacy340.h"
#include "nvidia/g_nv_name_released.h"

#define NVIDIA_VENDOR_ID 0x10de

enum ENUM_PACKED nv_device_type {
   NV_DEVICE_TYPE_IGP,
   NV_DEVICE_TYPE_DIS,
   NV_DEVICE_TYPE_SOC,
};

struct nv_device_info {
   enum nv_device_type type;

   uint16_t device_id;
   uint16_t chipset;

   char device_name[64];
   char chipset_name[16];

   /* Populated if type != NV_DEVICE_TYPE_SOC */
   struct {
      uint16_t domain;
      uint8_t bus;
      uint8_t dev;
      uint8_t func;
      uint8_t revision_id;
   } pci;

   uint8_t sm; /**< Shader model */

   uint8_t gpc_count;
   uint16_t tpc_count;
   uint8_t mp_per_tpc;
   uint8_t max_warps_per_mp;

   uint16_t cls_copy;
   uint16_t cls_eng2d;
   uint16_t cls_eng3d;
   uint16_t cls_m2mf;
   uint16_t cls_compute;

   uint64_t vram_size_B;
   uint64_t bar_size_B;
};

static inline void
nv_device_uuid(const struct nv_device_info *info, uint8_t *uuid, size_t len, bool vm_bind)
{
   uint16_t vendor_id = NVIDIA_VENDOR_ID;

   assert(len >= 16);

   memset(uuid, 0, len);
   memcpy(&uuid[0], &info->chipset, 2);
   memcpy(&uuid[2], &vendor_id, 2);
   memcpy(&uuid[4], &info->device_id, 2);

   if (info->type != NV_DEVICE_TYPE_SOC) {
      memcpy(&uuid[6], &info->pci.domain, 2);
      uuid[8]  = info->pci.bus;
      uuid[9]  = info->pci.dev;
      uuid[10] = info->pci.func;
   }
   uuid[11] = vm_bind;
}

static const char *
name_for_chip(uint32_t dev_id,
              uint16_t subsystem_id,
              uint16_t subsystem_vendor_id)
{
   const char *name = NULL;

   const size_t total_size = ARRAY_SIZE(sChipsLegacy340) + ARRAY_SIZE(sChipsReleased);
   const CHIPS_RELEASED *combinedChips[total_size];
   memcpy(combinedChips, sChipsLegacy340, sizeof(sChipsLegacy340));
   memcpy(combinedChips + ARRAY_SIZE(sChipsLegacy340), sChipsReleased, sizeof(sChipsReleased));

   for (size_t i = 0; i < total_size; i++) {
      const CHIPS_RELEASED *chip = combinedChips[i];

      if (dev_id != chip->devID)
         continue;

      if (chip->subSystemID == 0 && chip->subSystemVendorID == 0) {
         // When subSystemID and subSystemVendorID are both 0, this is the
         // default name for the given chip. A more specific name may exist
         // elsewhere in the list.
         assert(name == NULL);
         name = chip->name;
         continue;
      }

      // If we find a specific name, return it
      if (chip->subSystemID == subsystem_id &&
          chip->subSystemVendorID == subsystem_vendor_id)
         return chip->name;
   }

   return name;
}

#endif /* NV_DEVINFO_H */
