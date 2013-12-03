#include "hw/arm/tegra.h"
#include "hw/sysbus.h"
#include "sysemu/char.h"

//#define DEBUG
#ifndef DEBUG
#define DEBUG_PRINT 0
#else
#define DEBUG_PRINT 1
#endif

#define dprintf(fp, fmt, args...) ({  \
        if (DEBUG_PRINT)              \
            fprintf(fp, fmt, ##args); \
    })

#define TYPE_TEGRA_UART "nvidia,tegra20-uart"

typedef struct TegraUARTState {
    SysBusDevice base;

    CharDriverState *chr;
    MemoryRegion mmio;
    qemu_irq irq;

    unsigned int tx_fifo_length;
    uint8_t tx_fifo[8];
} TegraUARTState;

#define TEGRA_UART(obj) OBJECT_CHECK(TegraUARTState, obj, TYPE_TEGRA_UART)

#define UART_THR 0x00
#define UART_LSR 0x14
#define UART_LSR_THRE (1 << 5)

static uint64_t tegra_uart_read(void *opaque, hwaddr addr, unsigned int size)
{
    TegraUARTState *s = opaque;
    uint64_t ret = 0;

    dprintf(stderr, "> %s(opaque=%p, addr=%#" HWADDR_PRIx ", size=%u)\n",
            __func__, opaque, addr, size);

    switch (addr) {
    case UART_LSR:
        if (s->tx_fifo_length < sizeof(s->tx_fifo))
            ret |= UART_LSR_THRE;

        break;
    }

    dprintf(stderr, "< %s() = %#" PRIx64 "\n", __func__, ret);
    return ret;
}

static void tegra_uart_write(void *opaque, hwaddr addr, uint64_t value,
                 unsigned int size)
{
    TegraUARTState *s = opaque;
    uint8_t ch = value;
    int ret;

    dprintf(stderr, "> %s(opaque=%p, addr=%#" HWADDR_PRIx ", value=%#" PRIx64 ", size=%u)\n",
            __func__, opaque, addr, value, size);
    dprintf(stderr, "  chr: %p\n", s->chr);

    switch (addr) {
    case UART_THR:
        dprintf(stderr, "THR < %08x\n", (uint32_t)value);
        if (s->chr) {
            ret = qemu_chr_fe_write(s->chr, &ch, 1);
            dprintf(stderr, "  qemu_chr_fe_write(): %d\n", ret);
        }

        qemu_irq_raise(s->irq);

        break;
    }

    dprintf(stderr, "< %s()\n", __func__);
}

static const MemoryRegionOps tegra_uart_ops = {
    .read = tegra_uart_read,
    .write = tegra_uart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void tegra_uart_rx(void *opaque, const uint8_t *buf, int size)
{
    dprintf(stderr, "> %s(opaque=%p, buf=%p, size=%d)\n", __func__,
            opaque, buf, size);
    dprintf(stderr, "< %s()\n", __func__);
}

static int tegra_uart_can_rx(void *opaque)
{
    int ret = 1;
    dprintf(stderr, "> %s(opaque=%p)\n", __func__, opaque);
    dprintf(stderr, "< %s() = %d\n", __func__, ret);
    return ret;
}

static void tegra_uart_event(void *opaque, int event)
{
    dprintf(stderr, "> %s(opaque=%p, event=%d)\n", __func__, opaque, event);
    dprintf(stderr, "< %s()\n", __func__);
}

static int tegra_uart_init(SysBusDevice *dev)
{
    TegraUARTState *s = TEGRA_UART(dev);

    sysbus_init_irq(dev, &s->irq);

    memory_region_init_io(&s->mmio, OBJECT(s), &tegra_uart_ops, s,
                          TYPE_TEGRA_UART, 0x20);
    sysbus_init_mmio(dev, &s->mmio);

    /*
    s->chr = qemu_char_get_next_serial();
    */
    if (s->chr) {
        qemu_chr_add_handlers(s->chr, tegra_uart_can_rx, tegra_uart_rx,
                              tegra_uart_event, s);
        dprintf(stderr, "serial setup: %p\n", s->chr);
    }

    return 0;
}

static Property tegra_uart_properties[] = {
    DEFINE_PROP_CHR("chardev", TegraUARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void tegra_uart_class_init(ObjectClass *class, void *data)
{
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(class);
    DeviceClass *dc = DEVICE_CLASS(class);

    dc->props = tegra_uart_properties;
    sdc->init = tegra_uart_init;
}

static const TypeInfo tegra_uart_info = {
    .name = TYPE_TEGRA_UART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(TegraUARTState),
    .class_init = tegra_uart_class_init,
};

static void tegra_uart_register_types(void)
{
    type_register_static(&tegra_uart_info);
}
type_init(tegra_uart_register_types);

DeviceState *tegra_uart_create(hwaddr addr, qemu_irq irq, CharDriverState *chr)
{
    DeviceState *dev = qdev_create(NULL, TYPE_TEGRA_UART);
    SysBusDevice *bus = SYS_BUS_DEVICE(dev);

    dprintf(stderr, "> %s(addr=%#" HWADDR_PRIx ", irq=%p, chr=%p)\n",
            __func__, addr, irq, chr);

    qdev_prop_set_chr(dev, "chardev", chr);
    qdev_init_nofail(dev);

    sysbus_connect_irq(bus, 0, irq);
    sysbus_mmio_map(bus, 0, addr);

    dprintf(stderr, "< %s() = %p\n", __func__, dev);
    return dev;
}

/* vim: set et sts=4 sw=4 ts=4: */
