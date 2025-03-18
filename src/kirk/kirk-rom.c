// KIRK is the PSP CPU encryption chip. It can do symmetric encryption (AES-128), asymmetric signatures (ECDSA), random number generation, and hashing (SHA-1).
// It also stores secret parameters used by the PSP.
// The KIRK ROM has been dumped and uses a custom ISA using words of 4 bytes and word-addressing (contrary to most architectures, if x points to a 4-byte word,
// x+1 points to the next 4-byte word). It has no registers and access to a memory space of 0x1000 words, most of the space being used to contact the other devices.
// It works in big endian mode.
// It has two instructions, marked as functions here, that have a special role: __builtin_setmode() to define an action for the device and __builtin_intr() to trigger the action.

// KIRK has access to 8 different interfaces. All interfaces have associated 'enable' and 'status' register, plus other ones depending on the device.
// The status register is only used for initialization.
// To reset a device, use __builtin_setmode(XXX_REG, XXX_REG_RESET (= 0)) then wait for the status to be non-zero.
// Once reset, to use a device, use __builtin_setmode(XXX_REG, XXX_REG_<ACTION>) (XXX_REG_<ACTION> can be 1 or 2 depending on the devices and actions), then __builtin_intr(INTR_XXX)
// Exception: for SHA1, there is no call to __builtin_setmode() when doing an action (only on reset)
#define INTR_DMA  0x000001
#define INTR_CPU  0x000002
#define INTR_ECC  0x000100
#define INTR_SHA1 0x004000
#define INTR_AES  0x010000
#define INTR_PRNG 0x080000
#define INTR_TRNG 0x100000
#define INTR_MATH 0x400000

// Modules, by memory space:

// Module 1 - ECC - can make and verify signatures, and multiply points by scalars
//
// 0x000 - HW_ECC_REG - only 0 or 1
#define ECC_REG_RESET 0
#define ECC_REG_DO    1
// 0x001 - HW_ECC_STATUS
// 0x002 - HW_ECC_OP - 0 = make signature (s) from r, 1 = verify signature, 2 = multiply point by scalar
#define ECC_OP_SIGN 0
#define ECC_OP_CHECK_SIGN 1
#define ECC_OP_MUL 2

// 0x003 - HW_ECC_RESULT - only seen for commands 0 and 1, value 0 = OK
// 0x00B - HW_ECC_P - field size for the curve
// 0x013 - HW_ECC_A - parameter a for the curve
// 0x01B - HW_ECC_B - parameter b for the curve
// 0x023 - HW_ECC_N - modulus of the curve
// 0x02B - HW_ECC_UNK1 - unknown, seems to be a parameter of the curve
// 0x033 - HW_ECC_UNK2 - unknown, seems to be a parameter of the curve
// 0x03B - HW_ECC_Gx - first coordinate of the generator (or input point)
// 0x043 - HW_ECC_Gy - second coordinate of the generator (or input point)
// 0x04B - HW_ECC_R - first scalar (k for multiplication, r for signature)
// 0x053 - HW_ECC_S - second scalar (s for signature)
// 0x05B - HW_ECC_M - message (hash) to sign (or to verify signature)
// 0x063 - HW_ECC_Px - first coordinate of the public key (for signature check)
// 0x06B - HW_ECC_Py - second coordinate of the public key (for signature check)
// 0x08B - HW_ECC_RESULT_x - first coordinate of the result (for multiplication)
// 0x093 - HW_ECC_RESULT_y - second coordinate of the result (for multiplication)
//
// Module 2 - SHA1
//
// 0x330 - HW_SHA1_REG - only 0 (__builtin_setmode isn't used when doing SHA1's)
#define SHA1_REG_RESET 0
// 0x331 - HW_SHA1_STATUS
// 0x332 -
// 0x333 - HW_SHA1_IN_SIZE - size of the input data
// 0x334 - HW_SHA1_IN_DATA - buffer for the current 4 bytes of input data
// 0x335 - HW_SHA1_RESULT - result of the SHA1
//
// Module 3 - AES
//
// 0x380 - HW_AES_REG - 0 for reset, 1 for sending data to encrypt/decrypt, 2 to set key
#define AES_REG_RESET  0
#define AES_REG_ENCDEC 1
#define AES_REG_SETKEY 2
// 0x381 - HW_AES_STATUS
// 0x382 - HW_AES_CMD
//          bit 0 (0x01): 0 = encrypt, 1 = decrypt
//          bit 1 (0x02): 0 = not first block, 1 = first block
//          bit 4 (0x10): 0 = single block, 1 = multiple blocks
//          bit 5 (0x20): 0 = normal, 1 = CMAC

#define AES_CMD_ENCRYPT         0x00
#define AES_CMD_DECRYPT         0x01
#define AES_CMD_FIRST_BLOCK     0x02
#define AES_CMD_MULTIPLE_BLOCKS 0x10
#define AES_CMD_CMAC            0x20

// 0x383 - HW_AES_CMAC_SIZE - size of the data for the CMAC
// 0x384 - HW_AES_SRC_BUF - current block to encrypt/decrypt
// 0x388 - HW_AES_KEY - AES key
// 0x390 - HW_AES_IV - AES IV for CBC
// 0x399 - HW_AES_RESULT - result of the encryption/decryption
//
// Module 4 - an unknown deterministic PRNG (pseudo-random number generator)
//
// 0x420 - HW_PRNG_REG - only 0 or 1
#define PRNG_REG_RESET 0
#define PRNG_REG_DO    1
// 0x421 - HW_PRNG_STATUS
// 0x422 - HW_PRNG_MODE - 2 or 3? (3 is used for initialization & for the self-test, 2 is used the rest of the time)
// 0x423 - HW_PRNG_SEED - a 40-byte-long buffer
// 0x42D - HW_PRNG_RESULT - a 20-byte-long buffer
//
// Module 5 - might be some kind of TRNG (true random number generator)
//
// 0x440 - HW_TRNG_REG - only 0 or 1
#define TRNG_REG_RESET 0
#define TRNG_REG_DO    1
// 0x441 - HW_TRNG_STATUS
// 0x442 - HW_TRNG_OUT_SIZE - the output size of the true rng, in bits (probably)
// 0x443 - HW_TRNG_OUTPUT - output of the TRNG, size unknown
//
// Module 6 - math module, used to compute modulo's of large numbers
//
// 0x460 - HW_MATH_REG - only 0 or 1
#define MATH_REG_RESET 0
#define MATH_REG_DO    1
// 0x461 - HW_MATH_STATUS
// 0x462 - HW_MATH_IN - 64-byte input buffer
// 0x472 - HW_MATH_MODULUS - 32-byte input buffer, containing the modulus, which is always a prime number here
// 0x47A - HW_MATH_RESULT - result of the modulo
//
// Key parameters:
//
// 0x490 - HW_KEY_MESH_0 - read-only key seed
// 0x494 - HW_KEY_MESH_1 - read-only key seed
// 0x498 - HW_KEY_MESH_2 - read-only key seed
// 0x49C - HW_KEY_MESH_3 - read-only key seed
// 0x4A0 - HW_AES_KEYSLOT_ID - write-only, write ID to receive key in HW_HW_AES_PARAM. 0x00/0x01 = kbooti (devkit), 0x02 = kirk 1 (IPL), 0x03 = kirk 2, 0x04~0x83 = kirk 4/7, 
// 0x4A1 - HW_AES_KEYSLOT - read-only
// 0x4B0 - HW_ECDSA_KEYSLOT_ID - same as above, but with ECC parameters. 0/1 = kirk 1 public key, 2/3 = kirk 2 public key, 4 = kirk 2 private key, 5/6 = kirk 3 public key
// 0x4B1 - HW_ECDSA_KEYSLOT - the obtained parameter (scalar or coordinate)
//
// Module 7 - DMA module, to send/receive data from the main CPU
//
// 0xE00 - HW_DMA_REG - 0 to init, 1 to write data, 2 to read data
#define DMA_REG_RESET 0
#define DMA_REG_WRITE 1
#define DMA_REG_READ  2
// 0xE01 - HW_DMA_STATUS - unused (initialization is present in the ROM but never called)
// 0xE02 - HW_DMA_ADDR - address from which to read/write
// 0xE03 - HW_DMA_BUF_SIZE - size of the data to send/receive
// 0xE10 - HW_DMA_BUF - buffer of the data to send/receive
//
// Module 8 - PSP KIRK interface, interface with the registers at 0xBDE00000 on the CPU
//
// 0xE20 - HW_CPU_REG - only 0 or 1
#define CPU_REG_RESET 0
#define CPU_REG_DO    1
// 0xE21 - HW_CPU_STATUS - equal to 1 when KIRK is initialized
// 0xE22 - HW_CPU_PHASE - possibly the same phase as 0xBDE0001C
// 0xE30 - HW_CPU_CMD - KIRK command, same as 0xBDE00010
// 0xE31 - HW_CPU_SRC - source address, same as 0xBDE0002C
// 0xE32 - HW_CPU_DST - destination address, same as 0xBDE00030
// 0xE33 - HW_CPU_ERROR - return value, same as 0xBDE00014
//
// Tests:
//
// 0xFB0 - HW_TEST_MODE - read-only, LSB is 1 to run self-tests, 0 otherwise
// 0xFB8 - HW_TEST_RESULT - write-only, 3 if self-tests succeeded, 2 if they failed
//
// Free to use (non-initialized) memory space: 0xFC0 - 0xFFF
//
// 0xFC0 - SIZE - used as a size
// 0xFC1 - INPUT - used as an input address
// 0xFC2 - OUTPUT - used as an output address
// 0xFC3 - R3 - different uses
// 0xFC4 - R4 - different uses
// 0xFC5 - R5 - different uses
// 0xFC6 - R6 - different uses
// 0xFC7 - R7 - different uses
// 0xFC8 - PERCONSOLE_KEYSEED - used by aes_set_perconsole_key to define the per-console AES key to use. 1 for kirk 5/8, 2 for kirk 6/9, 3 for kirk 0x10, 4 for kirk 0x12, 6 for kirk 15 keyseed
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
// 0xFF8 - RNG_BUFFER - temporary space used by KIRK15 for a keyseed
// 0xFFF - RNG_FLAGS - bit 0 is set when command 15 is run (ie PRNG is seeded)
//                     bit 1 is set when using the "public" curve
#define RNG_FLAGS_SEEDED 1
#define RNG_FLAGS_PUB_CURVE 2

#define KIRK_OPERATION_SUCCESS 0
#define KIRK_NOT_ENABLED 1
#define KIRK_INVALID_MODE 2
#define KIRK_HEADER_SIG_INVALID 3
#define KIRK_DATA_SIG_INVALID 4
#define KIRK_ECDSA_DATA_INVALID 5
#define KIRK_NOT_INITIALIZED 12
#define KIRK_INVALID_OPERATION 13
#define KIRK_INVALID_ENC_SEED 14
#define KIRK_INVALID_DEC_SEED 15
#define KIRK_DATA_SIZE_ZERO 16

#define KIRK_KBOOTI_HEADER_SIZE_OFF 0x10

#define KIRK_SIGNED_HEADER_SIGN_MODE_OFF 0x60
#define KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA (1 << 24)
#define KIRK_SIGNED_HEADER_SIGN_MODE_OUTPUT_ECDSA (1 << 25)

void _reset(void) // 0x000
{
    do {
        // Wait
    } while( true );
}

/*
 * Main function. Read commands coming from the PSP CPU hardware register and run the commands accordingly.
 */
void _start(void) // 0x001
{
    // Run self-tests if enabled
    if (HW_TEST_MODE & 1) {
        kirk_self_test();
        inf_loop();
        return;
    }
    // Initialize interfaces
    kirk_modules_init();
    RNG_FLAGS = 0;
    HW_CPU_PHASE = 0;
    while (true) {
        // Read the next command and call the function accordingly
        __builtin_setmode(HW_CPU_REG, CPU_REG_DO);
        __builtin_intr(INTR_CPU);
        if (HW_CPU_CMD == 0) {
            kirk_cmd0_decrypt_bootrom();
        } else if (HW_CPU_CMD == 1) {
            kirk_cmd1_decrypt_private();
        } else if (HW_CPU_CMD == 2) {
            kirk_cmd2_dnas_encrypt();
        } else if (HW_CPU_CMD == 3) {
            kirk_cmd3_dnas_decrypt();
        } else if (HW_CPU_CMD == 4) {
            kirk_cmd4_encrypt_static();
        } else if (HW_CPU_CMD == 5) {
            kirk_cmd5_encrypt_perconsole();
        } else if (HW_CPU_CMD == 6) {
            kirk_cmd6_encrypt_user();
        } else if (HW_CPU_CMD == 7) {
            kirk_cmd7_decrypt_static();
        } else if (HW_CPU_CMD == 8) {
            kirk_cmd8_decrypt_perconsole();
        } else if (HW_CPU_CMD == 9) {
            kirk_cmd9_decrypt_user();
        } else if (HW_CPU_CMD == 10) {
            kirk_cmd10_priv_sigvry();
        } else if (HW_CPU_CMD == 11) {
            kirk_cmd11_hash();
        } else if (HW_CPU_CMD == 12) {
            kirk_cmd12_ecdsa_genkey();
        } else if (HW_CPU_CMD == 13) {
            kirk_cmd13_ecdsa_mul();
        } else if (HW_CPU_CMD == 14) {
            kirk_cmd14_gen_privkey();
        } else if (HW_CPU_CMD == 15) {
            kirk_cmd15_init();
        } else if (HW_CPU_CMD == 16) {
            kirk_cmd16_siggen();
        } else if (HW_CPU_CMD == 17) {
            kirk_cmd17_sigvry();
        } else if (HW_CPU_CMD == 18) {
            kirk_cmd18_certvry();
        } else {
            // Unknown command
            HW_CPU_RESULT = KIRK_INVALID_OPERATION;
            if (!(HW_CPU_STATUS & 2)) {
                crash();
                return;
            }
            HW_CPU_PHASE = 1;
        }
    }
}

/*
 * Infinite loop. Triggered on fatal error or end of operation.
 */
void inf_loop(void) // 0x074
{
    do {
        // Wait
    } while( true );
}

/*
 * Encrypt INPUT to OUTPUT in AES ECB using CUR_ENC_KEY.
 */
