#ifndef CHARDEV_H
#define CHARDEV_H

#include "fc1100.h"

int create_char_devs(struct fc1100_driver* drv);
int destroy_char_devs(void);

#endif