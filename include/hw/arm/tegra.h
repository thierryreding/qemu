#ifndef TEGRA_H
#define TEGRA_H

#include <stdint.h>

#include "exec/hwaddr.h"
#include "hw/irq.h"
#include "qemu/typedefs.h"

DeviceState *tegra_intc_create(hwaddr addr, qemu_irq irq);

DeviceState *tegra_uart_create(hwaddr addr, qemu_irq irq, CharDriverState *chr);

#endif