void aes_encrypt_ecb(void) // 0x075
{
    aes_set_null_iv();
    aes_encrypt_cbc();
}

/*
 * Encrypt INPUT to OUTPUT in AES CBC using CUR_ENC_KEY and after padding it with random.
 */
void aes_encrypt_cbc(void) // 0x076
{
    if (SIZE == 0) {
        return;
    }

    kirk_reseed_prng_2();
    HW_AES_KEY = CUR_ENC_KEY;
    HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_ENCRYPT;
    aes_setkey();
    HW_DMA_BUF_SIZE = 0x10;
    while (SIZE > 0x10) {
        SIZE -= 0x10;
        aes_dma_copy_and_do();
        HW_AES_IV = HW_AES_RESULT;
        dma_write_aes_result();
        INPUT += 0x10;
        OUTPUT += 0x10;
        HW_AES_CMD &= ~AES_CMD_FIRST_BLOCK;
    }
    HW_DMA_BUF_SIZE = SIZE;
    dma_read_input();
    HW_AES_CMD |= 2;
    aes_padding();
    aes_do();
    HW_DMA_BUF_SIZE = 0x10;
    dma_write_aes_result();
}

/*
 * Compute the AES CMAC of INPUT with a given SIZE and put the result in HW_AES_RESULT.
 */
void aes_cmac(void) // 0x098
{
    aes_set_null_iv();
    HW_AES_CMD = AES_CMD_CMAC | AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK;
    aes_setkey();
    HW_AES_CMAC_SIZE = SIZE;
    HW_DMA_BUF_SIZE = 0x10;
    aes_decrypt_ecb();
}

/*
 * Decrypt INPUT, of given SIZE and HW_DMA_BUF_SIZE (which should be the block size ie 0x10), with AES ECB, and put the result in HW_AES_RESULT. Key is supposed to already be set.
 */
void aes_decrypt_ecb(void) // 0x09F
{
    // Decrypt complete blocks
    while (SIZE > 0x10) {
        SIZE -= 0x10;
        aes_dma_copy_and_do();
        INPUT += 0x10;
        HW_AES_CMD &= 0xfffffffd;
    }
    // Decrypt the remaining of the data.
    HW_DMA_BUF_SIZE = SIZE;
    aes_dma_copy_and_do();
}

/*
 * Decrypt INPUT, of given SIZE, with AES CBC, and copy the result to OUTPUT.
 */
void aes_decrypt_cbc(void) // 0x0AD
{
    aes_set_null_iv();
    if (SIZE == 0) {
        return;
    }

    HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_DECRYPT;
    aes_setkey();
    HW_DMA_BUF_SIZE = 0x10;
    // Handle complete blocks
    while (SIZE > 0x10) {
        SIZE -= 0x10;
        aes_dma_copy_and_do();
        HW_AES_IV = HW_AES_RESULT;
        dma_write_aes_result();
        INPUT += 0x10;
        OUTPUT += 0x10;
        HW_AES_CMD &= 0xfffffffd;
    }
    aes_dma_copy_and_do();
    HW_DMA_BUF_SIZE = SIZE;
    dma_write_aes_result();
}

/*
 * Decrypt the body of Kirk command 0 body. Same as aes_decrypt_cbc() but reading shifted data.
 */
void kirk_cmd0_decrypt_body(void) // 0x0C9
{
    if (SIZE == 0) {
        return;
    }

    aes_set_null_iv();
    HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_DECRYPT;
    aes_setkey();
    while (SIZE > 0x10) {
        SIZE -= 0x10;
        kirk_cmd0_decrypt_block();
        HW_DMA_BUF_SIZE = 0x10;
        dma_write_aes_result();
        INPUT += 0x10;
        OUTPUT += 0x10;
        HW_AES_CMD &= 0xfffffffd;
    }
    kirk_cmd0_decrypt_block();
    HW_DMA_BUF_SIZE = SIZE;
    dma_write_aes_result();
}

/*
 * Decrypt one block of Kirk command 0 data. The only difference with AES CBC is that since it needs to skip two bytes at the beginning, it will read 20 bytes
 * instead of 16, and shift the data.
 */
void kirk_cmd0_decrypt_block(void) // 0x0E4
{
    HW_DMA_BUF_SIZE = 0x14;
    HW_DMA_ADDR = INPUT;
    dma_read();
    kbooti_block_shift();
    aes_copy_and_do();
}

/*
 * Copy block from given INPUT and SIZE, encrypt/decrypt/... its content and store the result in HW_AES_RESULT.
 */
void aes_dma_copy_and_do(void) // 0x0EB
{
    HW_DMA_ADDR = INPUT;
    dma_read();
    aes_copy_and_do();
}

/*
 * Encrypt/decrypt/CMAC AES data stored in the DMA buffer and store the result in HW_AES_RESULT.
 */
void aes_copy_and_do(void) // 0x0ED
{
    HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
    aes_do();
}

/*
 * Encrypt/decrypt/CMAC AES data stored in the AES hardware registers.
 */
void aes_do(void) // 0x0EE
{
    __builtin_setmode(HW_AES_REG, AES_REG_ENCDEC);
    __builtin_intr(INTR_AES);
}

/*
 * Set the AES key used by aes_do() actions.
 */
void aes_setkey(void) // 0x0F1
{
    __builtin_setmode(HW_AES_REG, AES_REG_SETKEY);
    __builtin_intr(INTR_AES);
}

/*
 * Set the AES IV to 0 (can be used to do AES ECB).
 */
void aes_set_null_iv(void) // 0x0F4
{
    HW_AES_IV[0] = 0;
    HW_AES_IV[1] = 0;
    HW_AES_IV[2] = 0;
    HW_AES_IV[3] = 0;
}

/*
 * Shift data in HW_DMA_BUF by two bytes on the left.
 */
void kirk_cmd0_shift_left(void) // 0x0FD
{
    HW_DMA_BUF[0] = (HW_DMA_BUF[0] << 16) | (HW_DMA_BUF[1] >> 16);
    HW_DMA_BUF[1] = (HW_DMA_BUF[1] << 16) | (HW_DMA_BUF[2] >> 16);
    HW_DMA_BUF[2] = (HW_DMA_BUF[2] << 16) | (HW_DMA_BUF[3] >> 16);
    HW_DMA_BUF[3] = (HW_DMA_BUF[3] << 16) | (HW_DMA_BUF[4] >> 16);
}

/*
 * Pad an AES block stored in HW_DMA_BUF, of size SIZE, with random taken from PRNG output HW_PRNG_RESULT, before writing it to HW_AES_SRC_BUF.
 */
void aes_padding(void) // 0x10E
{
    if (SIZE != 0) {
        R3 = &HW_PRNG_RESULT[0];
        R4 = &HW_DMA_BUF;
        R5 = 0;
        uint nextSize;
        // Find the first incomplete word
        while ((nextSize = SIZE - 4) != 0) {
            if (nextSize < 0) { // Found the first incomplete word
                // Complete this word's bytes with random data
                if (SIZE == 2) {
                    R6 = HW_PRNG_RESULT[0] & 0xffff;
                    *R4 &= 0xffff0000;
                }
                else if (SIZE < 2) { // SIZE = 1
                    R6 = HW_PRNG_RESULT[0] & 0xffffff;
                    *R4 &= 0xff000000;
                }
                else { // SIZE = 3
                    R6 = HW_PRNG_RESULT[0] & 0xff;
                    *R4 &= 0xffffff00;
                }
                *R4 |= R6;
                R3 = R3 + 1;
                nextSize = SIZE;
                break;
            }
            // Check next block
            R5 = R5 + 1;
            R4 = R4 + 1;
            SIZE = nextSize;
        }
        // Fill the remaining words with random data
        while (true) {
            R5 = R5 + 1;
            R4 = R4 + 1;
            if ((int)R5 > 4) {
                break;
            }
            *R4 = *R3;
            R3 = R3 + 1;
        }
    }
    HW_AES_SRC_BUF = HW_DMA_BUF;
}

/*
 * Kirk command 0 : decrypt the kbooti bootrom.
 */
void kirk_cmd0_decrypt_bootrom() // 0x143
{
    // TODO this might be to check this is the first command run on Kirk, but it's not clear when HW_CPU_STATUS switches values.
    if (HW_CPU_STATUS & 2) {
        HW_CPU_PHASE = 1;
        HW_CPU_RESULT = KIRK_INVALID_OPERATION;
        return;
    }
    // Read the body size at 0x10 (two bytes)
    HW_DMA_ADDR = HW_CPU_SRC + 0x10;
    HW_DMA_BUF_SIZE = 2;
    HW_DMA_BUF[0] = HW_DMA_ADDR;
    dma_read();
    KIRK_BODY_SIZE = __builtin_byteswap(HW_DMA_BUF[0]);
    KIRK_BODY_SIZE = KIRK_BODY_SIZE & 0xffff;
    if (KIRK_BODY_SIZE == 0) {
        goto error_size_zero;
    }
    // Retrieve the decryption AES key from key slot 1
    HW_AES_KEYSLOT_ID = 1;
    HW_AES_KEY = HW_AES_KEYSLOT;
    // Compute the CMAC of the beginning of the data
    INPUT = HW_CPU_SRC + 0x10;
    SIZE = KIRK_BODY_SIZE;
    align_size_16();
    SIZE = SIZE + 2;
    aes_set_null_iv();
    HW_AES_CMD = AES_CMD_CMAC | AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK;
    aes_setkey();
    HW_AES_CMAC_SIZE = SIZE;
    HW_DMA_BUF_SIZE = 0x10;
    // TODO that stuff looks suspicious, it should be double-checked
    if (SIZE - 0x10 < 1) {
        // If SIZE < 0x11 (ie there's only one block of data!?)
        HW_DMA_BUF_SIZE = SIZE;
        HW_DMA_BUF[0] = 0;
        HW_DMA_BUF[1] = 0;
        HW_DMA_BUF[2] = 0;
        HW_DMA_BUF[3] = 0;
        aes_dma_copy_and_do();
        if (!kirk_cmd0_check_data_size()) {
            goto error_size_zero;
        }
    }
    else {
        SIZE -= 0x10;
        aes_dma_copy_and_do();
        INPUT += 0x10;
        HW_AES_CMD &= 0xfffffffd;
        if (!kirk_cmd0_check_data_size()) { // TODO doesn't this just read twice at the same offset to check if the value is the same? Why?
            goto error_size_zero;
        }
        aes_decrypt_ecb();
    }
    // Check if the CMAC is the same as the value stored at offset 0x0.
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 0x10;
    dma_read();
    if (!memcmp_cmac()) {
        HW_CPU_RESULT = KIRK_NOT_ENABLED;
        crash();
        return;
    }
    // Read key slot 0 and decrypt the body with it.
    SIZE = KIRK_BODY_SIZE;
    INPUT = HW_CPU_SRC + 0x10;
    OUTPUT = HW_CPU_DST;
    HW_AES_KEYSLOT_ID = 0;
    HW_AES_KEY = HW_AES_KEYSLOT;
    kirk_cmd0_decrypt_body();
    HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    HW_CPU_PHASE = 2;
    return;

error_size_zero:
    HW_CPU_RESULT = KIRK_DATA_SIZE_ZERO;
    crash();
    return;
}

/*
 * Unused function: read the size of the command 0 header.
 */
void __unused_kirk_cmd0_header_get_size(void) // 0x19F
{
    HW_DMA_ADDR = HW_CPU_SRC + 0x10;
    bool nullAddr = HW_DMA_ADDR == 0;
    HW_DMA_BUF_SIZE = 2;
    HW_DMA_BUF[0] = HW_DMA_ADDR;
    dma_read();
    HW_DMA_BUF[0] = __builtin_byteswap(HW_DMA_BUF[0]);
    HW_DMA_BUF[0] = HW_DMA_BUF[0] & 0xffff;
    return nullAddr;
}

/*
 * Check if the Kirk body size is the same as the value in HW_DMA_BUF[0].
 */
bool kirk_cmd0_check_data_size(void) // 0x1AA
{
    HW_DMA_BUF[0] = __builtin_byteswap(HW_DMA_BUF[0]);
    HW_DMA_BUF[0] = HW_DMA_BUF[0] & 0xffff;
    return KIRK_BODY_SIZE == HW_DMA_BUF[0];
}

/*
 * Kirk command 1: decrypt IPL blocks.
 */
void kirk_cmd1_decrypt_private(void) // 0x1AF
{
    if (!kirk_check_enabled()) {
        return;
    }
    // Read the command mode at 0x60
    kirk_signed_header_read_cmd_mode();
    // Check if the command indicated in the block is valid
    if (HW_DMA_BUF[0] != 0x1000000) {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
        HW_CPU_PHASE = 1;
        return;
    }

    // Read body size and data offset from the header
    kirk_signed_header_read_body_info();
    if (!kirk_check_body_size_nonzero()) {
        HW_CPU_PHASE = 1;
        return;
    }

    // Get the AES key from key slot 2
    HW_AES_KEYSLOT_ID = 2;
    CUR_ENC_KEY = HW_AES_KEYSLOT;
    kirk_signed_header_read_sign_mode();
    if (HW_DMA_BUF[0] & KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA) {
        // Set the curve parameters
        ecc_set_priv_curve();
        // Get the public key associated to the signatures
        kirk_cmd1_get_public_key();
        // Check the signature of the header
        if (!kirk_ecdsa_check_header()) {
            HW_CPU_PHASE = 1;
            HW_CPU_RESULT = KIRK_HEADER_SIG_INVALID;
            return;
        }
        // Check the signature of the body
        if (!kirk_ecdsa_check_body()) {
            HW_CPU_PHASE = 1;
            HW_CPU_RESULT = KIRK_DATA_SIG_INVALID;
            return;
        }
    }
    else {
        // Check the CMAC of the header
        if (!kirk_check_cmac()) {
            HW_CPU_PHASE = 1;
            return;
        }
    }
    // Decrypt the body 
    kirk_signed_header_decrypt_body();
}

/*
 * Get the parameters x and y of the public key used for Kirk command 1 from the key slots 0 and 1.
 */
