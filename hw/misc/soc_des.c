/*
 * Data Encryption Standard(DES) device 
 * for virtex-ml507
 * Written by Liang Kaiyuan <kaiyuanl@tju.edu.cn>
 * for testing and research
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU GPLv2 as published by
 * Free Software Foundation, or any later version. See the COPING
 * file in the top-level directory.
 */
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu/main-loop.h"

#define DES_DEBUG

#define REGS_OFFSET 0x1000
#define REG_IN_OFFSET 0x0000
#define REG_OUT_OFFSET 0x0004
#define REG_KEY_OFFSET 0x0008
#define REG_MODE_OFFSET 0x000C

typedef struct SoCDESState{
    SysBusDevice parent_obj;
	MemoryRegion mmio;
	QEMUBH *bh;
	qemu_irq irq;
	
	uint32_t reg_in;
	uint32_t reg_out;
	uint32_t reg_key;
	uint32_t reg_mode;
}SoCDESState;

#define TYPE_SOC_DES "soc-des"
#define SOC_DES(obj) \
    OBJECT_CHECK(SoCDESState, (obj), TYPE_SOC_DES)
	

static uint64_t soc_des_read(void *opaque, hwaddr offset, unsigned size)
{
#ifdef DES_DEBUG
    printf("begin soc_des_read\n");
	printf("offset 0x%x\t size 0x%x\n", (unsigned int)offset, size);
#endif
    SoCDESState *s = opaque;
	uint32_t ret = 0;
	switch(offset)
	{
	case REG_OUT_OFFSET:
	    ret = s->reg_out;
	    break;
	case REG_KEY_OFFSET:
	    ret = s->reg_key;
	    break;
	default:
	    printf("bad offset 0x%x\n", (int)offset);
	}
	
#ifdef DES_DEBUG
    printf("offset 0x%x, value 0x%x\n", (int)offset, ret);
    printf("finish soc_des_read\n");
#endif
	return ret;
}

static void soc_des_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
#ifdef DES_DEBUG
    printf("begin soc_des_write\n");
	printf("offset 0x%x\t size 0x%x\n", (unsigned)offset, size);
	printf("value x0%x\n", (unsigned)value);
#endif
    SoCDESState *s = opaque;
	
	switch(offset)
	{
	case REG_IN_OFFSET:
	    s->reg_in = value;
	    break;
	case REG_KEY_OFFSET:
	    s->reg_key = value;
	    break;
	case REG_MODE_OFFSET:
	    s->reg_mode = value;
	    break;
	default:
	    printf("bad offset 0x%x\n", (int)offset);
	}
#ifdef DES_DEBUG
    printf("finish soc_des_write\n");
#endif
}


static const MemoryRegionOps soc_des_ops = {
    .read = soc_des_read,
	.write = soc_des_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
	.valid = {
	    .min_access_size = 4,
		.max_access_size = 4
	}
};

static const VMStateDescription vmstate_soc_des = {
    .name = "vsd-soc-des",
	.version_id = 0,
	.minimum_version_id = 0,
	.fields = (VMStateField[]) {
	    VMSTATE_UINT32(reg_in, SoCDESState),
		VMSTATE_UINT32(reg_out, SoCDESState),
		VMSTATE_UINT32(reg_key, SoCDESState),
		VMSTATE_UINT32(reg_mode, SoCDESState),		
	    VMSTATE_END_OF_LIST()
	}
};

static int soc_des_init(SysBusDevice *dev)
{
#ifdef DES_DEBUG
    printf("begin soc_des_init\n");
#endif
	
    SoCDESState *d = SOC_DES(dev);
	d->reg_in = 123;
	d->reg_out = 321;
	d->reg_key = 231;
	d->reg_mode = 213;
	
	sysbus_init_irq(dev, &d->irq);
	memory_region_init_io(&d->mmio, OBJECT(d), &soc_des_ops, d, TYPE_SOC_DES, REGS_OFFSET);
	sysbus_init_mmio(dev, &d->mmio);
#ifdef DES_DEBUG
    printf("finish soc_des_init\n");
#endif	
	return 0;
}

static void soc_des_class_init(ObjectClass *klass, void *data)
{
#ifdef DES_DEBUG
    printf("begin soc_des_class_init\n");
#endif
    DeviceClass *d = DEVICE_CLASS(klass);
	SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);
	
	k->init = soc_des_init;
	d->vmsd = &vmstate_soc_des;
#ifdef DES_DEBUG
    printf("finish soc_des_class_init\n");
#endif
}

static const TypeInfo soc_des_info = {
	.name = TYPE_SOC_DES,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(SoCDESState),
	.class_init = soc_des_class_init,
};

static void soc_des_register_types(void)
{
#ifdef DES_DEBUG
    printf("begin soc_des_register_types\n");
#endif
    type_register_static(&soc_des_info);
#ifdef DES_DEBUG
    printf("finish soc_des_register_types\n");
#endif
}

type_init(soc_des_register_types)
