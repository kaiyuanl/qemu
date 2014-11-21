/*
 * This is a demo program to verify the function of DES device
 */ 

#define UART16550_BASEADDR  0x83e01003
#define DES_BASEADDR 0x83c02000
#define ENCRYPTION_MODE 1
#define DECRYPTION_MODE 0
#define GENERATEKEY_MODE 2
#define DONE_MODE 3
#define REG_KEY_ADDR_OFFSET 0x0000
#define REG_IN_ADDR_OFFSET 0x0004
#define REG_OUT_ADDR_OFFSET 0x0008
#define REG_MODE_OFFSET 0x000C
#define OUT_SIZE_MAX 100

volatile unsigned int * const UART0DR = (unsigned int *) UART16550_BASEADDR;

void print_uart0(volatile char *s)
{
	while(*s != '\0') 
	{ /* Loop until end of string */
	    *UART0DR = (unsigned int)(*s); /* Transmit char */
	    s++; /* Next char */
    }
}

void hex_to_string(volatile unsigned int hex, char *str)
{
	int i;
	unsigned char num;
	//unsigned int divisor;
	unsigned int quotient;
	//divisor = 1;

	for(i = 0; i < 8; i++){
		quotient = (char)(hex>>(4*i));
		num = quotient%16;
		if(num >= 10)	str[7-i] = num-10+'a';
		else str[7-i] = num+'0';
		//if(i < 8) divisor *= 16;
	}
	str[8] = '\0';
	//return str;
}


void main()
{
    volatile unsigned int* reg_key_addr = (volatile unsigned int*)(DES_BASEADDR + REG_KEY_ADDR_OFFSET);
	volatile unsigned int* reg_in_addr = (volatile unsigned int*)(DES_BASEADDR + REG_IN_ADDR_OFFSET);
	volatile unsigned int* reg_out_addr = (volatile unsigned int*)(DES_BASEADDR + REG_OUT_ADDR_OFFSET);
	volatile unsigned int* reg_mode = (volatile unsigned int*)(DES_BASEADDR + REG_MODE_OFFSET);
	
    unsigned char key[9];
	unsigned char* plain = "hello, world";
	unsigned char cipher[OUT_SIZE_MAX];
    unsigned char decrypted[OUT_SIZE_MAX];
    unsigned char* pstr;

    print_uart0("hello, des\n");
    print_uart0("plain text is ");
    print_uart0(plain);
    print_uart0("\n");


	/*Generate key*/
    print_uart0("start generating key\n");
    *reg_key_addr = key;
    *reg_mode = GENERATEKEY_MODE;
	while(*reg_mode != DONE_MODE ) {
        //break;
	}
    pstr = (unsigned char*)(*reg_key_addr);
    print_uart0("key is");
    print_uart0(pstr);
	print_uart0("\n");

    /*Encryption*/
    print_uart0("start encrypting\n");
    *reg_in_addr = plain;
    *reg_out_addr = cipher;
    *reg_mode = ENCRYPTION_MODE;
    while(*reg_mode != DONE_MODE) {
    }
	pstr = (unsigned char*)(*reg_out_addr);
    print_uart0("cipher text is ");
    print_uart0(pstr);
    print_uart0("\n");
    
    /*Decryption*/
    print_uart0("start decryption\n");
    *reg_in_addr = cipher;
    *reg_out_addr = decrypted;
    *reg_mode = DECRYPTION_MODE;
    while(*reg_mode != DONE_MODE) {
    }
    pstr = (unsigned char*)(*reg_out_addr);
    print_uart0("decrypted text is");
    print_uart0(pstr);
    print_uart0("\n");
}
