// KIRK is the PSP CPU encryption chip. It can do symmetric encryption (AES-128), asymmetric signatures (ECDSA), random number generation, and hashing (SHA-1).
// It also stores secret parameters used by the PSP.
// The KIRK ROM has been dumped and uses a custom ISA using words of 4 bytes and word-addressing (contrary to most architectures, if x points to a 4-byte word,
// x+1 points to the next 4-byte word). It has no registers and access to a memory space of 0x1000 words, most of the space being used to contact the other devices.
// It has two instructions, marked as functions here, that seem to have a special role: __builtin_setmode() to contact devices and __builtin_dma() to transmit data.

// KIRK has access to 8 different interfaces. All interfaces have associated 'enable' and 'status' register, plus other ones depending on the device.
// The status register is only used for initialization.
// To reset a device, use __builtin_setmode(XXX_REG, 0) then wait for the status to be non-zero.
// Once reset, to use a device, use __builtin_setmode(XXX_REG,val) (val can be 1 or 2 depending on the devices and actions), then __builtin_dma(data_size, 0)
// Exceptions:
// - for SHA1, there is no call to __builtin_setmode()
// - for DMA communication (copies from/to main RAM), arguments to the second function are __builtin_dma(0,1)
// - for CPU hardware registers communication, arguments to the second function are __builtin_dma(0,2)
// 
// Module 1 - ECC - can make and verify signatures, and multiply points by scalars
//
// 0x000 - ECC_REG - only 0 or 1
// 0x001 - ECC_STATUS
// 0x002 - ECC_OP - 0 = make signature (s) from r, 1 = verify signature, 2 = multiply point by scalar
// 0x003 - ECC_RESULT - only seen for commands 0 and 1, value 0 = OK
// 0x00B - ECC_P - field size for the curve
// 0x013 - ECC_A - parameter a for the curve
// 0x01B - ECC_B - parameter b for the curve
// 0x023 - ECC_N - modulus of the curve
// 0x02B - ECC_UNK1 - unknown, seems to be a parameter of the curve
// 0x033 - ECC_UNK2 - unknown, seems to be a parameter of the curve
// 0x03B - ECC_Gx - first coordinate of the generator (or input point)
// 0x043 - ECC_Gy - second coordinate of the generator (or input point)
// 0x04B - ECC_R - first scalar (k for multiplication, r for signature)
// 0x053 - ECC_S - second scalar (s for signature)
// 0x05B - ECC_M - message (hash) to sign (or to verify signature)
// 0x063 - ECC_Px - first coordinate of the public key (for signature check)
// 0x06B - ECC_Py - second coordinate of the public key (for signature check)
// 0x08B - ECC_RESULT_x - first coordinate of the result (for multiplication)
// 0x093 - ECC_RESULT_y - second coordinate of the result (for multiplication)
//
// Module 2 - SHA1
//
// 0x330 - SHA1_REG - only 0 (__builtin_setmode isn't used when doing SHA1's)
// 0x331 - SHA1_STATUS
// 0x332 -
// 0x333 - SHA1_IN_SIZE - size of the input data
// 0x334 - SHA1_IN_DATA - buffer for the current 4 bytes of input data
// 0x335 - SHA1_RESULT - result of the SHA1
//
// Module 3 - AES
//
// 0x380 - AES_REG - 0 for reset, 1 for sending data to encrypt/decrypt, 2 to set key
// 0x381 - AES_STATUS
// 0x382 - AES_CMD
//          bit 0 (0x01): 0 = encrypt, 1 = decrypt
//          bit 1 (0x02): 0 = not first block, 1 = first block (for CBC)
//          bit 2 (0x04): ?
//          bit 3 (0x08): ?
//          bit 4 (0x10): 0 = ECB, 1 = CBC
//          bit 5 (0x20): 0 = normal, 1 = CMAC

#define AES_CMD_ENCRYPT     0x00
#define AES_CMD_DECRYPT     0x01
#define AES_CMD_FIRST_BLOCK 0x02
#define AES_CMD_ECB         0x00
#define AES_CMD_CBC         0x10 // TODO seems incorrect
#define AES_CMD_CMAC        0x20

// 0x383 - AES_END - ???
// 0x384 - AES_SRC_BUF - current block to encrypt/decrypt
// 0x388 - AES_KEY - AES key
// 0x390 - AES_IV - AES IV for CBC
// 0x399 - AES_RESULT - result of the encryption/decryption
//
// Module 4 - some kind of PRNG?
//
// 0x420 - PRNG_REG - only 0 or 1
// 0x421 - PRNG_STATUS
// 0x422 - PRNG_MODE - 2 or 3?
// 0x423 - PRNG_SEED - a 40-byte-long buffer
// 0x42D - PRNG_RESULT - a 20-byte-long buffer
//
// Module 5 - maybe some TRNG?
//
// 0x440 - TRNG_REG - only 0 or 1
// 0x441 - TRNG_STATUS
// 0x442 - TRNG_OUT_SIZE - maybe the output size of the true rng
// 0x443 - TRNG_OUTPUT - output of the TRNG, at least 32-byte long, maybe 256-byte long
//
// Module 6 - "math" module, some kind of deterministic generator related to elliptic curve prime numbers, maybe something like Dual EC DRBG?
//
// 0x460 - MATH_REG - only 0 or 1
// 0x461 - MATH_STATUS
// 0x462 - MATH_IN - unknown 64-byte input buffer
// 0x472 - MATH_IN_X - unknown 32-byte input buffer, contains a prime number
// 0x47A - MATH_RESULT - output of the generator
//
// Key parameters:
//
// 0x490 - KEY_SEED0 - read-only key seed
// 0x494 - KEY_SEED1 - read-only key seed
// 0x498 - KEY_SEED2 - read-only key seed
// 0x49C - HW_AES_PARAM_ID - write-only, write ID to receive key in HW_AES_PARAM. 0x00/0x01 = kbooti (devkit), 0x02 = kirk 1 (IPL), 0x03 = kirk 2, 0x04~0x83 = kirk 4/7, 
// 0x4A0 - HW_AES_PARAM - read-only
//
// 0x4B0 - HW_CURVE_PARAM_ID - same as above, but with ECC parameters. 0/1 = kirk 1 public key, 2/3 = kirk 2 public key, 4 = kirk 2 private key, 5/6 = kirk 3 public key
// 0x4B1 - HW_CURVE_PARAM - the obtained parameter (scalar or coordinate)
//
// Module 7 - DMA module, to send/receive data from the main CPU
//
// 0xE00 - DMA_REG - 0 to init, 1 to write data, 2 to read data
// 0xE01 - DMA_STATUS - unused (initialization is present in the ROM but never called)
// 0xE02 - DMA_ADDR - address from which to read/write
// 0xE03 - DMA_BUF_SIZE - size of the data to send/receive
// 0xE10 - DMA_BUF - buffer of the data to send/receive
//
// Module 8 - PSP KIRK interface, interface with the registers at 0xBDE00000 on the CPU
//
// 0xE20 - PSP_KIRK_REG - only 0 or 1
// 0xE21 - PSP_KIRK_STATUS - equal to 1 when KIRK is initialized
// 0xE22 - PSP_KIRK_PHASE - possibly the same phase as 0xBDE0001C
// 0xE30 - PSP_KIRK_CMD - KIRK command, same as 0xBDE00010
// 0xE31 - PSP_KIRK_SRC - source address, same as 0xBDE0002C
// 0xE32 - PSP_KIRK_DST - destination address, same as 0xBDE00030
// 0xE33 - PSP_KIRK_ERROR - return value, same as 0xBDE00014
//
// Tests:
//
// 0xFB0 - HW_TEST_DISABLED - read-only, 0 to just run self-tests, 1 for normal operation
// 0xFB8 - HW_TEST_RESULT - write-only, 3 if self-tests succeeded, 2 if they failed
//
// Free to use (non-initialized) memory space: 0xFC0 - 0x4000
//
// 0xFC0 - SIZE - used as a size
// 0xFC1 - INPUT - used as an input address
// 0xFC2 - OUTPUT - used as an output address
// 0xFC3 - R3 - different uses
// 0xFC4 - R4 - different uses
// 0xFC5 - R5 - different uses
// 0xFC6 - R6 - different uses
// 0xFC7 - R7 - different uses
// 0xFC8 - AES_KEYSEED - used by aes_set_perconsole_key to define the per-console AES key to use. 1 for kirk 5/8, 2 for kirk 6/9, 3 for kirk 0x10, 4 for kirk 0x12, 6 for kirk 15 keyseed
// 0xFCB - R11 - different uses
// 0xFCC - R12 - different uses
// 0xFCD - R13 - different uses
// 0xFCE - R14 - different uses
// 0xFCF - ECDSA_MODE - temporary variable for cmd2
// 0xFD0 -
// 0xFD1 - KIRK_BODY_SIZE - used to store the body size for KIRK input data
// 0xFD2 - KIRK_DATA_OFFSET - used to store the data offset after the header for KIRK input data
// ...
// 0xFF4 - CUR_ENC_KEY - temporary variable to keep the AES decryption key to use for the current KIRK command
// 0xFF8 - KIRK15_KEYSEED - temporary space used by KIRK15 for a keyseed
// 0xFFF - UNK_FLAGS - unknown flags, set to 0 at the beginning
//                      bit 0 is set when command 15 is run
//                      bit 1 is set then reset when generating random values for cmd 12, 14, 16 ("public" stuff) ; it is reset when generating signatures
//                      if its value is 1, random number generator uses the kirk1 prime, otherwise it uses the other prime
//                      if its value is non-zero, most commands will set PSP_KIRK_PHASE to 1 and exit immediately returning KIRK_NOT_INITIALIZED
//                    -> it looks like it should be reset to 0 when CMD15 finishes?

#define KIRK_OPERATION_SUCCESS 0
#define KIRK_NOT_ENABLED 1
#define KIRK_INVALID_MODE 2
#define KIRK_HEADER_HASH_INVALID 3
#define KIRK_DATA_HASH_INVALID 4
#define KIRK_SIG_CHECK_INVALID 5
#define KIRK_NOT_INITIALIZED 0xC
#define KIRK_INVALID_OPERATION 0xD
#define KIRK_INVALID_SEED_CODE 0xE
#define KIRK_INVALID_SIZE 0xF
#define KIRK_DATA_SIZE_ZERO 0x10

void _reset(void) // 0x000
{
    do {
                    // WARNING: Do nothing block with infinite loop
    } while( true );
}

/*
 * Main function. Read commands coming from the PSP CPU hardware register and run the commands accordingly.
 */
void _start(void) // 0x001
{
    if (HW_TEST_DISABLED != 0) {
        kirk_self_test();
        inf_loop();
        return;
    }
    kirk_modules_init();
    UNK_FLAGS = 0;
    PSP_KIRK_PHASE = 0;
    while (true) {
        __builtin_setmode(PSP_KIRK_REG,1);
        __builtin_dma(0,2);
        if (PSP_KIRK_CMD == 0) {
            kirk_cmd0_decrypt_bootrom();
        } else if (PSP_KIRK_CMD == 1) {
            kirk_cmd1_decrypt_private();
        } else if (PSP_KIRK_CMD == 2) {
            kirk_cmd2_dnas_encrypt();
        } else if (PSP_KIRK_CMD == 3) {
            kirk_cmd3_dnas_decrypt();
        } else if (PSP_KIRK_CMD == 4) {
            kirk_cmd4_encrypt_static();
        } else if (PSP_KIRK_CMD == 5) {
            kirk_cmd5_encrypt_perconsole();
        } else if (PSP_KIRK_CMD == 6) {
            kirk_cmd6_encrypt_user();
        } else if (PSP_KIRK_CMD == 7) {
            kirk_cmd7_decrypt_static();
        } else if (PSP_KIRK_CMD == 8) {
            kirk_cmd8_decrypt_perconsole();
        } else if (PSP_KIRK_CMD == 9) {
            kirk_cmd9_decrypt_user();
        } else if (PSP_KIRK_CMD == 10) {
            kirk_cmd10_priv_sigvry();
        } else if (PSP_KIRK_CMD == 11) {
            kirk_cmd11_hash();
        } else if (PSP_KIRK_CMD == 12) {
            kirk_cmd12_ecdsa_genkey();
        } else if (PSP_KIRK_CMD == 13) {
            kirk_cmd13_ecdsa_mul();
        } else if (PSP_KIRK_CMD == 14) {
            kirk_cmd14_prngen();
        } else if (PSP_KIRK_CMD == 15) {
            kirk_cmd15_init();
        } else if (PSP_KIRK_CMD == 16) {
            kirk_cmd16_siggen();
        } else if (PSP_KIRK_CMD == 17) {
            kirk_cmd17_sigvry();
        } else if (PSP_KIRK_CMD == 18) {
            kirk_cmd18_certvry();
        } else {
            PSP_KIRK_RESULT = KIRK_INVALID_OPERATION;
            if (PSP_KIRK_STATUS != 1) {
                crash();
                return;
            }
            PSP_KIRK_PHASE = 1;
        }
    }
}

