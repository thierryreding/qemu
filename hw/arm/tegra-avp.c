#include "exec/address-spaces.h"
#include "hw/arm/arm.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/sysbus.h"
#include "sysemu/char.h"
#include "sysemu/sysemu.h"

#include "hw/arm/tegra.h"

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

#define TEGRA_INTC_BASE  0x60004000
#define TEGRA_UARTA_BASE 0x70006000
#define TEGRA_UARTD_BASE 0x70006300

#define AVP_IRAM_BASE 0x40000000
#define AVP_IRAM_SIZE 0x00040000

typedef struct AVPState {
    ARMCPU *cpu;

    MemoryRegion iram;
} AVPState;

static struct arm_boot_info boot_info = {
    .entry = AVP_IRAM_BASE,
};

static void avp_reset(void *opaque)
{
    ARMCPU *cpu = opaque;
    CPUARMState *env = &cpu->env;
    const struct arm_boot_info *info = env->boot_info;

    cpu_reset(CPU(cpu));

    if (info) {
        printf("initializing r15 with %08lx\n", info->entry);
        env->regs[15] = info->entry & 0xfffffffc;
        env->thumb = 0;
    }
}

static void avp_init(MachineState *machine)
{
    CharDriverState *chr = qemu_char_get_next_serial();
    MemoryRegion *system_mem = get_system_memory();
    DeviceState *dev;
    AVPState *avp;
    qemu_irq irq;
    int err;

    avp = g_new(AVPState, 1);
    if (!avp) {
        fprintf(stderr, "Unable to allocate AVP state\n");
        exit(1);
    }

    avp->cpu = cpu_arm_init("arm720t");
    if (!avp->cpu) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }

    memory_region_init_ram(&avp->iram, NULL, "iram", AVP_IRAM_SIZE,
                           &error_abort);
    memory_region_add_subregion(system_mem, AVP_IRAM_BASE, &avp->iram);

    irq = qdev_get_gpio_in(DEVICE(avp->cpu), ARM_CPU_IRQ);
    dev = tegra_intc_create(TEGRA_INTC_BASE, irq);

    irq = qdev_get_gpio_in(dev, 36);
    tegra_uart_create(TEGRA_UARTA_BASE, irq, NULL);

    irq = qdev_get_gpio_in(dev, 90);
    tegra_uart_create(TEGRA_UARTD_BASE, irq, chr);

    boot_info.kernel_filename = machine->kernel_filename;

    err = load_image_targphys(machine->kernel_filename, AVP_IRAM_BASE,
                              AVP_IRAM_SIZE);
    printf("load_image_targphys(): %d\n", err);

    qemu_register_reset(avp_reset, avp->cpu);
    avp->cpu->env.boot_info = &boot_info;
}

static QEMUMachine avp_machine = {
    .name = "avp",
    .desc = "NVIDIA Tegra AVP",
    .init = avp_init,
    .max_cpus = 1,
};

static void avp_machine_init(void)
{
    qemu_register_machine(&avp_machine);
}
machine_init(avp_machine_init);

/* vim: set et sts=4 sw=4 ts=4: */