void kirk_cmd1_get_public_key(void) // 0x1D6
{
    HW_ECDSA_KEYSLOT_ID = 0;
    HW_ECC_Px._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_Px[4] = HW_ECDSA_KEYSLOT[4];
    HW_ECDSA_KEYSLOT_ID = 1;
    HW_ECC_Py._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_Py[4] = HW_ECDSA_KEYSLOT[4];
}

/*
 * Kirk command 2: decrypt DRM-encrypted data and reencrypt it using a per-console key.
 */
void kirk_cmd2_dnas_encrypt(void) // 0x1DF
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Check the command indicated in the header
    kirk_signed_header_read_cmd_mode();
    if (HW_DMA_BUF[0] != 0x2000000) {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
        kirk_set_phase1();
        return;
    }
    if (!kirk_is_seeded()) {
        HW_CPU_PHASE = 1;
        return;
    }

    // Read body info (size and offset)
    kirk_signed_header_read_body_info();
    if (!kirk_check_body_size_nonzero()) {
        HW_CPU_PHASE = 1;
        return;
    }

    // Read AES slotted key 3
    HW_AES_KEYSLOT_ID = 3;
    CUR_ENC_KEY = HW_AES_KEYSLOT;
    kirk_signed_header_read_sign_mode();
    ECDSA_MODE = HW_DMA_BUF[0];
    if (HW_DMA_BUF[0] & KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA) {
        // Set the curve parameters and public key associated to this command
        ecc_set_priv_curve();
        kirk_cmd2_get_public_key();
        // Check signature of the header and body
        if (!kirk_ecdsa_check_header()) {
            HW_CPU_RESULT = KIRK_HEADER_SIG_INVALID;
            kirk_set_phase1();
            return;
        }
        if (!kirk_ecdsa_check_body()) {
            HW_CPU_RESULT = KIRK_DATA_SIG_INVALID;
            kirk_set_phase1();
            return;
        }
    } else {
        // Check the CMAC of the data
        if (!kirk_check_cmac()) {
            HW_CPU_PHASE = 1;
            return;
        }
    }
    // Decrypt the key stored in the header with the slotted key
    HW_AES_KEY = CUR_ENC_KEY;
    kirk_signed_decrypt_key();
    // Copy input header to output (which will be a Kirk command 3 header)
    SIZE = KIRK_DATA_OFFSET + 0x90;
    INPUT = HW_CPU_SRC;
    OUTPUT = HW_CPU_DST;
    dma_memcpy();
    // Set command in output header
    HW_DMA_ADDR = HW_CPU_DST + 0x60;
    HW_DMA_BUF[1] = HW_DMA_ADDR; // TODO: useless?
    HW_DMA_BUF[0] = 0x3000000;
    HW_DMA_BUF_SIZE = 4;
    dma_write();
    // Read the signature mode to know if the output should be ECDSA or CMAC-signed, and set the mode of the output block accordingly
    HW_DMA_ADDR = HW_CPU_DST + 0x64;
    HW_DMA_BUF[1] = HW_DMA_ADDR; // TODO: useless?
    HW_DMA_BUF_SIZE = 4;
    dma_read();
    if (ECDSA_MODE & KIRK_SIGNED_HEADER_SIGN_MODE_OUTPUT_ECDSA) {
        HW_DMA_BUF[0] |= KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA;
    } else {
        HW_DMA_BUF[0] &= ~KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA;
    }
    dma_write();
    // Decrypt the body of the data using the decrypted key
    INPUT = HW_CPU_SRC;
    OUTPUT = HW_CPU_DST;
    kirk_cmd2_set_body_params();
    aes_decrypt_cbc();
    // Generate a new key to encrypt the output, and encrypt it using a per-console key (seed = 0)
    kirk_reseed_prng_2();
    HW_DMA_BUF._0_16_ = HW_PRNG_RESULT._4_16_;
    aes_set_perconsole_key_0();
    HW_AES_CMD = AES_CMD_ENCRYPT;
    aes_setkey();
    HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
    aes_do();
    HW_DMA_BUF[0] = HW_CPU_DST;
    aes_output();
    // If the output is using CMAC signing, build a CMAC key
    if (!(ECDSA_MODE & KIRK_SIGNED_HEADER_SIGN_MODE_OUTPUT_ECDSA)) {
        // Use the previous block (encrypted encryption key) as the IV
        HW_AES_IV = HW_AES_RESULT;
        // Compute a random CMAC key
        kirk_reseed_prng_2();
        HW_DMA_BUF._0_16_ = HW_PRNG_RESULT._4_16_;
        // Encrypt this CMAC key with the per-console key (seed = 0)
        aes_set_perconsole_key_0();
        HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_ENCRYPT;
        aes_setkey();
        HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
        aes_do();
        // Store the result at offset 0x10
        HW_DMA_BUF[0] = HW_CPU_DST + 0x10;
        aes_output();
    }
    // Get the data encryption key by re-decrypting the first block
    aes_set_perconsole_key_0();
    CUR_ENC_KEY = HW_AES_RESULT;
    INPUT = HW_CPU_DST;
    OUTPUT = HW_CPU_DST;
    HW_AES_KEY = CUR_ENC_KEY;
    kirk_cmd2_decrypt_key();
    CUR_ENC_KEY = HW_AES_RESULT;
    // Encrypt the body in ECB mode
    kirk_cmd2_set_body_params();
    aes_encrypt_ecb();
    // Compute header & body signature in CMAC or ECDSA mode
    if (!(ECDSA_MODE & KIRK_SIGNED_HEADER_SIGN_MODE_OUTPUT_ECDSA)) {
        // Re-decrypt the CMAC key as stored in the header
        aes_set_perconsole_key_0();
        HW_DMA_ADDR = HW_CPU_DST;
        HW_DMA_BUF_SIZE = 0x20;
        dma_read();
        HW_AES_IV = HW_DMA_BUF._0_16_;
        HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_DECRYPT;
        aes_setkey();
        HW_AES_SRC_BUF = HW_DMA_BUF._16_16_;
        aes_do();
        HW_AES_KEY = HW_AES_RESULT;
        // Compute the CMAC of header 0x60..0x90
        INPUT = HW_CPU_DST + 0x60;
        SIZE = 0x30;
        aes_cmac();
        // Store the result at 0x20
        HW_DMA_BUF[0] = HW_CPU_DST + 0x20;
        aes_output();
        // Compute the CMAC of the header + the body
        SIZE = KIRK_BODY_SIZE + KIRK_DATA_OFFSET + 0x30;
        align_size_16();
        INPUT = HW_CPU_DST + 0x60;
        aes_cmac();
        // Store the result at 0x30
        HW_DMA_BUF[0] = HW_CPU_DST + 0x30;
        aes_output();
    } else {
        // Set the curve and private key to sign the data
        ecc_set_priv_curve();
        kirk_cmd3_get_private_key();
        // Sign the header 0x60..0x90
        INPUT = HW_CPU_DST + 0x60;
        SIZE = 0x30;
        ecc_sign_gen_m_r();
        // Store the resulting point at 0x10
        OUTPUT = HW_CPU_DST + 0x10;
        ecc_output_point();
        // Sign the header + the data
        SIZE = KIRK_BODY_SIZE + KIRK_DATA_OFFSET + 0x30;
        align_size_16();
        INPUT = HW_CPU_DST + 0x60;
        ecc_sign_gen_m_r();
        // Store the resulting point at 0x38
        OUTPUT = HW_CPU_DST + 0x38;
        ecc_output_point();
    }
    kirk_result_success();
}

/*
 * Unused function, returns from a command returning an "invalid mode" error.
 */
void __unused_kirk_result_invalid(void) // 0x289
{
    HW_CPU_RESULT = KIRK_INVALID_MODE;
    kirk_set_phase1();
}

/*
 * Returns from a command returning a success.
 */
void kirk_result_success(void) // 0x28C
{
    HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    kirk_set_phase1();
}

/*
 * Set the return phase to 1 (all commands except command 0 do it except when doing a crash(), which is unrecoverable).
 */
void kirk_set_phase1(void) // 0x28F
{
    HW_CPU_PHASE = 1;
}

/*
 * Take as an input INPUT and OUTPUT pointing to kirk command 2 & 3 headers, and set INPUT, OUTPUT and SIZE to point to the body content
 */
void kirk_cmd2_set_body_params(void) // 0x292
{
    SIZE = KIRK_BODY_SIZE;
    align_size_16();
    INPUT = KIRK_DATA_OFFSET + INPUT + 0x90;
    OUTPUT = KIRK_DATA_OFFSET + OUTPUT + 0x90;
}

/*
 * Output HW_AES_RESULT to the address stored at HW_DMA_BUF[0]
 */
void aes_output(void) // 0x29B
{
    HW_DMA_ADDR = HW_DMA_BUF[0];
    HW_DMA_BUF._0_16_ = HW_AES_RESULT;
    HW_DMA_BUF_SIZE = 0x10;
    dma_write();
}

/*
 * Get the Kirk command 2 public key (used to verify signature in kirk 2 blocks) from the key slots 2 and 3.
 */
void kirk_cmd2_get_public_key(void) // 0x2A1
{
    HW_ECDSA_KEYSLOT_ID = 2;
    HW_ECC_Px._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_Px[4] = HW_ECDSA_KEYSLOT[4];
    HW_ECDSA_KEYSLOT_ID = 3;
    HW_ECC_Py._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_Py[4] = HW_ECDSA_KEYSLOT[4];
}

/*
 * Get the Kirk command 3 private key (used to sign blocks for command 3) from key slot 4.
 */
void kirk_cmd3_get_private_key(void) // 0x2AA
{
    HW_ECDSA_KEYSLOT_ID = 4;
    HW_ECC_S._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_S[4] = HW_ECDSA_KEYSLOT[4];
}

/*
 * Kirk command 3: decrypt data coming from Kirk command 2 and encrypted using a per-console key. Same as Kirk command 1, only the keys differ.
 */