void inf_loop(void) // 0x074
{
    do {
                    // WARNING: Do nothing block with infinite loop
    } while( true );
}

/*
 * Encrypt INPUT to OUTPUT in AES ECB using CUR_ENC_KEY.
 */
void aes_encrypt_ecb(void)
{
    aes_set_null_iv();
    aes_encrypt_cbc();
}

void aes_encrypt_cbc(void)
{
    if (SIZE != 0) {
        kirk_reseed_prng_2();
        AES_KEY = CUR_ENC_KEY;
        AES_CMD = 0x12;
        hw_aes_setkey();
        DMA_BUF_SIZE = 0x10;
        while (0 < SIZE + -0x10) {
            SIZE = SIZE + -0x10;
            hw_aes_dma_copy_and_do();
            AES_IV = AES_RESULT;
            hw_dma_write_aes_result();
            INPUT = INPUT + 0x10;
            OUTPUT = OUTPUT + 0x10;
            AES_CMD &= 0xfffffffd;
        }
        DMA_BUF_SIZE = SIZE;
        hw_dma_read_input();
        AES_CMD |= 2;
        aes_padding();
        hw_aes_do();
        DMA_BUF_SIZE = 0x10;
        hw_dma_write_aes_result();
    }
    return;
}

void aes_cmac(void)
{
    aes_set_null_iv();
    AES_CMD = 0x32;
    hw_aes_setkey();
    AES_END = SIZE;
    DMA_BUF_SIZE = 0x10;
    aes_decrypt_ecb();
}

void aes_decrypt_ecb(void)

{
    uint uVar1;
    
    while (0 < SIZE + -0x10) {
        SIZE = SIZE + -0x10;
        hw_aes_dma_copy_and_do();
        INPUT = INPUT + 0x10;
        uVar1 = AES_CMD;
        AES_CMD = uVar1 & 0xfffffffd;
    }
    DMA_BUF_SIZE = SIZE;
    hw_aes_dma_copy_and_do();
    return;
}



void aes_decrypt_cbc(void)

{
    uint uVar1;
    uint auVar2 [4];
    
    aes_set_null_iv();
    if (SIZE != 0) {
        AES_CMD = 0x13;
        hw_aes_setkey();
        DMA_BUF_SIZE = 0x10;
        while (0 < SIZE + -0x10) {
            SIZE = SIZE + -0x10;
            hw_aes_dma_copy_and_do();
            auVar2 = AES_RESULT;
            AES_IV = auVar2;
            hw_dma_write_aes_result();
            INPUT = INPUT + 0x10;
            OUTPUT = OUTPUT + 0x10;
            uVar1 = AES_CMD;
            AES_CMD = uVar1 & 0xfffffffd;
        }
        hw_aes_dma_copy_and_do();
        DMA_BUF_SIZE = SIZE;
        hw_dma_write_aes_result();
    }
    return;
}



void decrypt_kbooti_body(void)

{
    uint uVar1;
    
    if (SIZE != 0) {
        aes_set_null_iv();
        AES_CMD = 0x13;
        hw_aes_setkey();
        while (0 < SIZE + -0x10) {
            SIZE = SIZE + -0x10;
            decrypt_kbooti_block();
            DMA_BUF_SIZE = 0x10;
            hw_dma_write_aes_result();
            INPUT = INPUT + 0x10;
            OUTPUT = OUTPUT + 0x10;
            uVar1 = AES_CMD;
            AES_CMD = uVar1 & 0xfffffffd;
        }
        decrypt_kbooti_block();
        DMA_BUF_SIZE = SIZE;
        hw_dma_write_aes_result();
    }
    return;
}



void decrypt_kbooti_block(void)

{
    DMA_BUF_SIZE = 0x14;
    DMA_ADDR = INPUT;
    hw_dma_read();
    block_shift_2();
    hw_aes_copy_and_do();
    return;
}

void hw_aes_dma_copy_and_do(void)
{
    DMA_ADDR = INPUT;
    hw_dma_read();
    hw_aes_copy_and_do();
}

void hw_aes_copy_and_do(void)
{
    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
    hw_aes_do();
}

void hw_aes_do(void)
{
    __builtin_setmode(AES_REG, 1);
    __builtin_dma(0x10, 0);
}

void hw_aes_setkey(void)
{
    __builtin_setmode(AES_REG, 2);
    __builtin_dma(0x10, 0);
}

void aes_set_null_iv(void)

{
    AES_IV[0] = 0;
    AES_IV[1] = 0;
    AES_IV[2] = 0;
    AES_IV[3] = 0;
    return;
}



void block_shift_2(void)

{
    uint uVar1;
    uint uVar2;
    
    uVar1 = DMA_BUF[0] >> 0x10;
    DMA_BUF[5]._0_2_ = (undefined2)DMA_BUF[1];
    DMA_BUF[0] = CONCAT22(DMA_BUF[5]._0_2_,(short)uVar1);
    uVar1 = DMA_BUF[1] >> 0x10;
    DMA_BUF[5]._0_2_ = (undefined2)DMA_BUF[2];
    DMA_BUF[1] = CONCAT22(DMA_BUF[5]._0_2_,(short)uVar1);
    uVar1 = DMA_BUF[2] >> 0x10;
    DMA_BUF[5]._0_2_ = (undefined2)DMA_BUF[3];
    DMA_BUF[2] = CONCAT22(DMA_BUF[5]._0_2_,(short)uVar1);
    DMA_BUF[5] = DMA_BUF[4] >> 0x10 | DMA_BUF[4] << 0x10;
    uVar2 = DMA_BUF[5];
    uVar1 = DMA_BUF[3] >> 0x10;
    DMA_BUF[5]._0_2_ = (undefined2)DMA_BUF[4];
    DMA_BUF[3] = CONCAT22(DMA_BUF[5]._0_2_,(short)uVar1);
    DMA_BUF[5] = uVar2;
    return;
}



// WARNING: Unknown calling convention -- yet parameter storage is locked

void aes_padding(void)

{
    uint uVar1;
    
    if (SIZE != 0) {
                    // address to PRNG_RESULT[0]
        R3 = 0x42d;
                    // address to DMA_BUF
        R4 = 0xe10;
        R5 = 0;
        while (uVar1 = SIZE - 4, uVar1 != 0) {
            if (0x7fffffff < uVar1) {
                uVar1 = PRNG_RESULT[0];
                if (SIZE == 2) {
                    R6 = uVar1 & 0xffff;
                    *(uint *)R4 = *(uint *)R4 & 0xffff0000;
                }
                else if (SIZE < 2) {
                    R6 = uVar1 & 0xffffff;
                    *(uint *)R4 = *(uint *)R4 & 0xff000000;
                }
                else {
                    R6 = uVar1 & 0xff;
                    *(uint *)R4 = *(uint *)R4 & 0xffffff00;
                }
                *(uint *)R4 = R6 | *(uint *)R4;
                R3 = R3 + 1;
                uVar1 = SIZE;
                break;
            }
            R5 = R5 + 1;
            R4 = R4 + 1;
            SIZE = uVar1;
        }
        while( true ) {
            SIZE = uVar1;
            R5 = R5 + 1;
            R4 = R4 + 1;
            if (4 < (int)R5) break;
            *(undefined4 *)R4 = *(undefined4 *)R3;
            R3 = R3 + 1;
            uVar1 = SIZE;
        }
    }
    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
    return;
}



void kirk_cmd0_decrypt_bootrom(void)

{
    uint uVar1;
    uint auVar2 [4];
    bool bVar3;
    
    if (PSP_KIRK_STATUS == 1) {
        PSP_KIRK_PHASE = 1;
        PSP_KIRK_RESULT = 0xd;
    }
    else {
        DMA_ADDR = PSP_KIRK_SRC + 0x10;
        DMA_BUF_SIZE = 2;
        DMA_BUF[0] = DMA_ADDR;
        hw_dma_read();
        KIRK_BODY_SIZE = __builtin_byteswap(DMA_BUF[0]);
        KIRK_BODY_SIZE = KIRK_BODY_SIZE & 0xffff;
        if (KIRK_BODY_SIZE == 0) {
LAB_rom_0000019b:
            PSP_KIRK_RESULT = 0x10;
            crash();
            return;
        }
        HW_AES_PARAM_ID = 1;
        auVar2 = HW_AES_PARAM;
        AES_KEY = auVar2;
        INPUT = PSP_KIRK_SRC + 0x10;
        SIZE = KIRK_BODY_SIZE;
        align16();
        SIZE = SIZE + 2;
        aes_set_null_iv();
        AES_CMD = 0x32;
        hw_aes_setkey();
        AES_END = SIZE;
        DMA_BUF_SIZE = 0x10;
        if (SIZE + -0x10 < 1) {
            DMA_BUF_SIZE = SIZE;
            DMA_BUF[0] = 0;
            DMA_BUF[1] = 0;
            DMA_BUF[2] = 0;
            DMA_BUF[3] = 0;
            hw_aes_dma_copy_and_do();
            bVar3 = (bool)kirk0_check_data_size();
            if (!bVar3) goto LAB_rom_0000019b;
        }
        else {
            SIZE = SIZE + -0x10;
            hw_aes_dma_copy_and_do();
            INPUT = INPUT + 0x10;
            uVar1 = AES_CMD;
            AES_CMD = uVar1 & 0xfffffffd;
            bVar3 = (bool)kirk0_check_data_size();
            if (!bVar3) goto LAB_rom_0000019b;
            aes_decrypt_ecb();
        }
        DMA_ADDR = PSP_KIRK_SRC;
        DMA_BUF_SIZE = 0x10;
        hw_dma_read();
        bVar3 = memcmp_cmac();
        if (!bVar3) {
            PSP_KIRK_RESULT = 1;
            crash();
            return;
        }
        SIZE = KIRK_BODY_SIZE;
        INPUT = PSP_KIRK_SRC + 0x10;
        OUTPUT = PSP_KIRK_DST;
        HW_AES_PARAM_ID = 0;
        auVar2 = HW_AES_PARAM;
        AES_KEY = auVar2;
        decrypt_kbooti_body();
        PSP_KIRK_RESULT = 0;
        PSP_KIRK_PHASE = 2;
    }
    return;
}



undefined __unused_kirk_aes_header_get_size(void)

{
    undefined uVar1;
    
    DMA_ADDR = PSP_KIRK_SRC + 0x10;
    uVar1 = DMA_ADDR == 0;
    DMA_BUF_SIZE = 2;
    DMA_BUF[0] = DMA_ADDR;
    hw_dma_read();
    DMA_BUF[0] = __builtin_byteswap(DMA_BUF[0]);
    DMA_BUF[0] = DMA_BUF[0] & 0xffff;
    return uVar1;
}



bool kirk0_check_data_size(void)

{
    DMA_BUF[0] = __builtin_byteswap(DMA_BUF[0]);
    DMA_BUF[0] = DMA_BUF[0] & 0xffff;
    return KIRK_BODY_SIZE == DMA_BUF[0];
}



void kirk_cmd1_decrypt_private(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (!bVar1) {
        return;
    }
    kirk_header_read_cmd_mode();
    if (DMA_BUF[0] == 0x1000000) {
        read_kirk_header();
        bVar1 = (bool)kirk_check_body_size_nonzero();
        if (bVar1) {
            HW_AES_PARAM_ID = 2;
            CUR_ENC_KEY = HW_AES_PARAM;
            kirk_header_read_ecdsa_flag();
            if (DMA_BUF[0] == 0x18) {
                set_kirk1_curve();
                kirk1_get_public_key();
                bVar1 = (bool)kirk_ecdsa_check_header();
                if (!bVar1) {
                    PSP_KIRK_PHASE = 1;
                    PSP_KIRK_RESULT = 3;
                    return;
                }
                bVar1 = (bool)kirk_ecdsa_check_body();
                if (!bVar1) {
                    PSP_KIRK_PHASE = 1;
                    PSP_KIRK_RESULT = 4;
                    return;
                }
            }
            else {
                bVar1 = (bool)kirk_check_cmac();
                if (!bVar1) {
                    PSP_KIRK_PHASE = 1;
                    return;
                }
            }
            kirk_aes_decrypt_body();
        }
    }
    else {
        PSP_KIRK_RESULT = 2;
    }
    PSP_KIRK_PHASE = 1;
    return;
}



