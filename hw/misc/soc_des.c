/*
 * Data Encryption Standard (DES) device
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

#define ENCRYPTION_MODE 1
#define DECRYPTION_MODE 0
#define GENERATEKEY_MODE 2
#define DONE_MODE 3

typedef struct {
	unsigned char k[8];
	unsigned char c[4];
	unsigned char d[4];
} key_set;

void generate_key(unsigned char* key);
void generate_sub_keys(unsigned char* main_key, key_set* key_sets);
void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set* key_sets, int mode);
uint32_t chars_to_uint32(unsigned char* chars, int start, int end);
void uint32_to_chars(uint32_t high, uint32_t low, unsigned char* chars);
void print_char_as_binary(char input);
void print_key_set(key_set key_set);

#define DES_DEBUG

#define REGS_OFFSET         0x1000

#define REG_IN_HIGH_OFFSET     0x0000
#define REG_IN_LOW_OFFSET     0x0004

#define REG_OUT_HIGH_OFFSET    0x0008
#define REG_OUT_LOW_OFFSET     0x000C

#define REG_KEY_HIGH_OFFSET    0x0010
#define REG_KEY_LOW_OFFSET 0x0014

#define REG_MODE_OFFSET 0x0018

typedef struct SoCDESState{
    SysBusDevice parent_obj;
    MemoryRegion mmio;
    QEMUBH *bh;
    qemu_irq irq;
    
    uint32_t reg_in_high;
    uint32_t reg_in_low;
    uint32_t reg_out_high;
    uint32_t reg_out_low;
    uint32_t reg_key_high;
    uint32_t reg_key_low;
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
    case REG_OUT_HIGH_OFFSET:
        ret = s->reg_out_high;
        break;
    case REG_OUT_LOW_OFFSET:
        ret = s->reg_out_low;
        break;
    case REG_KEY_HIGH_OFFSET:
        ret = s->reg_key_high;
        break;
    case REG_KEY_LOW_OFFSET:
        ret = s->reg_key_low;
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
    
    /* variables */
    unsigned char *des_key;
    unsigned char* data_block;
    unsigned char* processed_block;
    key_set* key_sets;
    uint32_t key_high;
    uint32_t key_low;
    uint32_t out_high;
    uint32_t out_low;
    /**/
    
    switch(offset)
    {
    case REG_IN_HIGH_OFFSET:
        s->reg_in_high = value;
        break;
    case REG_IN_LOW_OFFSET:
        s->reg_in_low = value;
        break;
    case REG_KEY_HIGH_OFFSET:
        s->reg_key_high = value;
        break;
    case REG_KEY_LOW_OFFSET:
        s->reg_key_low = value;
        break;
    case REG_MODE_OFFSET:
        s->reg_mode = value;
        switch(s->reg_mode)
        {
        case GENERATEKEY_MODE:
            des_key = (unsigned char*)malloc(8*sizeof(char));
            generate_key(des_key);
            key_high = chars_to_uint32(des_key, 0, 4);
            key_low = chars_to_uint32(des_key, 4, 8);
            s->reg_key_high = key_high;
            s->reg_key_low = key_low;            
            break;
        case ENCRYPTION_MODE:
            data_block = (unsigned char*) malloc(8*sizeof(char));
            processed_block = (unsigned char*) malloc(8*sizeof(char));
            key_sets = (key_set*)malloc(17*sizeof(key_set));
			des_key = (unsigned char*)malloc(8*sizeof(char));
			uint32_to_chars(s->reg_key_high, s->reg_key_low, des_key);
            generate_sub_keys(des_key, key_sets);
            printf("encrypting..\n");
            process_message(data_block, processed_block, key_sets, ENCRYPTION_MODE);
            out_high = chars_to_uint32(processed_block, 0, 4);
            out_low = chars_to_uint32(processed_block, 4, 8);
            s->reg_out_high = out_high;
            s->reg_out_low = out_low;
            break;
        case DECRYPTION_MODE:
            data_block = (unsigned char*) malloc(8*sizeof(char));
            processed_block = (unsigned char*) malloc(8*sizeof(char));
            key_sets = (key_set*)malloc(17*sizeof(key_set));
			des_key = (unsigned char*)malloc(8*sizeof(char));
			uint32_to_chars(s->reg_key_high, s->reg_key_low, des_key);
            generate_sub_keys(des_key, key_sets);
            printf("decrypting..\n");
            process_message(data_block, processed_block, key_sets, DECRYPTION_MODE);
            out_high = chars_to_uint32(processed_block, 0, 4);
            out_low = chars_to_uint32(processed_block, 4, 8);
            s->reg_out_high = out_high;
            s->reg_out_low = out_low;
            break;
        }
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
    .name = "soc-des",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(reg_in_high, SoCDESState),
        VMSTATE_UINT32(reg_in_low, SoCDESState),
        VMSTATE_UINT32(reg_out_high, SoCDESState),
        VMSTATE_UINT32(reg_out_low, SoCDESState),
        VMSTATE_UINT32(reg_key_high, SoCDESState),
        VMSTATE_UINT32(reg_key_low, SoCDESState),
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
    d->reg_in_high = 123;
    d->reg_in_low = 231;
    d->reg_out_high = 321;
    d->reg_out_low = 321;
    d->reg_key_high = 231;
    d->reg_key_low = 323;
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

int initial_key_permutaion[] = {57, 49,  41, 33,  25,  17,  9,
								 1, 58,  50, 42,  34,  26, 18,
								10,  2,  59, 51,  43,  35, 27,
								19, 11,   3, 60,  52,  44, 36,
								63, 55,  47, 39,  31,  23, 15,
								 7, 62,  54, 46,  38,  30, 22,
								14,  6,  61, 53,  45,  37, 29,
								21, 13,   5, 28,  20,  12,  4};

int initial_message_permutation[] =	   {58, 50, 42, 34, 26, 18, 10, 2,
										60, 52, 44, 36, 28, 20, 12, 4,
										62, 54, 46, 38, 30, 22, 14, 6,
										64, 56, 48, 40, 32, 24, 16, 8,
										57, 49, 41, 33, 25, 17,  9, 1,
										59, 51, 43, 35, 27, 19, 11, 3,
										61, 53, 45, 37, 29, 21, 13, 5,
										63, 55, 47, 39, 31, 23, 15, 7};

int key_shift_sizes[] = {-1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

int sub_key_permutation[] =    {14, 17, 11, 24,  1,  5,
								 3, 28, 15,  6, 21, 10,
								23, 19, 12,  4, 26,  8,
								16,  7, 27, 20, 13,  2,
								41, 52, 31, 37, 47, 55,
								30, 40, 51, 45, 33, 48,
								44, 49, 39, 56, 34, 53,
								46, 42, 50, 36, 29, 32};

int message_expansion[] =  {32,  1,  2,  3,  4,  5,
							 4,  5,  6,  7,  8,  9,
							 8,  9, 10, 11, 12, 13,
							12, 13, 14, 15, 16, 17,
							16, 17, 18, 19, 20, 21,
							20, 21, 22, 23, 24, 25,
							24, 25, 26, 27, 28, 29,
							28, 29, 30, 31, 32,  1};

int S1[] = {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
			 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
			 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
			15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13};

int S2[] = {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
			 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
			 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
			13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9};

int S3[] = {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
			13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
			13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
			 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12};

int S4[] = { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
			13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
			10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
			 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14};

int S5[] = { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
			14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
			 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
			11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3};

int S6[] = {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
			10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
			 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
			 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13};

int S7[] = { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
			13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
			 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
			 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12};

int S8[] = {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
			 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
			 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
			 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11};

int right_sub_message_permutation[] =    {16,  7, 20, 21,
									29, 12, 28, 17,
									 1, 15, 23, 26,
									 5, 18, 31, 10,
									 2,  8, 24, 14,
									32, 27,  3,  9,
									19, 13, 30,  6,
									22, 11,  4, 25};

int final_message_permutation[] =  {40,  8, 48, 16, 56, 24, 64, 32,
									39,  7, 47, 15, 55, 23, 63, 31,
									38,  6, 46, 14, 54, 22, 62, 30,
									37,  5, 45, 13, 53, 21, 61, 29,
									36,  4, 44, 12, 52, 20, 60, 28,
									35,  3, 43, 11, 51, 19, 59, 27,
									34,  2, 42, 10, 50, 18, 58, 26,
									33,  1, 41,  9, 49, 17, 57, 25};


void print_char_as_binary(char input) {
	int i;
	for (i=0; i<8; i++) {
		char shift_byte = 0x01 << (7-i);
		if (shift_byte & input) {
			printf("1");
		} else {
			printf("0");
		}
	}
}

uint32_t chars_to_uint32(unsigned char* chars, int start, int end)
{
    int i, j;
	uint32_t val = 0;
    for(i = start; i < end; i++)
	{
	    for(j = 0; j < 8; j++)
		{
		    char shift_byte = 0x01 << (7 - j);
			if(shift_byte & chars[i])
			{
			    val += 1;
			}
			val <<= 1;
		}
	}
	return val;
}

void uint32_to_chars(uint32_t high, uint32_t low, unsigned char* chars)
{
    int i, j;
	for(i = 7; i >= 4; i--)
	{
	    for(j = 0; j < 4; j++)
		{
		    char shift_byte = 0xF << (4 * j);
			char val = low & shift_byte;
			val >>= (4 * j);
			chars[i] = val;
		}
	}
	
	for(i = 3; i >= 0; i--)
	{
	    for(j = 0; j <4; j++)
		{
		    char shift_byte = 0xF << (4 * j);
			char val = high & shift_byte;
			val >>= (4 * j);
			chars[i] = val;
		}
	}
}

void generate_key(unsigned char* key) {
	int i;
	for (i=0; i<8; i++) {
		key[i] = rand()%255;
	}
}

void print_key_set(key_set key_set){
	int i;
	printf("K: \n");
	for (i=0; i<8; i++) {
		printf("%02X : ", key_set.k[i]);
		print_char_as_binary(key_set.k[i]);
		printf("\n");
	}
	printf("\nC: \n");

	for (i=0; i<4; i++) {
		printf("%02X : ", key_set.c[i]);
		print_char_as_binary(key_set.c[i]);
		printf("\n");
	}
	printf("\nD: \n");

	for (i=0; i<4; i++) {
		printf("%02X : ", key_set.d[i]);
		print_char_as_binary(key_set.d[i]);
		printf("\n");
	}
	printf("\n");
}

void generate_sub_keys(unsigned char* main_key, key_set* key_sets) {
	int i, j;
	int shift_size;
	unsigned char shift_byte, first_shift_bits, second_shift_bits, third_shift_bits, fourth_shift_bits;

	for (i=0; i<8; i++) {
		key_sets[0].k[i] = 0;
	}

	for (i=0; i<56; i++) {
		shift_size = initial_key_permutaion[i];
		shift_byte = 0x80 >> ((shift_size - 1)%8);
		shift_byte &= main_key[(shift_size - 1)/8];
		shift_byte <<= ((shift_size - 1)%8);

		key_sets[0].k[i/8] |= (shift_byte >> i%8);
	}

	for (i=0; i<3; i++) {
		key_sets[0].c[i] = key_sets[0].k[i];
	}

	key_sets[0].c[3] = key_sets[0].k[3] & 0xF0;

	for (i=0; i<3; i++) {
		key_sets[0].d[i] = (key_sets[0].k[i+3] & 0x0F) << 4;
		key_sets[0].d[i] |= (key_sets[0].k[i+4] & 0xF0) >> 4;
	}

	key_sets[0].d[3] = (key_sets[0].k[6] & 0x0F) << 4;


	for (i=1; i<17; i++) {
		for (j=0; j<4; j++) {
			key_sets[i].c[j] = key_sets[i-1].c[j];
			key_sets[i].d[j] = key_sets[i-1].d[j];
		}

		shift_size = key_shift_sizes[i];
		if (shift_size == 1){
			shift_byte = 0x80;
		} else {
			shift_byte = 0xC0;
		}

		// Process C
		first_shift_bits = shift_byte & key_sets[i].c[0];
		second_shift_bits = shift_byte & key_sets[i].c[1];
		third_shift_bits = shift_byte & key_sets[i].c[2];
		fourth_shift_bits = shift_byte & key_sets[i].c[3];

		key_sets[i].c[0] <<= shift_size;
		key_sets[i].c[0] |= (second_shift_bits >> (8 - shift_size));

		key_sets[i].c[1] <<= shift_size;
		key_sets[i].c[1] |= (third_shift_bits >> (8 - shift_size));

		key_sets[i].c[2] <<= shift_size;
		key_sets[i].c[2] |= (fourth_shift_bits >> (8 - shift_size));

		key_sets[i].c[3] <<= shift_size;
		key_sets[i].c[3] |= (first_shift_bits >> (4 - shift_size));

		// Process D
		first_shift_bits = shift_byte & key_sets[i].d[0];
		second_shift_bits = shift_byte & key_sets[i].d[1];
		third_shift_bits = shift_byte & key_sets[i].d[2];
		fourth_shift_bits = shift_byte & key_sets[i].d[3];

		key_sets[i].d[0] <<= shift_size;
		key_sets[i].d[0] |= (second_shift_bits >> (8 - shift_size));

		key_sets[i].d[1] <<= shift_size;
		key_sets[i].d[1] |= (third_shift_bits >> (8 - shift_size));

		key_sets[i].d[2] <<= shift_size;
		key_sets[i].d[2] |= (fourth_shift_bits >> (8 - shift_size));

		key_sets[i].d[3] <<= shift_size;
		key_sets[i].d[3] |= (first_shift_bits >> (4 - shift_size));

		for (j=0; j<48; j++) {
			shift_size = sub_key_permutation[j];
			if (shift_size <= 28) {
				shift_byte = 0x80 >> ((shift_size - 1)%8);
				shift_byte &= key_sets[i].c[(shift_size - 1)/8];
				shift_byte <<= ((shift_size - 1)%8);
			} else {
				shift_byte = 0x80 >> ((shift_size - 29)%8);
				shift_byte &= key_sets[i].d[(shift_size - 29)/8];
				shift_byte <<= ((shift_size - 29)%8);
			}

			key_sets[i].k[j/8] |= (shift_byte >> j%8);
		}
	}
}

void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set* key_sets, int mode) {
	int i, k;
	int shift_size;
	unsigned char shift_byte;

	unsigned char initial_permutation[8];
	memset(initial_permutation, 0, 8);
	memset(processed_piece, 0, 8);

	for (i=0; i<64; i++) {
		shift_size = initial_message_permutation[i];
		shift_byte = 0x80 >> ((shift_size - 1)%8);
		shift_byte &= message_piece[(shift_size - 1)/8];
		shift_byte <<= ((shift_size - 1)%8);

		initial_permutation[i/8] |= (shift_byte >> i%8);
	}

	unsigned char l[4], r[4];
	for (i=0; i<4; i++) {
		l[i] = initial_permutation[i];
		r[i] = initial_permutation[i+4];
	}

	unsigned char ln[4], rn[4], er[6], ser[4];

	int key_index;
	for (k=1; k<=16; k++) {
		memcpy(ln, r, 4);

		memset(er, 0, 6);

		for (i=0; i<48; i++) {
			shift_size = message_expansion[i];
			shift_byte = 0x80 >> ((shift_size - 1)%8);
			shift_byte &= r[(shift_size - 1)/8];
			shift_byte <<= ((shift_size - 1)%8);

			er[i/8] |= (shift_byte >> i%8);
		}

		if (mode == DECRYPTION_MODE) {
			key_index = 17 - k;
		} else {
			key_index = k;
		}

		for (i=0; i<6; i++) {
			er[i] ^= key_sets[key_index].k[i];
		}

		unsigned char row, column;

		for (i=0; i<4; i++) {
			ser[i] = 0;
		}

		// 0000 0000 0000 0000 0000 0000
		// rccc crrc cccr rccc crrc cccr

		// Byte 1
		row = 0;
		row |= ((er[0] & 0x80) >> 6);
		row |= ((er[0] & 0x04) >> 2);

		column = 0;
		column |= ((er[0] & 0x78) >> 3);

		ser[0] |= ((unsigned char)S1[row*16+column] << 4);

		row = 0;
		row |= (er[0] & 0x02);
		row |= ((er[1] & 0x10) >> 4);

		column = 0;
		column |= ((er[0] & 0x01) << 3);
		column |= ((er[1] & 0xE0) >> 5);

		ser[0] |= (unsigned char)S2[row*16+column];

		// Byte 2
		row = 0;
		row |= ((er[1] & 0x08) >> 2);
		row |= ((er[2] & 0x40) >> 6);

		column = 0;
		column |= ((er[1] & 0x07) << 1);
		column |= ((er[2] & 0x80) >> 7);

		ser[1] |= ((unsigned char)S3[row*16+column] << 4);

		row = 0;
		row |= ((er[2] & 0x20) >> 4);
		row |= (er[2] & 0x01);

		column = 0;
		column |= ((er[2] & 0x1E) >> 1);

		ser[1] |= (unsigned char)S4[row*16+column];

		// Byte 3
		row = 0;
		row |= ((er[3] & 0x80) >> 6);
		row |= ((er[3] & 0x04) >> 2);

		column = 0;
		column |= ((er[3] & 0x78) >> 3);

		ser[2] |= ((unsigned char)S5[row*16+column] << 4);

		row = 0;
		row |= (er[3] & 0x02);
		row |= ((er[4] & 0x10) >> 4);

		column = 0;
		column |= ((er[3] & 0x01) << 3);
		column |= ((er[4] & 0xE0) >> 5);

		ser[2] |= (unsigned char)S6[row*16+column];

		// Byte 4
		row = 0;
		row |= ((er[4] & 0x08) >> 2);
		row |= ((er[5] & 0x40) >> 6);

		column = 0;
		column |= ((er[4] & 0x07) << 1);
		column |= ((er[5] & 0x80) >> 7);

		ser[3] |= ((unsigned char)S7[row*16+column] << 4);

		row = 0;
		row |= ((er[5] & 0x20) >> 4);
		row |= (er[5] & 0x01);

		column = 0;
		column |= ((er[5] & 0x1E) >> 1);

		ser[3] |= (unsigned char)S8[row*16+column];

		for (i=0; i<4; i++) {
			rn[i] = 0;
		}

		for (i=0; i<32; i++) {
			shift_size = right_sub_message_permutation[i];
			shift_byte = 0x80 >> ((shift_size - 1)%8);
			shift_byte &= ser[(shift_size - 1)/8];
			shift_byte <<= ((shift_size - 1)%8);

			rn[i/8] |= (shift_byte >> i%8);
		}

		for (i=0; i<4; i++) {
			rn[i] ^= l[i];
		}

		for (i=0; i<4; i++) {
			l[i] = ln[i];
			r[i] = rn[i];
		}
	}

	unsigned char pre_end_permutation[8];
	for (i=0; i<4; i++) {
		pre_end_permutation[i] = r[i];
		pre_end_permutation[4+i] = l[i];
	}

	for (i=0; i<64; i++) {
		shift_size = final_message_permutation[i];
		shift_byte = 0x80 >> ((shift_size - 1)%8);
		shift_byte &= pre_end_permutation[(shift_size - 1)/8];
		shift_byte <<= ((shift_size - 1)%8);

		processed_piece[i/8] |= (shift_byte >> i%8);
	}
}