void kirk_cmd3_dnas_decrypt(void) // 0x2AF
{
    if (!kirk_check_enabled()) {
        return;
    }
    // Check that the command set in the header is command 3
    kirk_signed_header_read_cmd_mode();
    if (HW_DMA_BUF[0] == 0x3000000) {
        // Read body size & offset and check that the size is non-zero
        kirk_signed_header_read_body_info();
        if (kirk_check_body_size_nonzero()) {
            // Set the current key to the per-console key
            aes_set_perconsole_key_0();
            CUR_ENC_KEY = HW_AES_RESULT;
            // Verify the ECDSA or CMAC signatures
            kirk_signed_header_read_sign_mode();
            if (HW_DMA_BUF[0] & KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA) {
                // Set the ECC curve and public key
                ecc_set_priv_curve();
                kirk_cmd3_get_public_key();
                if (!kirk_ecdsa_check_header()) {
                    HW_CPU_PHASE = 1;
                    HW_CPU_RESULT = KIRK_HEADER_SIG_INVALID;
                    return;
                }
                if (!kirk_ecdsa_check_body()) {
                    HW_CPU_PHASE = 1;
                    HW_CPU_RESULT = KIRK_DATA_SIG_INVALID;
                    return;
                }
            }
            else {
                if (!kirk_check_cmac()) {
                    HW_CPU_PHASE = 1;
                    return;
                }
            }
            // Decrypt the body of the data
            kirk_signed_header_decrypt_body();
        }
    }
    else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Get the Kirk command 3 public key, used to verify command 3 data generated by command 2, and stored in key slots 5 and 6.
 */
void kirk_cmd3_get_public_key(void) // 0x2D5
{
    HW_ECDSA_KEYSLOT_ID = 5;
    HW_ECC_Px._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_Px[4] = HW_ECDSA_KEYSLOT[4];
    HW_ECDSA_KEYSLOT_ID = 6;
    HW_ECC_Py._0_16_ = HW_ECDSA_KEYSLOT._0_16_;
    HW_ECC_Py[4] = HW_ECDSA_KEYSLOT[4];
}

/*
 * Kirk command 4: encrypt data with a key given by an index called keyseed.
 */
void kirk_cmd4_encrypt_static(void) // 0x2DE
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Read the header of the command (including keyseed but not body size)
    kirk_unsigned_header_read();
    // Check that command is 4 (common to commands 4/5/6) and flags are 0
    if ((HW_DMA_BUF[0] == 0x4000000) && (HW_DMA_BUF[3] == 0)) {
        if (kirk_is_seeded()) {
            kirk_unsigned_header_read_body_size();
            if (kirk_check_body_size_nonzero()) {
                // Read keyseed from the header
                kirk_unsigned_header_read();
                if ((int)HW_DMA_BUF[2] < 0x40) { // Cannot do encryption for keyseed 0x40..0x7f (only decryption is allowed)
                    // Retrieve the appropriate key from its keyslot (index is 4 + <keyseed>)
                    HW_AES_KEYSLOT_ID = HW_DMA_BUF[2] + 4;
                    HW_AES_KEY = HW_AES_KEYSLOT;
                    CUR_ENC_KEY = HW_AES_KEYSLOT;
                    // If keyseed is between 0x20 and 0x2f, reverse bits of the last word (!?)
                    if ((HW_KEY_MESH_3 & 0x80000000) && (int)HW_DMA_BUF[2] > 0x1f && (int)HW_DMA_BUF[2] < 0x30) {
                        CUR_ENC_KEY[3] = CUR_ENC_KEY[3] ^ 0xffffffff;
                    }
                    // If keyseed is between 0x28 and 0x2f, XOR bits of the first word
                    if ((int)HW_DMA_BUF[2] > 0x27 && (int)HW_DMA_BUF[2] < 0x30) {
                        CUR_ENC_KEY[0] ^= HW_KEY_MESH_3;
                    }
                    HW_AES_KEY = CUR_ENC_KEY;
                    // Set the output to match the key slot
                    HW_DMA_BUF[2] = HW_DMA_BUF[2] + 4;
                    // Write the resulting header & body
                    kirk_unsigned_encrypt();
                }
                else {
                    HW_CPU_RESULT = KIRK_INVALID_ENC_SEED;
                }
            }
        }
    }
    else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Generate a Kirk command 7/8 output from commands 4/5.
 */
void kirk_unsigned_encrypt(void) // 0x319
{
    // Write the header. Only the command changes (4 for encryption -> 5 for decryption).
    INPUT = HW_CPU_SRC;
    OUTPUT = HW_CPU_DST;
    SIZE = 0x14;
    dma_memcpy();
    HW_DMA_BUF[0] = 0x5000000;
    HW_DMA_ADDR = HW_CPU_DST;
    HW_DMA_BUF_SIZE = 4;
    dma_write();
    // Encrypt the body using CBC mode with a null IV.
    aes_set_null_iv();
    INPUT = HW_CPU_SRC + 0x14;
    OUTPUT = HW_CPU_DST + 0x14;
    SIZE = KIRK_BODY_SIZE;
    aes_encrypt_cbc();
    HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
}

/*
 * Kirk command 5: encrypt data with a per-console key.
 */
void kirk_cmd5_encrypt_perconsole(void) // 0x330
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Read & check the header (similar to command 4)
    kirk_unsigned_header_read();
    if ((HW_DMA_BUF[0] == 0x4000000) && (HW_DMA_BUF[3] == 0x10000)) {
        if (kirk_is_seeded()) {
            kirk_unsigned_header_read_body_size();
            if (kirk_check_body_size_nonzero()) {
                // Set the AES key to the per-console key with seed 1.
                PERCONSOLE_KEYSEED = 1;
                aes_set_perconsole_key();
                CUR_ENC_KEY = HW_AES_RESULT;
                // Encrypt similarly to command 4
                kirk_unsigned_encrypt();
            }
        }
    }
    else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 6: encrypt data with a random key, and return the encrypted data along with 
 */
void kirk_cmd6_encrypt_user(void) // 0x34A
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Check the header similarly to commands 4 and 5.
    kirk_unsigned_header_read();
    if ((HW_DMA_BUF[0] == 0x4000000) && (HW_DMA_BUF[3] == 0x20000)) {
        if (kirk_is_seeded()) {
            kirk_unsigned_header_read_body_size();
            if (kirk_check_body_size_nonzero()) {
                // Copy the 0x24-sized header from the input to the output, just modifying the command (encrypt -> decrypt).
                INPUT = HW_CPU_SRC;
                OUTPUT = HW_CPU_DST;
                SIZE = 0x24;
                dma_memcpy();
                HW_DMA_BUF[0] = 0x5000000;
                HW_DMA_ADDR = HW_CPU_DST;
                HW_DMA_BUF_SIZE = 4;
                dma_write();

                // Generate a random buffer and encrypt it using per-console key with seed 2
                kirk_reseed_prng_2();
                PERCONSOLE_KEYSEED = 2;
                aes_set_perconsole_key();
                HW_AES_SRC_BUF = HW_PRNG_RESULT._4_16_;
                HW_AES_CMD = AES_CMD_ENCRYPT;
                aes_setkey();
                aes_do();
                // Read the first block of the body in R11
                SIZE = KIRK_BODY_SIZE;
                HW_DMA_ADDR = HW_CPU_SRC + 0x24;
                if ((int)KIRK_BODY_SIZE < 0x10) {
                    HW_DMA_BUF_SIZE = KIRK_BODY_SIZE;
                }
                else {
                    HW_DMA_BUF_SIZE = 0x10;
                }
                HW_DMA_BUF[0] = HW_DMA_ADDR; // TODO: useless?
                dma_read();
                R11._0_16_ = HW_DMA_BUF._0_16_;
                // Store the encrypted random buffer at offset 0x24
                HW_DMA_BUF[0] = HW_CPU_DST + 0x24;
                aes_output();
                // Encrypt the random buffer with the key located at 0x14, and use this as a key to encrypt the body
                HW_DMA_ADDR = HW_CPU_DST + 0x14;
                HW_DMA_BUF_SIZE = 0x10;
                HW_DMA_BUF[0] = HW_DMA_ADDR; // TODO: useless?
                dma_read();
                HW_AES_KEY = HW_DMA_BUF._0_16_;
                HW_AES_SRC_BUF = HW_PRNG_RESULT._4_16_;
                HW_AES_CMD = AES_CMD_ENCRYPT;
                aes_setkey();
                aes_do();
                CUR_ENC_KEY = HW_AES_RESULT;
                // Encrypt the body of the data
                INPUT = HW_CPU_SRC + 0x24;
                OUTPUT = HW_CPU_DST + 0x34;
                kirk_cmd6_encrypt_body();
                HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
            }
        }
    }
    else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Encrypt INPUT to OUTPUT in AES CBC using CUR_ENC_KEY. Only used by command 6, written differently from aes_encrypt_cbc()
 * to use a temporary buffer so that the data can be encrypted "in-place" but 16 bytes further (the output has a shorter header).
 */
void kirk_cmd6_encrypt_body(void) // 0x39D
{
    aes_set_null_iv();
    kirk_reseed_prng_2();
    HW_AES_KEY = CUR_ENC_KEY;
    HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_ENCRYPT;
    aes_setkey();
    HW_DMA_BUF_SIZE = 0x10;
    while (SIZE > 0x10) {
        HW_AES_SRC_BUF = R11._0_16_;
        SIZE -= 0x10;
        aes_do();
        HW_AES_IV = HW_AES_RESULT;
        INPUT += 0x10;
        if (SIZE < 0x10) {
            HW_DMA_BUF_SIZE = SIZE;
        }
        dma_read_input();
        R11._0_16_ = HW_DMA_BUF._0_16_;
        HW_DMA_BUF_SIZE = 0x10;
        dma_write_aes_result();
        OUTPUT += 0x10;
        HW_AES_CMD &= 0xfffffffd;
    }
    HW_DMA_BUF._0_16_ = R11._0_16_;
    aes_padding();
    HW_AES_CMD |= 2;
    aes_do();
    HW_DMA_BUF_SIZE = 0x10;
    dma_write_aes_result();
}

/*
 * Kirk command 7: decryption counterpart of command 4. The only difference is that more keyseeds are allowed.
 */
void kirk_cmd7_decrypt_static(void) // 0x3C6
{
    if (!kirk_check_enabled()) {
        return;
    }

    kirk_unsigned_header_read();
    if ((HW_DMA_BUF[0] == 0x5000000) && (HW_DMA_BUF[3] == 0)) {
        kirk_unsigned_header_read_body_size();
        if (kirk_check_body_size_nonzero()) {
            kirk_unsigned_header_read();
            if ((int)HW_DMA_BUF[2] < 0x80) {
                HW_AES_KEYSLOT_ID = HW_DMA_BUF[2] + 4;
                HW_AES_KEY = HW_AES_KEYSLOT;
                CUR_ENC_KEY = HW_AES_KEYSLOT;
                // If the MSB of HW_KEY_MESH_3 is 1 and the keyseed is in the 0x20..0x2f or 0x6c..0x7b range, invert bits of the last word of the key
                if ((HW_KEY_MESH_3 & 0x80000000) && (int)HW_DMA_BUF[2] > 0x1f && (int)HW_DMA_BUF[2] < 0x7c
                   && ((int)HW_DMA_BUF[2] < 0x30 || (int)HW_DMA_BUF[2] > 0x6b)) {
                    CUR_ENC_KEY[3] ^= 0xffffffff;
                }
                // If the keyseed is in the 0x27..0x2f or 0x73..0x7b range, XOR first word of the key with HW_KEY_MESH_3
                if ((int)HW_DMA_BUF[2] > 0x27 && (int)HW_DMA_BUF[2] < 0x7c &&
                   ((int)HW_DMA_BUF[2] < 0x30 || 0x73 < (int)HW_DMA_BUF[2])) {
                    CUR_ENC_KEY[0] ^= HW_KEY_MESH_3;
                }
                HW_AES_KEY = CUR_ENC_KEY;
                HW_DMA_BUF[2] = HW_DMA_BUF[2] + 4;
                INPUT = 0x14;
                kirk_unsigned_decrypt();
            }
            else {
                HW_CPU_RESULT = KIRK_INVALID_DEC_SEED;
            }
        }
    }
    else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 8: decryption counterpart of command 5.
 */
void kirk_cmd8_decrypt_perconsole(void) // 0x40D
{
    if (!kirk_check_enabled()) {
        return;
    }

    kirk_unsigned_header_read();
    if ((HW_DMA_BUF[0] == 0x5000000) && (HW_DMA_BUF[3] == 0x10000)) {
        kirk_unsigned_header_read_body_size();
        if (kirk_check_body_size_nonzero()) {
            PERCONSOLE_KEYSEED = 1;
            aes_set_perconsole_key();
            INPUT = 0x14;
            kirk_unsigned_decrypt();
        }
    }
    else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 9: decryption counterpart of command 6.
 */
void kirk_cmd9_decrypt_user(void) // 0x426
{
    if (!kirk_check_enabled()) {
        return;
    }

    kirk_unsigned_header_read();
    if ((HW_DMA_BUF[0] == 0x5000000) && (HW_DMA_BUF[3] == 0x20000)) {
        kirk_unsigned_header_read_body_size();
        if (kirk_check_body_size_nonzero()) {
            // Compute the per-console key with seed 2
            PERCONSOLE_KEYSEED = 2;
            aes_set_perconsole_key();
            // Read the keyseed and encrypted key
            HW_DMA_ADDR = HW_CPU_SRC + 0x14;
            HW_DMA_BUF_SIZE = 0x20;
            HW_DMA_BUF[0] = HW_DMA_ADDR;
            dma_read();
            // Compute the real key by decrypting 0x24 with the per-console key and re-encrypting it with the key at 0x14
            HW_AES_CMD = AES_CMD_DECRYPT;
            HW_AES_SRC_BUF = HW_DMA_BUF._16_16_;
            aes_setkey();
            aes_do();
            HW_AES_CMD = AES_CMD_ENCRYPT;
            HW_AES_KEY = HW_DMA_BUF._0_16_;
            HW_AES_SRC_BUF = HW_AES_RESULT;
            aes_setkey();
            aes_do();
            HW_AES_KEY = HW_AES_RESULT;
            INPUT = 0x34;
            // Decrypt the data body
            kirk_unsigned_decrypt();
        }
    } else {
        HW_CPU_RESULT = KIRK_INVALID_MODE;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Compute the hash of the message given its INPUT address and SIZE, and sign it using the private key in 's'
 */
void ecc_sign_gen_m_r(void) // 0x452
{
    sha1_do();
    HW_ECC_M._0_16_ = HW_SHA1_RESULT._0_16_;
    HW_ECC_M[4] = HW_SHA1_RESULT[4];
    ecc_sign_gen_r();
}

/*
 * Compute a signature for a given message using the private key in 's'
 */
void ecc_sign_gen_r(void) // 0x455
{
    // This probably loops until we find a valid r
    do {
        // Compute a random scalar r
        math_generate_random();
        HW_ECC_R._0_16_ = HW_MATH_RESULT._12_16_;
        HW_ECC_R[4] = HW_MATH_RESULT[7];
        // Compute the resulting s
        HW_ECC_OP = ECC_OP_SIGN;
        ecc_do();
    } while (!(HW_ECC_RESULT & 1));
    RNG_FLAGS &= ~RNG_FLAGS_PUB_CURVE;
}

/*
 * Compute random data in a given range using an unknown PRNG.
 */
void math_generate_random(void) // 0x45F
{
    do {
        // Define the range depending on the bit 1 of RNG_FLAGS
        if (RNG_FLAGS & RNG_FLAGS_PUB_CURVE) {
            math_set_public_modulus();
        } else {
            math_set_private_modulus();
        }
        // Fill HW_MATH_IN 0 to 5 with 0's and 6 to 15 with random
        HW_MATH_IN[0] = 0;
        HW_MATH_IN[1] = 0;
        HW_MATH_IN[2] = 0;
        HW_MATH_IN[3] = 0;
        HW_MATH_IN[4] = 0;
        HW_MATH_IN[5] = 0;
        kirk_reseed_prng_2();
        HW_MATH_IN._24_16_ = HW_PRNG_RESULT._0_16_;
        HW_MATH_IN[10] = HW_PRNG_RESULT[4];
        kirk_reseed_prng_2();
        HW_MATH_IN._44_16_ = HW_PRNG_RESULT._0_16_;
        HW_MATH_IN[15] = HW_PRNG_RESULT[4];
        // Compute the result in the given range
        __builtin_setmode(HW_MATH_REG, MATH_REG_DO);
        __builtin_intr(INTR_MATH);
        // If the buffer is only zero's, retry
        R13 = 5;
        R11 = &HW_MATH_RESULT[3]
    } while (!buffer_is_nonzero());
}

/*
 * Hash a message and verify its ECDSA signature.
 */
bool ecc_sign_hash_and_check(void) // 0x47F
{
    if (SIZE != 0) {
        sha1_do();
        if (!ecc_sign_check()) {
            return true;
        }
    }
    return false;
}

/*
 * Verify an ECDSA signature of the data stored in HW_SHA1_RESULT.
 */
bool ecc_sign_check(void) // 0x483
{
    HW_ECC_M._0_16_ = HW_SHA1_RESULT._0_16_;
    HW_ECC_M[4] = HW_SHA1_RESULT[4];
    HW_ECC_OP = ECC_OP_CHECK_SIGN;
    ecc_do();
    return (HW_ECC_RESULT & 1);
}

/*
 * Read r and s parameters of a signature from INPUT.
 */
void ecc_read_signature(void) // 0x48C
{
    HW_DMA_BUF_SIZE = 0x14;
    dma_read_input();
    HW_ECC_R._0_16_ = HW_DMA_BUF._0_16_;
    HW_ECC_R[4] = HW_DMA_BUF[4];
    INPUT = INPUT + 0x14;
    dma_read_input();
    HW_ECC_S._0_16_ = HW_DMA_BUF._0_16_;
    HW_ECC_S[4] = HW_DMA_BUF[4];
}

/*
 * Output the resulting point to OUTPUT.
 */
void ecc_output_point(void) // 0x497
{
    HW_DMA_BUF._0_16_ = HW_ECC_RESULT_X._0_16_;
    HW_DMA_BUF[4] = HW_ECC_RESULT_X[4];
    HW_DMA_BUF_SIZE = 0x14;
    HW_DMA_ADDR = OUTPUT;
    dma_write();
    HW_DMA_BUF._0_16_ = HW_ECC_RESULT_Y._0_16_;
    HW_DMA_BUF[4] = HW_ECC_RESULT_Y[4];
    HW_DMA_ADDR = OUTPUT + 0x14;
    OUTPUT = HW_DMA_ADDR;
    dma_write();
}

/*
 * Compute the SHA1 of the INPUT and put the result in HW_SHA1_RESULT.
 */
void sha1_do(void) // 0x4A4
{
    // If size is zero, do nothing.
    if (SIZE == 0) {
        return;
    }

    // Set the real input size
    HW_SHA1_IN_SIZE = SIZE;
    HW_DMA_BUF_SIZE = 4;
    // Send data by words
    SIZE = (SIZE + 3) / 4;
    do {
        dma_read_input();
        HW_SHA1_IN_DATA = HW_DMA_BUF[0];
        INPUT += 4;
        SIZE -= 1;
    } while (SIZE != 0);
    // Retrieve result
    __builtin_intr(INTR_SHA1);
    SIZE = 0;
}

/*
 * Do an ECC operation depending on the value of HW_ECC_CMD.
 */
void ecc_do(void) // 0x4B5
{
    __builtin_setmode(HW_ECC_REG, ECC_REG_DO);
    __builtin_intr(INTR_ECC);
}

/*
 * Set the ECC parameters to the "private" curve used by Kirk commands 1/2/3. Parameters are:
 *
 * p = 0xFFFFFFFFFFFFFFFF0001B5C617F290EAE1DBAD8F
 * G = (0x2259ACEE15489CB096A882F0AE1CF9FD8EE5F8FA, 0x604358456D0A1CB2908DE90F27D75C82BEC108C0)
 * n = 0xFFFFFFFFFFFFFFFF00000001FFFFFFFFFFFFFFFF
 * a = -3
 * b = 0x65D1488C0359E234ADC95BD3908014BD91A525F9
 */
void ecc_set_priv_curve(void) // 0x4B8
{
    HW_ECC_A[0] = 0xffffffff;
    HW_ECC_A[1] = 0xffffffff;
    HW_ECC_A[2] = 1;
    HW_ECC_A[3] = 0xffffffff;
    HW_ECC_A[4] = 0xfffffffc;
    HW_ECC_B[0] = 0x65d1488c;
    HW_ECC_B[1] = 0x359e234;
    HW_ECC_B[2] = 0xadc95bd3;
    HW_ECC_B[3] = 0x908014bd;
    HW_ECC_B[4] = 0x91a525f9;
    HW_ECC_P[0] = 0xffffffff;
    HW_ECC_P[1] = 0xffffffff;
    HW_ECC_P[2] = 1;
    HW_ECC_P[3] = 0xffffffff;
    HW_ECC_P[4] = 0xffffffff;
    HW_ECC_N[0] = 0xffffffff;
    HW_ECC_N[1] = 0xffffffff;
    HW_ECC_N[2] = 0x1b5c6;
    HW_ECC_N[3] = 0x17f290ea;
    HW_ECC_N[4] = 0xe1dbad8f;
    HW_ECC_Gx[0] = 0x2259acee;
    HW_ECC_Gx[1] = 0x15489cb0;
    HW_ECC_Gx[2] = 0x96a882f0;
    HW_ECC_Gx[3] = 0xae1cf9fd;
    HW_ECC_Gx[4] = 0x8ee5f8fa;
    HW_ECC_Gy[0] = 0x60435845;
    HW_ECC_Gy[1] = 0x6d0a1cb2;
    HW_ECC_Gy[2] = 0x908de90f;
    HW_ECC_Gy[3] = 0x27d75c82;
    HW_ECC_Gy[4] = 0xbec108c0;
    // TODO: unknown parameters
    HW_ECC_UNK1[0] = 4;
    HW_ECC_UNK1[1] = 0xfffffffc;
    HW_ECC_UNK1[2] = 4;
    HW_ECC_UNK1[3] = 0;
    HW_ECC_UNK1[4] = 0xfffffffd;
    HW_ECC_UNK2[0] = 0xbcb8c536;
    HW_ECC_UNK2[1] = 0x6c7b11e5;
    HW_ECC_UNK2[2] = 0xa251c108;
    HW_ECC_UNK2[3] = 0xc0b3500c;
    HW_ECC_UNK2[4] = 0x60f7e9f7;
}

/*
 * Set the ECC parameters to the "public" curve used by Kirk commands 12/13/14/16/17. Parameters are:
 *
 * p = 0xFFFFFFFFFFFFFFFEFFFFB5AE3C523E63944F2127
 * G = (0x128EC4256487FD8FDF64E2437BC0A1F6D5AFDE2C, 0x5958557EB1DB001260425524DBC379D5AC5F4ADF)
 * n = 0xFFFFFFFFFFFFFFFF00000001FFFFFFFFFFFFFFFF
 * a = -3
 * b = 0xA68BEDC33418029C1D3CE33B9A321FCCBB9E0F0B
 */
void ecc_set_public_curve(void) // 0x509
{
    HW_ECC_A[0] = 0xffffffff;
    HW_ECC_A[1] = 0xffffffff;
    HW_ECC_A[2] = 1;
    HW_ECC_A[3] = 0xffffffff;
    HW_ECC_A[4] = 0xfffffffc;
    HW_ECC_B[0] = 0xa68bedc3;
    HW_ECC_B[1] = 0x3418029c;
    HW_ECC_B[2] = 0x1d3ce33b;
    HW_ECC_B[3] = 0x9a321fcc;
    HW_ECC_B[4] = 0xbb9e0f0b;
    HW_ECC_P[0] = 0xffffffff;
    HW_ECC_P[1] = 0xffffffff;
    HW_ECC_P[2] = 1;
    HW_ECC_P[3] = 0xffffffff;
    HW_ECC_P[4] = 0xffffffff;
    HW_ECC_N[0] = 0xffffffff;
    HW_ECC_N[1] = 0xfffffffe;
    HW_ECC_N[2] = 0xffffb5ae;
    HW_ECC_N[3] = 0x3c523e63;
    HW_ECC_N[4] = 0x944f2127;
    HW_ECC_Gx[0] = 0x128ec425;
    HW_ECC_Gx[1] = 0x6487fd8f;
    HW_ECC_Gx[2] = 0xdf64e243;
    HW_ECC_Gx[3] = 0x7bc0a1f6;
    HW_ECC_Gx[4] = 0xd5afde2c;
    HW_ECC_Gy[0] = 0x5958557e;
    HW_ECC_Gy[1] = 0xb1db0012;
    HW_ECC_Gy[2] = 0x60425524;
    HW_ECC_Gy[3] = 0xdbc379d5;
    HW_ECC_Gy[4] = 0xac5f4adf;
    // TODO: unknown parameters
    HW_ECC_UNK1[0] = 4;
    HW_ECC_UNK1[1] = 0xfffffffc;
    HW_ECC_UNK1[2] = 4;
    HW_ECC_UNK1[3] = 0;
    HW_ECC_UNK1[4] = 0xfffffffd;
    HW_ECC_UNK2[0] = 0x9ceee277;
    HW_ECC_UNK2[1] = 0xb4d7bac8;
    HW_ECC_UNK2[2] = 0xe17d0c9e;
    HW_ECC_UNK2[3] = 0x5b6bb2a8;
    HW_ECC_UNK2[4] = 0x64d06c1c;
}

/*
 * Set the math module modulus to the order of the "private" curve
 */
void math_set_private_modulus(void) // 0x55A
{
    HW_MATH_MODULUS[0] = 0;
    HW_MATH_MODULUS[1] = 0;
    HW_MATH_MODULUS[2] = 0;
    HW_MATH_MODULUS[3] = 0xffffffff;
    HW_MATH_MODULUS[4] = 0xffffffff;
    HW_MATH_MODULUS[5] = 0x0001b5c6;
    HW_MATH_MODULUS[6] = 0x17f290ea;
    HW_MATH_MODULUS[7] = 0xe1dbad8f;
}

/*
 * Set the math module modulus to the order of the "public" curve
 */
void math_set_public_modulus(void) // 0x56B
{
    HW_MATH_MODULUS[0] = 0;
    HW_MATH_MODULUS[1] = 0;
    HW_MATH_MODULUS[2] = 0;
    HW_MATH_MODULUS[3] = 0xffffffff;
    HW_MATH_MODULUS[4] = 0xfffffffe;
    HW_MATH_MODULUS[5] = 0xffffb5ae;
    HW_MATH_MODULUS[6] = 0x3c523e63;
    HW_MATH_MODULUS[7] = 0x944f2127;
}

/*
 * Read body size and data offset from a Kirk signed header (commands 1/2/3/10)
 */
void kirk_signed_header_read_body_info(void) // 0x57C
{
    // Body size is at 0x70, and data offset is at 0x74
    HW_DMA_ADDR = HW_CPU_SRC + 0x70;
    HW_DMA_BUF_SIZE = 8;
    HW_DMA_BUF[0] = HW_DMA_ADDR;
    dma_read();
    // Fix endianness
    HW_DMA_BUF[0] = __builtin_byteswap(HW_DMA_BUF[0]);
    KIRK_BODY_SIZE = HW_DMA_BUF[0];
    HW_DMA_BUF[1] = __builtin_byteswap(HW_DMA_BUF[1]);
    KIRK_DATA_OFFSET = HW_DMA_BUF[1];
}

/*
 * Read body size from a Kirk unsigned header (commands 4/5/6/7/8/9)
 */
void kirk_unsigned_header_read_body_size(void) // 0x588
{
    HW_DMA_ADDR = HW_CPU_SRC + 0x10;
    HW_DMA_BUF_SIZE = 4;
    HW_DMA_BUF[0] = HW_DMA_ADDR;
    dma_read();
    HW_DMA_BUF[0] = __builtin_byteswap(HW_DMA_BUF[0]);
    KIRK_BODY_SIZE = HW_DMA_BUF[0];
}

/*
 * Check the CMAC of the header and body for commands 1/2/3
 */
bool kirk_check_cmac(void) // 0x592
{
    HW_AES_KEY = CUR_ENC_KEY;
    if (kirk_cmac_check_header()) {
        if (kirk_cmac_check_body()) {
            return 1;
        }
        kirk_signed_wipe_data();
    }
    return 0;
}

/*
 * Wipe data if the signature of the body is invalid and 0x68 is set to 1
 */
void kirk_signed_wipe_data(void) // 0x59C
{
    // Read the LSB bit at 0x68 (when in little endian)
    HW_DMA_ADDR = HW_CPU_SRC + 0x68;
    HW_DMA_BUF_SIZE = 4;
    HW_DMA_BUF[0] = HW_DMA_ADDR;
    dma_read();
    if (HW_DMA_BUF[0] & 0x01000000) {
        // Overwrite the full header + body with zero's
        kirk_signed_get_full_size();
        OUTPUT = HW_CPU_SRC;
        dma_bzero();
    }
}

/*
 * Check the CMAC signature of a signed header.
 */
void kirk_cmac_check_header(void) // 0x5A9
{
    kirk_signed_header_decrypt_cmac_key();
    // Compute the CMAC of 0x60..0x90
    INPUT = HW_CPU_SRC + 0x60;
    aes_set_null_iv();
    HW_AES_CMD = AES_CMD_CMAC | AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK;
    aes_setkey();
    HW_AES_CMAC_SIZE = 0x30;
    HW_DMA_BUF_SIZE = 0x10;
    aes_dma_copy_and_do();
    INPUT += 0x10;
    HW_AES_CMD &= 0xfffffffd;
    aes_dma_copy_and_do();
    INPUT += 0x10;
    // Verify the data is what we read earlier (maybe to avoid attacks based on overwriting?)
    HW_DMA_BUF[0] = __builtin_byteswap(HW_DMA_BUF[0]);
    if (HW_DMA_BUF[0] == KIRK_BODY_SIZE) {
        HW_DMA_BUF[1] = __builtin_byteswap(HW_DMA_BUF[1]);
        if (HW_DMA_BUF[1] == KIRK_DATA_OFFSET) {
            aes_dma_copy_and_do();
            // Check if the CMAC is the same as offset 0x20
            INPUT = HW_CPU_SRC + 0x20;
            HW_DMA_BUF_SIZE = 0x10;
            dma_read_input();
            if (memcmp_cmac()) {
                return;
            }
        }
    }
    HW_CPU_RESULT = KIRK_HEADER_SIG_INVALID;
}

/*
 * Check the CMAC signature of the header + body of the command.
 */
void kirk_cmac_check_body(void) // 0x5CF
{
    // Compute the CMAC of 0x60..(end of body)
    INPUT = HW_CPU_SRC + 0x60;
    kirk_signed_header_get_size_header2_body();
    aes_cmac();
    // Compare this value with the value at 0x30
    INPUT = HW_CPU_SRC + 0x30;
    HW_DMA_BUF_SIZE = 0x10;
    dma_read_input();
    if (!memcmp_cmac()) {
        HW_CPU_RESULT = KIRK_DATA_SIG_INVALID;
    }
}

/*
 * Check the ECDSA signature of a signed header.
 */
void kirk_ecdsa_check_header(void) // 0x5DF
{
    // Read the signature at offset 0x10
    INPUT = HW_CPU_SRC + 0x10;
    ecc_read_signature();
    // Hash the data 0x60..0x80
    INPUT = HW_CPU_SRC + 0x60;
    SIZE = 0x30;
    HW_SHA1_IN_SIZE = 0x30;
    HW_DMA_BUF_SIZE = 0x20;
    dma_read_input();
    HW_SHA1_IN_DATA = HW_DMA_BUF[0];
    HW_SHA1_IN_DATA = HW_DMA_BUF[1];
    HW_SHA1_IN_DATA = HW_DMA_BUF[2];
    HW_SHA1_IN_DATA = HW_DMA_BUF[3];
    HW_SHA1_IN_DATA = HW_DMA_BUF[4];
    HW_SHA1_IN_DATA = HW_DMA_BUF[5];
    HW_SHA1_IN_DATA = HW_DMA_BUF[6];
    HW_SHA1_IN_DATA = HW_DMA_BUF[7];
    // Check that the size/offset are still what we read earlier
    HW_DMA_BUF[4] = __builtin_byteswap(HW_DMA_BUF[4]);
    HW_DMA_BUF[5] = __builtin_byteswap(HW_DMA_BUF[5]);
    if (HW_DMA_BUF[4] == KIRK_BODY_SIZE && HW_DMA_BUF[5] == KIRK_DATA_OFFSET) {
        // Hash 0x80..0x90
        INPUT += 0x20;
        HW_DMA_BUF_SIZE = 0x10;
        dma_read_input();
        HW_SHA1_IN_DATA = HW_DMA_BUF[0];
        HW_SHA1_IN_DATA = HW_DMA_BUF[1];
        HW_SHA1_IN_DATA = HW_DMA_BUF[2];
        HW_SHA1_IN_DATA = HW_DMA_BUF[3];
        __builtin_intr(INTR_SHA1);
        // Verify the signature
        if (ecc_sign_check()) {
            return true;
        }
    }
    HW_CPU_RESULT = KIRK_HEADER_SIG_INVALID;
    return false;
}

/*
 * Check the ECDSA signature of the second header + the body of a signed command.
 */
void kirk_ecdsa_check_body(void) // 0x60A
{
    // Read the signature at 0x38
    INPUT = HW_CPU_SRC + 0x38;
    ecc_read_signature();
    // Hash the data at 0x60..(end of body) and verify the signature
    INPUT = HW_CPU_SRC + 0x60;
    kirk_signed_header_get_size_header2_body();
    if (!ecc_sign_hash_and_check()) {
        kirk_signed_wipe_data();
        HW_CPU_RESULT = KIRK_DATA_SIG_INVALID;
    }
}

/*
 * Decrypt the body of an unsigned command (commands 7/8/9) from the offset given in INPUT.
 */
void kirk_unsigned_decrypt(void) // 0x618
{
    INPUT += HW_CPU_SRC;
    OUTPUT = HW_CPU_DST;
    SIZE = KIRK_BODY_SIZE;
    aes_decrypt_cbc();
    HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
}

/*
 * Decrypt the body of a signed command (commands 1/3).
 */
void kirk_signed_header_decrypt_body(void) // 0x61F
{
    HW_AES_KEY = CUR_ENC_KEY;
    kirk_signed_decrypt_key();
    INPUT = KIRK_DATA_OFFSET + HW_CPU_SRC + 0x90;
    OUTPUT = HW_CPU_DST;
    SIZE = KIRK_BODY_SIZE;
    aes_decrypt_cbc();
    HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
}

/*
 * Get the size of the full header + body of signed data.
 */
void kirk_signed_get_full_size(void) // 0x62B
{
    SIZE = KIRK_DATA_OFFSET + KIRK_BODY_SIZE + 0x90;
    align_size_16();
}

/*
 * Get the size of the second header + body of signed data.
 */
void kirk_signed_header_get_size_header2_body(void) // 0x62E
{
    SIZE = KIRK_DATA_OFFSET + KIRK_BODY_SIZE + 0x30;
    align_size_16();
}

/*
 * Read the sign mode from the header of a signed command. Bit 0 is 1 if ECDSA, CMAC otherwise; bit 1 indicates to command
 * 2 if it should use ECDSA (value 1) or CMAC (value 2) for the resulting Kirk 3 block.
 */
void kirk_signed_header_read_sign_mode(void) // 0x634
{
    HW_DMA_BUF[0] = HW_CPU_SRC + 0x64;
    HW_DMA_ADDR = HW_DMA_BUF[0];
    HW_DMA_BUF_SIZE = 4;
    dma_read();
}

/*
 * Read the command mode from a signed header. This should be equal to the command ID.
 */
void kirk_signed_header_read_cmd_mode(void) // 0x637
{
    HW_DMA_BUF[0] = HW_CPU_SRC + 0x60;
    HW_DMA_ADDR = HW_DMA_BUF[0];
    HW_DMA_BUF_SIZE = 4;
    dma_read();
}

/*
 * Decrypt the CMAC key from the header (at offset 0x20) using the current HW_AES_KEY.
 */
void kirk_signed_header_decrypt_cmac_key(void) // 0x63F
{
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 0x20;
    dma_read();
    // Note: this looks like they could've just set HW_AES_IV to HW_DMA_BUF._0_16_ since the result of the first block is ignored
    aes_set_null_iv();
    HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_DECRYPT;
    aes_setkey();
    HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
    aes_do();
    HW_AES_SRC_BUF = HW_DMA_BUF._16_16_;
    HW_AES_CMD &= 0xfffffffd;
    aes_do();
    HW_AES_KEY = HW_AES_RESULT;
}

/*
 * Decrypt the data encryption key for Kirk command 2, stored in INPUT, using the current HW_AES_KEY, and store the result in HW_AES_KEY.
 */
void kirk_cmd2_decrypt_key(void) // 0x64E
{
    HW_DMA_ADDR = INPUT;
    HW_DMA_BUF_SIZE = 0x10;
    dma_read();
    HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
    HW_AES_CMD = AES_CMD_DECRYPT;
    aes_setkey();
    aes_do();
    HW_AES_KEY = HW_AES_RESULT;
}

/*
 * Same as kirk_cmd2_decrypt_key but taking as an input the beginning of the Kirk header.
 */
void kirk_signed_decrypt_key(void) // 0x650
{
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 0x10;
    dma_read();
    HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
    HW_AES_CMD = AES_CMD_DECRYPT;
    aes_setkey();
    aes_do();
    HW_AES_KEY = HW_AES_RESULT;
}

/*
 * Unused function, reading the size of the decrypted data from the header.
 */
void __unused_kirk_signed_header_get_decrypted_size(void) // 0x65B
{
    HW_DMA_BUF[0] = HW_CPU_SRC + 0x70;
    HW_DMA_ADDR = HW_DMA_BUF[0];
    HW_DMA_BUF_SIZE = 4;
    dma_read();
    HW_DMA_BUF[0] = __builtin_byteswap(HW_DMA_BUF[0]);
}

/*
 * Write SIZE 0's to the OUTPUT.
 */
void dma_bzero(void) // 0x667
{
    HW_DMA_BUF[0] = 0;
    HW_DMA_BUF[1] = 0;
    HW_DMA_BUF[2] = 0;
    HW_DMA_BUF[3] = 0;
    HW_DMA_BUF._16_16_ = 0;
    HW_DMA_BUF_SIZE = 0x20;
    while (SIZE > 0x20) {
        SIZE -= 0x20;
        dma_write_output();
        OUTPUT += 0x20;
    }
    HW_DMA_BUF_SIZE = SIZE;
    dma_write_output();
}

/*
 * Read the header of an unsigned command, excluding the body size.
 */
void kirk_unsigned_header_read(void) // 0x67C
{
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 0x10;
    dma_read();
    HW_DMA_BUF[2] = HW_DMA_BUF[3] >> 24; // Key seed
    HW_DMA_BUF[3] = HW_DMA_BUF[3] & 0x30000; // 0 = Kirk 4/7, 1 = Kirk 5/8, 2 = Kirk 6/9
}

/*
 * Copy INPUT to OUTPUT with given SIZE.
 */
void dma_memcpy(void) // 0x685
{
    HW_DMA_BUF_SIZE = 0x20;
    while (SIZE > 0x20) {
        SIZE -= 0x20;
        dma_read_input();
        dma_write_output();
        INPUT += 0x20;
        OUTPUT += 0x20;
    }
    HW_DMA_BUF_SIZE = SIZE;
    dma_read_input();
    dma_write_output();
}

/*
 * Compare HW_AES_RESULT to HW_DMA_BUF contents, to check the result of a CMAC.
 */
bool memcmp_cmac(void) // 0x698
{
    R11 = &HW_AES_RESULT;
    R12 = &HW_DMA_BUF;
    R13 = 4;
    do {
        if (*(int *)R11 != *(int *)R12) {
            return false;
        }
        R11++;
        R12++;
        R13--;
    } while (R13 != 0);
    return true;
}

/*
 * Check if the PSP interface is enabled.
 */
bool kirk_check_enabled(void) // 0x6A5
{
    if (!(HW_CPU_STATUS & 2)) {
        HW_CPU_RESULT = KIRK_NOT_ENABLED;
        crash();
    }
    return true;
}

/*
 * Check if command 15 (init/reseed) was run.
 */
bool kirk_is_seeded(void) // 0x6AE
{
    if (!(RNG_FLAGS & RNG_FLAGS_SEEDED)) {
        HW_CPU_RESULT = KIRK_NOT_INITIALIZED;
        return false;
    }
    return true;
}

/*
 * Align SIZE to the upper multiple of 16.
 */
void align_size_16(void) // 0x6B4
{
    SIZE = (SIZE + 0xf) & 0xfffffff0;
}

/*
 * Write HW_AES_RESULT to OUTPUT.
 */
void dma_write_aes_result(void) // 0x6B9
{
    HW_DMA_BUF._0_16_ = HW_AES_RESULT._0_16_;
    dma_write_output();
}

/*
 * Write HW_DMA_BUF to OUTPUT.
 */
void dma_write_output(void) // 0x6BA
{
    HW_DMA_ADDR = OUTPUT;
    dma_write();
}

/*
 * Write HW_DMA_BUF to HW_DMA_ADDR.
 */
void dma_write(void) // 0x6BB
{
    __builtin_setmode(HW_DMA_REG, DMA_REG_WRITE);
    __builtin_intr(INTR_DMA);
}

/*
 * Read data from INPUT.
 */
void dma_read_input(void) // 0x6BE
{
    HW_DMA_ADDR = INPUT;
    dma_read();
}

/*
 * Read data from HW_DMA_ADDR.
 */
void dma_read(void) // 0x6BF
{
    __builtin_setmode(HW_DMA_REG, DMA_REG_READ);
    __builtin_intr(INTR_DMA);
}

/*
 * Check if the read Kirk body size is non-zero.
 */
bool kirk_check_body_size_nonzero(void) // 0x6C2
{
    if (KIRK_BODY_SIZE == 0) {
        HW_CPU_RESULT = KIRK_DATA_SIZE_ZERO;
    }
    return KIRK_BODY_SIZE != 0;
}

/*
 * Check if a buffer, given by its pointer (R11) and size (R13), is not entirely zero's.
 */
bool buffer_is_nonzero(void) // 0x6CB
{
    do {
        int cur = *(int *)R11;
        R11++;
        if (cur != 0) {
            return true;
        }
        R13--;
    } while (R13 != 0);
    return false;
}

/*
 * Triggered on fatal error. Enter an infinite loop.
 */
void crash(void) // 0x6D4
{
    HW_CPU_PHASE = 3;
    __builtin_setmode(HW_CPU_REG, CPU_REG_DO);
    __builtin_intr(INTR_CPU);
    inf_loop();
}

/*
 * Kirk command 10: verify signatures of command 1/2/3 headers.
 */
void kirk_cmd10_priv_sigvry(void) // 0x6D9
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Set the appropriate CMAC & ECDSA keys depending on the command
    kirk_signed_header_read_cmd_mode();
    if (HW_DMA_BUF[0] == 0x1000000) {
        HW_AES_KEYSLOT_ID = 2;
        CUR_ENC_KEY = HW_AES_KEYSLOT;
        kirk_cmd1_get_public_key();
    }
    else if (HW_DMA_BUF[0] == 0x2000000) {
        HW_AES_KEYSLOT_ID = 3;
        CUR_ENC_KEY = HW_AES_KEYSLOT;
        kirk_cmd2_get_public_key();
    }
    else {
        if (HW_DMA_BUF[0] != 0x3000000) {
            HW_CPU_PHASE = 1;
            HW_CPU_RESULT = KIRK_INVALID_MODE;
            return;
        }
        PERCONSOLE_KEYSEED = 0;
        aes_set_perconsole_key();
        CUR_ENC_KEY = HW_AES_RESULT;
        kirk3_get_public_key();
    }
    kirk_signed_header_read_body_info();
    if (kirk_check_body_size_nonzero()) {
        // Depending on the signature mode, verify CMAC or ECDSA signature
        kirk_signed_header_read_sign_mode();
        if (HW_DMA_BUF[0] & KIRK_SIGNED_HEADER_SIGN_MODE_ECDSA) {
            ecc_set_priv_curve();
            if (!kirk_ecdsa_check_header()) {
                HW_CPU_PHASE = 1;
                HW_CPU_RESULT = KIRK_HEADER_SIG_INVALID;
                return;
            }
        } else {
            HW_AES_KEY = CUR_ENC_KEY;
            if (!kirk_cmac_check_header()) {
                HW_CPU_PHASE = 1;
                // Note: missing return value?
                return;
            }
        }
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Initialize the ECC module.
 */
void init_ECC(void) // 0x710
{
    __builtin_setmode(HW_ECC_REG, ECC_REG_RESET);
    while (!(HW_ECC_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the SHA1 module.
 */
void init_SHA1(void) // 0x714
{
    __builtin_setmode(HW_SHA1_REG, SHA1_REG_RESET);
    while (!(HW_SHA1_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the AES module.
 */
void init_AES(void) // 0x718
{
    __builtin_setmode(HW_AES_REG, AES_REG_RESET);
    while (!(HW_AES_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the PRNG module.
 */
void init_PRNG(void) // 0x71C
{
    __builtin_setmode(HW_PRNG_REG, PRNG_REG_RESET);
    while (!(HW_PRNG_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the TRNG module.
 */
void init_TRNG(void) // 0x720
{
    __builtin_setmode(HW_TRNG_REG, TRNG_REG_RESET);
    while (!(HW_TRNG_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the MATH module.
 */
void init_MATH(void) // 0x724
{
    __builtin_setmode(HW_MATH_REG, MATH_REG_RESET);
    while (!(HW_MATH_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the DMA module (unused).
 */
void __unused_init_dma(void) // 0x728
{
    __builtin_setmode(HW_DMA_REG, DMA_REG_RESET);
    while (!(HW_DMA_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize the PSP interface.
 */
void init_PSP(void) // 0x72C
{
    __builtin_setmode(HW_CPU_REG, CPU_REG_RESET);
    while (!(HW_CPU_STATUS & 1)) {
        // Wait
    }
}

/*
 * Initialize all Kirk submodules.
 */
void kirk_modules_init(void) // 0x730
{
    init_ECC();
    init_SHA1();
    init_AES();
    init_PRNG();
    init_TRNG();
    init_MATH();
    init_PSP();
}

/*
 * Kirk command 11: compute the SHA1 of an input.
 */
void kirk_cmd11_hash(void) // 0x738
{
    if (!kirk_check_enabled()) {
        return;
    }
    // Read the size of the data
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 4;
    dma_read();
    SIZE = __builtin_byteswap(HW_DMA_BUF[0]);
    if (SIZE == 0) {
        HW_CPU_RESULT = KIRK_DATA_SIZE_ZERO;
    } else {
        // Compute the SHA1
        INPUT = HW_CPU_SRC + 4;
        sha1_do();
        // Write the output
        HW_DMA_BUF._0_16_ = HW_SHA1_RESULT._0_16_;
        HW_DMA_BUF[4] = HW_SHA1_RESULT[4];
        HW_DMA_ADDR = HW_CPU_DST;
        HW_DMA_BUF_SIZE = 0x14;
        dma_write();
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 12: generate a private/public key pair for the "public" curve.
 */
void kirk_cmd12_ecdsa_genkey(void) // 0x755
{
    if (!kirk_check_enabled()) {
        return;
    }

    if (kirk_is_seeded()) {
        // Generate a random private key
        RNG_FLAGS |= RNG_FLAGS_PUB_CURVE;
        math_generate_random();
        RNG_FLAGS &= ~RNG_FLAGS_PUB_CURVE;
        // Write the private key
        HW_DMA_BUF._0_16_ = HW_MATH_RESULT._12_16_;
        HW_DMA_BUF[4] = HW_MATH_RESULT[7];
        HW_DMA_ADDR = HW_CPU_DST;
        HW_DMA_BUF_SIZE = 0x14;
        dma_write();
        // Compute the public key from the private key
        ecc_set_public_curve();
        HW_ECC_R._0_16_ = HW_MATH_RESULT._12_16_;
        HW_ECC_R[4] = HW_MATH_RESULT[7];
        HW_ECC_OP = ECC_OP_MUL;
        ecc_do();
        // Output the public key
        OUTPUT = HW_CPU_DST + 0x14;
        ecc_output_point();
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 13 - multiply an ECDSA point by a scalar
 */
void kirk_cmd13_ecdsa_mul(void) // 0x771
{
    if (!kirk_check_enabled()) {
        return;
    }
    ecc_set_public_curve();
    // Read the scalar
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 0x14;
    dma_read();
    R3._0_16_ = HW_DMA_BUF._0_16_;
    R7 = HW_DMA_BUF[4];
    if (ecc_check_scalar()) {
        // Write scalar to 'r'
        HW_ECC_R._0_16_ = R3._0_16_
        HW_ECC_R[4] = R7;
        // Read x coordinate
        HW_DMA_ADDR = HW_CPU_SRC + 0x14;
        HW_DMA_BUF_SIZE = 0x14;
        INPUT = HW_DMA_ADDR;
        dma_read();
        HW_ECC_Gx._0_16_ = HW_DMA_BUF._0_16_;
        HW_ECC_Gx[4] = HW_DMA_BUF[4];
        // Read y coordinate
        HW_DMA_ADDR = INPUT + 0x14;
        INPUT = HW_DMA_ADDR;
        dma_read();
        HW_ECC_Gy._0_16_ = HW_DMA_BUF._0_16_;
        HW_ECC_Gy[4] = HW_DMA_BUF[4];
        // Compute the multiplication
        HW_ECC_OP = ECC_OP_MUL;
        ecc_do();
        // Output the result
        OUTPUT = HW_CPU_DST;
        ecc_output_point();
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    } else {
        HW_CPU_RESULT = KIRK_ECDSA_DATA_INVALID;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 14 - generate an ECDSA private key.
 */
void kirk_cmd14_gen_privkey(void) // 0x79A
{
    if (!kirk_check_enabled()) {
        return;
    }

    if (kirk_is_seeded()) {
        // Generate the private key
        RNG_FLAGS |= RNG_FLAGS_PUB_CURVE;
        math_generate_random();
        RNG_FLAGS &= ~RNG_FLAGS_PUB_CURVE;
        // Output the private key
        HW_DMA_BUF._0_16_ = (undefined  [16])HW_MATH_RESULT._12_16_;
        HW_DMA_BUF[4] = HW_MATH_RESULT[7];
        HW_DMA_ADDR = HW_CPU_DST;
        HW_DMA_BUF_SIZE = 0x14;
        dma_write();
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 16 - generate a signature from an encrypted private key.
 */
void kirk_cmd16_siggen(void) // 0x7AC
{
    if (!kirk_check_enabled()) {
        return;
    }

    if (kirk_is_seeded()) {
        // Decrypt the private key given in input using per-console key with seed 3
        PERCONSOLE_KEYSEED = 3;
        aes_set_perconsole_key();
        INPUT = HW_CPU_SRC;
        HW_DMA_BUF_SIZE = 0x10;
        aes_set_null_iv();
        HW_AES_CMD = AES_CMD_MULTIPLE_BLOCKS | AES_CMD_FIRST_BLOCK | AES_CMD_DECRYPT;
        aes_setkey();
        aes_dma_copy_and_do();
        HW_AES_CMD &= 0xfffffffd;
        R3._0_16_ = HW_AES_RESULT;
        INPUT += 0x10;
        aes_dma_copy_and_do();
        R7 = HW_AES_RESULT[0];
        // Check if the scalar is valid
        if (ecc_check_scalar()) {
            ecc_set_public_curve();
            HW_ECC_S._0_16_ = R3._0_16_;
            HW_ECC_S[4] = R7;
            // Read the input data to sign
            HW_DMA_ADDR = HW_CPU_SRC + 0x20;
            HW_DMA_BUF_SIZE = 0x14;
            INPUT = HW_DMA_ADDR;
            dma_read();
            // Sign the message
            HW_ECC_M._0_16_ = HW_DMA_BUF._0_16_;
            HW_ECC_M[4] = HW_DMA_BUF[4];
            RNG_FLAGS |= RNG_FLAGS_PUB_CURVE;
            ecc_sign_gen_r();
            RNG_FLAGS &= ~RNG_FLAGS_PUB_CURVE;
            // Output the signature
            OUTPUT = HW_CPU_DST;
            ecc_output_point();
            HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
        }
        else {
            HW_CPU_RESULT = KIRK_ECDSA_DATA_INVALID;
        }
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 17 - verify the signature of a hash.
 */
void kirk_cmd17_sigvry(void) // 0x7DC
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Use the "public" curve
    ecc_set_public_curve();
    // Read coordinate x of the public key
    HW_DMA_BUF_SIZE = 0x14;
    INPUT = HW_CPU_SRC;
    dma_read_input();
    HW_ECC_Px._0_16_ = HW_DMA_BUF._0_16_;
    HW_ECC_Px[4] = HW_DMA_BUF[4];
    // Read the coordinate y of the public key
    INPUT += 0x14;
    dma_read_input();
    HW_ECC_Py._0_16_ = (undefined  [16])HW_DMA_BUF._0_16_;
    HW_ECC_Py[4] = HW_DMA_BUF[4];
    // Read the message (hash) M
    INPUT += 0x14;
    dma_read_input();
    HW_ECC_M._0_16_ = (undefined  [16])HW_DMA_BUF._0_16_;
    HW_ECC_M[4] = HW_DMA_BUF[4];
    // Read the signature (r, s)
    INPUT = INPUT + 0x14;
    ecc_read_signature();
    // Verify the signature
    HW_ECC_OP = ECC_OP_CHECK_SIGN;
    ecc_do();
    if (HW_ECC_RESULT & 1) {
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    } else {
        HW_CPU_RESULT = KIRK_ECDSA_DATA_INVALID;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Kirk command 18: verify the IDStorage CMAC using a per-console key.
 */
void kirk_cmd18_certvry(void) // 0x7FF
{
    if (!kirk_check_enabled()) {
        return;
    }

    // Compute per-console AES key with seed 4
    PERCONSOLE_KEYSEED = 4;
    aes_set_perconsole_key();
    // Compute the CMAC of 0xa8 bytes of data
    INPUT = HW_CPU_SRC;
    SIZE = 0xa8;
    aes_cmac();
    // Compare the CMAC to the value stored at 0xa8
    HW_DMA_ADDR = HW_CPU_SRC + 0xa8;
    HW_DMA_BUF_SIZE = 0x10;
    HW_DMA_BUF[0] = HW_DMA_ADDR; // TODO: useless?
    dma_read();
    if (memcmp_cmac()) {
        HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    } else {
        HW_CPU_RESULT = KIRK_ECDSA_DATA_INVALID;
    }
    HW_CPU_PHASE = 1;
}

/*
 * Check if an ECDSA scalar (private key) is valid
 */
bool ecc_check_scalar(void) // 0x819
{
    R11 = &R3;
    R13 = 5;
    // Check if the scalar is non-zero and lesser than the order of the curve
    if (buffer_is_nonzero() &&
        (R3 < 0xffffffff
      || R4 < 0xfffffffe
      || R5 < 0xffffb5ae
      || R6 < 0x3c523e63
      || R7 < 0x944f2127)) {
        return true;
    }
    return false;
}

/*
 * Kirk command 15: seed the RNG buffer
 */
void kirk_cmd15_init(void) // 0x833
{
    if (!kirk_check_enabled()) {
        return;
    }

    RNG_FLAGS |= RNG_FLAGS_SEEDED;

    // Read 28 bytes of data from the input
    HW_DMA_ADDR = HW_CPU_SRC;
    HW_DMA_BUF_SIZE = 0x1c;
    dma_read();

    // 64-bit addition, HW_DMA_BUF[0] is the MSB, HW_DMA_BUF[1] is the LSB
    HW_DMA_BUF._0_8_ = HW_DMA_BUF._0_8_ + 1;
    // Seed the RNG buffer with the counter
    RNG_BUFFER[0] = 0;
    RNG_BUFFER[1] = 0;
    RNG_BUFFER[2] = HW_DMA_BUF[0];
    // Use the rest of the input as a seed for the PRNG
    HW_PRNG_SEED[0] = HW_DMA_BUF[2];
    HW_PRNG_SEED._4_16_ = HW_DMA_BUF._12_16_;
    // Get data from the TRNG
    HW_TRNG_OUT_SIZE = 0x100;
    __builtin_setmode(HW_TRNG_REG, TRNG_REG_DO);
    __builtin_intr(INTR_TRNG);
    // Hash the data coming from the TRNG
    HW_SHA1_IN_SIZE = 0x20;
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[0];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[1];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[2];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[3];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[4];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[5];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[6];
    HW_SHA1_IN_DATA = HW_TRNG_OUTPUT[7];
    __builtin_intr(INTR_SHA1);
    // XOR the PRNG seed with the previous hash
    HW_PRNG_SEED[0] ^= HW_SHA1_RESULT[0];
    HW_PRNG_SEED[1] ^= HW_SHA1_RESULT[1];
    HW_PRNG_SEED[2] ^= HW_SHA1_RESULT[2];
    HW_PRNG_SEED[3] ^= HW_SHA1_RESULT[3];
    HW_PRNG_SEED[4] ^= HW_SHA1_RESULT[4];
    // Reseed the RNG buffer
    HW_PRNG_MODE = 3;
    RNG_BUFFER[3] = HW_DMA_BUF[1];
    kirk_reseed();
    // Write the resulting data as the output
    HW_DMA_BUF[2] = HW_PRNG_SEED[0];
    HW_DMA_BUF._12_16_ = HW_PRNG_SEED._4_16_;
    HW_DMA_ADDR = HW_CPU_DST;
    dma_write();
    HW_CPU_RESULT = KIRK_OPERATION_SUCCESS;
    HW_CPU_PHASE = 1;
}

/*
 * Reseed the PRNG in mode 2 (ie everything except for the initial reseeding).
 */
void kirk_reseed_prng_2(void) // 0x866
{
    HW_PRNG_MODE = 2;
    kirk_reseed();
}

/*
 * Reseed the PRNG.
 */
void kirk_reseed(void) // 0x868
{
    // Encrypt the RNG buffer using per-console key 6
    PERCONSOLE_KEYSEED = 6;
    aes_set_perconsole_key();
    aes_setkey();
    HW_AES_SRC_BUF = RNG_BUFFER;
    aes_do();
    RNG_BUFFER = HW_AES_RESULT;
    // Reseed the PRNG with the RNG buffer
    HW_PRNG_SEED[5] = 0;
    HW_PRNG_SEED._24_16_ = RNG_BUFFER;
    prng_do();
}

/*
 * Set HW_AES_KEY to the per-console key with seed 0.
 */
void aes_set_perconsole_key_0(void) // 0x874
{
    PERCONSOLE_KEYSEED = 0;
    aes_set_perconsole_key();
}

/*
 * Set HW_AES_KEY to the per-console key with given seed.
 */
void aes_set_perconsole_key(void) // 0x876
{
    // Set the initial key depending on the seed LSB
    HW_AES_CMD = AES_CMD_ENCRYPT;
    HW_AES_KEY = HW_KEY_MESH_2;
    aes_setkey();
    if (PERCONSOLE_KEYSEED & 1) {
        HW_AES_SRC_BUF = HW_KEY_MESH_1;
    } else {
        HW_AES_SRC_BUF = HW_KEY_MESH_0;
    }
    // Encrypt the result several times depending on the seed
    PERCONSOLE_KEYSEED = (PERCONSOLE_KEYSEED >> 1) + 1;
    do {
        aes_do();
        HW_AES_SRC_BUF = HW_AES_RESULT;
        PERCONSOLE_KEYSEED = PERCONSOLE_KEYSEED - 1;
    } while (PERCONSOLE_KEYSEED != 0);

    // Put the result in HW_AES_KEY
    HW_AES_KEY = HW_AES_RESULT;
}

/*
 * Compute random data with the PRNG.
 */
void prng_do(void) // 0x887
{
    __builtin_setmode(HW_PRNG_REG, PRNG_REG_DO);
    __builtin_intr(INTR_PRNG);
}

/*
 * Run Kirk self-tests.
 */
void kirk_self_test(void) // 0x88A
{
    // Initialize all modules
    kirk_modules_init();
    // Test all modules
    if (test_ECC()
        && test_SHA1()
        && test_AES()
        && test_PRNG()
        && test_TRNG()
        && test_MATH()) {
        HW_TEST_RESULT = 3;
    }
    else {
        HW_TEST_RESULT = 2;
    }
    // Initialize communication with the CPU
    HW_CPU_PHASE = 0;
    __builtin_setmode(HW_CPU_REG, CPU_REG_DO);
    __builtin_intr(INTR_CPU);

    // Encrypt two-block input in AES ECB with the given key
    HW_AES_CMD = AES_CMD_ENCRYPT;
    HW_AES_KEY[0] = 0x2b7e1516;
    HW_AES_KEY[1] = 0x28aed2a6;
    HW_AES_KEY[2] = 0xabf71588;
    HW_AES_KEY[3] = 0x09cf4f3c;
    aes_setkey();
    HW_DMA_BUF_SIZE = 0x20;
    HW_DMA_ADDR = HW_CPU_SRC;
    dma_read();
    HW_AES_SRC_BUF = HW_DMA_BUF._0_16_;
    aes_do();
    HW_DMA_BUF._0_16_ = HW_AES_RESULT;
    HW_AES_SRC_BUF = HW_DMA_BUF._16_16_;
    aes_do();
    HW_DMA_BUF._16_16_ = HW_AES_RESULT;
    // Write the result to the output
    HW_DMA_ADDR = HW_CPU_DST;
    dma_write();
    // Update CPU communication
    HW_CPU_PHASE = 1;
    __builtin_setmode(HW_CPU_REG, CPU_REG_DO);
    __builtin_intr(INTR_CPU);
}

/*
 * Test ECC scalar multiplication:
 *
 * >>> psp_curve_test = {
 *     'name': 'psp_kirk_test',
 *     'type': 'weierstrass',
 *     'size': 160,
 *     'order':     0xe5d8c140e15a3be238d81bd77229b1b8008a143d,
 *     'field':     0xacb0f43a68487d86975b98aae0a0f6a581da1e8d,
 *     'generator': (0xa60cd01542a205545b6696f3421e00cde8bbc06c,
 * 0x76bd9970281e307cd30ea2eba0f88400334bcba8),
 *     'a': 0xacb0f43a68487d86975b98aae0a0f6a581da1e8a,
 *     'b': 0x4fb97afa73a001a1a6386705de41e517543933a7,
 *     'cofactor': 1
 * }
 * >>> crvkirk = ecpy.curves.WeierstrassCurve(psp_curve_test)
 * >>> print(crvkirk.mul_point(crvkirk.generator, 3))
 * (0x6df3fa63c3deede13d46d1faf6cbc2b64319589a , 0x78ef881d7450e85022c8e6f07d9076e92f4e082e)
 */
bool test_ECC(void) // 0x8BD
{
    HW_ECC_P[0] = 0xacb0f43a;
    HW_ECC_P[1] = 0x68487d86;
    HW_ECC_P[2] = 0x975b98aa;
    HW_ECC_P[3] = 0xe0a0f6a5;
    HW_ECC_P[4] = 0x81da1e8d;
    HW_ECC_A[0] = 0xacb0f43a;
    HW_ECC_A[1] = 0x68487d86;
    HW_ECC_A[2] = 0x975b98aa;
    HW_ECC_A[3] = 0xe0a0f6a5;
    HW_ECC_A[4] = 0x81da1e8a;
    HW_ECC_B[0] = 0x4fb97afa;
    HW_ECC_B[1] = 0x73a001a1;
    HW_ECC_B[2] = 0xa6386705;
    HW_ECC_B[3] = 0xde41e517;
    HW_ECC_B[4] = 0x543933a7;
    HW_ECC_N[0] = 0;
    HW_ECC_N[1] = 0;
    HW_ECC_N[2] = 0;
    HW_ECC_N[3] = 0;
    HW_ECC_N[4] = 3;
    // TODO: unknown parameters
    HW_ECC_UNK1[0] = 0x44e8319d;
    HW_ECC_UNK1[1] = 0x2ea03d11;
    HW_ECC_UNK1[2] = 0xdc6e8341;
    HW_ECC_UNK1[3] = 0xfffe8429;
    HW_ECC_UNK1[4] = 0x94ffc5a9;
    HW_ECC_UNK2[0] = 0xa9c56aab;
    HW_ECC_UNK2[1] = 0x686453b1;
    HW_ECC_UNK2[2] = 0xf34f7d6f;
    HW_ECC_UNK2[3] = 0x61954b7d;
    HW_ECC_UNK2[4] = 0x0834bf46;
    HW_ECC_Gx[0] = 0xa60cd015;
    HW_ECC_Gx[1] = 0x42a20554;
    HW_ECC_Gx[2] = 0x5b6696f3;
    HW_ECC_Gx[3] = 0x421e00cd;
    HW_ECC_Gx[4] = 0xe8bbc06c;
    HW_ECC_Gy[0] = 0x76bd9970;
    HW_ECC_Gy[1] = 0x281e307c;
    HW_ECC_Gy[2] = 0xd30ea2eb;
    HW_ECC_Gy[3] = 0xa0f88400;
    HW_ECC_Gy[4] = 0x334bcba8;
    HW_ECC_R[0] = 0;
    HW_ECC_R[1] = 0;
    HW_ECC_R[2] = 0;
    HW_ECC_R[3] = 0;
    HW_ECC_R[4] = 3;
    HW_ECC_OP = ECC_OP_MUL;
    ecc_do();
    if (HW_ECC_RESULT_X[0] == 0x6df3fa63
     && HW_ECC_RESULT_X[1] == 0xc3deede1
     && HW_ECC_RESULT_X[2] == 0x3d46d1fa
     && HW_ECC_RESULT_X[3] == 0xf6cbc2b6
     && HW_ECC_RESULT_X[4] == 0x4319589a
     && HW_ECC_RESULT_Y[0] == 0x78ef881d
     && HW_ECC_RESULT_Y[1] == 0x7450e850
     && HW_ECC_RESULT_Y[2] == 0x22c8e6f0
     && HW_ECC_RESULT_Y[3] == 0x7d9076e9
     && HW_ECC_RESULT_Y[4] == 0x2f4e082e) {
        return 1;
    }
    return 0;
}

/*
 * Test SHA1: compute the SHA1 of "abc"
 */
bool test_SHA1(void) // 0x93C
{
    HW_SHA1_IN_SIZE = 3;
    HW_SHA1_IN_DATA = 0x61626300; // "abc"
    __builtin_intr(INTR_SHA1);
    if (HW_SHA1_RESULT[0] == 0xa9993e36
     && HW_SHA1_RESULT[1] == 0x4706816a
     && HW_SHA1_RESULT[2] == 0xba3e2571
     && HW_SHA1_RESULT[3] == 0x7850c26c
     && HW_SHA1_RESULT[4] == 0x9cd0d89d) {
        return 1;
    }
    return 0;
}

/*
 * Test AES: encrypt some data:
 * >>> cipher = AES.new(bytes.fromhex('2b7e151628aed2a6abf7158809cf4f3c'), AES.MODE_ECB)
 * >>> cipher.encrypt(bytes.fromhex('3243f6a8885a308d313198a2e0370734')).hex()
 * '3925841d02dc09fbdc118597196a0b32'
 */
bool test_AES(void) // 0x954
{
    HW_AES_CMD = AES_CMD_ENCRYPT;
    HW_AES_KEY[0] = 0x2b7e1516;
    HW_AES_KEY[1] = 0x28aed2a6;
    HW_AES_KEY[2] = 0xabf71588;
    HW_AES_KEY[3] = 0x09cf4f3c;
    aes_setkey();
    HW_AES_SRC_BUF[0] = 0x3243f6a8;
    HW_AES_SRC_BUF[1] = 0x885a308d;
    HW_AES_SRC_BUF[2] = 0x313198a2;
    HW_AES_SRC_BUF[3] = 0xe0370734;
    aes_do();
    if (HW_AES_RESULT[0] == 0x3925841d
     && HW_AES_RESULT[1] == 0x02dc09fb
     && HW_AES_RESULT[2] == 0xdc118597
     && HW_AES_RESULT[3] == 0x196a0b32) {
        return true;
    }
    return false;
}

/*
 * Test the PRNG.
 */
bool test_PRNG(void) // 0x978
{
    HW_PRNG_MODE = 3;
    HW_PRNG_SEED[0] = 0xf067cf18;
    HW_PRNG_SEED[1] = 0x1f101685;
    HW_PRNG_SEED[2] = 0x11d8483c;
    HW_PRNG_SEED[3] = 0xf3e5538f;
    HW_PRNG_SEED[4] = 0x415117cb;
    HW_PRNG_SEED[5] = 0x12153524;
    HW_PRNG_SEED[6] = 0xc0895e81;
    HW_PRNG_SEED[7] = 0x8484d609;
    HW_PRNG_SEED[8] = 0xb1f05663;
    HW_PRNG_SEED[9] = 0x06b97b0d;
    prng_do();
    if (HW_PRNG_RESULT[0] == 0x1df50b98
     && HW_PRNG_RESULT[1] == 0x7d6c2369
     && HW_PRNG_RESULT[2] == 0x1794af3b
     && HW_PRNG_RESULT[3] == 0xab98a128
     && HW_PRNG_RESULT[4] == 0x4cd53d2d
     && HW_PRNG_SEED[0] == 0x0e5cdab0
     && HW_PRNG_SEED[1] == 0x9c7c39ee
     && HW_PRNG_SEED[2] == 0x296cf778
     && HW_PRNG_SEED[3] == 0x9f7df4b7
     && HW_PRNG_SEED[4] == 0x8e2654f9) {
        return 1;
    }
    return 0;
}

/*
 * Test the TRNG module.
 */
void test_TRNG(void) // 0x9B1
{
    HW_TRNG_OUT_SIZE = 0x100;
    __builtin_setmode(HW_TRNG_REG, TRNG_REG_DO);
    __builtin_intr(INTR_TRNG);
    SIZE = HW_TRNG_OUTPUT._0_16_; // arbitrary register
    R4 = HW_TRNG_OUTPUT._16_16_;  // arbitrary register
}

/*
 * Test the math (modulus) module.
 */
bool test_MATH(void) // 0x9B8
{
    // 0x2c6c3246fcedb0546f969b8619d03bf8e16b88ca7b31acf2c800afa8c1945fa12719c7f1c7c99bb7
    // % 0xe5d8c140e15a3be238d81bd77229b1b8008a143d
    // == 0x5ba7b73f60c6fa9487a618586350a9025c1713ae
    HW_MATH_IN[0] = 0;
    HW_MATH_IN[1] = 0;
    HW_MATH_IN[2] = 0;
    HW_MATH_IN[3] = 0;
    HW_MATH_IN[4] = 0;
    HW_MATH_IN[5] = 0;
    HW_MATH_IN[6] = 0x2c6c3246;
    HW_MATH_IN[7] = 0xfcedb054;
    HW_MATH_IN[8] = 0x6f969b86;
    HW_MATH_IN[9] = 0x19d03bf8;
    HW_MATH_IN[10] = 0xe16b88ca;
    HW_MATH_IN[11] = 0x7b31acf2;
    HW_MATH_IN[12] = 0xc800afa8;
    HW_MATH_IN[13] = 0xc1945fa1;
    HW_MATH_IN[14] = 0x2719c7f1;
    HW_MATH_IN[15] = 0xc7c99bb7;
    HW_MATH_MODULUS[0] = 0;
    HW_MATH_MODULUS[1] = 0;
    HW_MATH_MODULUS[2] = 0;
    HW_MATH_MODULUS[3] = 0xe5d8c140;
    HW_MATH_MODULUS[4] = 0xe15a3be2;
    HW_MATH_MODULUS[5] = 0x38d81bd7;
    HW_MATH_MODULUS[6] = 0x7229b1b8;
    HW_MATH_MODULUS[7] = 0x008a143d;
    __builtin_setmode(HW_MATH_REG, MATH_REG_DO);
    __builtin_intr(INTR_MATH);
    if (HW_MATH_RESULT[0] == 0
     && HW_MATH_RESULT[1] == 0
     && HW_MATH_RESULT[2] == 0
     && HW_MATH_RESULT[3] == 0x5ba7b73f
     && HW_MATH_RESULT[4] == 0x60c6fa94
     && HW_MATH_RESULT[5] == 0x87a61858
     && HW_MATH_RESULT[6] == 0x6350a902
     && HW_MATH_RESULT[7] == 0x5c1713ae) {
        // 0xfea54c798eac6e6a78f14231687faea7865fac876e8a7f6a76e5afa9c87fe87a56b9a87e18756ffe % 3 == 2
        HW_MATH_IN[0] = 0;
        HW_MATH_IN[1] = 0;
        HW_MATH_IN[2] = 0;
        HW_MATH_IN[3] = 0;
        HW_MATH_IN[4] = 0;
        HW_MATH_IN[5] = 0;
        HW_MATH_IN[6] = 0xfea54c79;
        HW_MATH_IN[7] = 0x8eac6e6a;
        HW_MATH_IN[8] = 0x78f14231;
        HW_MATH_IN[9] = 0x687faea7;
        HW_MATH_IN[10] = 0x865fac87;
        HW_MATH_IN[11] = 0x6e8a7f6a;
        HW_MATH_IN[12] = 0x76e5afa9;
        HW_MATH_IN[13] = 0xc87fe87a;
        HW_MATH_IN[14] = 0x56b9a87e;
        HW_MATH_IN[15] = 0x18756ffe;
        HW_MATH_MODULUS[0] = 0;
        HW_MATH_MODULUS[1] = 0;
        HW_MATH_MODULUS[2] = 0;
        HW_MATH_MODULUS[3] = 0;
        HW_MATH_MODULUS[4] = 0;
        HW_MATH_MODULUS[5] = 0;
        HW_MATH_MODULUS[6] = 0;
        HW_MATH_MODULUS[7] = 3;
        __builtin_setmode(HW_MATH_REG, MATH_REG_DO);
        __builtin_intr(INTR_MATH);
        if (HW_MATH_RESULT[0] == 0
         && HW_MATH_RESULT[1] == 0
         && HW_MATH_RESULT[2] == 0
         && HW_MATH_RESULT[3] == 0
         && HW_MATH_RESULT[4] == 0
         && HW_MATH_RESULT[5] == 0
         && HW_MATH_RESULT[6] == 0
         && HW_MATH_RESULT[7] == 2) {
            return 1;
        }
    }
    return 0;
}

