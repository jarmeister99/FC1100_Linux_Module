#ifndef FC_1100_H
#define FC_1100_H

#include <linux/types.h>

#define FC1100_VENDOR_ID 0x15EC
#define FC1100_DEVICE_ID 0x1100

struct fc1100_driver {
	u8 __iomem *hwmem;
};

#endif