void kirk1_get_public_key(void)

{
    uint auVar1 [4];
    uint uVar2;
    
    HW_CURVE_PARAM_ID = 0;
    auVar1 = HW_CURVE_PARAM;
    ECC_Px._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_Px[4] = uVar2;
    HW_CURVE_PARAM_ID = 1;
    auVar1 = HW_CURVE_PARAM;
    ECC_Py._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_Py[4] = uVar2;
    return;
}



void kirk_cmd2_dnas_encrypt(void)

{
    uint auVar1 [4];
    bool bVar2;
    
    bVar2 = (bool)hw_kirk_check_enabled();
    if (bVar2) {
        kirk_header_read_cmd_mode();
        if (DMA_BUF[0] != 0x2000000) {
            PSP_KIRK_RESULT = 2;
            kirk_set_phase1();
            return;
        }
        bVar2 = (bool)CheckKirkInitialized();
        if (bVar2) {
            read_kirk_header();
            bVar2 = (bool)kirk_check_body_size_nonzero();
            if (bVar2) {
                HW_AES_PARAM_ID = 3;
                CUR_ENC_KEY = HW_AES_PARAM;
                kirk_header_read_ecdsa_flag();
                ECDSA_MODE = DMA_BUF[0];
                if (DMA_BUF[0] == 0x18) {
                    set_kirk1_curve();
                    kirk2_get_public_key();
                    bVar2 = (bool)kirk_ecdsa_check_header();
                    if (!bVar2) {
                        PSP_KIRK_RESULT = 3;
                        kirk_set_phase1();
                        return;
                    }
                    bVar2 = (bool)kirk_ecdsa_check_body();
                    if (!bVar2) {
                        PSP_KIRK_RESULT = 4;
                        kirk_set_phase1();
                        return;
                    }
LAB_rom_000001fb:
                    AES_KEY = CUR_ENC_KEY;
                    kirk_aes_decrypt_key();
                    SIZE = KIRK_DATA_OFFSET + 0x90;
                    INPUT = PSP_KIRK_SRC;
                    OUTPUT = PSP_KIRK_DST;
                    dma_memcpy();
                    DMA_ADDR = PSP_KIRK_DST + 0x60;
                    DMA_BUF[1] = DMA_ADDR;
                    DMA_BUF[0] = 0x3000000;
                    DMA_BUF_SIZE = 4;
                    hw_dma_write();
                    DMA_ADDR = PSP_KIRK_DST + 100;
                    DMA_BUF[1] = DMA_ADDR;
                    DMA_BUF_SIZE = 4;
                    hw_dma_read();
                    if (ECDSA_MODE == 0x19) {
                        DMA_BUF[0] = DMA_BUF[0] | 0x1000000;
                    }
                    else {
                        DMA_BUF[0] = DMA_BUF[0] & 0xfeffffff;
                    }
                    hw_dma_write();
                    INPUT = PSP_KIRK_SRC;
                    OUTPUT = PSP_KIRK_DST;
                    kirk2_set_args();
                    aes_decrypt_cbc();
                    kirk_reseed_prng_2();
                    DMA_BUF._0_16_ = (undefined  [16])PRNG_RESULT._4_16_;
                    aes_set_perconsole_key_0();
                    AES_CMD = 0;
                    hw_aes_setkey();
                    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
                    hw_aes_do();
                    DMA_BUF[0] = PSP_KIRK_DST;
                    hw_aes_output();
                    if (ECDSA_MODE != 0x19) {
                        auVar1 = AES_RESULT;
                        AES_IV = auVar1;
                        kirk_reseed_prng_2();
                        DMA_BUF._0_16_ = (undefined  [16])PRNG_RESULT._4_16_;
                        aes_set_perconsole_key_0();
                        AES_CMD = 0x12;
                        hw_aes_setkey();
                        AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
                        hw_aes_do();
                        DMA_BUF[0] = PSP_KIRK_DST + 0x10;
                        hw_aes_output();
                    }
                    aes_set_perconsole_key_0();
                    CUR_ENC_KEY = AES_RESULT;
                    INPUT = PSP_KIRK_DST;
                    OUTPUT = PSP_KIRK_DST;
                    AES_KEY = CUR_ENC_KEY;
                    kirk_cmd2_decrypt_key();
                    CUR_ENC_KEY = AES_RESULT;
                    kirk2_set_args();
                    aes_encrypt_ecb();
                    if (ECDSA_MODE != 0x19) {
                        aes_set_perconsole_key_0();
                        DMA_ADDR = PSP_KIRK_DST;
                        DMA_BUF_SIZE = 0x20;
                        hw_dma_read();
                        AES_IV = (uint  [4])DMA_BUF._0_16_;
                        AES_CMD = 0x13;
                        hw_aes_setkey();
                        AES_SRC_BUF = (uint  [4])DMA_BUF._16_16_;
                        hw_aes_do();
                        auVar1 = AES_RESULT;
                        AES_KEY = auVar1;
                        INPUT = PSP_KIRK_DST + 0x60;
                        SIZE = 0x30;
                        aes_cmac();
                        DMA_BUF[0] = PSP_KIRK_DST + 0x20;
                        hw_aes_output();
                        SIZE = KIRK_DATA_OFFSET + 0x30;
                        SIZE = KIRK_BODY_SIZE + SIZE;
                        align16();
                        INPUT = PSP_KIRK_DST + 0x60;
                        aes_cmac();
                        DMA_BUF[0] = PSP_KIRK_DST + 0x30;
                        hw_aes_output();
                        kirk_result_0();
                        return;
                    }
                    set_kirk1_curve();
                    kirk2_get_private_key();
                    INPUT = PSP_KIRK_DST + 0x60;
                    SIZE = 0x30;
                    ec_sign_gen_m_r();
                    OUTPUT = PSP_KIRK_DST + 0x10;
                    dma_ec_output_point();
                    SIZE = KIRK_DATA_OFFSET + 0x30;
                    SIZE = KIRK_BODY_SIZE + SIZE;
                    align16();
                    INPUT = PSP_KIRK_DST + 0x60;
                    ec_sign_gen_m_r();
                    OUTPUT = PSP_KIRK_DST + 0x38;
                    dma_ec_output_point();
                    kirk_result_0();
                    return;
                }
                bVar2 = (bool)kirk_check_cmac();
                if (bVar2) goto LAB_rom_000001fb;
            }
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void __unused_kirk_result_2(void)

{
    PSP_KIRK_RESULT = 2;
    kirk_set_phase1();
    return;
}



void kirk_result_0(void)

{
    PSP_KIRK_RESULT = 0;
    kirk_set_phase1();
    return;
}



void kirk_set_phase1(void)

{
    PSP_KIRK_PHASE = 1;
    return;
}



bool kirk2_set_args(void)

{
    int iVar1;
    
    SIZE = KIRK_BODY_SIZE;
    align16();
    INPUT = KIRK_DATA_OFFSET + INPUT + 0x90;
    iVar1 = OUTPUT + 0x90;
    OUTPUT = KIRK_DATA_OFFSET + iVar1;
    return iVar1 == 0;
}



void hw_aes_output(void)

{
    DMA_ADDR = DMA_BUF[0];
    DMA_BUF._0_16_ = (undefined  [16])AES_RESULT;
    DMA_BUF_SIZE = 0x10;
    hw_dma_write();
    return;
}



void kirk2_get_public_key(void)

{
    uint auVar1 [4];
    uint uVar2;
    
    HW_CURVE_PARAM_ID = 2;
    auVar1 = HW_CURVE_PARAM;
    ECC_Px._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_Px[4] = uVar2;
    HW_CURVE_PARAM_ID = 3;
    auVar1 = HW_CURVE_PARAM;
    ECC_Py._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_Py[4] = uVar2;
    return;
}



void kirk2_get_private_key(void)

{
    uint auVar1 [4];
    uint uVar2;
    
    HW_CURVE_PARAM_ID = 4;
    auVar1 = HW_CURVE_PARAM;
    ECC_S._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_S[4] = uVar2;
    return;
}



void kirk_cmd3_dnas_decrypt(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (!bVar1) {
        return;
    }
    kirk_header_read_cmd_mode();
    if (DMA_BUF[0] == 0x3000000) {
        read_kirk_header();
        bVar1 = (bool)kirk_check_body_size_nonzero();
        if (bVar1) {
            aes_set_perconsole_key_0();
            CUR_ENC_KEY = AES_RESULT;
            kirk_header_read_ecdsa_flag();
            if (DMA_BUF[0] == 0x18) {
                set_kirk1_curve();
                kirk3_get_public_key();
                bVar1 = (bool)kirk_ecdsa_check_header();
                if (!bVar1) {
                    PSP_KIRK_PHASE = 1;
                    PSP_KIRK_RESULT = 3;
                    return;
                }
                bVar1 = (bool)kirk_ecdsa_check_body();
                if (!bVar1) {
                    PSP_KIRK_PHASE = 1;
                    PSP_KIRK_RESULT = 4;
                    return;
                }
            }
            else {
                bVar1 = (bool)kirk_check_cmac();
                if (!bVar1) {
                    PSP_KIRK_PHASE = 1;
                    return;
                }
            }
            kirk_aes_decrypt_body();
        }
    }
    else {
        PSP_KIRK_RESULT = 2;
    }
    PSP_KIRK_PHASE = 1;
    return;
}



void kirk3_get_public_key(void)

{
    uint auVar1 [4];
    uint uVar2;
    
    HW_CURVE_PARAM_ID = 5;
    auVar1 = HW_CURVE_PARAM;
    ECC_Px._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_Px[4] = uVar2;
    HW_CURVE_PARAM_ID = 6;
    auVar1 = HW_CURVE_PARAM;
    ECC_Py._0_16_ = (undefined  [16])auVar1;
    uVar2 = DAT_ram_000004b5;
    ECC_Py[4] = uVar2;
    return;
}



void kirk_cmd4_encrypt_static(void)

{
    uint uVar1;
    uint auVar2 [4];
    bool bVar3;
    
    bVar3 = (bool)hw_kirk_check_enabled();
    if (bVar3) {
        kirk_aes_read_header();
        if ((DMA_BUF[0] == 0x4000000) && (DMA_BUF[3] == 0)) {
            bVar3 = (bool)CheckKirkInitialized();
            if (bVar3) {
                kirk_header_read_body_size();
                bVar3 = (bool)kirk_check_body_size_nonzero();
                if (bVar3) {
                    kirk_aes_read_header();
                    if ((int)DMA_BUF[2] < 0x40) {
                        HW_AES_PARAM_ID = DMA_BUF[2] + 4;
                        auVar2 = HW_AES_PARAM;
                        AES_KEY = auVar2;
                        CUR_ENC_KEY = HW_AES_PARAM;
                        uVar1 = KEY_SEED3;
                        if (((uVar1 == 0x1f) && (0x1f < (int)DMA_BUF[2])) &&
                           ((int)DMA_BUF[2] < 0x30)) {
                            CUR_ENC_KEY[3] = CUR_ENC_KEY[3] ^ 0xffffffff;
                        }
                        if ((0x27 < (int)DMA_BUF[2]) && ((int)DMA_BUF[2] < 0xb0)) {
                            uVar1 = KEY_SEED3;
                            CUR_ENC_KEY[0] = uVar1 ^ CUR_ENC_KEY[0];
                        }
                        AES_KEY = CUR_ENC_KEY;
                        DMA_BUF[2] = DMA_BUF[2] + 4;
                        kirk4_process();
                    }
                    else {
                        PSP_KIRK_RESULT = 0xe;
                    }
                }
            }
        }
        else {
            PSP_KIRK_RESULT = 2;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk4_process(void)

{
    INPUT = PSP_KIRK_SRC;
    OUTPUT = PSP_KIRK_DST;
    SIZE = 0x14;
    dma_memcpy();
    DMA_BUF[0] = 0x5000000;
    DMA_ADDR = PSP_KIRK_DST;
    DMA_BUF_SIZE = 4;
    hw_dma_write();
    aes_set_null_iv();
    INPUT = PSP_KIRK_SRC + 0x14;
    OUTPUT = PSP_KIRK_DST + 0x14;
    SIZE = KIRK_BODY_SIZE;
    aes_encrypt_cbc();
    PSP_KIRK_RESULT = 0;
    return;
}



void kirk_cmd5_encrypt_perconsole(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (bVar1) {
        kirk_aes_read_header();
        if ((DMA_BUF[0] == 0x4000000) && (DMA_BUF[3] == 0x10000)) {
            bVar1 = (bool)CheckKirkInitialized();
            if (bVar1) {
                kirk_header_read_body_size();
                bVar1 = (bool)kirk_check_body_size_nonzero();
                if (bVar1) {
                    AES_KEYSEED = 1;
                    aes_set_perconsole_key();
                    CUR_ENC_KEY = AES_RESULT;
                    kirk4_process();
                }
            }
        }
        else {
            PSP_KIRK_RESULT = 2;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void kirk_cmd6_encrypt_user(void)

{
    uint auVar1 [4];
    bool bVar2;
    
    bVar2 = (bool)hw_kirk_check_enabled();
    if (bVar2) {
        kirk_aes_read_header();
        if ((DMA_BUF[0] == 0x4000000) && (DMA_BUF[3] == 0x20000)) {
            bVar2 = (bool)CheckKirkInitialized();
            if (bVar2) {
                kirk_header_read_body_size();
                bVar2 = (bool)kirk_check_body_size_nonzero();
                if (bVar2) {
                    INPUT = PSP_KIRK_SRC;
                    OUTPUT = PSP_KIRK_DST;
                    SIZE = 0x24;
                    dma_memcpy();
                    DMA_BUF[0] = 0x5000000;
                    DMA_ADDR = PSP_KIRK_DST;
                    DMA_BUF_SIZE = 4;
                    hw_dma_write();
                    kirk_reseed_prng_2();
                    AES_KEYSEED = 2;
                    aes_set_perconsole_key();
                    auVar1 = (uint  [4])PRNG_RESULT._4_16_;
                    AES_SRC_BUF = auVar1;
                    AES_CMD = 0;
                    hw_aes_setkey();
                    hw_aes_do();
                    SIZE = KIRK_BODY_SIZE;
                    DMA_ADDR = PSP_KIRK_SRC + 0x24;
                    if ((int)KIRK_BODY_SIZE < 0x10) {
                        DMA_BUF_SIZE = KIRK_BODY_SIZE;
                    }
                    else {
                        DMA_BUF_SIZE = 0x10;
                    }
                    DMA_BUF[0] = DMA_ADDR;
                    hw_dma_read();
                    R11 = DMA_BUF[0];
                    unique0x1000006c = DMA_BUF[1];
                    unique0x10000070 = DMA_BUF[2];
                    unique0x10000074 = DMA_BUF[3];
                    DMA_BUF[0] = PSP_KIRK_DST + 0x24;
                    hw_aes_output();
                    DMA_ADDR = PSP_KIRK_DST + 0x14;
                    DMA_BUF_SIZE = 0x10;
                    DMA_BUF[0] = DMA_ADDR;
                    hw_dma_read();
                    auVar1[0] = DMA_BUF[0];
                    auVar1[1] = DMA_BUF[1];
                    auVar1[2] = DMA_BUF[2];
                    auVar1[3] = DMA_BUF[3];
                    AES_KEY = auVar1;
                    auVar1 = (uint  [4])PRNG_RESULT._4_16_;
                    AES_SRC_BUF = auVar1;
                    AES_CMD = 0;
                    hw_aes_setkey();
                    hw_aes_do();
                    CUR_ENC_KEY = AES_RESULT;
                    INPUT = PSP_KIRK_SRC + 0x24;
                    OUTPUT = PSP_KIRK_DST + 0x34;
                    kirk6_process();
                    PSP_KIRK_RESULT = 0;
                }
            }
        }
        else {
            PSP_KIRK_RESULT = 2;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void kirk6_process(void)

{
    uint uVar1;
    uint auVar2 [4];
    
    aes_set_null_iv();
    kirk_reseed_prng_2();
    AES_KEY = CUR_ENC_KEY;
    AES_CMD = 0x12;
    hw_aes_setkey();
    DMA_BUF_SIZE = 0x10;
    while (0 < SIZE + -0x10) {
        AES_SRC_BUF = _R11;
        SIZE = SIZE + -0x10;
        hw_aes_do();
        auVar2 = AES_RESULT;
        AES_IV = auVar2;
        INPUT = INPUT + 0x10;
        if (SIZE < 0x10) {
            DMA_BUF_SIZE = SIZE;
        }
        hw_dma_read_input();
        R11 = DMA_BUF[0];
        R12 = DMA_BUF[1];
        R13 = DMA_BUF[2];
        R14 = DMA_BUF[3];
        DMA_BUF_SIZE = 0x10;
        hw_dma_write_aes_result();
        OUTPUT = OUTPUT + 0x10;
        uVar1 = AES_CMD;
        AES_CMD = uVar1 & 0xfffffffd;
    }
    DMA_BUF[0] = R11;
    DMA_BUF[1] = R12;
    DMA_BUF[2] = R13;
    DMA_BUF[3] = R14;
    aes_padding();
    uVar1 = AES_CMD;
    AES_CMD = uVar1 | 2;
    hw_aes_do();
    DMA_BUF_SIZE = 0x10;
    hw_dma_write_aes_result();
    return;
}



void kirk_cmd7_decrypt_static(void)

{
    uint uVar1;
    uint auVar2 [4];
    bool bVar3;
    
    bVar3 = (bool)hw_kirk_check_enabled();
    if (bVar3) {
        kirk_aes_read_header();
        if ((DMA_BUF[0] == 0x5000000) && (DMA_BUF[3] == 0)) {
            kirk_header_read_body_size();
            bVar3 = (bool)kirk_check_body_size_nonzero();
            if (bVar3) {
                kirk_aes_read_header();
                if ((int)DMA_BUF[2] < 0x80) {
                    HW_AES_PARAM_ID = DMA_BUF[2] + 4;
                    auVar2 = HW_AES_PARAM;
                    AES_KEY = auVar2;
                    CUR_ENC_KEY = HW_AES_PARAM;
                    uVar1 = KEY_SEED3;
                    if ((((uVar1 == 0x1f) && (0x1f < (int)DMA_BUF[2])) && ((int)DMA_BUF[2] < 0x7c))
                       && (((int)DMA_BUF[2] < 0x30 || (0x6b < (int)DMA_BUF[2])))) {
                        CUR_ENC_KEY[3] = CUR_ENC_KEY[3] ^ 0xffffffff;
                    }
                    if (((0x27 < (int)DMA_BUF[2]) && ((int)DMA_BUF[2] < 0x7c)) &&
                       (((int)DMA_BUF[2] < 0x30 || (0x73 < (int)DMA_BUF[2])))) {
                        uVar1 = KEY_SEED3;
                        CUR_ENC_KEY[0] = uVar1 ^ CUR_ENC_KEY[0];
                    }
                    AES_KEY = CUR_ENC_KEY;
                    DMA_BUF[2] = DMA_BUF[2] + 4;
                    INPUT = 0x14;
                    kirk_aes_decrypt();
                }
                else {
                    PSP_KIRK_RESULT = 0xe;
                }
            }
        }
        else {
            PSP_KIRK_RESULT = 2;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_cmd8_decrypt_perconsole(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (bVar1) {
        kirk_aes_read_header();
        if ((DMA_BUF[0] == 0x5000000) && (DMA_BUF[3] == 0x10000)) {
            kirk_header_read_body_size();
            bVar1 = (bool)kirk_check_body_size_nonzero();
            if (bVar1) {
                AES_KEYSEED = 1;
                aes_set_perconsole_key();
                INPUT = 0x14;
                kirk_aes_decrypt();
            }
        }
        else {
            PSP_KIRK_RESULT = 2;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_cmd9_decrypt_user(void)

{
    uint auVar1 [4];
    bool bVar2;
    
    bVar2 = (bool)hw_kirk_check_enabled();
    if (bVar2) {
        kirk_aes_read_header();
        if ((DMA_BUF[0] == 0x5000000) && (DMA_BUF[3] == 0x20000)) {
            kirk_header_read_body_size();
            bVar2 = (bool)kirk_check_body_size_nonzero();
            if (bVar2) {
                AES_KEYSEED = 2;
                aes_set_perconsole_key();
                DMA_ADDR = PSP_KIRK_SRC + 0x14;
                DMA_BUF_SIZE = 0x20;
                DMA_BUF[0] = DMA_ADDR;
                hw_dma_read();
                AES_CMD = 1;
                AES_SRC_BUF = (uint  [4])DMA_BUF._16_16_;
                hw_aes_setkey();
                hw_aes_do();
                AES_CMD = 0;
                auVar1[0] = DMA_BUF[0];
                auVar1[1] = DMA_BUF[1];
                auVar1[2] = DMA_BUF[2];
                auVar1[3] = DMA_BUF[3];
                AES_KEY = auVar1;
                auVar1 = AES_RESULT;
                AES_SRC_BUF = auVar1;
                hw_aes_setkey();
                hw_aes_do();
                auVar1 = AES_RESULT;
                AES_KEY = auVar1;
                INPUT = 0x34;
                kirk_aes_decrypt();
            }
        }
        else {
            PSP_KIRK_RESULT = 2;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}

void ec_sign_gen_m_r(void)
{
    hw_sha1_do();
    ECC_M._0_16_ = SHA1_RESULT._0_16_;
    ECC_M[4] = SHA1_RESULT[4];
    ec_sign_gen_r();
}

void ec_sign_gen_r(void)

{
    uint uVar1;
    undefined auVar2 [16];
    
    do {
        prng_generate();
        auVar2 = (undefined  [16])MATH_RESULT._12_16_;
        ECC_R._0_16_ = (undefined  [16])auVar2;
        uVar1 = MATH_RESULT[7];
        ECC_R[4] = uVar1;
        ECC_OP = 0;
        hw_ec_do();
        uVar1 = ECC_RESULT;
    } while (uVar1 != 0);
    UNK_FLAGS = UNK_FLAGS & 0xfffffffd;
    return;
}



void prng_generate(void)

{
    undefined auVar1 [16];
    uint uVar2;
    bool bVar3;
    
    do {
        if (UNK_FLAGS == 1) {
            set_other_prime();
        }
        else {
            set_kirk1_prime();
        }
        MATH_IN[0] = 0;
        MATH_IN[1] = 0;
        MATH_IN[2] = 0;
        MATH_IN[3] = 0;
        MATH_IN[4] = 0;
        MATH_IN[5] = 0;
        kirk_reseed_prng_2();
        auVar1 = (undefined  [16])PRNG_RESULT._0_16_;
        MATH_IN._24_16_ = (undefined  [16])auVar1;
        uVar2 = PRNG_RESULT[4];
        MATH_IN[10] = uVar2;
        kirk_reseed_prng_2();
        auVar1 = (undefined  [16])PRNG_RESULT._0_16_;
        MATH_IN._44_16_ = (undefined  [16])auVar1;
        uVar2 = PRNG_RESULT[4];
        MATH_IN[15] = uVar2;
        uVar2 = MATH_REG;
        __builtin_setmode(uVar2,1);
        __builtin_dma(0x400,0);
        R13 = 5;
        R11 = 0x47d;
        bVar3 = (bool)buffer_is_nonzero();
    } while (!bVar3);
    return;
}



bool ec_sign_hash_and_check(void)

{
    uint uVar1;
    undefined auVar2 [16];
    
    if (SIZE != 0) {
        hw_sha1_do();
        if (!ec_sign_check()) {
            return true;
        }
    }
    return false;
}



bool ec_sign_check(void)

{
    uint uVar1;
    undefined auVar2 [16];
    
    auVar2 = (undefined  [16])SHA1_RESULT._0_16_;
    ECC_M._0_16_ = (undefined  [16])auVar2;
    uVar1 = SHA1_RESULT[4];
    ECC_M[4] = uVar1;
    ECC_OP = 1;
    hw_ec_do();
    uVar1 = ECC_RESULT;
    return uVar1 == 0;
}



void ec_dma_read_scalars(void)

{
    DMA_BUF_SIZE = 0x14;
    hw_dma_read_input();
    ECC_R._0_16_ = (undefined  [16])DMA_BUF._0_16_;
    ECC_R[4] = DMA_BUF[4];
    INPUT = INPUT + 0x14;
    hw_dma_read_input();
    ECC_S._0_16_ = (undefined  [16])DMA_BUF._0_16_;
    ECC_S[4] = DMA_BUF[4];
    return;
}



void dma_ec_output_point(void)

{
    DMA_BUF._0_16_ = (undefined  [16])ECC_RESULT_X._0_16_;
    DMA_BUF[4] = ECC_RESULT_X[4];
    DMA_BUF_SIZE = 0x14;
    DMA_ADDR = OUTPUT;
    hw_dma_write();
    DMA_BUF._0_16_ = (undefined  [16])ECC_RESULT_Y._0_16_;
    DMA_BUF[4] = ECC_RESULT_Y[4];
    DMA_ADDR = OUTPUT + 0x14;
    OUTPUT = DMA_ADDR;
    hw_dma_write();
    return;
}



void hw_sha1_do(void)

{
    if (SIZE != 0) {
        SHA1_IN_SIZE = SIZE;
        DMA_BUF_SIZE = 4;
        SIZE = SIZE + 3U >> 2 | (SIZE + 3U) * 0x40000000;
        do {
            hw_dma_read_input();
            SHA1_IN_DATA = DMA_BUF[0];
            INPUT = INPUT + 4;
            SIZE = SIZE + -1;
        } while (SIZE != 0);
        __builtin_dma(4,0);
        SIZE = 0;
    }
    return;
}



void hw_ec_do(void)

{
    uint uVar1;
    
    uVar1 = ECC_REG;
    __builtin_setmode(uVar1,1);
    __builtin_dma(0,0x100);
    return;
}



void set_kirk1_curve(void)

{
    ECC_A[0] = 0xffffffff;
    ECC_A[1] = 0xffffffff;
    ECC_A[2] = 1;
    ECC_A[3] = 0xffffffff;
    ECC_A[4] = 0xfffffffc;
    ECC_B[0] = 0x65d1488c;
    ECC_B[1] = 0x359e234;
    ECC_B[2] = 0xadc95bd3;
    ECC_B[3] = 0x908014bd;
    ECC_B[4] = 0x91a525f9;
    ECC_P[0] = 0xffffffff;
    ECC_P[1] = 0xffffffff;
    ECC_P[2] = 1;
    ECC_P[3] = 0xffffffff;
    ECC_P[4] = 0xffffffff;
    ECC_N[0] = 0xffffffff;
    ECC_N[1] = 0xffffffff;
    ECC_N[2] = 0x1b5c6;
    ECC_N[3] = 0x17f290ea;
    ECC_N[4] = 0xe1dbad8f;
    ECC_Gx[0] = 0x2259acee;
    ECC_Gx[1] = 0x15489cb0;
    ECC_Gx[2] = 0x96a882f0;
    ECC_Gx[3] = 0xae1cf9fd;
    ECC_Gx[4] = 0x8ee5f8fa;
    ECC_Gy[0] = 0x60435845;
    ECC_Gy[1] = 0x6d0a1cb2;
    ECC_Gy[2] = 0x908de90f;
    ECC_Gy[3] = 0x27d75c82;
    ECC_Gy[4] = 0xbec108c0;
    ECC_UNK1[0] = 4;
    ECC_UNK1[1] = 0xfffffffc;
    ECC_UNK1[2] = 4;
    ECC_UNK1[3] = 0;
    ECC_UNK1[4] = 0xfffffffd;
    ECC_UNK2[0] = 0xbcb8c536;
    ECC_UNK2[1] = 0x6c7b11e5;
    ECC_UNK2[2] = 0xa251c108;
    ECC_UNK2[3] = 0xc0b3500c;
    ECC_UNK2[4] = 0x60f7e9f7;
    return;
}



void set_other_curve(void)

{
    ECC_A[0] = 0xffffffff;
    ECC_A[1] = 0xffffffff;
    ECC_A[2] = 1;
    ECC_A[3] = 0xffffffff;
    ECC_A[4] = 0xfffffffc;
    ECC_B[0] = 0xa68bedc3;
    ECC_B[1] = 0x3418029c;
    ECC_B[2] = 0x1d3ce33b;
    ECC_B[3] = 0x9a321fcc;
    ECC_B[4] = 0xbb9e0f0b;
    ECC_P[0] = 0xffffffff;
    ECC_P[1] = 0xffffffff;
    ECC_P[2] = 1;
    ECC_P[3] = 0xffffffff;
    ECC_P[4] = 0xffffffff;
    ECC_N[0] = 0xffffffff;
    ECC_N[1] = 0xfffffffe;
    ECC_N[2] = 0xffffb5ae;
    ECC_N[3] = 0x3c523e63;
    ECC_N[4] = 0x944f2127;
    ECC_Gx[0] = 0x128ec425;
    ECC_Gx[1] = 0x6487fd8f;
    ECC_Gx[2] = 0xdf64e243;
    ECC_Gx[3] = 0x7bc0a1f6;
    ECC_Gx[4] = 0xd5afde2c;
    ECC_Gy[0] = 0x5958557e;
    ECC_Gy[1] = 0xb1db0012;
    ECC_Gy[2] = 0x60425524;
    ECC_Gy[3] = 0xdbc379d5;
    ECC_Gy[4] = 0xac5f4adf;
    ECC_UNK1[0] = 4;
    ECC_UNK1[1] = 0xfffffffc;
    ECC_UNK1[2] = 4;
    ECC_UNK1[3] = 0;
    ECC_UNK1[4] = 0xfffffffd;
    ECC_UNK2[0] = 0x9ceee277;
    ECC_UNK2[1] = 0xb4d7bac8;
    ECC_UNK2[2] = 0xe17d0c9e;
    ECC_UNK2[3] = 0x5b6bb2a8;
    ECC_UNK2[4] = 0x64d06c1c;
    return;
}



void set_kirk1_prime(void)

{
    MATH_IN_X[0] = 0;
    MATH_IN_X[1] = 0;
    MATH_IN_X[2] = 0;
    MATH_IN_X[3] = 0xffffffff;
    MATH_IN_X[4] = 0xffffffff;
    MATH_IN_X[5] = 0x1b5c6;
    MATH_IN_X[6] = 0x17f290ea;
    MATH_IN_X[7] = 0xe1dbad8f;
    return;
}



void set_other_prime(void)

{
    MATH_IN_X[0] = 0;
    MATH_IN_X[1] = 0;
    MATH_IN_X[2] = 0;
    MATH_IN_X[3] = 0xffffffff;
    MATH_IN_X[4] = 0xfffffffe;
    MATH_IN_X[5] = 0xffffb5ae;
    MATH_IN_X[6] = 0x3c523e63;
    MATH_IN_X[7] = 0x944f2127;
    return;
}



void read_kirk_header(void)

{
    DMA_ADDR = PSP_KIRK_SRC + 0x70;
    DMA_BUF_SIZE = 8;
    DMA_BUF[0] = DMA_ADDR;
    hw_dma_read();
    DMA_BUF[0] = __builtin_byteswap(DMA_BUF[0]);
    KIRK_BODY_SIZE = DMA_BUF[0];
    DMA_BUF[1] = __builtin_byteswap(DMA_BUF[1]);
    KIRK_DATA_OFFSET = DMA_BUF[1];
    return;
}



void kirk_header_read_body_size(void)

{
    DMA_ADDR = PSP_KIRK_SRC + 0x10;
    DMA_BUF_SIZE = 4;
    DMA_BUF[0] = DMA_ADDR;
    hw_dma_read();
    DMA_BUF[0] = __builtin_byteswap(DMA_BUF[0]);
    KIRK_BODY_SIZE = DMA_BUF[0];
    return;
}



undefined kirk_check_cmac(void)

{
    bool bVar1;
    
    AES_KEY = CUR_ENC_KEY;
    bVar1 = (bool)kirk_cmac_check_header();
    if (bVar1) {
        bVar1 = (bool)kirk_cmac_check_body();
        if (bVar1) {
            return 1;
        }
        FUN_00001670();
    }
    return 0;
}



void FUN_00001670(void)

{
    DMA_ADDR = PSP_KIRK_SRC + 0x68;
    DMA_BUF_SIZE = 4;
    DMA_BUF[0] = DMA_ADDR;
    hw_dma_read();
    if (DMA_BUF[0] == 0x18) {
        kirk_set_size_offset_body();
        OUTPUT = PSP_KIRK_SRC;
        dma_bzero();
    }
    return;
}



void kirk_cmac_check_header(void)

{
    uint uVar1;
    bool bVar2;
    
    do_aeshwdecrypt_and_encrypt_plain();
    INPUT = PSP_KIRK_SRC + 0x60;
    aes_set_null_iv();
    AES_CMD = 0x32;
    hw_aes_setkey();
    AES_END = 0x30;
    DMA_BUF_SIZE = 0x10;
    hw_aes_dma_copy_and_do();
    INPUT = INPUT + 0x10;
    uVar1 = AES_CMD;
    AES_CMD = uVar1 & 0xfffffffd;
    hw_aes_dma_copy_and_do();
    INPUT = INPUT + 0x10;
    DMA_BUF[0] = __builtin_byteswap(DMA_BUF[0]);
    if ((DMA_BUF[0] == KIRK_BODY_SIZE) &&
       (DMA_BUF[1] = __builtin_byteswap(DMA_BUF[1]), DMA_BUF[1] == KIRK_DATA_OFFSET)) {
        hw_aes_dma_copy_and_do();
        INPUT = PSP_KIRK_SRC + 0x20;
        DMA_BUF_SIZE = 0x10;
        hw_dma_read_input();
        bVar2 = memcmp_cmac();
        if (bVar2) {
            return;
        }
    }
    PSP_KIRK_RESULT = 3;
    return;
}



void kirk_cmac_check_body(void)

{
    bool bVar1;
    
    INPUT = PSP_KIRK_SRC + 0x60;
    kirk_set_size_offset_header2();
    aes_cmac();
    INPUT = PSP_KIRK_SRC + 0x30;
    DMA_BUF_SIZE = 0x10;
    hw_dma_read_input();
    bVar1 = memcmp_cmac();
    if (!bVar1) {
        PSP_KIRK_RESULT = 4;
    }
    return;
}



void kirk_ecdsa_check_header(void)

{
    bool bVar1;
    
    INPUT = PSP_KIRK_SRC + 0x10;
    ec_dma_read_scalars();
    INPUT = PSP_KIRK_SRC + 0x60;
    SIZE = 0x30;
    SHA1_IN_SIZE = 0x30;
    DMA_BUF_SIZE = 0x20;
    hw_dma_read_input();
    SHA1_IN_DATA = DMA_BUF[0];
    SHA1_IN_DATA = DMA_BUF[1];
    SHA1_IN_DATA = DMA_BUF[2];
    SHA1_IN_DATA = DMA_BUF[3];
    SHA1_IN_DATA = DMA_BUF[4];
    SHA1_IN_DATA = DMA_BUF[5];
    SHA1_IN_DATA = DMA_BUF[6];
    SHA1_IN_DATA = DMA_BUF[7];
    DMA_BUF[4] = __builtin_byteswap(DMA_BUF[4]);
    if ((DMA_BUF[4] == KIRK_BODY_SIZE) &&
       (DMA_BUF[5] = __builtin_byteswap(DMA_BUF[5]), DMA_BUF[5] == KIRK_DATA_OFFSET)) {
        INPUT = INPUT + 0x20;
        DMA_BUF_SIZE = 0x10;
        hw_dma_read_input();
        SHA1_IN_DATA = DMA_BUF[0];
        SHA1_IN_DATA = DMA_BUF[1];
        SHA1_IN_DATA = DMA_BUF[2];
        SHA1_IN_DATA = DMA_BUF[3];
        __builtin_dma(4,0);
        bVar1 = (bool)ec_sign_check();
        if (bVar1) {
            return;
        }
    }
    PSP_KIRK_RESULT = 3;
    return;
}



void kirk_ecdsa_check_body(void)

{
    bool bVar1;
    
    INPUT = PSP_KIRK_SRC + 0x38;
    ec_dma_read_scalars();
    INPUT = PSP_KIRK_SRC + 0x60;
    kirk_set_size_offset_header2();
    bVar1 = (bool)ec_sign_hash_and_check();
    if (!bVar1) {
        FUN_00001670();
        PSP_KIRK_RESULT = 4;
    }
    return;
}



void kirk_aes_decrypt(void)

{
    INPUT = PSP_KIRK_SRC;
    OUTPUT = PSP_KIRK_DST;
    SIZE = KIRK_BODY_SIZE;
    aes_decrypt_cbc();
    PSP_KIRK_RESULT = 0;
    return;
}



void kirk_aes_decrypt_body(void)

{
    AES_KEY = CUR_ENC_KEY;
    kirk_aes_decrypt_key();
    INPUT = KIRK_DATA_OFFSET + PSP_KIRK_SRC;
    INPUT = INPUT + 0x90;
    OUTPUT = PSP_KIRK_DST;
    SIZE = KIRK_BODY_SIZE;
    aes_decrypt_cbc();
    PSP_KIRK_RESULT = 0;
    return;
}



void kirk_set_size_offset_body(void)

{
    SIZE = KIRK_BODY_SIZE + 0x90;
    SIZE = KIRK_DATA_OFFSET + SIZE;
    align16();
    return;
}



void kirk_set_size_offset_header2(void)

{
    SIZE = KIRK_BODY_SIZE + 0x30;
    SIZE = KIRK_DATA_OFFSET + SIZE;
    align16();
    return;
}



void kirk_header_read_ecdsa_flag(void)

{
    DMA_BUF[0] = PSP_KIRK_SRC + 0x64;
    DMA_ADDR = DMA_BUF[0];
    DMA_BUF_SIZE = 4;
    hw_dma_read();
    return;
}



void kirk_header_read_cmd_mode(void)

{
    DMA_BUF[0] = PSP_KIRK_SRC + 0x60;
    DMA_ADDR = DMA_BUF[0];
    DMA_BUF_SIZE = 4;
    hw_dma_read();
    return;
}



void do_aeshwdecrypt_and_encrypt_plain(void)

{
    uint uVar1;
    uint auVar2 [4];
    
    DMA_ADDR = PSP_KIRK_SRC;
    DMA_BUF_SIZE = 0x20;
    hw_dma_read();
    aes_set_null_iv();
    AES_CMD = 0x13;
    hw_aes_setkey();
    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
    hw_aes_do();
    AES_SRC_BUF = (uint  [4])DMA_BUF._16_16_;
    uVar1 = AES_CMD;
    AES_CMD = uVar1 & 0xfffffffd;
    hw_aes_do();
    auVar2 = AES_RESULT;
    AES_KEY = auVar2;
    return;
}



void kirk_cmd2_decrypt_key(void)

{
    uint auVar1 [4];
    
    DMA_ADDR = INPUT;
    DMA_BUF_SIZE = 0x10;
    hw_dma_read();
    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
    AES_CMD = 1;
    hw_aes_setkey();
    hw_aes_do();
    auVar1 = AES_RESULT;
    AES_KEY = auVar1;
    return;
}



void kirk_aes_decrypt_key(void)

{
    uint auVar1 [4];
    
    DMA_ADDR = PSP_KIRK_SRC;
    DMA_BUF_SIZE = 0x10;
    hw_dma_read();
    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
    AES_CMD = 1;
    hw_aes_setkey();
    hw_aes_do();
    auVar1 = AES_RESULT;
    AES_KEY = auVar1;
    return;
}



void __unused_kirk_header_get_decrypted_size(void)

{
    DMA_BUF[0] = PSP_KIRK_SRC + 0x70;
    DMA_ADDR = DMA_BUF[0];
    DMA_BUF_SIZE = 4;
    hw_dma_read();
    DMA_BUF[0] = __builtin_byteswap(DMA_BUF[0]);
    return;
}



void dma_bzero(void)

{
    DMA_BUF[0] = 0;
    DMA_BUF[1] = 0;
    DMA_BUF[2] = 0;
    DMA_BUF[3] = 0;
    DMA_BUF._16_16_ = ZEXT816(0) << 0x40;
    DMA_BUF_SIZE = 0x20;
    while (0 < SIZE + -0x20) {
        SIZE = SIZE + -0x20;
        hw_dma_write_output();
        OUTPUT = OUTPUT + 0x20;
    }
    DMA_BUF_SIZE = SIZE;
    hw_dma_write_output();
    return;
}



undefined kirk_aes_read_header(void)

{
    undefined in_tmpZR;
    
    DMA_ADDR = PSP_KIRK_SRC;
    DMA_BUF_SIZE = 0x10;
    hw_dma_read();
    DMA_BUF[2] = DMA_BUF[3] >> 0x18 | DMA_BUF[3] << 8;
    DMA_BUF[3] = DMA_BUF[3] & 0x30000;
    return in_tmpZR;
}



void dma_memcpy(void)

{
    DMA_BUF_SIZE = 0x20;
    while (0 < SIZE + -0x20) {
        SIZE = SIZE + -0x20;
        hw_dma_read_input();
        hw_dma_write_output();
        INPUT = INPUT + 0x20;
        OUTPUT = OUTPUT + 0x20;
    }
    DMA_BUF_SIZE = SIZE;
    hw_dma_read_input();
    hw_dma_write_output();
    return;
}

bool memcmp_cmac(void)

{
    R11 = 0x399;
    R12 = 0xe10;
    R13 = 4;
    do {
        if (*(int *)R11 != *(int *)R12) {
            return *(int *)R11 == *(int *)R12;
        }
        R11 = R11 + 1;
        R12 = R12 + 1;
        R13 = R13 - 1;
    } while (R13 != 0);
    return R13 == 0;
}



bool hw_kirk_check_enabled(void)

{
    bool bVar1;
    
    bVar1 = PSP_KIRK_STATUS != 1;
    if (bVar1) {
        PSP_KIRK_RESULT = 1;
        crash();
    }
    return !bVar1;
}



void CheckKirkInitialized(void)

{
    if (UNK_FLAGS != 0) {
        PSP_KIRK_RESULT = 0xc;
    }
    return;
}



bool align16(void)

{
    uint uVar1;
    
    uVar1 = SIZE + 0xf;
    SIZE = uVar1 & 0xfffffff0;
    return uVar1 == 0;
}



void hw_dma_write_aes_result(void)

{
    uint auVar1 [4];
    
    auVar1 = AES_RESULT;
    DMA_BUF[0] = auVar1[0];
    DMA_BUF[1] = auVar1[1];
    DMA_BUF[2] = auVar1[2];
    DMA_BUF[3] = auVar1[3];
    hw_dma_write_output();
    return;
}



void hw_dma_write_output(void)

{
    DMA_ADDR = OUTPUT;
    hw_dma_write();
}



void hw_dma_write(void)

{
    __builtin_setmode(DMA_REG,1);
    __builtin_dma(0,1);
    return;
}



void hw_dma_read_input(void)

{
    DMA_ADDR = INPUT;
    hw_dma_read();
}



void hw_dma_read(void)

{
    __builtin_setmode(DMA_REG,2);
    __builtin_dma(0,1);
    return;
}



bool kirk_check_body_size_nonzero(void)

{
    if (KIRK_BODY_SIZE == 0) {
        PSP_KIRK_RESULT = 0x10;
    }
    return KIRK_BODY_SIZE != 0;
}



undefined buffer_is_nonzero(void)

{
    int iVar1;
    
    do {
        iVar1 = *(int *)R11;
        R11 = R11 + 1;
        if (iVar1 != 0) {
            return 1;
        }
        R13 = R13 - 1;
    } while (R13 != 0);
    return 0;
}

void crash(void)
{
    PSP_KIRK_PHASE = 3;
    __builtin_setmode(PSP_KIRK_REG,1);
    __builtin_dma(0,2);
    inf_loop();
    return;
}

void kirk_cmd10_priv_sigvry(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (!bVar1) {
        return;
    }
    kirk_header_read_cmd_mode();
    if (DMA_BUF[0] == 0x1000000) {
        HW_AES_PARAM_ID = 2;
        CUR_ENC_KEY = HW_AES_PARAM;
        kirk1_get_public_key();
    }
    else if (DMA_BUF[0] == 0x2000000) {
        HW_AES_PARAM_ID = 3;
        CUR_ENC_KEY = HW_AES_PARAM;
        kirk2_get_public_key();
    }
    else {
        if (DMA_BUF[0] != 0x3000000) {
            PSP_KIRK_PHASE = 1;
            PSP_KIRK_RESULT = 2;
            return;
        }
        AES_KEYSEED = 0;
        aes_set_perconsole_key();
        CUR_ENC_KEY = AES_RESULT;
        kirk3_get_public_key();
    }
    read_kirk_header();
    bVar1 = (bool)kirk_check_body_size_nonzero();
    if (bVar1) {
        kirk_header_read_ecdsa_flag();
        if (DMA_BUF[0] == 0x18) {
            set_kirk1_curve();
            bVar1 = (bool)kirk_ecdsa_check_header();
            if (!bVar1) {
                PSP_KIRK_PHASE = 1;
                PSP_KIRK_RESULT = 3;
                return;
            }
        }
        else {
            AES_KEY = CUR_ENC_KEY;
            bVar1 = (bool)kirk_cmac_check_header();
            if (!bVar1) {
                PSP_KIRK_PHASE = 1;
                return;
            }
        }
        PSP_KIRK_RESULT = 0;
    }
    PSP_KIRK_PHASE = 1;
    return;
}



void init_ECC(void)

{
    uint uVar1;
    
    uVar1 = ECC_REG;
    __builtin_setmode(uVar1,0);
    do {
        uVar1 = ECC_STATUS;
    } while (uVar1 != 0);
    return;
}



void init_SHA1(void)

{
    uint uVar1;
    
    uVar1 = SHA1_REG;
    __builtin_setmode(uVar1,0);
    do {
        uVar1 = SHA1_STATUS;
    } while (uVar1 != 0);
    return;
}



void init_AES(void)

{
    uint uVar1;
    
    uVar1 = AES_REG;
    __builtin_setmode(uVar1,0);
    do {
        uVar1 = AES_STATUS;
    } while (uVar1 != 0);
    return;
}



void init_PRNG(void)

{
    uint uVar1;
    
    uVar1 = PRNG_REG;
    __builtin_setmode(uVar1,0);
    do {
        uVar1 = PRNG_STATUS;
    } while (uVar1 != 0);
    return;
}



void init_TRNG(void)

{
    uint uVar1;
    
    uVar1 = TRNG_REG;
    __builtin_setmode(uVar1,0);
    do {
        uVar1 = TRNG_STATUS;
    } while (uVar1 != 0);
    return;
}



void init_MATH(void)

{
    uint uVar1;
    
    uVar1 = MATH_REG;
    __builtin_setmode(uVar1,0);
    do {
        uVar1 = MATH_STATUS;
    } while (uVar1 != 0);
    return;
}



void init_dma(void)

{
    __builtin_setmode(DMA_REG,0);
    do {
    } while (DMA_STATUS != 0);
    return;
}



void init_PSP(void)

{
    __builtin_setmode(PSP_KIRK_REG,0);
    do {
    } while (PSP_KIRK_STATUS != 0);
    return;
}



void kirk_modules_init(void)

{
    init_ECC();
    init_SHA1();
    init_AES();
    init_PRNG();
    init_TRNG();
    init_MATH();
    init_PSP();
    return;
}



void kirk_cmd11_hash(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (bVar1) {
        DMA_ADDR = PSP_KIRK_SRC;
        DMA_BUF_SIZE = 4;
        hw_dma_read();
        SIZE = __builtin_byteswap(DMA_BUF[0]);
        if (SIZE == 0) {
            PSP_KIRK_RESULT = 0x10;
        }
        else {
            INPUT = PSP_KIRK_SRC + 4;
            hw_sha1_do();
            DMA_BUF._0_16_ = (undefined  [16])SHA1_RESULT._0_16_;
            DMA_BUF[4] = SHA1_RESULT[4];
            DMA_ADDR = PSP_KIRK_DST;
            DMA_BUF_SIZE = 0x14;
            hw_dma_write();
            PSP_KIRK_RESULT = 0;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_cmd12_ecdsa_genkey(void)

{
    undefined auVar1 [16];
    uint uVar2;
    bool bVar3;
    
    bVar3 = (bool)hw_kirk_check_enabled();
    if (bVar3) {
        bVar3 = (bool)CheckKirkInitialized();
        if (bVar3) {
            UNK_FLAGS = UNK_FLAGS | 2;
            prng_generate();
            UNK_FLAGS = UNK_FLAGS & 0xfffffffd;
            DMA_BUF._0_16_ = (undefined  [16])MATH_RESULT._12_16_;
            DMA_BUF[4] = MATH_RESULT[7];
            DMA_ADDR = PSP_KIRK_DST;
            DMA_BUF_SIZE = 0x14;
            hw_dma_write();
            set_other_curve();
            auVar1 = (undefined  [16])MATH_RESULT._12_16_;
            ECC_R._0_16_ = (undefined  [16])auVar1;
            uVar2 = MATH_RESULT[7];
            ECC_R[4] = uVar2;
            ECC_OP = 2;
            hw_ec_do();
            OUTPUT = PSP_KIRK_DST + 0x14;
            dma_ec_output_point();
            PSP_KIRK_RESULT = 0;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void kirk_cmd13_ecdsa_mul(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (bVar1) {
        set_other_curve();
        DMA_ADDR = PSP_KIRK_SRC;
        DMA_BUF_SIZE = 0x14;
        hw_dma_read();
        R3 = DMA_BUF[0];
        R4 = DMA_BUF[1];
        R5 = DMA_BUF[2];
        R6 = DMA_BUF[3];
        R7 = DMA_BUF[4];
        bVar1 = (bool)ec_check_scalar();
        if (bVar1) {
            ECC_R._0_16_ = (undefined  [16])_R3;
            ECC_R[4] = R7;
            DMA_ADDR = PSP_KIRK_SRC + 0x14;
            DMA_BUF_SIZE = 0x14;
            INPUT = DMA_ADDR;
            hw_dma_read();
            ECC_Gx._0_16_ = (undefined  [16])DMA_BUF._0_16_;
            ECC_Gx[4] = DMA_BUF[4];
            DMA_ADDR = INPUT + 0x14;
            INPUT = DMA_ADDR;
            hw_dma_read();
            ECC_Gy._0_16_ = (undefined  [16])DMA_BUF._0_16_;
            ECC_Gy[4] = DMA_BUF[4];
            ECC_OP = 2;
            hw_ec_do();
            OUTPUT = PSP_KIRK_DST;
            dma_ec_output_point();
            PSP_KIRK_RESULT = 0;
        }
        else {
            PSP_KIRK_RESULT = 5;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_cmd14_prngen(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (bVar1) {
        bVar1 = (bool)CheckKirkInitialized();
        if (bVar1) {
            UNK_FLAGS = UNK_FLAGS | 2;
            prng_generate();
            UNK_FLAGS = UNK_FLAGS & 0xfffffffd;
            DMA_BUF._0_16_ = (undefined  [16])MATH_RESULT._12_16_;
            DMA_BUF[4] = MATH_RESULT[7];
            DMA_ADDR = PSP_KIRK_DST;
            DMA_BUF_SIZE = 0x14;
            hw_dma_write();
            PSP_KIRK_RESULT = 0;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void kirk_cmd16_siggen(void)

{
    uint uVar1;
    bool bVar2;
    
    bVar2 = (bool)hw_kirk_check_enabled();
    if (bVar2) {
        bVar2 = (bool)CheckKirkInitialized();
        if (bVar2) {
            AES_KEYSEED = 3;
            aes_set_perconsole_key();
            INPUT = PSP_KIRK_SRC;
            DMA_BUF_SIZE = 0x10;
            aes_set_null_iv();
            AES_CMD = 0x13;
            hw_aes_setkey();
            hw_aes_dma_copy_and_do();
            uVar1 = AES_CMD;
            AES_CMD = uVar1 & 0xfffffffd;
            _R3 = AES_RESULT;
            INPUT = INPUT + 0x10;
            hw_aes_dma_copy_and_do();
            R7 = AES_RESULT[0];
            bVar2 = (bool)ec_check_scalar();
            if (bVar2) {
                set_other_curve();
                ECC_S._0_16_ = (undefined  [16])_R3;
                ECC_S[4] = R7;
                DMA_ADDR = PSP_KIRK_SRC + 0x20;
                DMA_BUF_SIZE = 0x14;
                INPUT = DMA_ADDR;
                hw_dma_read();
                ECC_M._0_16_ = (undefined  [16])DMA_BUF._0_16_;
                ECC_M[4] = DMA_BUF[4];
                UNK_FLAGS = UNK_FLAGS | 2;
                ec_sign_gen_r();
                UNK_FLAGS = UNK_FLAGS & 0xfffffffd;
                OUTPUT = PSP_KIRK_DST;
                dma_ec_output_point();
                PSP_KIRK_RESULT = 0;
            }
            else {
                PSP_KIRK_RESULT = 5;
            }
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_cmd17_sigvry(void)

{
    uint uVar1;
    bool bVar2;
    
    bVar2 = (bool)hw_kirk_check_enabled();
    if (bVar2) {
        set_other_curve();
        DMA_BUF_SIZE = 0x14;
        INPUT = PSP_KIRK_SRC;
        hw_dma_read_input();
        ECC_Px._0_16_ = (undefined  [16])DMA_BUF._0_16_;
        ECC_Px[4] = DMA_BUF[4];
        INPUT = INPUT + 0x14;
        hw_dma_read_input();
        ECC_Py._0_16_ = (undefined  [16])DMA_BUF._0_16_;
        ECC_Py[4] = DMA_BUF[4];
        INPUT = INPUT + 0x14;
        hw_dma_read_input();
        ECC_M._0_16_ = (undefined  [16])DMA_BUF._0_16_;
        ECC_M[4] = DMA_BUF[4];
        INPUT = INPUT + 0x14;
        ec_dma_read_scalars();
        ECC_OP = 1;
        hw_ec_do();
        uVar1 = ECC_RESULT;
        if (uVar1 == 0) {
            PSP_KIRK_RESULT = 0;
        }
        else {
            PSP_KIRK_RESULT = 5;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_cmd18_certvry(void)

{
    bool bVar1;
    
    bVar1 = (bool)hw_kirk_check_enabled();
    if (bVar1) {
        AES_KEYSEED = 4;
        aes_set_perconsole_key();
        INPUT = PSP_KIRK_SRC;
        SIZE = 0xa8;
        aes_cmac();
        DMA_ADDR = PSP_KIRK_SRC + 0xa8;
        DMA_BUF_SIZE = 0x10;
        DMA_BUF[0] = DMA_ADDR;
        hw_dma_read();
        bVar1 = memcmp_cmac();
        if (bVar1) {
            PSP_KIRK_RESULT = 0;
        }
        else {
            PSP_KIRK_RESULT = 5;
        }
        PSP_KIRK_PHASE = 1;
    }
    return;
}



undefined ec_check_scalar(void)

{
    bool bVar1;
    undefined uVar2;
    
    R11 = 0xfc3;
    R13 = 5;
    bVar1 = (bool)buffer_is_nonzero();
    if ((bVar1) &&
       (((((int)R3 < -1 || ((int)R4 < -2)) || ((int)R5 < -0x4a52)) ||
        (((int)R6 < 0x3c523e63 || ((int)R7 < -0x6bb0ded9)))))) {
        uVar2 = 1;
    }
    else {
        uVar2 = 0;
    }
    return uVar2;
}



void kirk_cmd15_init(void)

{
    uint uVar1;
    uint uVar2;
    bool bVar3;
    
    bVar3 = (bool)hw_kirk_check_enabled();
    if (bVar3) {
        UNK_FLAGS = UNK_FLAGS | 1;
        DMA_ADDR = PSP_KIRK_SRC;
        DMA_BUF_SIZE = 0x1c;
        hw_dma_read();
        DMA_BUF[1] = DMA_BUF[1] + 1;
        __unkown_op(0x21,DMA_BUF[0]);
        KIRK15_KEYSEED[0] = 0;
        KIRK15_KEYSEED[1] = 0;
        KIRK15_KEYSEED[2] = DMA_BUF[0];
        PRNG_SEED[0] = DMA_BUF[2];
        PRNG_SEED._4_16_ = (undefined  [16])DMA_BUF._12_16_;
        TRNG_OUT_SIZE = 0x100;
        uVar1 = TRNG_REG;
        __builtin_setmode(uVar1,1);
        __builtin_dma(0x100,0);
        SHA1_IN_SIZE = 0x20;
        uVar1 = TRNG_OUTBLOCK[0];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[1];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[2];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[3];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[4];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[5];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[6];
        SHA1_IN_DATA = uVar1;
        uVar1 = TRNG_OUTBLOCK[7];
        SHA1_IN_DATA = uVar1;
        __builtin_dma(4,0);
        uVar1 = SHA1_RESULT[0];
        uVar2 = PRNG_SEED[0];
        PRNG_SEED[0] = uVar1 ^ uVar2;
        uVar1 = SHA1_RESULT[1];
        uVar2 = PRNG_SEED[1];
        PRNG_SEED[1] = uVar1 ^ uVar2;
        uVar1 = SHA1_RESULT[2];
        uVar2 = PRNG_SEED[2];
        PRNG_SEED[2] = uVar1 ^ uVar2;
        uVar1 = SHA1_RESULT[3];
        uVar2 = PRNG_SEED[3];
        PRNG_SEED[3] = uVar1 ^ uVar2;
        uVar1 = SHA1_RESULT[4];
        uVar2 = PRNG_SEED[4];
        PRNG_SEED[4] = uVar1 ^ uVar2;
        PRNG_MODE = 3;
        KIRK15_KEYSEED[3] = DMA_BUF[1];
        kirk_reseed();
        DMA_BUF[2] = PRNG_SEED[0];
        DMA_BUF._12_16_ = (undefined  [16])PRNG_SEED._4_16_;
        DMA_ADDR = PSP_KIRK_DST;
        hw_dma_write();
        PSP_KIRK_RESULT = 0;
        PSP_KIRK_PHASE = 1;
    }
    return;
}



void kirk_reseed_prng_2(void)

{
    PRNG_MODE = 2;
    kirk_reseed();
}

void kirk_reseed(void)

{
    AES_KEYSEED = 6;
    aes_set_perconsole_key();
    hw_aes_setkey();
    AES_SRC_BUF = KIRK15_KEYSEED;
    hw_aes_do();
    KIRK15_KEYSEED = AES_RESULT;
    PRNG_SEED[5] = 0;
    PRNG_SEED._24_16_ = (undefined  [16])KIRK15_KEYSEED;
    hw_prng_do();
    return;
}

void aes_set_perconsole_key_0(void)
{
    AES_KEYSEED = 0;
    aes_set_perconsole_key();
}



void aes_set_perconsole_key(void)

{
    uint auVar1 [4];
    
    AES_CMD = 0;
    auVar1 = KEY_SEED2;
    AES_KEY = auVar1;
    hw_aes_setkey();
    if (AES_KEYSEED == 0) {
        auVar1 = KEY_SEED1;
        AES_SRC_BUF = auVar1;
    }
    else {
        auVar1 = KEY_SEED0;
        AES_SRC_BUF = auVar1;
    }
    AES_KEYSEED = AES_KEYSEED >> 1 | AES_KEYSEED << 0x1f;
    AES_KEYSEED = AES_KEYSEED + 1;
    do {
        hw_aes_do();
        auVar1 = AES_RESULT;
        AES_SRC_BUF = auVar1;
        AES_KEYSEED = AES_KEYSEED - 1;
    } while (AES_KEYSEED != 0);
    auVar1 = AES_RESULT;
    AES_KEY = auVar1;
    return;
}



void hw_prng_do(void)

{
    uint uVar1;
    
    uVar1 = PRNG_REG;
    __builtin_setmode(uVar1,1);
    __builtin_dma(0x80,0);
    return;
}



void kirk_self_test(void)

{
    bool bVar1;
    
    kirk_modules_init();
    bVar1 = (bool)test_ECC_scalar_mult();
    if ((((bVar1) && (bVar1 = (bool)test_SHA1(), bVar1)) && (bVar1 = (bool)test_AES(), bVar1)) &&
       (((bVar1 = (bool)test_PRNG(), bVar1 && (bVar1 = (bool)test_TRNG(), bVar1)) &&
        (bVar1 = (bool)test_MATH(), bVar1)))) {
        HW_TEST_RESULT = 3;
    }
    else {
        HW_TEST_RESULT = 2;
    }
    PSP_KIRK_PHASE = 0;
    __builtin_setmode(PSP_KIRK_REG,1);
    __builtin_dma(0,2);
    AES_CMD = 0;
    AES_KEY[0] = 0x2b7e1516;
    AES_KEY[1] = 0x28aed2a6;
    AES_KEY[2] = 0xabf71588;
    AES_KEY[3] = 0x9cf4f3c;
    hw_aes_setkey();
    DMA_BUF_SIZE = 0x20;
    DMA_ADDR = PSP_KIRK_SRC;
    hw_dma_read();
    AES_SRC_BUF = (uint  [4])DMA_BUF._0_16_;
    hw_aes_do();
    DMA_BUF._0_16_ = (undefined  [16])AES_RESULT;
    AES_SRC_BUF = (uint  [4])DMA_BUF._16_16_;
    hw_aes_do();
    DMA_BUF._16_16_ = (undefined  [16])AES_RESULT;
    DMA_ADDR = PSP_KIRK_DST;
    hw_dma_write();
    PSP_KIRK_PHASE = 1;
    __builtin_setmode(PSP_KIRK_REG,1);
    __builtin_dma(0,2);
    return;
}



undefined test_ECC_scalar_mult(void)

{
    uint uVar1;
    undefined uVar2;
    
    ECC_P[0] = 0xacb0f43a;
    ECC_P[1] = 0x68487d86;
    ECC_P[2] = 0x975b98aa;
    ECC_P[3] = 0xe0a0f6a5;
    ECC_P[4] = 0x81da1e8d;
    ECC_A[0] = 0xacb0f43a;
    ECC_A[1] = 0x68487d86;
    ECC_A[2] = 0x975b98aa;
    ECC_A[3] = 0xe0a0f6a5;
    ECC_A[4] = 0x81da1e8a;
    ECC_B[0] = 0x4fb97afa;
    ECC_B[1] = 0x73a001a1;
    ECC_B[2] = 0xa6386705;
    ECC_B[3] = 0xde41e517;
    ECC_B[4] = 0x543933a7;
    ECC_N[0] = 0;
    ECC_N[1] = 0;
    ECC_N[2] = 0;
    ECC_N[3] = 0;
    ECC_N[4] = 3;
    ECC_UNK1[0] = 0x44e8319d;
    ECC_UNK1[1] = 0x2ea03d11;
    ECC_UNK1[2] = 0x5c6e8341;
    ECC_UNK1[3] = 0xfffe8429;
    ECC_UNK1[4] = 0x94ffc5a9;
    ECC_UNK2[0] = 0xa9c56aab;
    ECC_UNK2[1] = 0x686453b1;
    ECC_UNK2[2] = 0xf34f7d6f;
    ECC_UNK2[3] = 0x61954b7d;
    ECC_UNK2[4] = 0x834bf46;
    ECC_Gx[0] = 0xa60cd015;
    ECC_Gx[1] = 0x42a20554;
    ECC_Gx[2] = 0x5b6696f3;
    ECC_Gx[3] = 0x421e00cd;
    ECC_Gx[4] = 0xe8bbc06c;
    ECC_Gy[0] = 0x76bd9970;
    ECC_Gy[1] = 0x281e307c;
    ECC_Gy[2] = 0xd30ea2eb;
    ECC_Gy[3] = 0xa0f88400;
    ECC_Gy[4] = 0x334bcba8;
    ECC_R[0] = 0;
    ECC_R[1] = 0;
    ECC_R[2] = 0;
    ECC_R[3] = 0;
    ECC_R[4] = 3;
    ECC_OP = 2;
    hw_ec_do();
    uVar1 = ECC_RESULT_X[0];
    if ((((((uVar1 == 0x6df3fa63) && (uVar1 = ECC_RESULT_X[1], uVar1 == 0xc3deede1)) &&
          (uVar1 = ECC_RESULT_X[2], uVar1 == 0x3d46d1fa)) &&
         ((uVar1 = ECC_RESULT_X[3], uVar1 == 0xf6cbc2b6 &&
          (uVar1 = ECC_RESULT_X[4], uVar1 == 0x4319589a)))) &&
        ((uVar1 = ECC_RESULT_Y[0], uVar1 == 0x78ef881d &&
         ((uVar1 = ECC_RESULT_Y[1], uVar1 == 0x7450e850 &&
          (uVar1 = ECC_RESULT_Y[2], uVar1 == 0x22c8e6f0)))))) &&
       ((uVar1 = ECC_RESULT_Y[3], uVar1 == 0x7d9076e9 &&
        (uVar1 = ECC_RESULT_Y[4], uVar1 == 0x2f4e082e)))) {
        uVar2 = 1;
    }
    else {
        uVar2 = 0;
    }
    return uVar2;
}



undefined test_SHA1(void)

{
    uint uVar1;
    undefined success;
    
    SHA1_IN_SIZE = 3;
    SHA1_IN_DATA = 0x61626300;
    __builtin_dma(4,0);
    uVar1 = SHA1_RESULT[0];
    if ((((uVar1 == 0xa9993e36) && (uVar1 = SHA1_RESULT[1], uVar1 == 0x4706816a)) &&
        (uVar1 = SHA1_RESULT[2], uVar1 == 0xba3e2571)) &&
       ((uVar1 = SHA1_RESULT[3], uVar1 == 0x7850c26c &&
        (uVar1 = SHA1_RESULT[4], uVar1 == 0x9cd0d89d)))) {
        success = 1;
    }
    else {
        success = 0;
    }
    return success;
}



undefined test_AES(void)

{
    uint uVar1;
    undefined uVar2;
    
    AES_CMD = 0;
    AES_KEY[0] = 0x2b7e1516;
    AES_KEY[1] = 0x28aed2a6;
    AES_KEY[2] = 0xabf71588;
    AES_KEY[3] = 0x9cf4f3c;
    hw_aes_setkey();
    AES_SRC_BUF[0] = 0x3243f6a8;
    AES_SRC_BUF[1] = 0x885a308d;
    AES_SRC_BUF[2] = 0x313198a2;
    AES_SRC_BUF[3] = 0xe0370734;
    hw_aes_do();
    uVar1 = AES_RESULT[0];
    if ((((uVar1 == 0x3925841d) && (uVar1 = AES_RESULT[1], uVar1 == 0x2dc09fb)) &&
        (uVar1 = AES_RESULT[2], uVar1 == 0xdc118597)) &&
       (uVar1 = AES_RESULT[3], uVar1 == 0x196a0b32)) {
        uVar2 = 1;
    }
    else {
        uVar2 = 0;
    }
    return uVar2;
}



undefined test_PRNG(void)

{
    uint uVar1;
    undefined uVar2;
    
    PRNG_MODE = 3;
    PRNG_SEED[0] = 0xf067cf18;
    PRNG_SEED[1] = 0x1f101685;
    PRNG_SEED[2] = 0x11d8483c;
    PRNG_SEED[3] = 0xf3e5538f;
    PRNG_SEED[4] = 0x415117cb;
    PRNG_SEED[5] = 0x12153524;
    PRNG_SEED[6] = 0xc0895e81;
    PRNG_SEED[7] = 0x8484d609;
    PRNG_SEED[8] = 0xb1f05663;
    PRNG_SEED[9] = 0x6b97b0d;
    hw_prng_do();
    uVar1 = PRNG_RESULT[0];
    if ((((((uVar1 == 0x1df50b98) && (uVar1 = PRNG_RESULT[1], uVar1 == 0x7d6c2369)) &&
          (uVar1 = PRNG_RESULT[2], uVar1 == 0x1794af3b)) &&
         ((uVar1 = PRNG_RESULT[3], uVar1 == 0xab98a128 &&
          (uVar1 = PRNG_RESULT[4], uVar1 == 0x4cd53d2d)))) &&
        ((uVar1 = PRNG_SEED[0], uVar1 == 0xe5cdab0 &&
         ((uVar1 = PRNG_SEED[1], uVar1 == 0x9c7c39ee && (uVar1 = PRNG_SEED[2], uVar1 == 0x296cf778))
         )))) && ((uVar1 = PRNG_SEED[3], uVar1 == 0x9f7df4b7 &&
                  (uVar1 = PRNG_SEED[4], uVar1 == 0x8e2654f9)))) {
        uVar2 = 1;
    }
    else {
        uVar2 = 0;
    }
    return uVar2;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void test_TRNG(void)

{
    uint uVar1;
    
    TRNG_OUT_SIZE = 0x100;
    uVar1 = TRNG_REG;
    __builtin_setmode(uVar1,1);
    __builtin_dma(0x100,0);
    _SIZE = (undefined  [16])TRNG_OUTBLOCK._0_16_;
    _R4 = (undefined  [16])TRNG_OUTBLOCK._16_16_;
    return;
}



undefined test_MATH(void)

{
    uint uVar1;
    
    MATH_IN[0] = 0;
    MATH_IN[1] = 0;
    MATH_IN[2] = 0;
    MATH_IN[3] = 0;
    MATH_IN[4] = 0;
    MATH_IN[5] = 0;
    MATH_IN[6] = 0x2c6c3246;
    MATH_IN[7] = 0xfcedb054;
    MATH_IN[8] = 0x6f969b86;
    MATH_IN[9] = 0x19d03bf8;
    MATH_IN[10] = 0xe16b88ca;
    MATH_IN[11] = 0x7b31acf2;
    MATH_IN[12] = 0xc800afa8;
    MATH_IN[13] = 0xc1945fa1;
    MATH_IN[14] = 0x2719c7f1;
    MATH_IN[15] = 0xc7c99bb7;
    MATH_IN_X[0] = 0;
    MATH_IN_X[1] = 0;
    MATH_IN_X[2] = 0;
    MATH_IN_X[3] = 0xe5d8c140;
    MATH_IN_X[4] = 0xe15a3be2;
    MATH_IN_X[5] = 0x38d81bd7;
    MATH_IN_X[6] = 0x7229b1b8;
    MATH_IN_X[7] = 0x8a143d;
    uVar1 = MATH_REG;
    __builtin_setmode(uVar1,1);
    __builtin_dma(0x400,0);
    uVar1 = MATH_RESULT[0];
    if ((((uVar1 == 0) && (uVar1 = MATH_RESULT[1], uVar1 == 0)) &&
        (uVar1 = MATH_RESULT[2], uVar1 == 0)) &&
       (((uVar1 = MATH_RESULT[3], uVar1 == 0x5ba7b73f &&
         (uVar1 = MATH_RESULT[4], uVar1 == 0x60c6fa94)) &&
        ((uVar1 = MATH_RESULT[5], uVar1 == 0x87a61858 &&
         ((uVar1 = MATH_RESULT[6], uVar1 == 0x6350a902 &&
          (uVar1 = MATH_RESULT[7], uVar1 == 0x5c1713ae)))))))) {
        MATH_IN[0] = 0;
        MATH_IN[1] = 0;
        MATH_IN[2] = 0;
        MATH_IN[3] = 0;
        MATH_IN[4] = 0;
        MATH_IN[5] = 0;
        MATH_IN[6] = 0xfea54c79;
        MATH_IN[7] = 0x8eac6e6a;
        MATH_IN[8] = 0x78f14231;
        MATH_IN[9] = 0x687faea7;
        MATH_IN[10] = 0x865fac87;
        MATH_IN[11] = 0x6e8a7f6a;
        MATH_IN[12] = 0x76e5afa9;
        MATH_IN[13] = 0xc87fe87a;
        MATH_IN[14] = 0x56b9a87e;
        MATH_IN[15] = 0x18756ffe;
        MATH_IN_X[0] = 0;
        MATH_IN_X[1] = 0;
        MATH_IN_X[2] = 0;
        MATH_IN_X[3] = 0;
        MATH_IN_X[4] = 0;
        MATH_IN_X[5] = 0;
        MATH_IN_X[6] = 0;
        MATH_IN_X[7] = 3;
        uVar1 = MATH_REG;
        __builtin_setmode(uVar1,1);
        __builtin_dma(0x400,0);
        uVar1 = MATH_RESULT[0];
        if ((uVar1 == 0) &&
           (((((uVar1 = MATH_RESULT[1], uVar1 == 0 && (uVar1 = MATH_RESULT[2], uVar1 == 0)) &&
              (uVar1 = MATH_RESULT[3], uVar1 == 0)) &&
             ((uVar1 = MATH_RESULT[4], uVar1 == 0 && (uVar1 = MATH_RESULT[5], uVar1 == 0)))) &&
            ((uVar1 = MATH_RESULT[6], uVar1 == 0 && (uVar1 = MATH_RESULT[7], uVar1 == 2)))))) {
            return 1;
        }
    }
    return 0;
}


