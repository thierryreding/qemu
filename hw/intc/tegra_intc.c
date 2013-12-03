#include "hw/arm/tegra.h"
#include "hw/sysbus.h"

#define DEBUG

#ifndef DEBUG
#define DEBUG_PRINT 0
#else
#define DEBUG_PRINT 1
#endif

#define dprintf(fp, fmt, args...) ({  \
        if (DEBUG_PRINT)              \
            fprintf(fp, fmt, ##args); \
    })

typedef struct TegraINTCState {
    SysBusDevice base;
    MemoryRegion mmio;
    qemu_irq parent;
} TegraINTCState;

#define TYPE_TEGRA_INTC "nvidia,tegra20-intc"
#define TEGRA_INTC(obj) OBJECT_CHECK(TegraINTCState, obj, TYPE_TEGRA_INTC)

static uint64_t tegra_intc_read(void *opaque, hwaddr addr, unsigned int size)
{
    uint64_t ret = 0;
    dprintf(stderr, "> %s(opaque=%p, addr=%#" HWADDR_PRIx ", size=%u)\n",
            __func__, opaque, addr, size);
    dprintf(stderr, "< %s() = %#" PRIx64 "\n", __func__, ret);
    return ret;
}

static void tegra_intc_write(void *opaque, hwaddr addr, uint64_t value,
                            unsigned int size)
{
    dprintf(stderr, "> %s(opaque=%p, addr=%#" HWADDR_PRIx ", value=%#" PRIx64
            ",size=%u)\n", __func__, opaque, addr, value, size);
    dprintf(stderr, "< %s()\n", __func__);
}

static const MemoryRegionOps tegra_intc_ops = {
    .read = tegra_intc_read,
    .write = tegra_intc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void tegra_intc_handler(void *opaque, int irq, int level)
{
}

static int tegra_intc_init(SysBusDevice *sbd)
{
    TegraINTCState *s = TEGRA_INTC(sbd);
    DeviceState *dev = DEVICE(sbd);

    dprintf(stderr, "> %s(sbd=%p)\n", __func__, sbd);

    qdev_init_gpio_in(dev, tegra_intc_handler, 128);
    sysbus_init_irq(sbd, &s->parent);

    memory_region_init_io(&s->mmio, OBJECT(s), &tegra_intc_ops, s,
                          TYPE_TEGRA_INTC, 0x400);
    sysbus_init_mmio(sbd, &s->mmio);

    dprintf(stderr, "< %s()\n", __func__);
    return 0;
}

static void tegra_intc_class_init(ObjectClass *class, void *data)
{
    SysBusDeviceClass *sbdc = SYS_BUS_DEVICE_CLASS(class);

    sbdc->init = tegra_intc_init;
}

static const TypeInfo tegra_intc_info = {
    .name = TYPE_TEGRA_INTC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(TegraINTCState),
    .class_init = tegra_intc_class_init,
};

static void tegra_intc_register_types(void)
{
    type_register_static(&tegra_intc_info);
}
type_init(tegra_intc_register_types)

DeviceState *tegra_intc_create(hwaddr addr, qemu_irq irq)
{
    DeviceState *dev = qdev_create(NULL, TYPE_TEGRA_INTC);
    SysBusDevice *bus = SYS_BUS_DEVICE(dev);

    qdev_init_nofail(dev);

    sysbus_connect_irq(bus, 0, irq);
    sysbus_mmio_map(bus, 0, addr);

    return dev;
}

/* vim: set et ts=4 sts=4 sw=4: */
