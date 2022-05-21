#include <common_imp.h>

#include "memlmd.h"
#include "threadman_kernel.h"

#include <string.h>

#define KIRK_NOT_INITIALIZED 0xC

// initialized memory 6C10-8D2C
u8 g_7000[40] = {0x77, 0x3F, 0x4B, 0xE1, 0x4C, 0x0A, 0xB4, 0x52, 0x67, 0x2B, 0x67, 0x56, 0x82, 0x4C, 0xCF, 0x42, 0xAA, 0x37, 0xFF, 0xC0, 0x89, 0x41, 0xE5, 0x63, 0x5E, 0x84, 0xE9, 0xFB, 0x53, 0xDA, 0x94, 0x9E, 0x9B, 0xB7, 0xC2, 0xA4, 0x22, 0x9F, 0xDF, 0x1F};
u8 g_7028[40] = {0x25, 0xDC, 0xFD, 0xE2, 0x12, 0x79, 0x89, 0x54, 0x79, 0x37, 0x13, 0x24, 0xEC, 0x25, 0x08, 0x81, 0x57, 0xAA, 0xF1, 0xD0, 0xA4, 0x64, 0x8C, 0x15, 0x42, 0x25, 0xF6, 0x90, 0x3F, 0x44, 0xE3, 0x6A, 0xE6, 0x64, 0x12, 0xFC, 0x80, 0x68, 0xBD, 0xC1};
u8 g_7050[40] = {0xE3, 0x5E, 0x4E, 0x7E, 0x2F, 0xA3, 0x20, 0x96, 0x75, 0x43, 0x94, 0xA9, 0x92, 0x01, 0x83, 0xA7, 0x85, 0xBD, 0xF6, 0x19, 0x1F, 0x44, 0x8F, 0x95, 0xE0, 0x43, 0x35, 0xA3, 0xF5, 0xE5, 0x05, 0x65, 0x5E, 0xD7, 0x59, 0x3F, 0xC6, 0xDB, 0xAF, 0x39};
// 
u8 g_82A8[4] = { 0x00, 0x00, 0x00, 0x06 };
u8 g_82AC[144] = {
  0x84, 0x15, 0x12, 0x8c, 0xa8, 0x83, 0xd7, 0x80, 0xef, 0x1e, 0x88, 0xdb, 0xbc, 0x61, 0x96, 0x23,
  0x2b, 0xf3, 0x88, 0xfc, 0xe5, 0x3f, 0xb5, 0xde, 0x98, 0x5a, 0xa0, 0x6b, 0xde, 0x0a, 0x55, 0xdb,
  0xf2, 0xf8, 0x44, 0x36, 0xeb, 0xd1, 0x94, 0x55, 0x4a, 0x39, 0x3e, 0x93, 0x7c, 0x3d, 0xe3, 0x02,
  0x12, 0x88, 0xe7, 0xf5, 0xf8, 0xf0, 0xc1, 0xeb, 0x25, 0x1b, 0x8d, 0xc6, 0xb8, 0x1e, 0x2b, 0x44,
  0xa5, 0xb7, 0x6a, 0x7e, 0xd0, 0x39, 0x46, 0x92, 0x6d, 0x71, 0xde, 0x07, 0x97, 0xb8, 0x2f, 0x10,
  0x18, 0xba, 0xdd, 0x53, 0xc6, 0x07, 0x2b, 0x98, 0x24, 0x8a, 0x74, 0x0d, 0x5c, 0x64, 0x5d, 0xfe,
  0x8e, 0xe7, 0x67, 0x43, 0x93, 0x96, 0xb3, 0xa1, 0xa1, 0xa8, 0xec, 0x12, 0xc4, 0xfb, 0x58, 0x44,
  0xfc, 0x55, 0x0a, 0x9c, 0x1e, 0x30, 0x37, 0xfa, 0x54, 0x24, 0xd3, 0x03, 0xe2, 0x92, 0xbd, 0x31,
  0x23, 0x03, 0xea, 0xf7, 0xe7, 0x73, 0xde, 0x09, 0x3b, 0xb3, 0x83, 0x79, 0xda, 0x17, 0x85, 0x0e
};
u8 g_8340[4] = { 0x03, 0x23, 0x2c, 0xe4 };
u8 g_8344[144] = {
  0x6d, 0x79, 0xf2, 0xf6, 0x37, 0x3d, 0xb7, 0xbe, 0xa2, 0x73, 0xa1, 0xae, 0x88, 0x70, 0xc9, 0xa3,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x90, 0x2b, 0x0b, 0x39, 0xf7, 0xdf, 0x19, 0xd7, 0x10, 0xea, 0x9f,
  0x02, 0xdb, 0x3f, 0xb1, 0x10, 0x9f, 0x26, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
  0x88, 0xd7, 0x76, 0xf5, 0xb7, 0x6d, 0xec, 0x0e, 0x00, 0x0b, 0x85, 0xe6, 0x4a, 0x37, 0x9b, 0xc9,
  0x00, 0x95, 0xea, 0x7a, 0xad, 0x66, 0x67, 0x09, 0x05, 0xf2, 0x09, 0x52, 0x47, 0x27, 0x8e, 0x4e,
  0x57, 0x83, 0xf7, 0x5d, 0x1f, 0x90, 0x11, 0x0e, 0xef, 0xf6, 0x97, 0x39, 0x83, 0xa1, 0x65, 0xd0,
  0x11, 0xca, 0x64, 0x96, 0x7a, 0xef, 0xc9, 0x18, 0x6b, 0x7d, 0x97, 0x7a, 0xf0, 0x32, 0x28, 0xa6,
  0xe2, 0xe3, 0x86, 0x64, 0xdf, 0xae, 0x03, 0xd2, 0xad, 0x7e, 0x7f, 0x46, 0x2e, 0x5f, 0x02, 0x95,
  0xe9, 0x35, 0x5c, 0x17, 0xad, 0x42, 0x54, 0x62, 0x84, 0x66, 0xc9, 0x35, 0x9c, 0x7b, 0x2a, 0xfa
};
u8 g_89E0[4] = {0x00, 0x00, 0x00, 0x0E};
u8 g_89E4[144] = {
  0xde, 0x57, 0xb7, 0x77, 0x17, 0xdd, 0x62, 0xee, 0x7b, 0x78, 0x03, 0x5d, 0x44, 0x86, 0xca, 0x59,
  0x20, 0x8d, 0xf6, 0x93, 0x28, 0x93, 0x81, 0x21, 0x71, 0x4e, 0xa7, 0x86, 0xca, 0x82, 0x24, 0x1b,
  0x58, 0xae, 0x74, 0x5f, 0x6c, 0x01, 0x8d, 0x56, 0x32, 0x88, 0x4d, 0x9a, 0x72, 0x43, 0xa2, 0x2e,
  0x84, 0xf4, 0x0c, 0x82, 0xb9, 0x06, 0xfc, 0xfc, 0x6a, 0xfb, 0x5b, 0x8a, 0xd7, 0x9c, 0x9f, 0xbf,
  0x01, 0x0d, 0x85, 0x15, 0xba, 0x5f, 0xed, 0x39, 0x93, 0x83, 0xc3, 0x4c, 0xaf, 0xde, 0x3a, 0xed,
  0xbf, 0x68, 0xa7, 0x1a, 0x77, 0x8a, 0xbd, 0x89, 0x65, 0x41, 0x56, 0x46, 0xd9, 0xdb, 0x33, 0x73,
  0x81, 0x6c, 0xe8, 0x62, 0x96, 0x9b, 0x29, 0x03, 0x5a, 0xae, 0xaf, 0x73, 0x20, 0x53, 0xa0, 0x40,
  0xe8, 0x4b, 0x66, 0x10, 0x99, 0x6a, 0xb7, 0xe5, 0x70, 0xdd, 0xe0, 0x29, 0x28, 0x24, 0x60, 0xea,
  0x30, 0xae, 0x42, 0x20, 0x32, 0x8d, 0x6f, 0x94, 0x71, 0x5f, 0x9e, 0xa2, 0xd5, 0x7f, 0x0c, 0x7c,
};
u8 g_8A78[4] = {0x03, 0xb4, 0xba, 0x63};
u8 g_8A7C[144] = {
  0x02, 0x2b, 0x67, 0x21, 0xe7, 0x86, 0xad, 0x91, 0x73, 0xbc, 0xc9, 0xde, 0xc5, 0x7a, 0x13, 0xa4,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x60, 0x73, 0xfd, 0xa2, 0x2d, 0xce, 0xf1, 0x11,
  0xe3, 0x82, 0x78, 0xf5, 0x34, 0xaa, 0x9d, 0xe6, 0xd2, 0x2d, 0x34, 0xbe, 0x55, 0xbb, 0x57, 0x7f,
  0x9b, 0x63, 0x70, 0x21, 0x94, 0x31, 0xeb, 0x4f, 0xdb, 0x97, 0xb7, 0xa3, 0x1d, 0x64, 0xe4, 0x19,
  0xf7, 0x13, 0x16, 0x5e, 0xb3, 0xc9, 0x0f, 0x3e, 0xbe, 0xc6, 0x41, 0xb5, 0x13, 0xd0, 0x7f, 0xb4,
  0x55, 0x16, 0x46, 0x95, 0x22, 0x9a, 0xe6, 0xf7, 0xaa, 0x67, 0xcb, 0xc4, 0xdd, 0xb7, 0x70, 0x67,
  0x52, 0x48, 0xb3, 0x26, 0x5e, 0xa9, 0x38, 0xff, 0x5f, 0x62, 0xea, 0xd4, 0x47, 0xac, 0xd0, 0x49,
  0xab, 0xc7, 0x7c, 0x48, 0x1b, 0x83, 0x02, 0x83, 0x30, 0x1b, 0x33, 0xc1, 0x7d, 0x0b, 0x52, 0xd4,
  0xbb, 0xea, 0x10, 0x39, 0x59, 0x3d, 0xf6, 0x96, 0x2f, 0xf0, 0x50, 0x42, 0xf4, 0x87, 0xc4, 0xee
};
u8 g_8A90[4] = { 0x00, 0x00, 0x00, 0x0f };
u8 g_8A94[144] = {
  0x60, 0x73, 0xfd, 0xa2, 0x2d, 0xce, 0xf1, 0x11, 0xe3, 0x82, 0x78, 0xf5, 0x34, 0xaa, 0x9d, 0xe6,
  0xd2, 0x2d, 0x34, 0xbe, 0x55, 0xbb, 0x57, 0x7f, 0x9b, 0x63, 0x70, 0x21, 0x94, 0x31, 0xeb, 0x4f,
  0xdb, 0x97, 0xb7, 0xa3, 0x1d, 0x64, 0xe4, 0x19, 0xf7, 0x13, 0x16, 0x5e, 0xb3, 0xc9, 0x0f, 0x3e,
  0xbe, 0xc6, 0x41, 0xb5, 0x13, 0xd0, 0x7f, 0xb4, 0x55, 0x16, 0x46, 0x95, 0x22, 0x9a, 0xe6, 0xf7,
  0xaa, 0x67, 0xcb, 0xc4, 0xdd, 0xb7, 0x70, 0x67, 0x52, 0x48, 0xb3, 0x26, 0x5e, 0xa9, 0x38, 0xff,
  0x5f, 0x62, 0xea, 0xd4, 0x47, 0xac, 0xd0, 0x49, 0xab, 0xc7, 0x7c, 0x48, 0x1b, 0x83, 0x02, 0x83,
  0x30, 0x1b, 0x33, 0xc1, 0x7d, 0x0b, 0x52, 0xd4, 0xbb, 0xea, 0x10, 0x39, 0x59, 0x3d, 0xf6, 0x96,
  0x2f, 0xf0, 0x50, 0x42, 0xf4, 0x87, 0xc4, 0xee, 0x29, 0x98, 0x4a, 0xa7, 0x77, 0x36, 0x11, 0xaf,
  0xe7, 0xf9, 0x9c, 0x42, 0xb6, 0x3a, 0x2a, 0x78, 0x0c, 0xfe, 0x8e, 0x55, 0x82, 0x66, 0x11, 0x4a
};
u8 g_8B28[4] = { 0xd1, 0x48, 0x26, 0x86 };
u8 g_8B2C[144] = {
  0x35, 0x2e, 0x7e, 0x08, 0xd6, 0x0a, 0xa8, 0xd0, 0xc2, 0xcf, 0xab, 0x01, 0x46, 0x6c, 0x5c, 0x81,
  0xf9, 0xe5, 0xa0, 0xad, 0x38, 0x06, 0x49, 0x19, 0x10, 0xae, 0x2f, 0xdb, 0xe2, 0x3b, 0xab, 0x97,
  0xd1, 0x56, 0x70, 0x9f, 0x54, 0x22, 0x07, 0x40, 0xaa, 0x37, 0x11, 0x1f, 0x78, 0x39, 0x42, 0xf2,
  0x1c, 0x44, 0xe5, 0x12, 0xa3, 0x31, 0xe9, 0xeb, 0xfc, 0xe8, 0x5e, 0x8a, 0xe7, 0xc3, 0x18, 0x7b,
  0xec, 0xa0, 0xbb, 0xd8, 0x11, 0xfb, 0x9c, 0xfd, 0xb8, 0x17, 0xc0, 0xb6, 0x3b, 0x54, 0xad, 0x2e,
  0xec, 0xfa, 0x13, 0xf2, 0xfc, 0x8b, 0x91, 0x27, 0xb1, 0x43, 0x93, 0xf0, 0x72, 0x80, 0xe2, 0x50,
  0x18, 0x69, 0xf9, 0x23, 0x69, 0xda, 0xf8, 0xc0, 0x54, 0xaa, 0x56, 0x80, 0xee, 0x03, 0x41, 0xd0,
  0xe7, 0xf9, 0x9d, 0xdd, 0x76, 0x09, 0xbc, 0xbf, 0x96, 0xac, 0x3e, 0x5e, 0x83, 0xa3, 0xec, 0xcb,
  0x0f, 0xab, 0x86, 0x9b, 0x02, 0xfc, 0x34, 0x1a, 0x06, 0x3f, 0xc8, 0xd9, 0xf0, 0x00, 0x4c, 0x17
};
u8 g_8BC0[4] = { 0x03, 0xfd, 0x11, 0x1b };
u8 g_8BC4[144] = {
  0x71, 0x39, 0xad, 0x80, 0xa1, 0x07, 0xdc, 0xa1, 0xe4, 0xe5, 0x59, 0x97, 0xeb, 0xb3, 0xff, 0x48,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x05, 0x3b, 0xd1, 0xe7, 0x47, 0x33, 0x64, 0xf9, 0x67, 0xfe, 0xde,
  0x61, 0x7c, 0xe4, 0x06, 0x13, 0x60, 0xad, 0x6d, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x06, 0x3b, 0xd1,
  0xb2, 0xdb, 0x96, 0xd9, 0x8b, 0x7e, 0x13, 0x95, 0x45, 0x55, 0x1c, 0xa3, 0xcb, 0x7e, 0x2e, 0xab,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x3b, 0xd1, 0x03, 0xbc, 0x7a, 0x54, 0xfb, 0x42, 0xdf, 0xc4,
  0x4e, 0x1e, 0x0c, 0xe1, 0xdb, 0xf6, 0x5b, 0xf3, 0x00, 0x00, 0x00, 0x00, 0x04, 0x2b, 0x74, 0x89,
  0xd7, 0xeb, 0xc9, 0x24, 0x7e, 0x23, 0x3d, 0x89, 0x46, 0xe7, 0x2e, 0x47, 0xad, 0xdb, 0x0d, 0x09,
  0xff, 0x5e, 0xf1, 0xe9, 0xb1, 0xc9, 0x3e, 0xc5, 0xdb, 0xe0, 0x67, 0x82, 0x95, 0x3a, 0x8e, 0xa5,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x24, 0xe9, 0x24, 0x84, 0xbe, 0x35, 0xf0, 0xc5, 0x91, 0xa3
};
u8 g_8BD8[4] = { 0xf0, 0x05, 0x3b, 0xd1 };
u8 g_8BDC[144] = {
  0xe7, 0x47, 0x33, 0x64, 0xf9, 0x67, 0xfe, 0xde, 0x61, 0x7c, 0xe4, 0x06, 0x13, 0x60, 0xad, 0x6d,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x06, 0x3b, 0xd1, 0xb2, 0xdb, 0x96, 0xd9, 0x8b, 0x7e, 0x13, 0x95,
  0x45, 0x55, 0x1c, 0xa3, 0xcb, 0x7e, 0x2e, 0xab, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x3b, 0xd1,
  0x03, 0xbc, 0x7a, 0x54, 0xfb, 0x42, 0xdf, 0xc4, 0x4e, 0x1e, 0x0c, 0xe1, 0xdb, 0xf6, 0x5b, 0xf3,
  0x00, 0x00, 0x00, 0x00, 0x04, 0x2b, 0x74, 0x89, 0xd7, 0xeb, 0xc9, 0x24, 0x7e, 0x23, 0x3d, 0x89,
  0x46, 0xe7, 0x2e, 0x47, 0xad, 0xdb, 0x0d, 0x09, 0xff, 0x5e, 0xf1, 0xe9, 0xb1, 0xc9, 0x3e, 0xc5,
  0xdb, 0xe0, 0x67, 0x82, 0x95, 0x3a, 0x8e, 0xa5, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x24, 0xe9,
  0x24, 0x84, 0xbe, 0x35, 0xf0, 0xc5, 0x91, 0xa3, 0x3d, 0xa5, 0x94, 0x12, 0x8f, 0xd0, 0x4c, 0x01,
  0x2a, 0x1b, 0xf2, 0xd5, 0x11, 0xf8, 0x93, 0x04, 0x9b, 0xf7, 0xb1, 0x7f, 0xc7, 0x8f, 0x6a, 0x11
};
u8 g_8BF0[4] = { 0xf0, 0x06, 0x3b, 0xd1 };
u8 g_8BF4[144] = {
  0xb2, 0xdb, 0x96, 0xd9, 0x8b, 0x7e, 0x13, 0x95, 0x45, 0x55, 0x1c, 0xa3, 0xcb, 0x7e, 0x2e, 0xab,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x3b, 0xd1, 0x03, 0xbc, 0x7a, 0x54, 0xfb, 0x42, 0xdf, 0xc4,
  0x4e, 0x1e, 0x0c, 0xe1, 0xdb, 0xf6, 0x5b, 0xf3, 0x00, 0x00, 0x00, 0x00, 0x04, 0x2b, 0x74, 0x89,
  0xd7, 0xeb, 0xc9, 0x24, 0x7e, 0x23, 0x3d, 0x89, 0x46, 0xe7, 0x2e, 0x47, 0xad, 0xdb, 0x0d, 0x09,
  0xff, 0x5e, 0xf1, 0xe9, 0xb1, 0xc9, 0x3e, 0xc5, 0xdb, 0xe0, 0x67, 0x82, 0x95, 0x3a, 0x8e, 0xa5,
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x24, 0xe9, 0x24, 0x84, 0xbe, 0x35, 0xf0, 0xc5, 0x91, 0xa3,
  0x3d, 0xa5, 0x94, 0x12, 0x8f, 0xd0, 0x4c, 0x01, 0x2a, 0x1b, 0xf2, 0xd5, 0x11, 0xf8, 0x93, 0x04,
  0x9b, 0xf7, 0xb1, 0x7f, 0xc7, 0x8f, 0x6a, 0x11, 0x00, 0x00, 0x00, 0x00, 0x04, 0x23, 0xf1, 0xf5,
  0xc0, 0xf0, 0x2d, 0x65, 0xc6, 0xa6, 0x56, 0x9b, 0xb8, 0xe8, 0x0e, 0x82, 0x3b, 0x56, 0xe2, 0xa9
};
u8 g_8C08[4] = { 0xf0, 0x08, 0x3b, 0xd1 };
u8 g_8C0C[144] = {
  0x03, 0xbc, 0x7a, 0x54, 0xfb, 0x42, 0xdf, 0xc4, 0x4e, 0x1e, 0x0c, 0xe1, 0xdb, 0xf6, 0x5b, 0xf3,
  0x00, 0x00, 0x00, 0x00, 0x04, 0x2b, 0x74, 0x89, 0xd7, 0xeb, 0xc9, 0x24, 0x7e, 0x23, 0x3d, 0x89,
  0x46, 0xe7, 0x2e, 0x47, 0xad, 0xdb, 0x0d, 0x09, 0xff, 0x5e, 0xf1, 0xe9, 0xb1, 0xc9, 0x3e, 0xc5,
  0xdb, 0xe0, 0x67, 0x82, 0x95, 0x3a, 0x8e, 0xa5, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x24, 0xe9,
  0x24, 0x84, 0xbe, 0x35, 0xf0, 0xc5, 0x91, 0xa3, 0x3d, 0xa5, 0x94, 0x12, 0x8f, 0xd0, 0x4c, 0x01,
  0x2a, 0x1b, 0xf2, 0xd5, 0x11, 0xf8, 0x93, 0x04, 0x9b, 0xf7, 0xb1, 0x7f, 0xc7, 0x8f, 0x6a, 0x11,
  0x00, 0x00, 0x00, 0x00, 0x04, 0x23, 0xf1, 0xf5, 0xc0, 0xf0, 0x2d, 0x65, 0xc6, 0xa6, 0x56, 0x9b,
  0xb8, 0xe8, 0x0e, 0x82, 0x3b, 0x56, 0xe2, 0xa9, 0x21, 0xea, 0xbe, 0x48, 0x63, 0xde, 0x22, 0x4b,
  0x3a, 0xdb, 0x81, 0x53, 0x30, 0x03, 0x54, 0x92, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x08, 0x28, 0x69
};

// sceMesgLed_driver - used by loadcore (decrypt kernel modules), arg9 = 8, 9, 6, 2, 1, 0
// sceResmap_driver - ??? arg9 = 0
// sceDbman_driver - ??? arg9 = 0
// sceNwman_driver - for devkit PSARs? arg9 = 2, 0
// sceResmgr_driver - ??? arg9 = 9, 6, 2
// sceResmgr - used by VSH
// sceMesgd_driver - for retail PSARs
// sceWmd_driver - for ME image
// sceDbsvr_driver - ???
// sceMesgIns_driver - used by psheet
// scePauth_driver - ???
// scePauth - ???

// TODO: ensure memcmp, memcpy, memset are all inline

u8 *g_8DC0;
SceUID g_8DFC;
void *g_8E4C, *g_8E50, *g_8EA4, *g_8EA8, *g_8EAC, *g_8EB0, *g_8EB4, *g_8EB8, *g_8EBC, *g_8EC0;
u8 g_8EE8[336]; //-0x9038
u8 g_9038[200]; //-0x90c8
u8 g_9100[384]; //-0x9250
u8 g_9280[256]; //-0x9334
u8 g_9380[32]; //-0x93A0
u8 g_93A0[40]; //-0x93C8
SceUID g_93F0, g_93F4;

int sub_0000(void *buf, int size, int keyId, int poll) // at 0x00000000
{
    g_8DC0 = buf;
    *(s32*)(buf + 0) = 5; // used for AES CBC
    *(s32*)(buf + 4) = 0;
    *(s32*)(buf + 8) = 0;
    *(s32*)(buf + 12) = keyId;
    *(s32*)(buf + 16) = size;
    if (!poll) {
        return sceUtilsBufferCopyWithRange(buf, size + 20, buf, size + 20, 7);
    } else {
        return sceUtilsBufferCopyByPollingWithRange(buf, size + 20, buf, size + 20, 7);
    }
}

// check if there exists i such that a1[i * 16 + 0..i * 16 + 15] == a0[320..320 + 15]
int sub_006C(u8 *a0, u8 *a1, u32 a2) // at 0x0000006C
{
    // 0080
    for (u32 i = 0; i < a2 / 16; i++) {
        if (memcmp(&a1[i * 16], &a0[320], 16)) {
            return 1;
        }
    }
    return 0;
}

// sub_00E0(&g_8A78, &g_8A7C, 81, pOut, cbIn, cbOut, 0, g_8EA8, 0, 2, 0, 0);
int sub_00E0(u8 tag[4], u8 arg1[144], int keyId, u8 *pOut, u32 cbIn, u32 *cbOut, int poll, u8 *arg7, u32 arg8, s32 arg9, u8 *arg10, u8 *arg11) // at 0x000000E0
{
    u8 sp0[40];
    u8 sp48[20];
    s32 sp80 = 0;
    s32 retVal;
    if (pOut == NULL || cbOut == NULL) {
        retVal = -201;
        goto end;
    }
    if (cbIn < 352) {
        retVal = -202;
        goto end;
    }
    if (((u32)pOut & 0x3F) != 0) {
        retVal = -203;
        goto end;
    }
    u32 v = ((u32)pOut >> 27) & 0x0000001F;
    if (!((0x220202 >> v) & 1)) {
        retVal = -204;
        goto end;
    }
    // 0190
    for (u32 i = 0; i < 336; i++) {
        g_8EE8[i] = pOut[i];
    }
    // 01E4
    if (memcmp(tag, (g_8EE8 + 0xD0), 4)) {
        // 19C8
        retVal = -301;
        goto end;
    }
    if ((arg9 == 9 || arg9 == 10) && cbIn < 352) {
        retVal = -202;
        goto end;
    }
    // 0208
    if (arg9 == 3) {
        // 1994
        // 19A4
        for (s32 i = 0; i < 24; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
    } else if (arg9 == 2) {
        // 1960
        // 1970
        for (s32 i = 0; i < 88; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
    } else if (arg9 == 5) {
        // 192C
        // 193C
        for (s32 i = 1; i < 88; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
        // 1884
        sp80 = g_8EE8[212];
    } else if (arg9 == 6) {
        // 18F8
        // 1908
        for (s32 i = 0; i < 56; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
    } else if (arg9 == 7) {
        // 18C4
        // 18D4
        for (s32 i = 1; i < 56; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
        // 1884
        sp80 = g_8EE8[212];
    } else if (arg9 == 9) {
        // 1890
        // 18A0
        for (s32 i = 0; i < 48; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
    } else if (arg9 == 10) {
        // 1858
        // 1864
        for (s32 i = 1; i < 48; i++) {
            if (g_8EE8[i + 212] != 0) {
                retVal = -302;
                goto end;
            }
        }
        // 1884
        sp80 = g_8EE8[212];
    }
    // 0240
    // 0244
    if (arg7 != NULL && arg8 != 0) {
        // 1838
        if (sub_006C(g_8EE8, arg7, arg8) == 1) {
            retVal = -305;
            goto end;
        }
    }
    // 0254
    u32 cb = g_8EE8[176] | (g_8EE8[177] << 8) | (g_8EE8[178] << 16) | (g_8EE8[179] << 24);
    *cbOut = cb;
    if (cbIn - 336 < cb) {
        retVal = -206;
        goto end;
    }
    if (arg9 == 2 || arg9 == 3 || arg9 == 4 || arg9 == 5 || arg9 == 6 || arg9 == 7 || arg9 == 9 || arg9 == 10) {
        // 17E0
        // 17F4
        for (s32 i = 0; i < 9; i++) {
            // 17FC
            for (u32 j = 0; j < 16; j++) {
                  (g_9100 + 0x14)[i * 16 + j] = arg1[i * 16 + j];
            }
            (g_9100 + 0x14)[i * 16] = i;
        }
    } else {
        // 02C8
        for (u32 i = 0; i < 144; i++) {
            (g_9100 + 0x14)[i] = arg1[i];
        }
    }
    // 02E8
    retVal = sub_0000(g_9100, 144, keyId, poll);
    if (retVal != 0) {
        // 1238
        if (retVal >= 0) {
            // 081C
            retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -104;
            goto reset;
        }
        retVal = -101;
        goto reset;
    }
    if (arg9 == 3 || arg9 == 5 || arg9 == 7 || arg9 == 10) {
        // 034C
        if (arg11 != NULL) {
            // 0358
            for (s32 i = 0; i < 144; i++) {
                g_9100[i] ^= arg11[i & 0xF];
            }
        }
    }
    // 0388
    // 0398
    for (u32 i = 0; i < 144; i++) {
        g_9038[i] = g_9100[i];
    }
    if (arg9 == 9 || arg9 == 10) {
        // 03D0
        for (u32 i = 0; i < 40; i++) {
            g_93A0[i] = pOut[i + 260];
        }
        // 03F0
        for (u32 i = 0; i < 40; i++) {
            pOut[i + 260] = 0;
        }
        // 0410
        for (u32 i = 0; i < 40; i++) {
            sp0[i] = g_93A0[i];
        }
        g_8DC0 = pOut;
        *(s32*)(pOut) = cbIn - 4;
        if (!poll) {
            // 17C0
            retVal = sceUtilsBufferCopyWithRange(pOut, cbIn, pOut, cbIn, 11);
        } else {
            retVal = sceUtilsBufferCopyByPollingWithRange(pOut, cbIn, pOut, cbIn, 11);
        }
        // 045C
        if (retVal != 0) {
            // 0810
            if (retVal < 0) {
                // 0908
                retVal = -102;
                goto reset;
            }
            // 081C
            retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -105;
            goto reset;
        }
        // 0468
        for (u32 i = 0; i < 20; i++) {
            sp48[i] = pOut[i];
        }
        // 0190
        for (u32 i = 0; i < 32; i++) {
            pOut[i] = g_8EE8[i];
        }
        u8 *a2 = g_9280;
        if (tag[2] == 0x16) {
            // 1790
            // 179C
            for (u32 i = 0; i < 40; i++) {
                *(s8*)(a2 + i) = g_7050[i];
            }
        } else if (tag[2] == 0x5e) {
            // 1764
            // 176C
            for (u32 i = 0; i < 40; i++) {
                *(s8*)(a2 + i) = g_7028[i];
            }
        } else {
            // 04D4
            for (u32 i = 0; i < 40; i++) {
                *(s8*)(a2 + i) = g_7000[i];
            }
        }
        a2 = a2 + 40;
        // 04F4
        // 04F8
        for (u32 i = 0; i < 20; i++) {
            *(s8*)(a2 + i) = sp48[i];
        }
        a2 = a2 + 20;
        // 051C
        for (u32 i = 0; i < 40; i++) {
            *(s8*)(a2 + i) = sp0[i];
        }
        if (!poll) {
            // 1744
            retVal = sceUtilsBufferCopyWithRange(NULL, 0, g_9280, 100, 17);
        } else {
            retVal = sceUtilsBufferCopyByPollingWithRange(NULL, 0, g_9280, 100, 17);
        }
        // 055C
        if (retVal != 0) {
            retVal = -306;
            goto reset;
        }
    }
    // XXXXXXXXXXXXXXXXX4
    // 0568
    if (arg9 == 3) {
        // 145C
        u8 *a2 = g_9100;
        // 1470
        for (u32 i = 0; i < 64; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0xec)[i];
        }
        a2 = a2 + 64;
        // 1494
        for (u32 i = 0; i < 80; i++) {
            *(s8*)(a2 + i) = 0;
        }
        a2 = a2 + 32;
        *(s8*)(a2 + 0) = 3;
        a2 = a2 + 16;
        *(s8*)(a2 + 0) = 80;
        a2 = a2 + 32;
        // 14D0
        for (u32 i = 0; i < 48; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0x80)[i];
        }
        a2 = a2 + 48;
        // 14FC
        for (u32 i = 0; i < 16; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0xD0)[i];
        }
        a2 = a2 + 16;
        // 1528
        for (u32 i = 0; i < 16; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0x12C)[i];
        }
        // 1550
        for (s32 i = 0; i < 80; i++) {
            if (arg11 != NULL) {
                g_9100[144 + i] ^= arg11[i & 0xF];
            }
            // 1580
            g_9100[144 + i] ^= arg10[i];
        }
        retVal = sceUtilsBufferCopyWithRange(g_9280, 180, g_9100, 336, 3);
        if (retVal != 0) {
            // 0A78
            if (retVal >= 0) {
                // 081C
                retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -304;
                goto reset;
            }
            retVal = -303;
            goto reset;
        }
        a2 = g_9100;
        // 15D4
        for (u32 i = 0; i < 4; i++) {
            a2[i] = (g_8EE8 + 0xD0)[i];
        }
        a2 = a2 + 4;
        // 15F8
        for (u32 i = 0; i < 88; i++) {
            a2[i] = 0;
        }
        a2 = a2 + 88;
        // 161C
        for (u32 i = 0; i < 16; i++) {
            a2[i] = (g_8EE8 + 0x140)[i];
        }
        a2 = a2 + 16;
        // 1648
        for (u32 i = 0; i < 20; i++) {
            a2[i] = (g_8EE8 + 0x12C)[i];
        }
        // 1670
        for (u32 i = 0; i < 16; i++) {
            a2[i] = g_9280[0x40 + i];
        }
        a2 = a2 + 20;
        // 169C
        for (u32 i = 0; i < 48; i++) {
            a2[i] = g_9280[i];
        }
        a2 = a2 + 48;
        // 16C8
        for (u32 i = 0; i < 16; i++) {
            a2[i] = g_9280[0x30 + i];
        }
        a2 = a2 + 16;
        // 16F4
        for (u32 i = 0; i < 16; i++) {
            a2[i] = (g_8EE8 + 0xB0)[i];
        }
        a2 = a2 + 16;
        // 1720
        for (u32 i = 0; i < 128; i++) {
            a2[i] = g_8EE8[i];
        }
    } else if (arg9 == 5 || arg9 == 7 || arg9 == 10) {
        // 113C
        u8 *a2 = (g_9100 + 0x14);
        // 114C
        for (u32 i = 0; i < 48; i++) {
            a2[i] = (g_8EE8 + 0x80)[i];
        }
        a2 = a2 + 48;
        for (u32 i = 0; i < 16; i++) {
            a2[i] = (g_8EE8 + 0xD0)[i];
        }
        a2 = a2 + 16;
        // 11A4
        for (u32 i = 0; i < 16; i++) {
            a2[i] = (g_8EE8 + 0x12C)[i];
        }
        // 11CC
        for (s32 i = 0; i < 80; i++) {
            if (arg11 != NULL) {
                g_9100[20 + i] ^= arg11[i & 0xF];
            }
            // 11FC
            g_9100[20 + i] ^= arg10[i & 0xF];
        }
        retVal = sub_0000(g_9100, 80, keyId, poll);
        if (retVal != 0) {
            // 1238
            if (retVal >= 0) {
                // 081C
                retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -104;
                goto reset;
            }
            retVal = -101;
            goto reset;
        }
        // 124C
        a2 = g_9100;
        // 125C
        for (u32 i = 0; i < 80; i++) {
            g_9280[i] = a2[i];
        }
        // 1284
        for (u32 i = 0; i < 4; i++) {
            a2[i] = (g_8EE8 + 0xD0)[i];
        }
        a2 = a2 + 4;
        // 12A8
        for (u32 i = 0; i < 88; i++) {
            a2[i] = 0;
        }
        a2 = a2 + 88;
        if (arg9 == 7) {
            // 1428
            a2 = a2 - 32;
            // 1438
            for (u32 i = 0; i < 32; i++) {
                a2[i] = (g_8EE8 + 0x10C)[i];
            }
            a2 = a2 + 32;
        }
        // 12C8
        // 12D4
        for (u32 i = 0; i < 16; i++) {
            a2[i] = (g_8EE8 + 0x140)[i];
        }
        a2 = a2 + 16;
        // 1300
        for (u32 i = 0; i < 20; i++) {
            a2[i] = (g_8EE8 + 0x12C)[i];
        }
        // 1328
        for (u32 i = 0; i < 16; i++) {
            a2[i] = g_9280[0x40 + i];
        }
        a2 = a2 + 20;
        // 1354
        for (u32 i = 0; i < 48; i++) {
            a2[i] = g_9280[i];
        }
        a2 = a2 + 48;
        // 1380
        for (u32 i = 0; i < 16; i++) {
            a2[i] = g_9280[0x30 + i];
        }
        a2 = a2 + 16;
        // 13AC
        for (u32 i = 0; i < 16; i++) {
            a2[i] = (g_8EE8 + 0xB0)[i];
        }
        a2 = a2 + 16;
        // 13D8
        for (u32 i = 0; i < 128; i++) {
            a2[i] = g_8EE8[i];
        }
        if (arg9 == 10) {
            // 140C
            for (u32 i = 0; i < 40; i++) {
                (g_9100 + 0x34)[i] = 0;
            }
        }
    } else if (arg9 != 2 && arg9 != 4 && arg9 != 6 && arg9 != 9) {
        // 10A4
        // 10B4
        for (u32 i = 0; i < 128; i++) {
            g_9100[i] = (g_8EE8 + 0xD0)[i];
        }
        // 10E4
        for (u32 i = 0; i < 128; i++) {
            (g_9100 + 0x80)[i] = (g_8EE8 + 0x80)[i];
        }
        // 1114
        for (u32 i = 0; i < 128; i++) {
            (g_9100 + 0xD0)[i] = g_8EE8[i];
        }
    } else {
        // 05CC
        u8 *a2 = g_9100;
        // 05DC
        for (u32 i = 0; i < 92; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0xD0)[i];
        }
        a2 = a2 + 92;
        // 0608
        for (u32 i = 0; i < 16; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0x140)[i];
        }
        a2 = a2 + 16;
        // 0634
        for (u32 i = 0; i < 20; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0x12C)[i];
        }
        a2 = a2 + 20;
        // 0660
        for (u32 i = 0; i < 48; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0x80)[i];
        }
        a2 = a2 + 48;
        // 068C
        for (u32 i = 0; i < 16; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0xD0)[i];
        }
        a2 = a2 + 16;
        // 06B8
        for (u32 i = 0; i < 16; i++) {
            *(s8*)(a2 + i) = (g_8EE8 + 0xB0)[i];
        }
        a2 = a2 + 16;
        // 06E4
        for (u32 i = 0; i < 128; i++) {
            *(s8*)(a2 + i) = g_8EE8[i];
        }
        if (arg9 == 9) {
            // 1080
            // 1088
            for (u32 i = 0; i < 40; i++) {
                (g_9100 + 0x34)[i] = 0;
            }
        }
    }
    // XXXXXXXXXXXXXXXXX3
    // 0710
    if (arg9 == 1) {
        // 0FF8
        // 100C
        for (u32 i = 0; i < 160; i++) {
            g_9280[0x14 + i] = (g_9100 + 0x10)[i];
        }
        retVal = sub_0000(g_9280, 160, keyId, poll);
        if (retVal != 0) {
            // 09B8
            if (retVal >= 0) {
                // 081C
                retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -106;
                goto reset;
            }
            retVal = -103;
            goto reset;
        }
        // 105C
        for (u32 i = 0; i < 160; i++) {
            (g_9100 + 0x10)[i] = g_9280[i];
        }
    } else if (arg9 == 2 || arg9 == 3 || arg9 == 4 || arg9 == 5 || arg9 == 6 || arg9 == 7 || arg9 == 9 || arg9 == 10) {
        // 0F0C
        // 0F1C
        for (u32 i = 0; i < 96; i++) {
            g_9280[0x14 + i] = (g_9100 + 0x5C)[i];
        }
        if (arg9 == 3 || arg9 == 5 || arg9 == 7 || arg9 == 10) {
            // 0F70
            // 0F78
            for (s32 i = 0; i < 96; i++) {
                g_9280[20 + i] ^= arg10[i & 0xF];
            }
        }
        // 0FA4
        retVal = sub_0000(g_9280, 96, keyId, poll);
        if (retVal != 0) {
            // 09B8
            if (retVal >= 0) {
                // 081C
                retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -106;
                goto reset;
            }
            retVal = -103;
            goto reset;
        }
        // 0FD4
        for (u32 i = 0; i < 96; i++) {
            (g_9100 + 0x5C)[i] = g_9280[i];
        }
    }
    // XXXXXXXXXXXXXXXXX2
    // 073C
    if (arg9 == 2 || arg9 == 3 || arg9 == 4 || arg9 == 5 || arg9 == 6 || arg9 == 7 || arg9 == 9 || arg9 == 10) {
        // 0D44
        // 0D54
        for (u32 i = 0; i < 20; i++) {
            g_9280[i] = (g_9100 + 0x6C)[i];
        }
        if (arg9 == 4) {
            // 0EE0
            // 0EEC
            for (s32 i = 103; i >= 0; i--) {
                g_9100[24 + i] = g_9100[4 + i];
            }
        } else {
            // 0D84
            for (u32 i = 0; i < 16; i++) {
                (g_9100 + 0x70)[i] = (g_9100 + 0x70)[i - 20];
            }
            if (arg9 != 6 && arg9 != 7) {
                // 0EC4
                for (u32 i = 0; i < 88; i++) {
                    (g_9100 + 0x18)[i] = 0;
                }
            } else {
                // 0DC8
                for (u32 i = 0; i < 32; i++) {
                    g_9380[i] = (g_9100 + 0x3C)[i];
                }
                // 0DF8
                for (u32 i = 0; i < 32; i++) {
                    (g_9100 + 0x50)[i] = g_9380[i];
                }
                // 0E20
                for (u32 i = 0; i < 56; i++) {
                    (g_9100 + 0x18)[i] = 0;
                }
            }
            // 0E38
            if (sp80 == 128)
            {
                // 0EB8
                g_9100[24] = 128;
            }
        }
        // 0E48
        g_8DC0 = g_9100;
        // 0E5C
        for (u32 i = 0; i < 4; i++) {
            g_9100[4 + i] = g_9100[i];
        }
        *(s32*)g_8DC0 = 332;
        // 0E94
        for (u32 i = 0; i < 16; i++) {
            g_9100[8 + i] = g_9038[i];
        }
    } else {
        // 0770
        for (u32 i = 0; i < 20; i++) {
            g_9280[i] = g_9100[4 + i];
        }
        g_8DC0 = g_9100;
        *(s32*)g_9100 = 332;
        // 07B4
        for (u32 i = 0; i < 20; i++) {
            g_9100[4 + i] = g_9038[i];
        }
    }
    // XXXXXXXXXXXXXXXXX1
    // 07D0
    if (!poll) {
        // 0D24
        retVal = sceUtilsBufferCopyWithRange(g_9100, 336, g_9100, 336, 11);
    } else {
        retVal = sceUtilsBufferCopyByPollingWithRange(g_9100, 336, g_9100, 336, 11);
    }
    // 07F8
    if (retVal != 0) {
        // 0810
        if (retVal < 0) {
            // 0908
            retVal = -102;
            goto reset;
        }
        // 081C
        retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -105;
        goto reset;
    }
    // 0910, 0D1C, 0934
    if (memcmp(g_9100, g_9280, 20)) {
        // 0B14
        retVal = -302;
        goto reset;
    }
    // XXXXXXXXXXXXXXXXX0
    if (arg9 == 2 || arg9 == 3 || arg9 == 4 || arg9 == 5 || arg9 == 6 || arg9 == 7 || arg9 == 9 || arg9 == 10) {
        // 0B34
        // 0B44
        for (s32 i = 0; i < 64; i++) {
            g_9100[128 + i] ^= g_9038[16 + i];
        }
        retVal = sub_0000((g_9100 + 0x6C), 64, keyId, poll);
        if (retVal != 0) {
            // 09B8
            if (retVal >= 0) {
                // 081C
                retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -106;
                goto reset;
            }
            retVal = -103;
            goto reset;
        }
        // 0B9C
        for (s32 i = 0; i < 64; i++) {
            pOut[64 + i] = (g_9100 + 0x6C)[i] ^ g_9038[80 + i];
        }
        if (arg9 == 6 || arg9 == 7) {
            // 0C90
            for (u32 i = 0; i < 48; i++) {
                pOut[128 + i] = 0;
            }
            pOut[160] = 1;
            // 0CB8
            for (u32 i = 0; i < 16; i++) {
                pOut[176 + i] = (g_9100 + 0xC0)[i];
            }
            // 0CD8
            for (u32 i = 0; i < 16; i++) {
                pOut[192 + i] = 0;
            }
            // 0CF8
            for (u32 i = 0; i < 128; i++) {
                pOut[208 + i] = (g_9100 + 0xD0)[i];
            }
        } else {
            // 0BE0
            for (u32 i = 0; i < 32; i++) {
                pOut[128 + i] = g_9380[i];
            }
            // 0C00
            for (u32 i = 0; i < 16; i++) {
                pOut[160 + i] = 0;
            }
            pOut[164] = 1;
            pOut[160] = 1;
            // 0C2C
            for (u32 i = 0; i < 16; i++) {
                pOut[176 + i] = (g_9100 + 0xC0)[i];
            }
            // 0C4C
            for (u32 i = 0; i < 16; i++) {
                pOut[192 + i] = 0;
            }
            // 0C6C
            for (u32 i = 0; i < 128; i++) {
                pOut[208 + i] = (g_9100 + 0xD0)[i];
            }
        }
    } else {
        // 0970
        for (s32 i = 0; i < 112; i++) {
            g_9100[64 + i] ^= g_9038[20 + i];
        }
        retVal = sub_0000((g_9100 + 0x2C), 112, keyId, poll);
        if (retVal != 0) {
            // 09B8
            if (retVal >= 0) {
                // 081C
                retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -106;
                goto reset;
            }
            retVal = -103;
            goto reset;
        }
        // 09CC
        // 09DC
        for (s32 i = 0; i < 112; i++) {
            pOut[64 + i] = (g_9100 + 0x2C)[i] ^ g_9038[32 + i];
        }
        // 0A10
        for (u32 i = 0; i < 160; i++) {
            pOut[176 + i] = (g_9100 + 0xB0)[i];
        }
        if (arg9 == 8) {
            // 0B1C
            if (pOut[164] != 1) {
                retVal = -303;
                goto reset;
            }
        }
    }
    // 0A38
    if (sp80 == 128)
    {
        // 0B00
        if (pOut[1424] != 0) {
            // 0B14
            retVal = -302;
            goto reset;
        }
        pOut[1424] |= 0xffffff80;
    }
    // 0A44
    if (!poll) {
        // 0AE4
        retVal = sceUtilsBufferCopyWithRange(pOut, cbIn, pOut + 64, cbIn - 64, 1);
    } else {
        retVal = sceUtilsBufferCopyByPollingWithRange(pOut, cbIn, pOut + 64, cbIn - 64, 1);
    }
    // 0A60
    if (retVal == 0) {
        // 0A98
        if (*cbOut < 336) {
            // 0ABC
            u32 cb = *cbOut;
            for (u32 i = 0; i < 336 - cb; i++) {
                pOut[cb + i] = 0;
            }
        }
        retVal = 0;
        goto end;
    }
    if (sp80 == 128) {
        // 0A8C
        *(s8*)(pOut + 1424) &= 0x7F;
    }
    // 0A78
    if (retVal >= 0) {
        // 081C
        retVal = (retVal == KIRK_NOT_INITIALIZED) ? -107 : -304;
        goto reset;
    }
    retVal = -303;
    goto reset;

reset:
    // 0828
    // 0834
    for (u32 i = 0; i < 336; i++) {
        pOut[i] = g_8EE8[i];
    }
    *(s32*)(cbOut + 0) = 0;

end:
    // 0860
    for (u32 i = 0; i < 144; i++) {
        g_9038[i] = 0;
    }
    // 0880
    for (u32 i = 0; i < 336; i++) {
        g_9100[i] = 0;
    }
    // 08A0
    for (u32 i = 0; i < 180; i++) {
        g_9280[i] = 0;
    }
    // 08C0
    for (u32 i = 0; i < 336; i++) {
        g_8EE8[i] = 0;
    }
    return retVal;
}

#if 0
sub_000019D8(...) // at 0x000019D8
{
    sp = sp - 32;
    *(s32*)(sp + 0) = s0;
    s0 = a2;
    t5 = a0;
    *(s32*)(sp + 16) = s4;
    a2 = t2;
    s4 = a1;
    *(s32*)(sp + 8) = s2;
    a1 = t1;
    s2 = t3;
    *(s32*)(sp + 20) = ra;
    t4 = -201;
    *(s32*)(sp + 12) = s3;
    *(s32*)(sp + 4) = s1;
    if (s0 == 0)
        goto loc_00001C44;
    v0 = (u32)a3 < (u32)352;
    t4 = -202;
    if (v0 != 0)
        goto loc_00001C44;
    v0 = s0 & 0x3F;
    t4 = -203;
    if (v0 != 0)
        goto loc_00001C44;
    v0 = 0x220000;
    v1 = (s0 >> 27) & 0x0000001F;
    v0 = v0 | 0x202;
    v0 = (s32)v0 >> v1;
    v0 = v0 & 0x1;
    t4 = -204;
    if (v0 == 0)
        goto loc_00001C44;
    t1 = 0x10000; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = 0;
    t0 = t1 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000

loc_00001A54:
    v0 = s0 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a3 + t0;
    a3 = a3 + 1;
    v0 = (u32)a3 < (u32)336;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00001A54;
    s3 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t0 = s3 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = 0;

loc_00001A7C:
    v0 = s0 + a3;
    a0 = *(u8*)(v0 + 236);
    v1 = t0 + a3;
    a3 = a3 + 1;
    v0 = (u32)a3 < (u32)64;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00001A7C;
    t0 = t0 + 64;
    a0 = 0;

loc_00001AA0:
    v1 = t0 + a0;
    a0 = a0 + 1;
    v0 = (u32)a0 < (u32)80;
    *(s8*)(v1 + 0) = 0;
    if (v0 != 0)
        goto loc_00001AA0;
    t0 = t0 + 32;
    v0 = 2;
    *(s8*)(t0 + 0) = v0;
    t0 = t0 + 16;
    v0 = 80;
    *(s8*)(t0 + 0) = v0;
    a3 = 0;
    t0 = t0 + 32;

loc_00001AD4:
    v0 = s0 + a3;
    a0 = *(u8*)(v0 + 128);
    v1 = t0 + a3;
    a3 = a3 + 1;
    v0 = (u32)a3 < (u32)48;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00001AD4;
    t0 = t0 + 48;
    a3 = 0;

loc_00001AF8:
    v0 = s0 + a3;
    a0 = *(u8*)(v0 + 192);
    v1 = t0 + a3;
    a3 = a3 + 1;
    v0 = (u32)a3 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00001AF8;
    t0 = t0 + 16;
    a3 = 0;

loc_00001B1C:
    v0 = s0 + a3;
    a0 = *(u8*)(v0 + 300);
    v1 = t0 + a3;
    a3 = a3 + 1;
    v0 = (u32)a3 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00001B1C;
    v0 = 0x10000; // Data ref 0x00008FB8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = t5;
    v0 = v0 - 28744; // Data ref 0x00008FB8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t4 = 0;
    a3 = 0;
    t3 = *(s8*)(a0 + 0);

loc_00001B50:
    t0 = *(s8*)(v0 + 0);
    a3 = a3 + 1;
    v1 = (s32)a3 < (s32)4;
    a0 = a0 + 1;
    v0 = v0 + 1;
    if (t3 != t0)
        goto loc_00001DB4;
    if (v1 != 0)
    {
        t3 = *(s8*)(a0 + 0);
        goto loc_00001B50;
    }

loc_00001B70:
    v0 = 0x10000; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = v0 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = 0;
    if (t4 != 0)
        goto loc_00001DAC;
    v0 = a3 + a0;

loc_00001B84:
    v1 = *(u8*)(v0 + 212);
    a3 = a3 + 1;
    v0 = (s32)a3 < (s32)24;
    t4 = -302;
    if (v1 != 0)
        goto loc_00001C44;
    cond = (v0 != 0);
    v0 = a3 + a0;
    if (cond)
        goto loc_00001B84;
    v0 = (u32)0 < (u32)a1;
    v1 = (u32)0 < (u32)a2;
    v0 = v0 & v1;
    if (v0 != 0)
        goto loc_00001D90;

loc_00001BB4:
    v0 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t1 = v0 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = 0;

loc_00001BC0:
    s1 = s3 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t0 = a3 + s1;
    a2 = a3 + t1;
    a1 = a3 & 0xF;
    if (s2 == 0)
        goto loc_00001BE8;
    v0 = s2 + a1;
    v1 = *(u8*)(a2 + 144);
    a0 = *(u8*)(v0 + 0);
    asm("xor        $v1, $v1, $a0");
    *(s8*)(a2 + 144) = v1;

loc_00001BE8:
    a0 = s4 + a1;
    v1 = *(u8*)(t0 + 144);
    v0 = *(u8*)(a0 + 0);
    a3 = a3 + 1;
    a1 = (s32)a3 < (s32)80;
    asm("xor        $v1, $v1, $v0");
    *(s8*)(t0 + 144) = v1;
    if (a1 != 0)
        goto loc_00001BC0;
    a0 = s1;
    a1 = 336;
    a2 = s1;
    a3 = 336;
    t0 = 2; // Data ref 0x00791AFD
    v0, v1 = sceUtilsBufferCopyWithRange(...);
    a2 = s1;
    if (v0 == 0)
        goto loc_00001CA8;
    t4 = -303;
    if ((s32)v0 < 0)
        goto loc_00001C44;
    a0 = v0 ^ 0xC;
    v0 = -107;
    v1 = -304;
    t4 = v0;
    if (a0 != 0)
        t4 = v1;

loc_00001C44:
    v0 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000

loc_00001C48:
    a2 = v0 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00001C50:
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)336;
    *(s8*)(v1 + 0) = 0;
    if (v0 != 0)
        goto loc_00001C50;
    v0 = 0x10000; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00001C70:
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)336;
    *(s8*)(v1 + 0) = 0;
    if (v0 != 0)
        goto loc_00001C70;
    ra = *(s32*)(sp + 20);
    s4 = *(s32*)(sp + 16);
    s3 = *(s32*)(sp + 12);
    s2 = *(s32*)(sp + 8);
    s1 = *(s32*)(sp + 4);
    s0 = *(s32*)(sp + 0);
    v0 = t4;
    sp = sp + 32;
    return (v1 << 32) | v0;

loc_00001CA8:
    a3 = 0;

loc_00001CAC:
    t2 = s3 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t1 = a3 + t2;
    t0 = a3 + a2;
    a1 = a3 & 0xF;
    if (s2 == 0)
        goto loc_00001CD4;
    v0 = s2 + a1;
    v1 = *(u8*)(t0 + 144);
    a0 = *(u8*)(v0 + 0);
    asm("xor        $v1, $v1, $a0");
    *(s8*)(t0 + 144) = v1;

loc_00001CD4:
    a0 = s4 + a1;
    v1 = *(u8*)(t1 + 144);
    v0 = *(u8*)(a0 + 0);
    a3 = a3 + 1;
    a1 = (s32)a3 < (s32)80;
    asm("xor        $v1, $v1, $v0");
    *(s8*)(t1 + 144) = v1;
    if (a1 != 0)
        goto loc_00001CAC;
    t0 = t2;
    a1 = 0;

loc_00001CFC:
    v0 = t0 + a1;
    a0 = *(u8*)(v0 + 0);
    v1 = s0 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)64;
    *(s8*)(v1 + 236) = a0;
    if (v0 != 0)
        goto loc_00001CFC;
    v0 = 0x10000; // Data ref 0x00009190 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t0 = v0 - 28272; // Data ref 0x00009190 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00001D24:
    v0 = t0 + a1;
    a0 = *(u8*)(v0 + 0);
    v1 = s0 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)48;
    *(s8*)(v1 + 128) = a0;
    if (v0 != 0)
        goto loc_00001D24;
    t0 = t0 + 48;
    a1 = 0;

loc_00001D48:
    v0 = t0 + a1;
    a0 = *(u8*)(v0 + 0);
    v1 = s0 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 192) = a0;
    if (v0 != 0)
        goto loc_00001D48;
    t0 = t0 + 16;
    a1 = 0;

loc_00001D6C:
    v0 = t0 + a1;
    a0 = *(u8*)(v0 + 0);
    v1 = s0 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 300) = a0;
    if (v0 != 0)
        goto loc_00001D6C;
    t4 = 0;
    goto loc_00001C44;

loc_00001D90:
    a0 = t1 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0, v1 = sub_0000006C(...);
    v1 = 1;
    t4 = -305;
    if (v0 != v1)
        goto loc_00001BB4;
    v0 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    goto loc_00001C48;

loc_00001DAC:
    t4 = -301;
    goto loc_00001C44;

loc_00001DB4:
    t4 = t3 - t0;
    goto loc_00001B70;
}
#endif

int module_start() // at 0x00001DBC
{
    g_8DFC = 0;
    SceUID id1 = sceKernelCreateSema("SceUtils4096" /* 0x72D8 */, 0, 1, 1, NULL);
    g_93F0 = id1;
    if (id1 <= 0) {
        return 1;
    }
    SceUID id2 = sceKernelCreateSema("SceUtils5120" /* 0x72E8 */, 0, 1, 1, NULL);
    g_93F4 = id2;
    if (id2 <= 0) {
        // 1E40
        sceKernelDeleteSema(g_93F0);
        return 1;
    }
    return 0;
}

#if 0
sceMesgLed_driver_5C3A61FE(...) // at 0x00001E50
{
    sp = sp - 48;
    *(s32*)(sp + 32) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 36) = ra;
    s0 = -108; // Data ref 0x078B1AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x00000; // Data ref 0x00007F40 ... 0x380290F0 0x966B4AF9 0x0AEE3F79 0x7E8DC804
    a1 = 0x00000; // Data ref 0x00007F44 ... 0x966B4AF9 0x0AEE3F79 0x7E8DC804 0xCF3A385F
    a0 = a0 + 32576; // Data ref 0x00007F40 ... 0x380290F0 0x966B4AF9 0x0AEE3F79 0x7E8DC804
    a1 = a1 + 32580; // Data ref 0x00007F44 ... 0x966B4AF9 0x0AEE3F79 0x7E8DC804 0xCF3A385F
    a2 = 90;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    if (v0 != 0)
        goto loc_00001F24;
    v0 = 0x10000; // Data ref 0x00008E10 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29168); // Data ref 0x00008E10 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 9;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    a0 = 0x00000; // Data ref 0x00007398 ... 0x02000000 0xB32F8172 0xBD3D5A39 0x74108A38
    a1 = 0x00000; // Data ref 0x0000739C ... 0xB32F8172 0xBD3D5A39 0x74108A38 0xDFB15596
    t3 = 0x00000; // Data ref 0x0000742C ... 0x3EF1638C 0x67817D08 0x8412BCC7 0xEAF2BAF8
    v0 = -301;
    a0 = a0 + 29592; // Data ref 0x00007398 ... 0x02000000 0xB32F8172 0xBD3D5A39 0x74108A38
    a1 = a1 + 29596; // Data ref 0x0000739C ... 0xB32F8172 0xBD3D5A39 0x74108A38 0xDFB15596
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t3 = t3 + 29740; // Data ref 0x0000742C ... 0x3EF1638C 0x67817D08 0x8412BCC7 0xEAF2BAF8
    a2 = 69;
    t2 = 0;
    if (s0 == v0)
        goto loc_00001F48;

loc_00001F10:
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00001F24:
    v0 = s0;
    ra = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00001F48:
    v0 = 2832;
    v1 = 8;
    *(s32*)(sp + 0) = v0;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00001F10;
}

sceMesgLed_driver_3783B0AD(...) // at 0x00001F6C
{
    sp = sp - 48;
    *(s32*)(sp + 32) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 36) = ra;
    s0 = -108; // Data ref 0x07C41AF9
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x00000; // Data ref 0x00007F40 ... 0x380290F0 0x966B4AF9 0x0AEE3F79 0x7E8DC804
    a1 = 0x00000; // Data ref 0x00007F44 ... 0x966B4AF9 0x0AEE3F79 0x7E8DC804 0xCF3A385F
    a0 = a0 + 32576; // Data ref 0x00007F40 ... 0x380290F0 0x966B4AF9 0x0AEE3F79 0x7E8DC804
    a1 = a1 + 32580; // Data ref 0x00007F44 ... 0x966B4AF9 0x0AEE3F79 0x7E8DC804 0xCF3A385F
    a2 = 90;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    if (v0 != 0)
        goto loc_0000203C;
    v0 = 0x10000; // Data ref 0x00008E10 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29168); // Data ref 0x00008E10 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 9;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    a0 = 0x00000; // Data ref 0x00007398 ... 0x02000000 0xB32F8172 0xBD3D5A39 0x74108A38
    a1 = 0x00000; // Data ref 0x0000739C ... 0xB32F8172 0xBD3D5A39 0x74108A38 0xDFB15596
    t3 = 0x00000; // Data ref 0x0000742C ... 0x3EF1638C 0x67817D08 0x8412BCC7 0xEAF2BAF8
    v0 = -301;
    a0 = a0 + 29592; // Data ref 0x00007398 ... 0x02000000 0xB32F8172 0xBD3D5A39 0x74108A38
    a1 = a1 + 29596; // Data ref 0x0000739C ... 0xB32F8172 0xBD3D5A39 0x74108A38 0xDFB15596
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t3 = t3 + 29740; // Data ref 0x0000742C ... 0x3EF1638C 0x67817D08 0x8412BCC7 0xEAF2BAF8
    a2 = 69;
    t2 = 1;
    if (s0 == v0)
        goto loc_00002060;

loc_00002028:
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_0000203C:
    v0 = s0;
    ra = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00002060:
    v0 = 2832;
    v1 = 8;
    *(s32*)(sp + 0) = v0;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00002028;
}

sceMesgLed_driver_2CB700EC(...) // at 0x00002084
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x080A1AF7
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_0000213C;
    v0 = 0x10000; // Data ref 0x00008E40 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29120); // Data ref 0x00008E40 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008160 ... 0x457B90F0 0x476176BA 0x72A8558B 0x6D791589
    a1 = 0x10000; // Data ref 0x00008164 ... 0x476176BA 0x72A8558B 0x6D791589 0x0E782FD7
    v0 = 9;
    a0 = a0 - 32416; // Data ref 0x00008160 ... 0x457B90F0 0x476176BA 0x72A8558B 0x6D791589
    a1 = a1 - 32412; // Data ref 0x00008164 ... 0x476176BA 0x72A8558B 0x6D791589 0x0E782FD7
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    s4 = -301;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s4)
        goto loc_00002164;

loc_00002124:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_0000213C:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00002164:
    v0 = 0x10000; // Data ref 0x00008E3C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29124); // Data ref 0x00008E3C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008148 ... 0x457B8AF0 0x1560EC47 0xE0E32C12 0x316F224A
    a1 = 0x10000; // Data ref 0x0000814C ... 0x1560EC47 0xE0E32C12 0x316F224A 0x3E97FA9F
    a0 = a0 - 32440; // Data ref 0x00008148 ... 0x457B8AF0 0x1560EC47 0xE0E32C12 0x316F224A
    a1 = a1 - 32436; // Data ref 0x0000814C ... 0x1560EC47 0xE0E32C12 0x316F224A 0x3E97FA9F
    s5 = 6;
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E30 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29136); // Data ref 0x00008E30 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008100 ... 0x457B80F0 0x021835D4 0xA0FB6829 0xEDA5A96A
    a1 = 0x10000; // Data ref 0x00008104 ... 0x021835D4 0xA0FB6829 0xEDA5A96A 0x9D2EFD78
    a0 = a0 - 32512; // Data ref 0x00008100 ... 0x457B80F0 0x021835D4 0xA0FB6829 0xEDA5A96A
    a1 = a1 - 32508; // Data ref 0x00008104 ... 0x021835D4 0xA0FB6829 0xEDA5A96A 0x9D2EFD78
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E38 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29128); // Data ref 0x00008E38 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008130 ... 0x457B0CF0 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA
    a1 = 0x10000; // Data ref 0x00008134 ... 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA 0xA2F1DFDF
    a0 = a0 - 32464; // Data ref 0x00008130 ... 0x457B0CF0 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA
    a1 = a1 - 32460; // Data ref 0x00008134 ... 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA 0xA2F1DFDF
    s5 = 2;
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E34 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29132); // Data ref 0x00008E34 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008118 ... 0x457B0BF0 0x2772947B 0x3B54CC4C 0x3746DFAE
    a1 = 0x10000; // Data ref 0x0000811C ... 0x2772947B 0x3B54CC4C 0x3746DFAE 0x874D01AC
    a0 = a0 - 32488; // Data ref 0x00008118 ... 0x457B0BF0 0x2772947B 0x3B54CC4C 0x3746DFAE
    a1 = a1 - 32484; // Data ref 0x0000811C ... 0x2772947B 0x3B54CC4C 0x3746DFAE 0x874D01AC
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E2C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29140); // Data ref 0x00008E2C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080E8 ... 0x457B0AF0 0x062FBEE8 0xB92A05B1 0xE3031818
    a1 = 0x10000; // Data ref 0x000080EC ... 0x062FBEE8 0xB92A05B1 0xE3031818 0x267D64EB
    a0 = a0 - 32536; // Data ref 0x000080E8 ... 0x457B0AF0 0x062FBEE8 0xB92A05B1 0xE3031818
    a1 = a1 - 32532; // Data ref 0x000080EC ... 0x062FBEE8 0xB92A05B1 0xE3031818 0x267D64EB
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E28 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29144); // Data ref 0x00008E28 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080D0 ... 0x457B08F0 0xAB8F60A4 0x65A5DEAB 0xD13A435D
    a1 = 0x10000; // Data ref 0x000080D4 ... 0xAB8F60A4 0x65A5DEAB 0xD13A435D 0xEAFFC35E
    a0 = a0 - 32560; // Data ref 0x000080D0 ... 0x457B08F0 0xAB8F60A4 0x65A5DEAB 0xD13A435D
    a1 = a1 - 32556; // Data ref 0x000080D4 ... 0xAB8F60A4 0x65A5DEAB 0xD13A435D 0xEAFFC35E
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E24 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29148); // Data ref 0x00008E24 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080B8 ... 0x457B06F0 0x26630715 0x3469E2DB 0x932A0856
    a1 = 0x10000; // Data ref 0x000080BC ... 0x26630715 0x3469E2DB 0x932A0856 0xB28A4B4E
    a0 = a0 - 32584; // Data ref 0x000080B8 ... 0x457B06F0 0x26630715 0x3469E2DB 0x932A0856
    a1 = a1 - 32580; // Data ref 0x000080BC ... 0x26630715 0x3469E2DB 0x932A0856 0xB28A4B4E
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E20 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29152); // Data ref 0x00008E20 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080A0 ... 0x457B05F0 0x9BC69B40 0x7F84FBA9 0x36D22172
    a1 = 0x10000; // Data ref 0x000080A4 ... 0x9BC69B40 0x7F84FBA9 0x36D22172 0x74095596
    a0 = a0 - 32608; // Data ref 0x000080A0 ... 0x457B05F0 0x9BC69B40 0x7F84FBA9 0x36D22172
    a1 = a1 - 32604; // Data ref 0x000080A4 ... 0x9BC69B40 0x7F84FBA9 0x36D22172 0x74095596
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E1C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29156); // Data ref 0x00008E1C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008088 ... 0x76202403 0x7C6EACF3 0xE7230A04 0x24D8330D
    a1 = 0x10000; // Data ref 0x0000808C ... 0x7C6EACF3 0xE7230A04 0x24D8330D 0x4A2B3973
    a0 = a0 - 32632; // Data ref 0x00008088 ... 0x76202403 0x7C6EACF3 0xE7230A04 0x24D8330D
    a1 = a1 - 32628; // Data ref 0x0000808C ... 0x7C6EACF3 0xE7230A04 0x24D8330D 0x4A2B3973
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E18 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29160); // Data ref 0x00008E18 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007FF0 ... 0x3ACE4DCE 0x00AE662F 0xEE660201 0x0C58C6A7
    a1 = 0x00000; // Data ref 0x00007FF4 ... 0x00AE662F 0xEE660201 0x0C58C6A7 0x88B21201
    v0 = 1;
    a0 = a0 + 32752; // Data ref 0x00007FF0 ... 0x3ACE4DCE 0x00AE662F 0xEE660201 0x0C58C6A7
    a1 = a1 + 32756; // Data ref 0x00007FF4 ... 0x00AE662F 0xEE660201 0x0C58C6A7 0x88B21201
    a2 = 91;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002124;
    v0 = 0x10000; // Data ref 0x00008E14 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29164); // Data ref 0x00008E14 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007F58 ... 0x03000000 0x013B4E22 0x2C3FC8DE 0x9E070025
    a1 = 0x00000; // Data ref 0x00007F5C ... 0x013B4E22 0x2C3FC8DE 0x9E070025 0xCBBC4816
    a0 = a0 + 32600; // Data ref 0x00007F58 ... 0x03000000 0x013B4E22 0x2C3FC8DE 0x9E070025
    a1 = a1 + 32604; // Data ref 0x00007F5C ... 0x013B4E22 0x2C3FC8DE 0x9E070025 0xCBBC4816
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 70;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00002124;
}

sceMesgLed_driver_308D37FF(...) // at 0x00002488
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x08491AF9
    v0, v1 = sceKernelPollSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_0000253C;
    v0 = 0x10000; // Data ref 0x00008E40 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29120); // Data ref 0x00008E40 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008160 ... 0x457B90F0 0x476176BA 0x72A8558B 0x6D791589
    a1 = 0x10000; // Data ref 0x00008164 ... 0x476176BA 0x72A8558B 0x6D791589 0x0E782FD7
    v0 = 9;
    a0 = a0 - 32416; // Data ref 0x00008160 ... 0x457B90F0 0x476176BA 0x72A8558B 0x6D791589
    a1 = a1 - 32412; // Data ref 0x00008164 ... 0x476176BA 0x72A8558B 0x6D791589 0x0E782FD7
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    s4 = -301;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s4)
        goto loc_00002564;

loc_00002524:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_0000253C:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00002564:
    v0 = 0x10000; // Data ref 0x00008E3C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29124); // Data ref 0x00008E3C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008148 ... 0x457B8AF0 0x1560EC47 0xE0E32C12 0x316F224A
    a1 = 0x10000; // Data ref 0x0000814C ... 0x1560EC47 0xE0E32C12 0x316F224A 0x3E97FA9F
    a0 = a0 - 32440; // Data ref 0x00008148 ... 0x457B8AF0 0x1560EC47 0xE0E32C12 0x316F224A
    a1 = a1 - 32436; // Data ref 0x0000814C ... 0x1560EC47 0xE0E32C12 0x316F224A 0x3E97FA9F
    s5 = 6;
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E30 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29136); // Data ref 0x00008E30 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008100 ... 0x457B80F0 0x021835D4 0xA0FB6829 0xEDA5A96A
    a1 = 0x10000; // Data ref 0x00008104 ... 0x021835D4 0xA0FB6829 0xEDA5A96A 0x9D2EFD78
    a0 = a0 - 32512; // Data ref 0x00008100 ... 0x457B80F0 0x021835D4 0xA0FB6829 0xEDA5A96A
    a1 = a1 - 32508; // Data ref 0x00008104 ... 0x021835D4 0xA0FB6829 0xEDA5A96A 0x9D2EFD78
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E38 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29128); // Data ref 0x00008E38 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008130 ... 0x457B0CF0 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA
    a1 = 0x10000; // Data ref 0x00008134 ... 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA 0xA2F1DFDF
    a0 = a0 - 32464; // Data ref 0x00008130 ... 0x457B0CF0 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA
    a1 = a1 - 32460; // Data ref 0x00008134 ... 0xB1BA34AC 0x6FAE8D97 0xD6B1E8BA 0xA2F1DFDF
    s5 = 2;
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E34 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29132); // Data ref 0x00008E34 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008118 ... 0x457B0BF0 0x2772947B 0x3B54CC4C 0x3746DFAE
    a1 = 0x10000; // Data ref 0x0000811C ... 0x2772947B 0x3B54CC4C 0x3746DFAE 0x874D01AC
    a0 = a0 - 32488; // Data ref 0x00008118 ... 0x457B0BF0 0x2772947B 0x3B54CC4C 0x3746DFAE
    a1 = a1 - 32484; // Data ref 0x0000811C ... 0x2772947B 0x3B54CC4C 0x3746DFAE 0x874D01AC
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E2C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29140); // Data ref 0x00008E2C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080E8 ... 0x457B0AF0 0x062FBEE8 0xB92A05B1 0xE3031818
    a1 = 0x10000; // Data ref 0x000080EC ... 0x062FBEE8 0xB92A05B1 0xE3031818 0x267D64EB
    a0 = a0 - 32536; // Data ref 0x000080E8 ... 0x457B0AF0 0x062FBEE8 0xB92A05B1 0xE3031818
    a1 = a1 - 32532; // Data ref 0x000080EC ... 0x062FBEE8 0xB92A05B1 0xE3031818 0x267D64EB
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E28 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29144); // Data ref 0x00008E28 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080D0 ... 0x457B08F0 0xAB8F60A4 0x65A5DEAB 0xD13A435D
    a1 = 0x10000; // Data ref 0x000080D4 ... 0xAB8F60A4 0x65A5DEAB 0xD13A435D 0xEAFFC35E
    a0 = a0 - 32560; // Data ref 0x000080D0 ... 0x457B08F0 0xAB8F60A4 0x65A5DEAB 0xD13A435D
    a1 = a1 - 32556; // Data ref 0x000080D4 ... 0xAB8F60A4 0x65A5DEAB 0xD13A435D 0xEAFFC35E
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E24 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29148); // Data ref 0x00008E24 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080B8 ... 0x457B06F0 0x26630715 0x3469E2DB 0x932A0856
    a1 = 0x10000; // Data ref 0x000080BC ... 0x26630715 0x3469E2DB 0x932A0856 0xB28A4B4E
    a0 = a0 - 32584; // Data ref 0x000080B8 ... 0x457B06F0 0x26630715 0x3469E2DB 0x932A0856
    a1 = a1 - 32580; // Data ref 0x000080BC ... 0x26630715 0x3469E2DB 0x932A0856 0xB28A4B4E
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E20 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29152); // Data ref 0x00008E20 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000080A0 ... 0x457B05F0 0x9BC69B40 0x7F84FBA9 0x36D22172
    a1 = 0x10000; // Data ref 0x000080A4 ... 0x9BC69B40 0x7F84FBA9 0x36D22172 0x74095596
    a0 = a0 - 32608; // Data ref 0x000080A0 ... 0x457B05F0 0x9BC69B40 0x7F84FBA9 0x36D22172
    a1 = a1 - 32604; // Data ref 0x000080A4 ... 0x9BC69B40 0x7F84FBA9 0x36D22172 0x74095596
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E1C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29156); // Data ref 0x00008E1C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008088 ... 0x76202403 0x7C6EACF3 0xE7230A04 0x24D8330D
    a1 = 0x10000; // Data ref 0x0000808C ... 0x7C6EACF3 0xE7230A04 0x24D8330D 0x4A2B3973
    a0 = a0 - 32632; // Data ref 0x00008088 ... 0x76202403 0x7C6EACF3 0xE7230A04 0x24D8330D
    a1 = a1 - 32628; // Data ref 0x0000808C ... 0x7C6EACF3 0xE7230A04 0x24D8330D 0x4A2B3973
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s5;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E18 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29160); // Data ref 0x00008E18 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007FF0 ... 0x3ACE4DCE 0x00AE662F 0xEE660201 0x0C58C6A7
    a1 = 0x00000; // Data ref 0x00007FF4 ... 0x00AE662F 0xEE660201 0x0C58C6A7 0x88B21201
    v0 = 1;
    a0 = a0 + 32752; // Data ref 0x00007FF0 ... 0x3ACE4DCE 0x00AE662F 0xEE660201 0x0C58C6A7
    a1 = a1 + 32756; // Data ref 0x00007FF4 ... 0x00AE662F 0xEE660201 0x0C58C6A7 0x88B21201
    a2 = 91;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002524;
    v0 = 0x10000; // Data ref 0x00008E14 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29164); // Data ref 0x00008E14 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007F58 ... 0x03000000 0x013B4E22 0x2C3FC8DE 0x9E070025
    a1 = 0x00000; // Data ref 0x00007F5C ... 0x013B4E22 0x2C3FC8DE 0x9E070025 0xCBBC4816
    a0 = a0 + 32600; // Data ref 0x00007F58 ... 0x03000000 0x013B4E22 0x2C3FC8DE 0x9E070025
    a1 = a1 + 32604; // Data ref 0x00007F5C ... 0x013B4E22 0x2C3FC8DE 0x9E070025 0xCBBC4816
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 70;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00002524;
}

sceResmap_driver_E5659590(...) // at 0x00002888
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    *(s32*)(sp + 32) = ra;
    a1 = 1; // Data ref 0x09491AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008178 ... 0x04000000 0x418EA0A3 0xC9BCE082 0xC57F229D
    a1 = 0x10000; // Data ref 0x0000817C ... 0x418EA0A3 0xC9BCE082 0xC57F229D 0xBE9C3C9C
    t0 = s0;
    a0 = a0 - 32392; // Data ref 0x00008178 ... 0x04000000 0x418EA0A3 0xC9BCE082 0xC57F229D
    a1 = a1 - 32388; // Data ref 0x0000817C ... 0x418EA0A3 0xC9BCE082 0xC57F229D 0xBE9C3C9C
    a3 = s2;
    t1 = s1;
    a2 = 71;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_00002920;
    v0 = 0x10000; // Data ref 0x00008E44 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29116); // Data ref 0x00008E44 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00002920:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceResmap_driver_4434E59F(...) // at 0x00002940
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 32) = ra;
    s0 = a2;
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x10000; // Data ref 0x00008178 ... 0x04000000 0x418EA0A3 0xC9BCE082 0xC57F229D
    a1 = 0x10000; // Data ref 0x0000817C ... 0x418EA0A3 0xC9BCE082 0xC57F229D 0xBE9C3C9C
    t1 = s0;
    a0 = a0 - 32392; // Data ref 0x00008178 ... 0x04000000 0x418EA0A3 0xC9BCE082 0xC57F229D
    a1 = a1 - 32388; // Data ref 0x0000817C ... 0x418EA0A3 0xC9BCE082 0xC57F229D 0xBE9C3C9C
    a3 = s2;
    t0 = s1;
    a2 = 71;
    t2 = 1;
    s0 = -108;
    if (v0 != 0)
        goto loc_000029D4;
    v0 = 0x10000; // Data ref 0x00008E44 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29116); // Data ref 0x00008E44 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000029D4:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceDbman_driver_B2B8C3F9(...) // at 0x000029F4
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    *(s32*)(sp + 32) = ra;
    a1 = 1;
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008210 ... 0x05000000 0xDD5E43F3 0x41CA413A 0x56BD22C6
    a1 = 0x10000; // Data ref 0x00008214 ... 0xDD5E43F3 0x41CA413A 0x56BD22C6 0xB31842A5
    t0 = s0;
    a0 = a0 - 32240; // Data ref 0x00008210 ... 0x05000000 0xDD5E43F3 0x41CA413A 0x56BD22C6
    a1 = a1 - 32236; // Data ref 0x00008214 ... 0xDD5E43F3 0x41CA413A 0x56BD22C6 0xB31842A5
    a3 = s2;
    t1 = s1;
    a2 = 72;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_00002A8C;
    v0 = 0x10000; // Data ref 0x00008E48 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29112); // Data ref 0x00008E48 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00002A8C:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceDbman_driver_34B53D46(...) // at 0x00002AAC
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 32) = ra;
    s0 = a2;
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x10000; // Data ref 0x00008210 ... 0x05000000 0xDD5E43F3 0x41CA413A 0x56BD22C6
    a1 = 0x10000; // Data ref 0x00008214 ... 0xDD5E43F3 0x41CA413A 0x56BD22C6 0xB31842A5
    t1 = s0;
    a0 = a0 - 32240; // Data ref 0x00008210 ... 0x05000000 0xDD5E43F3 0x41CA413A 0x56BD22C6
    a1 = a1 - 32236; // Data ref 0x00008214 ... 0xDD5E43F3 0x41CA413A 0x56BD22C6 0xB31842A5
    a3 = s2;
    t0 = s1;
    a2 = 72;
    t2 = 1;
    s0 = -108;
    if (v0 != 0)
        goto loc_00002B40;
    v0 = 0x10000; // Data ref 0x00008E48 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29112); // Data ref 0x00008E48 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00002B40:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}
#endif

int sceNwman_driver_9555D68D(void *pOut, int cbIn, void *cbOut) // at 0x00002B60
{
    if (sceKernelWaitSema(g_93F0, 1, NULL) != 0)
        return -108;
    int ret = sub_00E0(g_8340, g_8344, 0x49, pOut, cbIn, cbOut, 0, g_8E50, 0, 2, 0, 0);
    if (ret == -301) {
        // 2C50
        ret = sub_00E0(g_82A8, g_82AC, 0x49, pOut, cbIn, cbOut, 0, g_8E4C, 0, 0, 0, 0);
    }
    // 2C18
    if (sceKernelSignalSema(g_93F0, 1) != 0)
        return -109;
    return ret;
}

#if 0
sceResmgr_driver_9DC14891(...) // at 0x00002C74 - Aliases: sceResmgr_9DC14891
{
    v0 = a0 + a1;
    sp = sp - 48;
    v0 = v0 | a0;
    *(s32*)(sp + 20) = s1;
    v0 = v0 | a1;
    s1 = a0;
    v1 = a2 + 4;
    a0 = k1 << 11;
    v1 = v1 | a2;
    v0 = a0 & v0;
    *(s32*)(sp + 32) = s4;
    v1 = v1 & a0;
    s4 = -110;
    *(s32*)(sp + 28) = s3;
    s3 = k1;
    k1 = a0;
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    *(s32*)(sp + 16) = s0;
    s0 = a2;
    *(s32*)(sp + 40) = ra;
    *(s32*)(sp + 36) = s5;
    if ((s32)v0 < 0)
        goto loc_00002CE0;
    s5 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    a2 = 0;
    if ((s32)v1 >= 0)
        goto loc_00002D0C;

loc_00002CE0:
    v0 = s4;
    k1 = s3;
    ra = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00002D0C:
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s4 = -108; // Data ref 0x0B061AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008358 ... 0x0B2B90F0 0x19DFF739 0x9FEA10D7 0xB13FDB02
    a1 = 0x10000; // Data ref 0x0000835C ... 0x19DFF739 0x9FEA10D7 0xB13FDB02 0x6B269F10
    a0 = a0 - 31912; // Data ref 0x00008358 ... 0x0B2B90F0 0x19DFF739 0x9FEA10D7 0xB13FDB02
    a1 = a1 - 31908; // Data ref 0x0000835C ... 0x19DFF739 0x9FEA10D7 0xB13FDB02 0x6B269F10
    a3 = s1;
    t0 = s2;
    t1 = s0;
    a2 = 92;
    t2 = 0;
    if (v0 != 0)
        goto loc_00002CE0;
    v0 = 0x10000; // Data ref 0x00008E54 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29100); // Data ref 0x00008E54 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 9;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s4 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s4 = v1;
    goto loc_00002CE0;
}

sceMesgLed_driver_337D0DD3(...) // at 0x00002D7C
{
    sp = sp - 48;
    *(s32*)(sp + 36) = s5;
    s5 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 40) = s6;
    *(s32*)(sp + 32) = s4; // Data ref 0x0B381AF7
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00002E58;
    v0 = 0x10000; // Data ref 0x00008DE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29208); // Data ref 0x00008DE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a1 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    v0 = 9;
    a0 = a0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a1 = a1 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    s4 = -301;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s4)
        goto loc_000030D8;

loc_00002E1C:
    t0 = 0x10000; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = *(s32*)(t0 - 29188); // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 1;
    cond = (v1 == v0);
    v0 = 3;
    if (cond)
        goto loc_00003010;
    cond = (v1 == v0);
    v0 = 4;
    if (cond)
        goto loc_00002F48;
    cond = (v1 == v0);
    v0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    if (cond)
        goto loc_00002E80;

loc_00002E40:
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00002E58:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00002E80:
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002E90:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002E90;
    v0 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    v1 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a2 = v1 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002EC0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002EC0;
    v0 = 0x10000; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29276; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002EF0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002EF0;
    v0 = 0x10000; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29272; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002F20:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002F20;
    v0 = 2;

loc_00002F40:
    *(s32*)(t0 - 29188) = v0; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    goto loc_00002E40;

loc_00002F48:
    a0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    v0 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = a0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a2 = v0 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002F5C:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002F5C;
    a1 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = a1 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a3 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = 0;

loc_00002F8C:
    v0 = a2 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a2 = a2 + 1;
    v0 = (u32)a2 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002F8C;
    v0 = 0x10000; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29296; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002FBC:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002FBC;
    v0 = 0x10000; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29292; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00002FEC:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00002FEC;
    v0 = 2;
    goto loc_00002F40;

loc_00003010:
    a0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    v0 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = a0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a2 = v0 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00003024:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003024;
    a1 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = a1 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a3 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = 0;

loc_00003054:
    v0 = a2 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a2 = a2 + 1;
    v0 = (u32)a2 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003054;
    v0 = 0x10000; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29316; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00003084:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003084;
    v0 = 0x10000; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29312; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000030B4:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000030B4;
    v0 = 2;
    goto loc_00002F40;

loc_000030D8:
    v0 = 0x10000; // Data ref 0x00008DE4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29212); // Data ref 0x00008DE4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a1 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    v0 = 6;
    a0 = a0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a1 = a1 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0; // Data ref 0x0BD00038
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DE0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29216); // Data ref 0x00008DE0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a1 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a0 = a0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a1 = a1 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    s6 = 2;
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29220); // Data ref 0x00008DDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007208 ... 0xD9160BF0 0x37F18383 0xFCBED053 0x5232A78D
    a1 = 0x00000; // Data ref 0x0000720C ... 0x37F18383 0xFCBED053 0x5232A78D 0xC2C20A46
    a0 = a0 + 29192; // Data ref 0x00007208 ... 0xD9160BF0 0x37F18383 0xFCBED053 0x5232A78D
    a1 = a1 + 29196; // Data ref 0x0000720C ... 0x37F18383 0xFCBED053 0x5232A78D 0xC2C20A46
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DD8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29224); // Data ref 0x00008DD8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071F0 ... 0xD9160AF0 0x16ACA910 0x7EC019AE 0x8677603B
    a1 = 0x00000; // Data ref 0x000071F4 ... 0x16ACA910 0x7EC019AE 0x8677603B 0x63F26F01
    a0 = a0 + 29168; // Data ref 0x000071F0 ... 0xD9160AF0 0x16ACA910 0x7EC019AE 0x8677603B
    a1 = a1 + 29172; // Data ref 0x000071F4 ... 0x16ACA910 0x7EC019AE 0x8677603B 0x63F26F01
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DD4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29228); // Data ref 0x00008DD4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071D8 ... 0xD91606F0 0x36E010ED 0xF383FEC4 0xF65E7075
    a1 = 0x00000; // Data ref 0x000071DC ... 0x36E010ED 0xF383FEC4 0xF65E7075 0xF70540A4
    a0 = a0 + 29144; // Data ref 0x000071D8 ... 0xD91606F0 0x36E010ED 0xF383FEC4 0xF65E7075
    a1 = a1 + 29148; // Data ref 0x000071DC ... 0x36E010ED 0xF383FEC4 0xF65E7075 0xF70540A4
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DD0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29232); // Data ref 0x00008DD0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071C0 ... 0xD91605F0 0x8B458CB8 0xB86EE7B6 0x53A65951
    a1 = 0x00000; // Data ref 0x000071C4 ... 0x8B458CB8 0xB86EE7B6 0x53A65951 0x31865E7C
    a0 = a0 + 29120; // Data ref 0x000071C0 ... 0xD91605F0 0x8B458CB8 0xB86EE7B6 0x53A65951
    a1 = a1 + 29124; // Data ref 0x000071C4 ... 0x8B458CB8 0xB86EE7B6 0x53A65951 0x31865E7C
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DCC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29236); // Data ref 0x00008DCC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071A8 ... 0x8004FD03 0xE1F4AEF4 0x9CD2DD86 0xA642C57C
    a1 = 0x00000; // Data ref 0x000071AC ... 0xE1F4AEF4 0x9CD2DD86 0xA642C57C 0x8883A095
    a0 = a0 + 29096; // Data ref 0x000071A8 ... 0x8004FD03 0xE1F4AEF4 0x9CD2DD86 0xA642C57C
    a1 = a1 + 29100; // Data ref 0x000071AC ... 0xE1F4AEF4 0x9CD2DD86 0xA642C57C 0x8883A095
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29240); // Data ref 0x00008DC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007110 ... 0xC0CB167C 0x3927AA8F 0x17A29465 0xF228BF41
    a1 = 0x00000; // Data ref 0x00007114 ... 0x3927AA8F 0x17A29465 0xF228BF41 0x0F77AA58
    v0 = 1;
    a0 = a0 + 28944; // Data ref 0x00007110 ... 0xC0CB167C 0x3927AA8F 0x17A29465 0xF228BF41
    a1 = a1 + 28948; // Data ref 0x00007114 ... 0x3927AA8F 0x17A29465 0xF228BF41 0x0F77AA58
    a2 = 93;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00002E1C;
    v0 = 0x10000; // Data ref 0x00008DC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29244); // Data ref 0x00008DC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007078 ... 0x08000000 0x7591165B 0x02C5C544 0xC094766C
    a1 = 0x00000; // Data ref 0x0000707C ... 0x7591165B 0x02C5C544 0xC094766C 0x9512F155
    a0 = a0 + 28792; // Data ref 0x00007078 ... 0x08000000 0x7591165B 0x02C5C544 0xC094766C
    a1 = a1 + 28796; // Data ref 0x0000707C ... 0x7591165B 0x02C5C544 0xC094766C 0x9512F155
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 75;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00002E1C;
}

sceMesgLed_driver_792A6126(...) // at 0x0000336C
{
    sp = sp - 48;
    *(s32*)(sp + 36) = s5;
    s5 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 40) = s6;
    *(s32*)(sp + 32) = s4; // Data ref 0x0B871AF9
    v0, v1 = sceKernelPollSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00003444;
    v0 = 0x10000; // Data ref 0x00008DE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29208); // Data ref 0x00008DE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a1 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    v0 = 9;
    a0 = a0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a1 = a1 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    s4 = -301;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s4)
        goto loc_000036C4;

loc_00003408:
    t0 = 0x10000; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = *(s32*)(t0 - 29188); // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 1;
    cond = (v1 == v0);
    v0 = 3;
    if (cond)
        goto loc_000035FC;
    cond = (v1 == v0);
    v0 = 4;
    if (cond)
        goto loc_00003534;
    cond = (v1 == v0);
    v0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    if (cond)
        goto loc_0000346C;

loc_0000342C:
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00003444:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_0000346C:
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_0000347C:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000347C;
    v0 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    v1 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a2 = v1 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000034AC:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000034AC;
    v0 = 0x10000; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29276; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000034DC:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000034DC;
    v0 = 0x10000; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29272; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_0000350C:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000350C;
    v0 = 2;

loc_0000352C:
    *(s32*)(t0 - 29188) = v0; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    goto loc_0000342C;

loc_00003534:
    a0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    v0 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = a0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a2 = v0 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00003548:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003548;
    a1 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = a1 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a3 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = 0;

loc_00003578:
    v0 = a2 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a2 = a2 + 1;
    v0 = (u32)a2 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003578;
    v0 = 0x10000; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29296; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000035A8:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000035A8;
    v0 = 0x10000; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29292; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000035D8:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000035D8;
    v0 = 2;
    goto loc_0000352C;

loc_000035FC:
    a0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    v0 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = a0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a2 = v0 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00003610:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003610;
    a1 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = a1 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a3 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = 0;

loc_00003640:
    v0 = a2 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a2 = a2 + 1;
    v0 = (u32)a2 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003640;
    v0 = 0x10000; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29316; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00003670:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00003670;
    v0 = 0x10000; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29312; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000036A0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000036A0;
    v0 = 2;
    goto loc_0000352C;

loc_000036C4:
    v0 = 0x10000; // Data ref 0x00008DE4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29212); // Data ref 0x00008DE4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a1 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    v0 = 6;
    a0 = a0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a1 = a1 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0; // Data ref 0x0D4B0038
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DE0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29216); // Data ref 0x00008DE0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a1 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a0 = a0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a1 = a1 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    s6 = 2;
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29220); // Data ref 0x00008DDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007208 ... 0xD9160BF0 0x37F18383 0xFCBED053 0x5232A78D
    a1 = 0x00000; // Data ref 0x0000720C ... 0x37F18383 0xFCBED053 0x5232A78D 0xC2C20A46
    a0 = a0 + 29192; // Data ref 0x00007208 ... 0xD9160BF0 0x37F18383 0xFCBED053 0x5232A78D
    a1 = a1 + 29196; // Data ref 0x0000720C ... 0x37F18383 0xFCBED053 0x5232A78D 0xC2C20A46
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DD8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29224); // Data ref 0x00008DD8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071F0 ... 0xD9160AF0 0x16ACA910 0x7EC019AE 0x8677603B
    a1 = 0x00000; // Data ref 0x000071F4 ... 0x16ACA910 0x7EC019AE 0x8677603B 0x63F26F01
    a0 = a0 + 29168; // Data ref 0x000071F0 ... 0xD9160AF0 0x16ACA910 0x7EC019AE 0x8677603B
    a1 = a1 + 29172; // Data ref 0x000071F4 ... 0x16ACA910 0x7EC019AE 0x8677603B 0x63F26F01
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DD4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29228); // Data ref 0x00008DD4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071D8 ... 0xD91606F0 0x36E010ED 0xF383FEC4 0xF65E7075
    a1 = 0x00000; // Data ref 0x000071DC ... 0x36E010ED 0xF383FEC4 0xF65E7075 0xF70540A4
    a0 = a0 + 29144; // Data ref 0x000071D8 ... 0xD91606F0 0x36E010ED 0xF383FEC4 0xF65E7075
    a1 = a1 + 29148; // Data ref 0x000071DC ... 0x36E010ED 0xF383FEC4 0xF65E7075 0xF70540A4
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DD0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29232); // Data ref 0x00008DD0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071C0 ... 0xD91605F0 0x8B458CB8 0xB86EE7B6 0x53A65951
    a1 = 0x00000; // Data ref 0x000071C4 ... 0x8B458CB8 0xB86EE7B6 0x53A65951 0x31865E7C
    a0 = a0 + 29120; // Data ref 0x000071C0 ... 0xD91605F0 0x8B458CB8 0xB86EE7B6 0x53A65951
    a1 = a1 + 29124; // Data ref 0x000071C4 ... 0x8B458CB8 0xB86EE7B6 0x53A65951 0x31865E7C
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DCC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29236); // Data ref 0x00008DCC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x000071A8 ... 0x8004FD03 0xE1F4AEF4 0x9CD2DD86 0xA642C57C
    a1 = 0x00000; // Data ref 0x000071AC ... 0xE1F4AEF4 0x9CD2DD86 0xA642C57C 0x8883A095
    a0 = a0 + 29096; // Data ref 0x000071A8 ... 0x8004FD03 0xE1F4AEF4 0x9CD2DD86 0xA642C57C
    a1 = a1 + 29100; // Data ref 0x000071AC ... 0xE1F4AEF4 0x9CD2DD86 0xA642C57C 0x8883A095
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29240); // Data ref 0x00008DC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007110 ... 0xC0CB167C 0x3927AA8F 0x17A29465 0xF228BF41
    a1 = 0x00000; // Data ref 0x00007114 ... 0x3927AA8F 0x17A29465 0xF228BF41 0x0F77AA58
    v0 = 1;
    a0 = a0 + 28944; // Data ref 0x00007110 ... 0xC0CB167C 0x3927AA8F 0x17A29465 0xF228BF41
    a1 = a1 + 28948; // Data ref 0x00007114 ... 0x3927AA8F 0x17A29465 0xF228BF41 0x0F77AA58
    a2 = 93;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s4)
        goto loc_00003408;
    v0 = 0x10000; // Data ref 0x00008DC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29244); // Data ref 0x00008DC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007078 ... 0x08000000 0x7591165B 0x02C5C544 0xC094766C
    a1 = 0x00000; // Data ref 0x0000707C ... 0x7591165B 0x02C5C544 0xC094766C 0x9512F155
    a0 = a0 + 28792; // Data ref 0x00007078 ... 0x08000000 0x7591165B 0x02C5C544 0xC094766C
    a1 = a1 + 28796; // Data ref 0x0000707C ... 0x7591165B 0x02C5C544 0xC094766C 0x9512F155
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 75;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00003408;
}

sceMesgLed_driver_4EAB9850(...) // at 0x00003958
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x0D021AF7
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00003A10;
    v0 = 0x10000; // Data ref 0x00008E6C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29076); // Data ref 0x00008E6C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084E8 ... 0x7B0508F0 0x0A3E7DC9 0xC76E8154 0x74997413
    a1 = 0x10000; // Data ref 0x000084EC ... 0x0A3E7DC9 0xC76E8154 0x74997413 0xDDE71862
    a0 = a0 - 31512; // Data ref 0x000084E8 ... 0x7B0508F0 0x0A3E7DC9 0xC76E8154 0x74997413
    a1 = a1 - 31508; // Data ref 0x000084EC ... 0x0A3E7DC9 0xC76E8154 0x74997413 0xDDE71862
    s4 = 2;
    a2 = 94;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_00003A38;

loc_000039F8:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00003A10:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00003A38:
    v0 = 0x10000; // Data ref 0x00008E68 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29080); // Data ref 0x00008E68 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084D0 ... 0x7B0506F0 0x87D21A78 0x96A2BD24 0x36893F18
    a1 = 0x10000; // Data ref 0x000084D4 ... 0x87D21A78 0x96A2BD24 0x36893F18 0x85929072
    a0 = a0 - 31536; // Data ref 0x000084D0 ... 0x7B0506F0 0x87D21A78 0x96A2BD24 0x36893F18
    a1 = a1 - 31532; // Data ref 0x000084D4 ... 0x87D21A78 0x96A2BD24 0x36893F18 0x85929072
    a2 = 94;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000039F8;
    v0 = 0x10000; // Data ref 0x00008E64 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29084); // Data ref 0x00008E64 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084B8 ... 0x7B0505F0 0x3A77862D 0xDD4FA456 0x9371163C
    a1 = 0x10000; // Data ref 0x000084BC ... 0x3A77862D 0xDD4FA456 0x9371163C 0x43118EAA
    a0 = a0 - 31560; // Data ref 0x000084B8 ... 0x7B0505F0 0x3A77862D 0xDD4FA456 0x9371163C
    a1 = a1 - 31556; // Data ref 0x000084BC ... 0x3A77862D 0xDD4FA456 0x9371163C 0x43118EAA
    a2 = 94;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000039F8;
    v0 = 0x10000; // Data ref 0x00008E60 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29088); // Data ref 0x00008E60 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084A0 ... 0x0A35EA03 0x0C3848F9 0x74A78896 0x54A0654F
    a1 = 0x10000; // Data ref 0x000084A4 ... 0x0C3848F9 0x74A78896 0x54A0654F 0xB8D976C2
    a0 = a0 - 31584; // Data ref 0x000084A0 ... 0x0A35EA03 0x0C3848F9 0x74A78896 0x54A0654F
    a1 = a1 - 31580; // Data ref 0x000084A4 ... 0x0C3848F9 0x74A78896 0x54A0654F 0xB8D976C2
    a2 = 94;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000039F8;
    v0 = 0x10000; // Data ref 0x00008E5C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29092); // Data ref 0x00008E5C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008408 ... 0xBB67C59F 0x69737882 0xE21F870D 0xA01FD649
    a1 = 0x10000; // Data ref 0x0000840C ... 0x69737882 0xE21F870D 0xA01FD649 0x47F8DA58
    v0 = 1;
    a0 = a0 - 31736; // Data ref 0x00008408 ... 0xBB67C59F 0x69737882 0xE21F870D 0xA01FD649
    a1 = a1 - 31732; // Data ref 0x0000840C ... 0x69737882 0xE21F870D 0xA01FD649 0x47F8DA58
    a2 = 94;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000039F8;
    v0 = 0x10000; // Data ref 0x00008E58 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29096); // Data ref 0x00008E58 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008370 ... 0x09000000 0xF576D788 0x0EEC6DB7 0xE6850B00
    a1 = 0x10000; // Data ref 0x00008374 ... 0xF576D788 0x0EEC6DB7 0xE6850B00 0xC99B374A
    a0 = a0 - 31888; // Data ref 0x00008370 ... 0x09000000 0xF576D788 0x0EEC6DB7 0xE6850B00
    a1 = a1 - 31884; // Data ref 0x00008374 ... 0xF576D788 0x0EEC6DB7 0xE6850B00 0xC99B374A
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 76;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_000039F8;
}

sceMesgLed_driver_4BE02A12(...) // at 0x00003BA4
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x0E7E1AF9
    v0, v1 = sceKernelPollSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00003C58;
    v0 = 0x10000; // Data ref 0x00008E6C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29076); // Data ref 0x00008E6C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084E8 ... 0x7B0508F0 0x0A3E7DC9 0xC76E8154 0x74997413
    a1 = 0x10000; // Data ref 0x000084EC ... 0x0A3E7DC9 0xC76E8154 0x74997413 0xDDE71862
    a0 = a0 - 31512; // Data ref 0x000084E8 ... 0x7B0508F0 0x0A3E7DC9 0xC76E8154 0x74997413
    a1 = a1 - 31508; // Data ref 0x000084EC ... 0x0A3E7DC9 0xC76E8154 0x74997413 0xDDE71862
    s4 = 2;
    a2 = 94;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_00003C80;

loc_00003C40:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00003C58:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00003C80:
    v0 = 0x10000; // Data ref 0x00008E68 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29080); // Data ref 0x00008E68 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084D0 ... 0x7B0506F0 0x87D21A78 0x96A2BD24 0x36893F18
    a1 = 0x10000; // Data ref 0x000084D4 ... 0x87D21A78 0x96A2BD24 0x36893F18 0x85929072
    a0 = a0 - 31536; // Data ref 0x000084D0 ... 0x7B0506F0 0x87D21A78 0x96A2BD24 0x36893F18
    a1 = a1 - 31532; // Data ref 0x000084D4 ... 0x87D21A78 0x96A2BD24 0x36893F18 0x85929072
    a2 = 94;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00003C40;
    v0 = 0x10000; // Data ref 0x00008E64 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29084); // Data ref 0x00008E64 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084B8 ... 0x7B0505F0 0x3A77862D 0xDD4FA456 0x9371163C
    a1 = 0x10000; // Data ref 0x000084BC ... 0x3A77862D 0xDD4FA456 0x9371163C 0x43118EAA
    a0 = a0 - 31560; // Data ref 0x000084B8 ... 0x7B0505F0 0x3A77862D 0xDD4FA456 0x9371163C
    a1 = a1 - 31556; // Data ref 0x000084BC ... 0x3A77862D 0xDD4FA456 0x9371163C 0x43118EAA
    a2 = 94;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00003C40;
    v0 = 0x10000; // Data ref 0x00008E60 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29088); // Data ref 0x00008E60 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000084A0 ... 0x0A35EA03 0x0C3848F9 0x74A78896 0x54A0654F
    a1 = 0x10000; // Data ref 0x000084A4 ... 0x0C3848F9 0x74A78896 0x54A0654F 0xB8D976C2
    a0 = a0 - 31584; // Data ref 0x000084A0 ... 0x0A35EA03 0x0C3848F9 0x74A78896 0x54A0654F
    a1 = a1 - 31580; // Data ref 0x000084A4 ... 0x0C3848F9 0x74A78896 0x54A0654F 0xB8D976C2
    a2 = 94;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00003C40;
    v0 = 0x10000; // Data ref 0x00008E5C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29092); // Data ref 0x00008E5C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008408 ... 0xBB67C59F 0x69737882 0xE21F870D 0xA01FD649
    a1 = 0x10000; // Data ref 0x0000840C ... 0x69737882 0xE21F870D 0xA01FD649 0x47F8DA58
    v0 = 1;
    a0 = a0 - 31736; // Data ref 0x00008408 ... 0xBB67C59F 0x69737882 0xE21F870D 0xA01FD649
    a1 = a1 - 31732; // Data ref 0x0000840C ... 0x69737882 0xE21F870D 0xA01FD649 0x47F8DA58
    a2 = 94;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00003C40;
    v0 = 0x10000; // Data ref 0x00008E58 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29096); // Data ref 0x00008E58 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008370 ... 0x09000000 0xF576D788 0x0EEC6DB7 0xE6850B00
    a1 = 0x10000; // Data ref 0x00008374 ... 0xF576D788 0x0EEC6DB7 0xE6850B00 0xC99B374A
    a0 = a0 - 31888; // Data ref 0x00008370 ... 0x09000000 0xF576D788 0x0EEC6DB7 0xE6850B00
    a1 = a1 - 31884; // Data ref 0x00008374 ... 0xF576D788 0x0EEC6DB7 0xE6850B00 0xC99B374A
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 76;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00003C40;
}

sceMesgLed_driver_B2CDAC3F(...) // at 0x00003DEC
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    *(s32*)(sp + 32) = ra;
    a1 = 1; // Data ref 0x0F101AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008500 ... 0x0A000000 0x5A89E480 0x7D39DB27 0xF27B7B6B
    a1 = 0x10000; // Data ref 0x00008504 ... 0x5A89E480 0x7D39DB27 0xF27B7B6B 0xDD92DF3D
    t0 = s0;
    a0 = a0 - 31488; // Data ref 0x00008500 ... 0x0A000000 0x5A89E480 0x7D39DB27 0xF27B7B6B
    a1 = a1 - 31484; // Data ref 0x00008504 ... 0x5A89E480 0x7D39DB27 0xF27B7B6B 0xDD92DF3D
    a3 = s2;
    t1 = s1;
    a2 = 77;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_00003E84;
    v0 = 0x10000; // Data ref 0x00008E70 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29072); // Data ref 0x00008E70 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00003E84:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_B5596BE4(...) // at 0x00003EA4
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 32) = ra;
    s0 = a2;
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x10000; // Data ref 0x00008500 ... 0x0A000000 0x5A89E480 0x7D39DB27 0xF27B7B6B
    a1 = 0x10000; // Data ref 0x00008504 ... 0x5A89E480 0x7D39DB27 0xF27B7B6B 0xDD92DF3D
    t1 = s0;
    a0 = a0 - 31488; // Data ref 0x00008500 ... 0x0A000000 0x5A89E480 0x7D39DB27 0xF27B7B6B
    a1 = a1 - 31484; // Data ref 0x00008504 ... 0x5A89E480 0x7D39DB27 0xF27B7B6B 0xDD92DF3D
    a3 = s2;
    t0 = s1;
    a2 = 77;
    t2 = 1;
    s0 = -108;
    if (v0 != 0)
        goto loc_00003F38;
    v0 = 0x10000; // Data ref 0x00008E70 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29072); // Data ref 0x00008E70 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00003F38:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_C79E3488(...) // at 0x00003F58
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    *(s32*)(sp + 32) = ra;
    a1 = 1;
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008598 ... 0x0B000000 0xE71C010B 0x836B1531 0xCC0D263E
    a1 = 0x10000; // Data ref 0x0000859C ... 0xE71C010B 0x836B1531 0xCC0D263E 0xCB123669
    t3 = 0x10000; // Data ref 0x0000862C ... 0xDDC4B7FD 0x6C2C4864 0x4258FE53 0xD33A13A1
    t0 = s0;
    a0 = a0 - 31336; // Data ref 0x00008598 ... 0x0B000000 0xE71C010B 0x836B1531 0xCC0D263E
    a1 = a1 - 31332; // Data ref 0x0000859C ... 0xE71C010B 0x836B1531 0xCC0D263E 0xCB123669
    a3 = s2;
    t1 = s1;
    t3 = t3 - 31188; // Data ref 0x0000862C ... 0xDDC4B7FD 0x6C2C4864 0x4258FE53 0xD33A13A1
    a2 = 78;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_00003FF8;
    v1 = 8;
    v0 = 144;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = v0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00003FF8:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_6BF453D3(...) // at 0x00004018
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 32) = ra;
    s0 = a2;
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x10000; // Data ref 0x00008598 ... 0x0B000000 0xE71C010B 0x836B1531 0xCC0D263E
    a1 = 0x10000; // Data ref 0x0000859C ... 0xE71C010B 0x836B1531 0xCC0D263E 0xCB123669
    t3 = 0x10000; // Data ref 0x0000862C ... 0xDDC4B7FD 0x6C2C4864 0x4258FE53 0xD33A13A1
    t1 = s0;
    a0 = a0 - 31336; // Data ref 0x00008598 ... 0x0B000000 0xE71C010B 0x836B1531 0xCC0D263E
    a1 = a1 - 31332; // Data ref 0x0000859C ... 0xE71C010B 0x836B1531 0xCC0D263E 0xCB123669
    a3 = s2;
    t0 = s1;
    t3 = t3 - 31188; // Data ref 0x0000862C ... 0xDDC4B7FD 0x6C2C4864 0x4258FE53 0xD33A13A1
    a2 = 78;
    t2 = 1;
    s0 = -108;
    if (v0 != 0)
        goto loc_000040B4;
    v1 = 8;
    v0 = 144;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = v0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000040B4:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_21AFFAAC(...) // at 0x000040D4
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4;
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_0000418C;
    v0 = 0x10000; // Data ref 0x00008E88 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29048); // Data ref 0x00008E88 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008838 ... 0xADF308F0 0x6E3962F6 0xCA4D2226 0x99166402
    a1 = 0x10000; // Data ref 0x0000883C ... 0x6E3962F6 0xCA4D2226 0x99166402 0xB8E79A7B
    a0 = a0 - 30664; // Data ref 0x00008838 ... 0xADF308F0 0x6E3962F6 0xCA4D2226 0x99166402
    a1 = a1 - 30660; // Data ref 0x0000883C ... 0x6E3962F6 0xCA4D2226 0x99166402 0xB8E79A7B
    s4 = 4;
    a2 = 96;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_000041B4;

loc_00004174:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_0000418C:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_000041B4:
    v0 = 0x10000; // Data ref 0x00008E84 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29052); // Data ref 0x00008E84 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008820 ... 0xADF306F0 0xE3D50547 0x9B811E56 0xDB062F09
    a1 = 0x10000; // Data ref 0x00008824 ... 0xE3D50547 0x9B811E56 0xDB062F09 0xE092126B
    a0 = a0 - 30688; // Data ref 0x00008820 ... 0xADF306F0 0xE3D50547 0x9B811E56 0xDB062F09
    a1 = a1 - 30684; // Data ref 0x00008824 ... 0xE3D50547 0x9B811E56 0xDB062F09 0xE092126B
    a2 = 96;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004174;
    v0 = 0x10000; // Data ref 0x00008E80 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29056); // Data ref 0x00008E80 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008808 ... 0xADF305F0 0x5E709912 0xD06C0724 0x7EFE062D
    a1 = 0x10000; // Data ref 0x0000880C ... 0x5E709912 0xD06C0724 0x7EFE062D 0x26110CB3
    a0 = a0 - 30712; // Data ref 0x00008808 ... 0xADF305F0 0x5E709912 0xD06C0724 0x7EFE062D
    a1 = a1 - 30708; // Data ref 0x0000880C ... 0x5E709912 0xD06C0724 0x7EFE062D 0x26110CB3
    a2 = 96;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004174;
    v0 = 0x10000; // Data ref 0x00008E7C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29060); // Data ref 0x00008E7C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000087F0 ... 0xD67B3303 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A
    a1 = 0x10000; // Data ref 0x000087F4 ... 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A 0x355CDC64
    v0 = 2;
    a0 = a0 - 30736; // Data ref 0x000087F0 ... 0xD67B3303 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A
    a1 = a1 - 30732; // Data ref 0x000087F4 ... 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A 0x355CDC64
    a2 = 96;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004174;
    v0 = 0x10000; // Data ref 0x00008E78 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29064); // Data ref 0x00008E78 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008758 ... 0x7F24BDCD 0x754397B2 0x5EA70C5E 0x807D5338
    a1 = 0x10000; // Data ref 0x0000875C ... 0x754397B2 0x5EA70C5E 0x807D5338 0x68EA6BD1
    v0 = 1;
    a0 = a0 - 30888; // Data ref 0x00008758 ... 0x7F24BDCD 0x754397B2 0x5EA70C5E 0x807D5338
    a1 = a1 - 30884; // Data ref 0x0000875C ... 0x754397B2 0x5EA70C5E 0x807D5338 0x68EA6BD1
    a2 = 96;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004174;
    v0 = 0x10000; // Data ref 0x00008E74 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29068); // Data ref 0x00008E74 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000086C0 ... 0x0C000000 0x18A54C82 0xEA6EC8D3 0xDC044117
    a1 = 0x10000; // Data ref 0x000086C4 ... 0x18A54C82 0xEA6EC8D3 0xDC044117 0xFC01C5EA
    a0 = a0 - 31040; // Data ref 0x000086C0 ... 0x0C000000 0x18A54C82 0xEA6EC8D3 0xDC044117
    a1 = a1 - 31036; // Data ref 0x000086C4 ... 0x18A54C82 0xEA6EC8D3 0xDC044117 0xFC01C5EA
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 79;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00004174;
}

sceMesgLed_driver_52B6E552(...) // at 0x00004324
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x105D1AF9
    v0, v1 = sceKernelPollSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_000043D8;
    v0 = 0x10000; // Data ref 0x00008E88 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29048); // Data ref 0x00008E88 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008838 ... 0xADF308F0 0x6E3962F6 0xCA4D2226 0x99166402
    a1 = 0x10000; // Data ref 0x0000883C ... 0x6E3962F6 0xCA4D2226 0x99166402 0xB8E79A7B
    a0 = a0 - 30664; // Data ref 0x00008838 ... 0xADF308F0 0x6E3962F6 0xCA4D2226 0x99166402
    a1 = a1 - 30660; // Data ref 0x0000883C ... 0x6E3962F6 0xCA4D2226 0x99166402 0xB8E79A7B
    s4 = 4;
    a2 = 96;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_00004400;

loc_000043C0:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_000043D8:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00004400:
    v0 = 0x10000; // Data ref 0x00008E84 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29052); // Data ref 0x00008E84 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008820 ... 0xADF306F0 0xE3D50547 0x9B811E56 0xDB062F09
    a1 = 0x10000; // Data ref 0x00008824 ... 0xE3D50547 0x9B811E56 0xDB062F09 0xE092126B
    a0 = a0 - 30688; // Data ref 0x00008820 ... 0xADF306F0 0xE3D50547 0x9B811E56 0xDB062F09
    a1 = a1 - 30684; // Data ref 0x00008824 ... 0xE3D50547 0x9B811E56 0xDB062F09 0xE092126B
    a2 = 96;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000043C0;
    v0 = 0x10000; // Data ref 0x00008E80 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29056); // Data ref 0x00008E80 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008808 ... 0xADF305F0 0x5E709912 0xD06C0724 0x7EFE062D
    a1 = 0x10000; // Data ref 0x0000880C ... 0x5E709912 0xD06C0724 0x7EFE062D 0x26110CB3
    a0 = a0 - 30712; // Data ref 0x00008808 ... 0xADF305F0 0x5E709912 0xD06C0724 0x7EFE062D
    a1 = a1 - 30708; // Data ref 0x0000880C ... 0x5E709912 0xD06C0724 0x7EFE062D 0x26110CB3
    a2 = 96;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000043C0;
    v0 = 0x10000; // Data ref 0x00008E7C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29060); // Data ref 0x00008E7C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000087F0 ... 0xD67B3303 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A
    a1 = 0x10000; // Data ref 0x000087F4 ... 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A 0x355CDC64
    v0 = 2;
    a0 = a0 - 30736; // Data ref 0x000087F0 ... 0xD67B3303 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A
    a1 = a1 - 30732; // Data ref 0x000087F4 ... 0x3C4F03C9 0xD0E84FDD 0x74EDDD9A 0x355CDC64
    a2 = 96;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000043C0;
    v0 = 0x10000; // Data ref 0x00008E78 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29064); // Data ref 0x00008E78 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008758 ... 0x7F24BDCD 0x754397B2 0x5EA70C5E 0x807D5338
    a1 = 0x10000; // Data ref 0x0000875C ... 0x754397B2 0x5EA70C5E 0x807D5338 0x68EA6BD1
    v0 = 1;
    a0 = a0 - 30888; // Data ref 0x00008758 ... 0x7F24BDCD 0x754397B2 0x5EA70C5E 0x807D5338
    a1 = a1 - 30884; // Data ref 0x0000875C ... 0x754397B2 0x5EA70C5E 0x807D5338 0x68EA6BD1
    a2 = 96;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_000043C0;
    v0 = 0x10000; // Data ref 0x00008E74 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29068); // Data ref 0x00008E74 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000086C0 ... 0x0C000000 0x18A54C82 0xEA6EC8D3 0xDC044117
    a1 = 0x10000; // Data ref 0x000086C4 ... 0x18A54C82 0xEA6EC8D3 0xDC044117 0xFC01C5EA
    a0 = a0 - 31040; // Data ref 0x000086C0 ... 0x0C000000 0x18A54C82 0xEA6EC8D3 0xDC044117
    a1 = a1 - 31036; // Data ref 0x000086C4 ... 0x18A54C82 0xEA6EC8D3 0xDC044117 0xFC01C5EA
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 79;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_000043C0;
}

sceMesgLed_driver_C00DAD75(...) // at 0x00004570
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x10F01AF7
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00004628;
    v0 = 0x10000; // Data ref 0x00008EA0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29024); // Data ref 0x00008EA0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000089C8 ... 0x279D08F0 0x857227C7 0xF0F7A7AB 0xCC86C14C
    a1 = 0x10000; // Data ref 0x000089CC ... 0x857227C7 0xF0F7A7AB 0xCC86C14C 0xCA177FE3
    a0 = a0 - 30264; // Data ref 0x000089C8 ... 0x279D08F0 0x857227C7 0xF0F7A7AB 0xCC86C14C
    a1 = a1 - 30260; // Data ref 0x000089CC ... 0x857227C7 0xF0F7A7AB 0xCC86C14C 0xCA177FE3
    s4 = 4;
    a2 = 97;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_00004650;

loc_00004610:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00004628:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00004650:
    v0 = 0x10000; // Data ref 0x00008E9C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29028); // Data ref 0x00008E9C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000089B0 ... 0x279D06F0 0x089E4076 0xA13B9BDB 0x8E968A47
    a1 = 0x10000; // Data ref 0x000089B4 ... 0x089E4076 0xA13B9BDB 0x8E968A47 0x9262F7F3
    a0 = a0 - 30288; // Data ref 0x000089B0 ... 0x279D06F0 0x089E4076 0xA13B9BDB 0x8E968A47
    a1 = a1 - 30284; // Data ref 0x000089B4 ... 0x089E4076 0xA13B9BDB 0x8E968A47 0x9262F7F3
    a2 = 97;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004610;
    v0 = 0x10000; // Data ref 0x00008E98 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29032); // Data ref 0x00008E98 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008998 ... 0x279D05F0 0xB53BDC23 0xEAD682A9 0x2B6EA363
    a1 = 0x10000; // Data ref 0x0000899C ... 0xB53BDC23 0xEAD682A9 0x2B6EA363 0x54E1E92B
    a0 = a0 - 30312; // Data ref 0x00008998 ... 0x279D05F0 0xB53BDC23 0xEAD682A9 0x2B6EA363
    a1 = a1 - 30308; // Data ref 0x0000899C ... 0xB53BDC23 0xEAD682A9 0x2B6EA363 0x54E1E92B
    a2 = 97;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004610;
    v0 = 0x10000; // Data ref 0x00008E94 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29036); // Data ref 0x00008E94 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008980 ... 0xD66DF703 0x68574322 0x65CE412F 0xC67CA34C
    a1 = 0x10000; // Data ref 0x00008984 ... 0x68574322 0x65CE412F 0xC67CA34C 0x60F3ACC4
    v0 = 2;
    a0 = a0 - 30336; // Data ref 0x00008980 ... 0xD66DF703 0x68574322 0x65CE412F 0xC67CA34C
    a1 = a1 - 30332; // Data ref 0x00008984 ... 0x68574322 0x65CE412F 0xC67CA34C 0x60F3ACC4
    a2 = 97;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004610;
    v0 = 0x10000; // Data ref 0x00008E90 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29040); // Data ref 0x00008E90 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000088E8 ... 0x1BC8D12B 0xE61811AE 0xB2F1B2ED 0xBFCACCA8
    a1 = 0x10000; // Data ref 0x000088EC ... 0xE61811AE 0xB2F1B2ED 0xBFCACCA8 0x878014A9
    v0 = 1;
    a0 = a0 - 30488; // Data ref 0x000088E8 ... 0x1BC8D12B 0xE61811AE 0xB2F1B2ED 0xBFCACCA8
    a1 = a1 - 30484; // Data ref 0x000088EC ... 0xE61811AE 0xB2F1B2ED 0xBFCACCA8 0x878014A9
    a2 = 97;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004610;
    v0 = 0x10000; // Data ref 0x00008E8C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29044); // Data ref 0x00008E8C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008850 ... 0x0D000000 0x47A407F5 0x2529E0D7 0x95E3C8B2
    a1 = 0x10000; // Data ref 0x00008854 ... 0x47A407F5 0x2529E0D7 0x95E3C8B2 0xE0695E3E
    a0 = a0 - 30640; // Data ref 0x00008850 ... 0x0D000000 0x47A407F5 0x2529E0D7 0x95E3C8B2
    a1 = a1 - 30636; // Data ref 0x00008854 ... 0x47A407F5 0x2529E0D7 0x95E3C8B2 0xE0695E3E
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 80;
    t2 = 0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00004610;
}

sceMesgLed_driver_F8485C9C(...) // at 0x000047C0
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x11841AF9
    v0, v1 = sceKernelPollSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00004874;
    v0 = 0x10000; // Data ref 0x00008EA0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29024); // Data ref 0x00008EA0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000089C8 ... 0x279D08F0 0x857227C7 0xF0F7A7AB 0xCC86C14C
    a1 = 0x10000; // Data ref 0x000089CC ... 0x857227C7 0xF0F7A7AB 0xCC86C14C 0xCA177FE3
    a0 = a0 - 30264; // Data ref 0x000089C8 ... 0x279D08F0 0x857227C7 0xF0F7A7AB 0xCC86C14C
    a1 = a1 - 30260; // Data ref 0x000089CC ... 0x857227C7 0xF0F7A7AB 0xCC86C14C 0xCA177FE3
    s4 = 4;
    a2 = 97;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_0000489C;

loc_0000485C:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00004874:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_0000489C:
    v0 = 0x10000; // Data ref 0x00008E9C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29028); // Data ref 0x00008E9C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000089B0 ... 0x279D06F0 0x089E4076 0xA13B9BDB 0x8E968A47
    a1 = 0x10000; // Data ref 0x000089B4 ... 0x089E4076 0xA13B9BDB 0x8E968A47 0x9262F7F3
    a0 = a0 - 30288; // Data ref 0x000089B0 ... 0x279D06F0 0x089E4076 0xA13B9BDB 0x8E968A47
    a1 = a1 - 30284; // Data ref 0x000089B4 ... 0x089E4076 0xA13B9BDB 0x8E968A47 0x9262F7F3
    a2 = 97;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_0000485C;
    v0 = 0x10000; // Data ref 0x00008E98 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29032); // Data ref 0x00008E98 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008998 ... 0x279D05F0 0xB53BDC23 0xEAD682A9 0x2B6EA363
    a1 = 0x10000; // Data ref 0x0000899C ... 0xB53BDC23 0xEAD682A9 0x2B6EA363 0x54E1E92B
    a0 = a0 - 30312; // Data ref 0x00008998 ... 0x279D05F0 0xB53BDC23 0xEAD682A9 0x2B6EA363
    a1 = a1 - 30308; // Data ref 0x0000899C ... 0xB53BDC23 0xEAD682A9 0x2B6EA363 0x54E1E92B
    a2 = 97;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_0000485C;
    v0 = 0x10000; // Data ref 0x00008E94 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29036); // Data ref 0x00008E94 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008980 ... 0xD66DF703 0x68574322 0x65CE412F 0xC67CA34C
    a1 = 0x10000; // Data ref 0x00008984 ... 0x68574322 0x65CE412F 0xC67CA34C 0x60F3ACC4
    v0 = 2;
    a0 = a0 - 30336; // Data ref 0x00008980 ... 0xD66DF703 0x68574322 0x65CE412F 0xC67CA34C
    a1 = a1 - 30332; // Data ref 0x00008984 ... 0x68574322 0x65CE412F 0xC67CA34C 0x60F3ACC4
    a2 = 97;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_0000485C;
    v0 = 0x10000; // Data ref 0x00008E90 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29040); // Data ref 0x00008E90 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x000088E8 ... 0x1BC8D12B 0xE61811AE 0xB2F1B2ED 0xBFCACCA8
    a1 = 0x10000; // Data ref 0x000088EC ... 0xE61811AE 0xB2F1B2ED 0xBFCACCA8 0x878014A9
    v0 = 1;
    a0 = a0 - 30488; // Data ref 0x000088E8 ... 0x1BC8D12B 0xE61811AE 0xB2F1B2ED 0xBFCACCA8
    a1 = a1 - 30484; // Data ref 0x000088EC ... 0xE61811AE 0xB2F1B2ED 0xBFCACCA8 0x878014A9
    a2 = 97;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_0000485C;
    v0 = 0x10000; // Data ref 0x00008E8C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29044); // Data ref 0x00008E8C ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008850 ... 0x0D000000 0x47A407F5 0x2529E0D7 0x95E3C8B2
    a1 = 0x10000; // Data ref 0x00008854 ... 0x47A407F5 0x2529E0D7 0x95E3C8B2 0xE0695E3E
    a0 = a0 - 30640; // Data ref 0x00008850 ... 0x0D000000 0x47A407F5 0x2529E0D7 0x95E3C8B2
    a1 = a1 - 30636; // Data ref 0x00008854 ... 0x47A407F5 0x2529E0D7 0x95E3C8B2 0xE0695E3E
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 80;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_0000485C;
}
#endif

int sceMesgd_driver_102DC8AF(void *pOut, int cbIn, void *cbOut) // at 0x00004A0C
{
    if (sceKernelWaitSema(g_93F0, 1, NULL) != 0) {
        return -108;
    }
    int ret = sub_00E0(g_8A78, g_8A7C, 81, pOut, cbIn, cbOut, 0, g_8EA8, 0, 2, 0, 0);
    if (ret == -301) {
        // 4AFC
        ret = sub_00E0(g_89E0, g_89E4, 81, pOut, cbIn, cbOut, 0, g_8EA4, 0, 0, 0, 0);
    }
    // 4AC4
    if (sceKernelSignalSema(g_93F0, 1) != 0) {
        return -109;
    }
    return ret;
}

#if 0
sceMesgd_driver_ADD0CB66(...) // at 0x00004B20
{
    sp = sp - 48;
    *(s32*)(sp + 32) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 36) = ra;
    s0 = -108; // Data ref 0x12B11AF9
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x10000; // Data ref 0x00008A78 ... 0x63BAB403 0x21672B02 0x91AD86E7 0xDEC9BC73
    a1 = 0x10000; // Data ref 0x00008A7C ... 0x21672B02 0x91AD86E7 0xDEC9BC73 0xA4137AC5
    a0 = a0 - 30088; // Data ref 0x00008A78 ... 0x63BAB403 0x21672B02 0x91AD86E7 0xDEC9BC73
    a1 = a1 - 30084; // Data ref 0x00008A7C ... 0x21672B02 0x91AD86E7 0xDEC9BC73 0xA4137AC5
    a2 = 81;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    if (v0 != 0)
        goto loc_00004BE8;
    v0 = 0x10000; // Data ref 0x00008EA8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29016); // Data ref 0x00008EA8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 2;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    a0 = 0x10000; // Data ref 0x000089E0 ... 0x0E000000 0x77B757DE 0xEE62DD17 0x5D03787B
    a1 = 0x10000; // Data ref 0x000089E4 ... 0x77B757DE 0xEE62DD17 0x5D03787B 0x59CA8644
    v0 = -301;
    a0 = a0 - 30240; // Data ref 0x000089E0 ... 0x0E000000 0x77B757DE 0xEE62DD17 0x5D03787B
    a1 = a1 - 30236; // Data ref 0x000089E4 ... 0x77B757DE 0xEE62DD17 0x5D03787B 0x59CA8644
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 81;
    t2 = 1;
    if (s0 == v0)
        goto loc_00004C0C;

loc_00004BD4:
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00004BE8:
    v0 = s0;
    ra = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00004C0C:
    v0 = 0x10000; // Data ref 0x00008EA4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29020); // Data ref 0x00008EA4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00004BD4;
}
#endif

int sceWmd_driver_7A0E484C(void *pOut, int cbIn, void *cbOut) // at 0x00004C30
{
    if (sceKernelWaitSema(g_93F0, 1, NULL) != 0) {
        return -108;
    }
    int ret = sub_00E0(g_8C08, g_8C0C, 0x52, pOut, cbIn, cbOut, 0, g_8EC0, 0, 2, 0, 0);
    if (ret == -301) {
        // 4D10
        ret = sub_00E0(g_8BF0, g_8BF4, 0x52, pOut, cbIn, cbOut, 0, g_8EBC, 0, 2, 0, 0);
        if (ret == -301) {
            ret = sub_00E0(g_8BD8, g_8BDC, 0x52, pOut, cbIn, cbOut, 0, g_8EB8, 0, 2, 0, 0);
            if (ret == -301) {
                ret = sub_00E0(g_8BC0, g_8BC4, 0x52, pOut, cbIn, cbOut, 0, g_8EB4, 0, 2, 0, 0);
                if (ret == -301) {
                    ret = sub_00E0(g_8B28, g_8B2C, 0x52, pOut, cbIn, cbOut, 0, g_8EB0, 0, 1, 0, 0);
                    if (ret == -301) {
                        ret = sub_00E0(g_8A90, g_8A94, 0x52, pOut, cbIn, cbOut, 0, g_8EAC, 0, 0, 0, 0);
                    }
                }
            }
        }
    }
    // 4CD0
    if (sceKernelSignalSema(g_93F0, 1) != 0) {
        return -109;
    }
    return ret;
}

#if 0
sceWmd_driver_B7CE9041(...) // at 0x00004E7C
{
    sp = sp - 48;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    a1 = 1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -108;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 36) = s5;
    *(s32*)(sp + 32) = s4; // Data ref 0x13341AF9
    v0, v1 = sceKernelPollSema(...);
    cond = (v0 != 0);
    v0 = s0;
    if (cond)
        goto loc_00004F30;
    v0 = 0x10000; // Data ref 0x00008EC0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28992); // Data ref 0x00008EC0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008C08 ... 0xD13B08F0 0x547ABC03 0xC4DF42FB 0xE10C1E4E
    a1 = 0x10000; // Data ref 0x00008C0C ... 0x547ABC03 0xC4DF42FB 0xE10C1E4E 0xF35BF6DB
    a0 = a0 - 29688; // Data ref 0x00008C08 ... 0xD13B08F0 0x547ABC03 0xC4DF42FB 0xE10C1E4E
    a1 = a1 - 29684; // Data ref 0x00008C0C ... 0x547ABC03 0xC4DF42FB 0xE10C1E4E 0xF35BF6DB
    s4 = 2;
    a2 = 82;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    s5 = -301;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 == s5)
        goto loc_00004F58;

loc_00004F18:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    v0 = s0;

loc_00004F30:
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00004F58:
    v0 = 0x10000; // Data ref 0x00008EBC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28996); // Data ref 0x00008EBC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008BF0 ... 0xD13B06F0 0xD996DBB2 0x95137E8B 0xA31C5545
    a1 = 0x10000; // Data ref 0x00008BF4 ... 0xD996DBB2 0x95137E8B 0xA31C5545 0xAB2E7ECB
    a0 = a0 - 29712; // Data ref 0x00008BF0 ... 0xD13B06F0 0xD996DBB2 0x95137E8B 0xA31C5545
    a1 = a1 - 29708; // Data ref 0x00008BF4 ... 0xD996DBB2 0x95137E8B 0xA31C5545 0xAB2E7ECB
    a2 = 82;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004F18;
    v0 = 0x10000; // Data ref 0x00008EB8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29000); // Data ref 0x00008EB8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008BD8 ... 0xD13B05F0 0x643347E7 0xDEFE67F9 0x06E47C61
    a1 = 0x10000; // Data ref 0x00008BDC ... 0x643347E7 0xDEFE67F9 0x06E47C61 0x6DAD6013
    a0 = a0 - 29736; // Data ref 0x00008BD8 ... 0xD13B05F0 0x643347E7 0xDEFE67F9 0x06E47C61
    a1 = a1 - 29732; // Data ref 0x00008BDC ... 0x643347E7 0xDEFE67F9 0x06E47C61 0x6DAD6013
    a2 = 82;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004F18;
    v0 = 0x10000; // Data ref 0x00008EB4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29004); // Data ref 0x00008EB4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008BC0 ... 0x1B11FD03 0x80AD3971 0xA1DC07A1 0x9759E5E4
    a1 = 0x10000; // Data ref 0x00008BC4 ... 0x80AD3971 0xA1DC07A1 0x9759E5E4 0x48FFB3EB
    a0 = a0 - 29760; // Data ref 0x00008BC0 ... 0x1B11FD03 0x80AD3971 0xA1DC07A1 0x9759E5E4
    a1 = a1 - 29756; // Data ref 0x00008BC4 ... 0x80AD3971 0xA1DC07A1 0x9759E5E4 0x48FFB3EB
    a2 = 82;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = s4;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004F18;
    v0 = 0x10000; // Data ref 0x00008EB0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29008); // Data ref 0x00008EB0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008B28 ... 0x862648D1 0x087E2E35 0xD0A80AD6 0x01ABCFC2
    a1 = 0x10000; // Data ref 0x00008B2C ... 0x087E2E35 0xD0A80AD6 0x01ABCFC2 0x815C6C46
    v0 = 1;
    a0 = a0 - 29912; // Data ref 0x00008B28 ... 0x862648D1 0x087E2E35 0xD0A80AD6 0x01ABCFC2
    a1 = a1 - 29908; // Data ref 0x00008B2C ... 0x087E2E35 0xD0A80AD6 0x01ABCFC2 0x815C6C46
    a2 = 82;
    a3 = s3;
    t0 = s2;
    t1 = s1;
    t2 = 1;
    *(s32*)(sp + 4) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    if (v0 != s5)
        goto loc_00004F18;
    v0 = 0x10000; // Data ref 0x00008EAC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29012); // Data ref 0x00008EAC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008A90 ... 0x0F000000 0xA2FD7360 0x11F1CE2D 0xF57882E3
    a1 = 0x10000; // Data ref 0x00008A94 ... 0xA2FD7360 0x11F1CE2D 0xF57882E3 0xE69DAA34
    a0 = a0 - 30064; // Data ref 0x00008A90 ... 0x0F000000 0xA2FD7360 0x11F1CE2D 0xF57882E3
    a1 = a1 - 30060; // Data ref 0x00008A94 ... 0xA2FD7360 0x11F1CE2D 0xF57882E3 0xE69DAA34
    a3 = s3;
    t0 = s2;
    t1 = s1;
    a2 = 82;
    t2 = 1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00004F18;
}

sceMesgLed_driver_CED2C075(...) // at 0x000050C4
{
    sp = sp - 48;
    *(s32*)(sp + 36) = s5;
    s5 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 40) = s6;
    s6 = 3;
    *(s32*)(sp + 32) = s4;
    s4 = a3;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 44) = ra;
    s0 = -108; // Data ref 0x13C61AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008C48 ... 0xE92408F0 0x35BE8424 0xA391C5F0 0x1294A53D
    a1 = 0x10000; // Data ref 0x00008C4C ... 0x35BE8424 0xA391C5F0 0x1294A53D 0x014CD08F
    a0 = a0 - 29624; // Data ref 0x00008C48 ... 0xE92408F0 0x35BE8424 0xA391C5F0 0x1294A53D
    a1 = a1 - 29620; // Data ref 0x00008C4C ... 0x35BE8424 0xA391C5F0 0x1294A53D 0x014CD08F
    a2 = 101;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    if (v0 != 0)
        goto loc_000051A4;
    v1 = 0x10000; // Data ref 0x00008EC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v1 - 28984); // Data ref 0x00008EC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008C5C ... 0xD5F21B2A 0x0493F811 0x7FB1F79B 0x116A8FC7
    v0 = v0 - 29604; // Data ref 0x00008C5C ... 0xD5F21B2A 0x0493F811 0x7FB1F79B 0x116A8FC7
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 12) = s4;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    a0 = 0x10000; // Data ref 0x00008C20 ... 0x89742B04 0x24C9EBD7 0x893D237E 0x472EE746
    a1 = 0x10000; // Data ref 0x00008C24 ... 0x24C9EBD7 0x893D237E 0x472EE746 0x090DDBAD
    v0 = -301;
    a0 = a0 - 29664; // Data ref 0x00008C20 ... 0x89742B04 0x24C9EBD7 0x893D237E 0x472EE746
    a1 = a1 - 29660; // Data ref 0x00008C24 ... 0x24C9EBD7 0x893D237E 0x472EE746 0x090DDBAD
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 101;
    t2 = 0;
    if (s0 == v0)
        goto loc_000051D0;

loc_00005190:
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000051A4:
    v0 = s0;
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_000051D0:
    v0 = 0x10000; // Data ref 0x00008EC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28988); // Data ref 0x00008EC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008C34 ... 0xE9F15EFF 0xC53EC9B1 0x8267E0DB 0xA58E3A95
    v0 = v0 - 29644; // Data ref 0x00008C34 ... 0xE9F15EFF 0xC53EC9B1 0x8267E0DB 0xA58E3A95
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s4;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00005190;
}

sceMesgIns_driver_D062B635(...) // at 0x000051FC
{
    sp = sp - 32;
    *(s32*)(sp + 16) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 12) = s3;
    s3 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 8) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 4) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 0) = s0;
    *(s32*)(sp + 20) = ra;
    s0 = -108; // Data ref 0x14641AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008C48 ... 0xE92408F0 0x35BE8424 0xA391C5F0 0x1294A53D
    a1 = 0x10000; // Data ref 0x00008C5C ... 0xD5F21B2A 0x0493F811 0x7FB1F79B 0x116A8FC7
    a0 = a0 - 29624; // Data ref 0x00008C48 ... 0xE92408F0 0x35BE8424 0xA391C5F0 0x1294A53D
    a1 = a1 - 29604; // Data ref 0x00008C5C ... 0xD5F21B2A 0x0493F811 0x7FB1F79B 0x116A8FC7
    a2 = s3;
    a3 = s1;
    t0 = 0;
    t2 = 0;
    t3 = s2;
    if (v0 != 0)
        goto loc_000052B4;
    v0 = 0x10000; // Data ref 0x00008EC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t1 = *(s32*)(v0 - 28984); // Data ref 0x00008EC8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0, v1 = sub_000019D8(...);
    s0 = v0;
    a0 = 0x10000; // Data ref 0x00008C20 ... 0x89742B04 0x24C9EBD7 0x893D237E 0x472EE746
    a1 = 0x10000; // Data ref 0x00008C34 ... 0xE9F15EFF 0xC53EC9B1 0x8267E0DB 0xA58E3A95
    v0 = -301;
    a0 = a0 - 29664; // Data ref 0x00008C20 ... 0x89742B04 0x24C9EBD7 0x893D237E 0x472EE746
    a1 = a1 - 29644; // Data ref 0x00008C34 ... 0xE9F15EFF 0xC53EC9B1 0x8267E0DB 0xA58E3A95
    a2 = s3;
    a3 = s1;
    t3 = s2;
    t0 = 0;
    t2 = 0;
    if (s0 == v0)
        goto loc_000052D8;

loc_000052A0:
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000052B4:
    v0 = s0;
    ra = *(s32*)(sp + 20);
    s4 = *(s32*)(sp + 16);
    s3 = *(s32*)(sp + 12);
    s2 = *(s32*)(sp + 8);
    s1 = *(s32*)(sp + 4);
    s0 = *(s32*)(sp + 0);
    sp = sp + 32;
    return (v1 << 32) | v0;

loc_000052D8:
    v0 = 0x10000; // Data ref 0x00008EC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t1 = *(s32*)(v0 - 28988); // Data ref 0x00008EC4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0, v1 = sub_000019D8(...);
    s0 = v0;
    goto loc_000052A0;
}

sceMesgLed_driver_C7D1C16B(...) // at 0x000052EC
{
    sp = sp - 48;
    *(s32*)(sp + 36) = s5;
    s5 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 28) = s3;
    s3 = a0;
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 40) = s6;
    s6 = 3;
    *(s32*)(sp + 32) = s4;
    s4 = a3;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 44) = ra;
    s0 = -108; // Data ref 0x14A81AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008C98 ... 0x692808F0 0xF8AD6677 0x6A041D69 0x4C46FE37
    a1 = 0x10000; // Data ref 0x00008C9C ... 0xF8AD6677 0x6A041D69 0x4C46FE37 0xDC4CE2EB
    a0 = a0 - 29544; // Data ref 0x00008C98 ... 0x692808F0 0xF8AD6677 0x6A041D69 0x4C46FE37
    a1 = a1 - 29540; // Data ref 0x00008C9C ... 0xF8AD6677 0x6A041D69 0x4C46FE37 0xDC4CE2EB
    a2 = 102;
    a3 = s3;
    t0 = s1;
    t1 = s2;
    t2 = 0;
    if (v0 != 0)
        goto loc_000053CC;
    v1 = 0x10000; // Data ref 0x00008ED0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v1 - 28976); // Data ref 0x00008ED0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008CAC ... 0x921567B6 0x8A4D3D49 0x0BF9E221 0xF364247E
    v0 = v0 - 29524; // Data ref 0x00008CAC ... 0x921567B6 0x8A4D3D49 0x0BF9E221 0xF364247E
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 12) = s4;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    a0 = 0x10000; // Data ref 0x00008C70 ... 0xF5F12304 0x652DF0C0 0x9B56A6C6 0x820EE8B8
    a1 = 0x10000; // Data ref 0x00008C74 ... 0x652DF0C0 0x9B56A6C6 0x820EE8B8 0xA9E2563B
    v0 = -301;
    a0 = a0 - 29584; // Data ref 0x00008C70 ... 0xF5F12304 0x652DF0C0 0x9B56A6C6 0x820EE8B8
    a1 = a1 - 29580; // Data ref 0x00008C74 ... 0x652DF0C0 0x9B56A6C6 0x820EE8B8 0xA9E2563B
    a3 = s3;
    t0 = s1;
    t1 = s2;
    a2 = 102;
    t2 = 0;
    if (s0 == v0)
        goto loc_000053F8;

loc_000053B8:
    a0 = *(s32*)(s5 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000053CC:
    v0 = s0;
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_000053F8:
    v0 = 0x10000; // Data ref 0x00008ECC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28980); // Data ref 0x00008ECC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008C84 ... 0x48BEEA21 0x4B22DE63 0x5381DB3A 0x92540330
    v0 = v0 - 29564; // Data ref 0x00008C84 ... 0x48BEEA21 0x4B22DE63 0x5381DB3A 0x92540330
    *(s32*)(sp + 4) = s6;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s4;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_000053B8;
}

sceMesgIns_driver_4A03F940(...) // at 0x00005424
{
    sp = sp - 32;
    *(s32*)(sp + 16) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 12) = s3;
    s3 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 8) = s2;
    s2 = a2;
    a2 = 0;
    *(s32*)(sp + 4) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 0) = s0;
    *(s32*)(sp + 20) = ra;
    s0 = -108; // Data ref 0x14EE1AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008C98 ... 0x692808F0 0xF8AD6677 0x6A041D69 0x4C46FE37
    a1 = 0x10000; // Data ref 0x00008CAC ... 0x921567B6 0x8A4D3D49 0x0BF9E221 0xF364247E
    a0 = a0 - 29544; // Data ref 0x00008C98 ... 0x692808F0 0xF8AD6677 0x6A041D69 0x4C46FE37
    a1 = a1 - 29524; // Data ref 0x00008CAC ... 0x921567B6 0x8A4D3D49 0x0BF9E221 0xF364247E
    a2 = s3;
    a3 = s1;
    t0 = 0;
    t2 = 0;
    t3 = s2;
    if (v0 != 0)
        goto loc_000054DC;
    v0 = 0x10000; // Data ref 0x00008ED0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t1 = *(s32*)(v0 - 28976); // Data ref 0x00008ED0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0, v1 = sub_000019D8(...);
    s0 = v0;
    a0 = 0x10000; // Data ref 0x00008C70 ... 0xF5F12304 0x652DF0C0 0x9B56A6C6 0x820EE8B8
    a1 = 0x10000; // Data ref 0x00008C84 ... 0x48BEEA21 0x4B22DE63 0x5381DB3A 0x92540330
    v0 = -301;
    a0 = a0 - 29584; // Data ref 0x00008C70 ... 0xF5F12304 0x652DF0C0 0x9B56A6C6 0x820EE8B8
    a1 = a1 - 29564; // Data ref 0x00008C84 ... 0x48BEEA21 0x4B22DE63 0x5381DB3A 0x92540330
    a2 = s3;
    a3 = s1;
    t3 = s2;
    t0 = 0;
    t2 = 0;
    if (s0 == v0)
        goto loc_00005500;

loc_000054C8:
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000054DC:
    v0 = s0;
    ra = *(s32*)(sp + 20);
    s4 = *(s32*)(sp + 16);
    s3 = *(s32*)(sp + 12);
    s2 = *(s32*)(sp + 8);
    s1 = *(s32*)(sp + 4);
    s0 = *(s32*)(sp + 0);
    sp = sp + 32;
    return (v1 << 32) | v0;

loc_00005500:
    v0 = 0x10000; // Data ref 0x00008ECC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t1 = *(s32*)(v0 - 28980); // Data ref 0x00008ECC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0, v1 = sub_000019D8(...);
    s0 = v0;
    goto loc_000054C8;
}

sceMesgLed_driver_EBB4613D(...) // at 0x00005514
{
    sp = sp - 48;
    *(s32*)(sp + 32) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    a1 = 1;
    *(s32*)(sp + 28) = s3;
    *(s32*)(sp + 36) = ra;
    s3 = a3; // Data ref 0x15321AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008CC0 ... 0x0DAA06F0 0xA27D26CA 0x6E24CEB9 0x97A832FD
    a1 = 0x10000; // Data ref 0x00008CC4 ... 0xA27D26CA 0x6E24CEB9 0x97A832FD 0x19197CF4
    t0 = s0;
    a0 = a0 - 29504; // Data ref 0x00008CC0 ... 0x0DAA06F0 0xA27D26CA 0x6E24CEB9 0x97A832FD
    a1 = a1 - 29500; // Data ref 0x00008CC4 ... 0xA27D26CA 0x6E24CEB9 0x97A832FD 0x19197CF4
    a3 = s2;
    t1 = s1;
    a2 = 101;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_000055C0;
    v0 = 0x10000; // Data ref 0x00008ED4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28972); // Data ref 0x00008ED4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008CD4 ... 0x31203277 0x1C4B7FDF 0xC3D2D78D 0xA9F8A923
    v1 = 5;
    v0 = v0 - 29484; // Data ref 0x00008CD4 ... 0x31203277 0x1C4B7FDF 0xC3D2D78D 0xA9F8A923
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s3;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000055C0:
    v0 = s0;
    ra = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_66B348B2(...) // at 0x000055E4
{
    sp = sp - 48;
    *(s32*)(sp + 32) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    a1 = 1;
    *(s32*)(sp + 28) = s3;
    *(s32*)(sp + 36) = ra;
    s3 = a3;
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008CE8 ... 0xE1ED06F0 0x664DB62D 0x4D8EA3CB 0x63B16F13
    a1 = 0x10000; // Data ref 0x00008CEC ... 0x664DB62D 0x4D8EA3CB 0x63B16F13 0xF221CC4C
    t0 = s0;
    a0 = a0 - 29464; // Data ref 0x00008CE8 ... 0xE1ED06F0 0x664DB62D 0x4D8EA3CB 0x63B16F13
    a1 = a1 - 29460; // Data ref 0x00008CEC ... 0x664DB62D 0x4D8EA3CB 0x63B16F13 0xF221CC4C
    a3 = s2;
    t1 = s1;
    a2 = 102;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_00005690;
    v0 = 0x10000; // Data ref 0x00008ED8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28968); // Data ref 0x00008ED8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008CFC ... 0x8A61ACA5 0xC44AD26B 0x5A3B7596 0x2F46F68C
    v1 = 5;
    v0 = v0 - 29444; // Data ref 0x00008CFC ... 0x8A61ACA5 0xC44AD26B 0x5A3B7596 0x2F46F68C
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s3;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00005690:
    v0 = s0;
    ra = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_B2D95FDF(...) // at 0x000056B4
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    *(s32*)(sp + 32) = ra;
    a1 = 1;
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x10000; // Data ref 0x00008D10 ... 0x3C2A08F0 0x49382E1E 0x0816D4DA 0xBCF32E27
    a1 = 0x10000; // Data ref 0x00008D14 ... 0x49382E1E 0x0816D4DA 0xBCF32E27 0x93807537
    t0 = s0;
    a0 = a0 - 29424; // Data ref 0x00008D10 ... 0x3C2A08F0 0x49382E1E 0x0816D4DA 0xBCF32E27
    a1 = a1 - 29420; // Data ref 0x00008D14 ... 0x49382E1E 0x0816D4DA 0xBCF32E27 0x93807537
    a3 = s2;
    t1 = s1;
    a2 = 103;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_00005750;
    v0 = 0x10000; // Data ref 0x00008EDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28964); // Data ref 0x00008EDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 2;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00005750:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_C9ABD2F2(...) // at 0x00005770
{
    sp = sp - 48;
    *(s32*)(sp + 28) = s3;
    s3 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a1;
    a1 = 1;
    *(s32*)(sp + 16) = s0;
    *(s32*)(sp + 32) = ra;
    s0 = a2;
    v0, v1 = sceKernelPollSema(...);
    a0 = 0x10000; // Data ref 0x00008D10 ... 0x3C2A08F0 0x49382E1E 0x0816D4DA 0xBCF32E27
    a1 = 0x10000; // Data ref 0x00008D14 ... 0x49382E1E 0x0816D4DA 0xBCF32E27 0x93807537
    t1 = s0;
    a0 = a0 - 29424; // Data ref 0x00008D10 ... 0x3C2A08F0 0x49382E1E 0x0816D4DA 0xBCF32E27
    a1 = a1 - 29420; // Data ref 0x00008D14 ... 0x49382E1E 0x0816D4DA 0xBCF32E27 0x93807537
    a3 = s2;
    t0 = s1;
    a2 = 103;
    t2 = 1;
    s0 = -108;
    if (v0 != 0)
        goto loc_00005808;
    v0 = 0x10000; // Data ref 0x00008EDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 28964); // Data ref 0x00008EDC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 2;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 8) = 0;
    *(s32*)(sp + 12) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s3 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_00005808:
    v0 = s0;
    ra = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_91E0A9AD(...) // at 0x00005828
{
    sp = sp - 48;
    *(s32*)(sp + 32) = s4;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    a1 = 1;
    *(s32*)(sp + 28) = s3;
    *(s32*)(sp + 36) = ra;
    s3 = a3;
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x00000; // Data ref 0x000072F8 ... 0x407810F0 0xF1CAADAF 0xEC915995 0x4ED0271B
    a1 = 0x00000; // Data ref 0x000072FC ... 0xF1CAADAF 0xEC915995 0x4ED0271B 0xE73DF38A
    t0 = s0;
    a0 = a0 + 29432; // Data ref 0x000072F8 ... 0x407810F0 0xF1CAADAF 0xEC915995 0x4ED0271B
    a1 = a1 + 29436; // Data ref 0x000072FC ... 0xF1CAADAF 0xEC915995 0x4ED0271B 0xE73DF38A
    a3 = s2;
    t1 = s1;
    a2 = 106;
    t2 = 0;
    s0 = -108;
    if (v0 != 0)
        goto loc_000058D4;
    v0 = 0x10000; // Data ref 0x00008E00 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29184); // Data ref 0x00008E00 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x00000; // Data ref 0x0000730C ... 0xFEF57B84 0x7AAD4DE8 0x0E2806B5 0xE181FA09
    v1 = 5;
    v0 = v0 + 29452; // Data ref 0x0000730C ... 0xFEF57B84 0x7AAD4DE8 0x0E2806B5 0xE181FA09
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s3;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;

loc_000058D4:
    v0 = s0;
    ra = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;
}

scePauth_driver_F7AA47F6(...) // at 0x000058F8 - Aliases: scePauth_F7AA47F6
{
    v0 = a0 + a1;
    v0 = v0 | a0;
    v1 = k1 << 11;
    v0 = v0 | a1;
    sp = sp - 64;
    v0 = v1 & v0;
    *(s32*)(sp + 36) = s5;
    s5 = k1;
    k1 = v1;
    *(s32*)(sp + 32) = s4;
    s4 = a3;
    *(s32*)(sp + 28) = s3;
    s3 = a2;
    *(s32*)(sp + 24) = s2;
    s2 = a1;
    *(s32*)(sp + 20) = s1;
    s1 = a0;
    *(s32*)(sp + 48) = ra;
    *(s32*)(sp + 44) = s7;
    *(s32*)(sp + 40) = s6;
    *(s32*)(sp + 16) = s0;
    if ((s32)v0 < 0)
        goto loc_00005978;
    v0 = a2 + 4;
    v0 = v0 | a2;
    v0 = v0 & v1;
    if ((s32)v0 < 0)
    {
        s0 = -110;
        goto loc_0000597C;
    }
    v0 = a3 + 16;
    v0 = v0 | a3;
    v0 = v0 & v1;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    if ((s32)v0 >= 0)
        goto loc_000059E4;

loc_00005978:
    s0 = -110;

loc_0000597C:
    v0 = (s32)s0 < (s32)-299;
    k1 = s5;
    if (v0 == 0)
        goto loc_000059C0;
    s0 = 0x80000000;
    s0 = s0 | 0x108;

loc_00005990:
    v0 = s0;
    ra = *(s32*)(sp + 48);
    s7 = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 64;
    return (v1 << 32) | v0;

loc_000059C0:
    v1 = 0x80000000;
    v1 = v1 | 0x21;
    a1 = (s32)s0 < (s32)0;
    v0 = 0x80000000;
    a0 = (s32)s0 < (s32)-199;
    v0 = v0 | 0x1FF;
    if (a1 != 0)
        s0 = v1;
    if (a0 != 0)
        s0 = v0;
    goto loc_00005990;

loc_000059E4:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    a2 = 0; // Data ref 0x16641AF7
    v0, v1 = sceKernelWaitSema(...);
    s0 = -108;
    if (v0 != 0)
        goto loc_0000597C;
    v0 = 0x10000; // Data ref 0x00008E08 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29176); // Data ref 0x00008E08 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007348 ... 0x2FD312F0 0x0369FBC5 0xBACF7A20 0xB8F8902C
    a1 = 0x00000; // Data ref 0x0000734C ... 0x0369FBC5 0xBACF7A20 0xB8F8902C 0xDEF1D24D
    v0 = 0x00000; // Data ref 0x0000735C ... 0x7BDD1EA9 0xB522BB09 0x6930A39D 0xD80E6E13
    v0 = v0 + 29532; // Data ref 0x0000735C ... 0x7BDD1EA9 0xB522BB09 0x6930A39D 0xD80E6E13
    a0 = a0 + 29512; // Data ref 0x00007348 ... 0x2FD312F0 0x0369FBC5 0xBACF7A20 0xB8F8902C
    a1 = a1 + 29516; // Data ref 0x0000734C ... 0x0369FBC5 0xBACF7A20 0xB8F8902C 0xDEF1D24D
    s7 = 5;
    a2 = 71;
    a3 = s1;
    t0 = s2;
    t1 = s3;
    t2 = 0;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s7;
    *(s32*)(sp + 12) = s4;
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    v0 = -301;
    cond = (s0 == v0);
    v0 = 0x10000; // Data ref 0x00008E04 ... 0x00000000 0x00000000 0x00000000 0x00000000
    if (cond)
        goto loc_00005A70;

loc_00005A58:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    goto loc_0000597C;

loc_00005A70:
    t3 = *(s32*)(v0 - 29180); // Data ref 0x00008E04 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x00007320 ... 0x2FD311F0 0x96486B3A 0x80C8A586 0x4BE66C69
    a1 = 0x00000; // Data ref 0x00007324 ... 0x96486B3A 0x80C8A586 0x4BE66C69 0x441704F6
    v0 = 0x00000; // Data ref 0x00007334 ... 0x7BDD1EA9 0xB522BB09 0x6930A39D 0xD80E6E13
    v0 = v0 + 29492; // Data ref 0x00007334 ... 0x7BDD1EA9 0xB522BB09 0x6930A39D 0xD80E6E13
    a0 = a0 + 29472; // Data ref 0x00007320 ... 0x2FD311F0 0x96486B3A 0x80C8A586 0x4BE66C69
    a1 = a1 + 29476; // Data ref 0x00007324 ... 0x96486B3A 0x80C8A586 0x4BE66C69 0x441704F6
    a3 = s1;
    t0 = s2;
    t1 = s3;
    a2 = 71;
    t2 = 0;
    *(s32*)(sp + 4) = s7;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s4;
    *(s32*)(sp + 0) = 0; // Data ref 0x165F0038
    v0, v1 = sub_000000E0(...);
    s0 = v0;
    goto loc_00005A58;
}

scePauth_driver_98B83B5D(...) // at 0x00005ABC - Aliases: scePauth_98B83B5D
{
    v1 = a0 + a1;
    sp = sp - 48;
    v1 = v1 | a0;
    *(s32*)(sp + 24) = s2;
    v1 = v1 | a1;
    s2 = a0;
    v0 = a2 + 4;
    a0 = k1 << 11;
    v0 = v0 | a2;
    v1 = a0 & v1;
    *(s32*)(sp + 36) = s5;
    v0 = v0 & a0;
    s5 = k1;
    *(s32*)(sp + 32) = s4;
    k1 = a0;
    s4 = a3;
    *(s32*)(sp + 28) = s3;
    s3 = a1;
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    *(s32*)(sp + 16) = s0;
    s0 = -110;
    *(s32*)(sp + 44) = ra;
    *(s32*)(sp + 40) = s6;
    if ((s32)v1 < 0)
        goto loc_00005B40;
    v1 = a3 + 16;
    if ((s32)v0 < 0)
        goto loc_00005B40;
    v0 = v1 | a3;
    v0 = v0 & a0;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    a2 = 0;
    if ((s32)v0 >= 0)
        goto loc_00005BA0;

loc_00005B40:
    v0 = 0x80000000;
    a1 = (s32)s0 < (s32)0;
    a2 = 0x80000000;
    v0 = v0 | 0x21;
    v1 = (s32)s0 < (s32)-199;
    a0 = (s32)s0 < (s32)-299;
    a2 = a2 | 0x1FF;
    if (a1 != 0)
        s0 = v0;
    k1 = s5;
    cond = (a0 == 0);
    if (v1 != 0)
        s0 = a2;
    if (cond)
        goto loc_00005B74;
    s0 = 0x80000000;
    s0 = s0 | 0x108;

loc_00005B74:
    v0 = s0;
    ra = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 48;
    return (v1 << 32) | v0;

loc_00005BA0:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = -108; // Data ref 0x16961AF7
    v0, v1 = sceKernelWaitSema(...);
    a0 = 0x00000; // Data ref 0x00007370 ... 0x2FD313F0 0x16C824B0 0x1CF0E843 0x7367308C
    a1 = 0x00000; // Data ref 0x00007374 ... 0x16C824B0 0x1CF0E843 0x7367308C 0xEF35963E
    a0 = a0 + 29552; // Data ref 0x00007370 ... 0x2FD313F0 0x16C824B0 0x1CF0E843 0x7367308C
    a1 = a1 + 29556; // Data ref 0x00007374 ... 0x16C824B0 0x1CF0E843 0x7367308C 0xEF35963E
    a3 = s2;
    t0 = s3;
    t1 = s1;
    a2 = 71;
    t2 = 0;
    if (v0 != 0)
        goto loc_00005B40;
    v0 = 0x10000; // Data ref 0x00008E0C ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29172); // Data ref 0x00008E0C ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x00000; // Data ref 0x00007384 ... 0x7BDD1EA9 0xB522BB09 0x6930A39D 0xD80E6E13
    v1 = 5;
    v0 = v0 + 29572; // Data ref 0x00007384 ... 0x7BDD1EA9 0xB522BB09 0x6930A39D 0xD80E6E13
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s4;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    a1 = 1;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = v0;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s0 = v1;
    goto loc_00005B40;
}

sceMesgLed_driver_31D6D8AA(...) // at 0x00005C18
{
    sp = sp - 64;
    *(s32*)(sp + 40) = s6;
    s6 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 24) = s2;
    s2 = a0;
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 20) = s1;
    s1 = a2;
    a2 = 0;
    *(s32*)(sp + 16) = s0;
    s0 = a1;
    a1 = 1;
    *(s32*)(sp + 32) = s4;
    s4 = -108;
    *(s32*)(sp + 28) = s3;
    s3 = a3;
    *(s32*)(sp + 48) = ra;
    *(s32*)(sp + 44) = s7;
    *(s32*)(sp + 36) = s5; // Data ref 0x16D01AF7
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s4;
    if (cond)
        goto loc_00005D04;
    v0 = 0x10000; // Data ref 0x00008DF8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29192); // Data ref 0x00008DF8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 10;
    *(s32*)(sp + 4) = v0;
    a0 = 0x10000; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a1 = 0x10000; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    v0 = 0x00000; // Data ref 0x0000727C ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    v0 = v0 + 29308; // Data ref 0x0000727C ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    a0 = a0 - 29276; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a1 = a1 - 29272; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a2 = 72;
    a3 = s2;
    t0 = s0;
    t1 = s1;
    t2 = 0;
    *(s32*)(sp + 8) = v0;
    s5 = -301;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 12) = s3;
    v0, v1 = sub_000000E0(...);
    s4 = v0;
    if (v0 == s5)
        goto loc_00005F88;

loc_00005CC8:
    t0 = 0x10000; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = *(s32*)(t0 - 29188); // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 1;
    cond = (v1 == v0);
    v0 = 3;
    if (cond)
        goto loc_00005EC0;
    cond = (v1 == v0);
    v0 = 4;
    if (cond)
        goto loc_00005DF8;
    cond = (v1 == v0);
    v0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    if (cond)
        goto loc_00005D30;

loc_00005CEC:
    a0 = *(s32*)(s6 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s4 = v1;
    v0 = s4;

loc_00005D04:
    ra = *(s32*)(sp + 48);
    s7 = *(s32*)(sp + 44);
    s6 = *(s32*)(sp + 40);
    s5 = *(s32*)(sp + 36);
    s4 = *(s32*)(sp + 32);
    s3 = *(s32*)(sp + 28);
    s2 = *(s32*)(sp + 24);
    s1 = *(s32*)(sp + 20);
    s0 = *(s32*)(sp + 16);
    sp = sp + 64;
    return (v1 << 32) | v0;

loc_00005D30:
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005D40:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005D40;
    v0 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    v1 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a2 = v1 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005D70:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005D70;
    v0 = 0x10000; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    v1 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29276; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a2 = v1 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005DA0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005DA0;
    v0 = 0x10000; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    v1 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29272; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a2 = v1 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005DD0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005DD0;
    v0 = 2;

loc_00005DF0:
    *(s32*)(t0 - 29188) = v0; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    goto loc_00005CEC;

loc_00005DF8:
    v0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005E0C:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005E0C;
    v0 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    v1 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a2 = v1 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005E3C:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005E3C;
    a0 = 0x10000; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    v0 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = a0 - 29296; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a2 = v0 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005E6C:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005E6C;
    a1 = 0x10000; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    v0 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = a1 - 29292; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a3 = v0 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = 0;

loc_00005E9C:
    v0 = a2 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a2 = a2 + 1;
    v0 = (u32)a2 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005E9C;
    v0 = 2;
    goto loc_00005DF0;

loc_00005EC0:
    v0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005ED4:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005ED4;
    v0 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    v1 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a2 = v1 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005F04:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005F04;
    a0 = 0x10000; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    v0 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = a0 - 29316; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a2 = v0 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00005F34:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005F34;
    a1 = 0x10000; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    v0 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = a1 - 29312; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    a3 = v0 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = 0;

loc_00005F64:
    v0 = a2 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a2 = a2 + 1;
    v0 = (u32)a2 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00005F64;
    v0 = 2;
    goto loc_00005DF0;

loc_00005F88:
    v0 = 0x10000; // Data ref 0x00008DF4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29196); // Data ref 0x00008DF4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a1 = 0x10000; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    v0 = 0x00000; // Data ref 0x00007268 ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    v1 = 7;
    v0 = v0 + 29288; // Data ref 0x00007268 ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    a0 = a0 - 29296; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a1 = a1 - 29292; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a2 = 72;
    a3 = s2;
    t0 = s0;
    t1 = s1;
    t2 = 0;
    *(s32*)(sp + 4) = v1;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 12) = s3; // Data ref 0x177C0038
    v0, v1 = sub_000000E0(...);
    s4 = v0;
    if (v0 != s5)
        goto loc_00005CC8;
    v0 = 0x10000; // Data ref 0x00008DF0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29200); // Data ref 0x00008DF0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x10000; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a1 = 0x10000; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    v0 = 0x00000; // Data ref 0x00007254 ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    v0 = v0 + 29268; // Data ref 0x00007254 ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    a0 = a0 - 29316; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a1 = a1 - 29312; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    s7 = 5;
    a2 = 72;
    a3 = s2;
    t0 = s0;
    t1 = s1;
    t2 = 0;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 0) = 0;
    *(s32*)(sp + 4) = s7;
    *(s32*)(sp + 12) = s3;
    v0, v1 = sub_000000E0(...);
    s4 = v0;
    if (v0 != s5)
        goto loc_00005CC8;
    v0 = 0x10000; // Data ref 0x00008DEC ... 0x00000000 0x00000000 0x00000000 0x00000000
    t3 = *(s32*)(v0 - 29204); // Data ref 0x00008DEC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0x00000; // Data ref 0x0000722C ... 0x2E5E10F0 0xAF5B5C9D 0x7E69D88C 0x96709F51
    a1 = 0x00000; // Data ref 0x00007230 ... 0xAF5B5C9D 0x7E69D88C 0x96709F51 0xE8C4D5E6
    v0 = 0x00000; // Data ref 0x00007240 ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    v0 = v0 + 29248; // Data ref 0x00007240 ... 0x3455BA69 0x71D6C0F0 0x97DB1FE3 0x2AD27CE0
    a0 = a0 + 29228; // Data ref 0x0000722C ... 0x2E5E10F0 0xAF5B5C9D 0x7E69D88C 0x96709F51
    a1 = a1 + 29232; // Data ref 0x00007230 ... 0xAF5B5C9D 0x7E69D88C 0x96709F51 0xE8C4D5E6
    a3 = s2;
    t0 = s0;
    t1 = s1;
    a2 = 72;
    t2 = 0;
    *(s32*)(sp + 4) = s7;
    *(s32*)(sp + 8) = v0;
    *(s32*)(sp + 12) = s3;
    *(s32*)(sp + 0) = 0;
    v0, v1 = sub_000000E0(...);
    s4 = v0;
    goto loc_00005CC8;
}

sceDbsvr_driver_94561901(...) // at 0x00006080
{
    sp = sp - 32;
    *(s32*)(sp + 12) = s3;
    s3 = 0x10000; // Data ref 0x000093F4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    *(s32*)(sp + 8) = s2;
    s2 = a0;
    a2 = 0;
    a0 = *(s32*)(s3 - 27660); // Data ref 0x000093F4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(sp + 4) = s1;
    s1 = -108;
    *(s32*)(sp + 16) = ra;
    *(s32*)(sp + 0) = s0; // Data ref 0x17321AF7
    v0, v1 = sceKernelWaitSema(...);
    cond = (v0 != 0);
    v0 = s1;
    if (cond)
        goto loc_00006118;
    s0 = 0x10000; // Data ref 0x00009340 ... 0x00000000 0x00000000 0x00000000 0x00000000
    s0 = s0 - 27840; // Data ref 0x00009340 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 20;
    a0 = s0;
    a2 = 0;
    a3 = 0;
    t0 = 14;
    v0, v1 = sceUtilsBufferCopyWithRange(...);
    a1 = 0;
    if (v0 == 0)
        goto loc_00006134;
    s1 = -101;
    if ((s32)v0 < 0)
        goto loc_00006100;
    a0 = v0 ^ 0xC;
    v0 = -107;
    v1 = -104;
    s1 = v0;
    if (a0 != 0)
        s1 = v1;

loc_00006100:
    a0 = *(s32*)(s3 - 27660); // Data ref 0x000093F4 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s1 = v1;
    v0 = s1;

loc_00006118:
    ra = *(s32*)(sp + 16);
    s3 = *(s32*)(sp + 12);
    s2 = *(s32*)(sp + 8);
    s1 = *(s32*)(sp + 4);
    s0 = *(s32*)(sp + 0);
    sp = sp + 32;
    return (v1 << 32) | v0;

loc_00006134:
    v0 = a1 + s0;
    a0 = *(u8*)(v0 + 0);
    v1 = s2 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)20;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006134;
    s1 = 0;
    goto loc_00006100;
}

sub_00006158(...) // at 0x00006158
{
    t0 = 0;
    if (a2 == 0)
        goto loc_00006188;
    v1 = *(u8*)(a0 + 0);

loc_00006164:
    v0 = *(u8*)(a1 + 0);
    t0 = t0 + 1;
    a3 = (u32)t0 < (u32)a2;
    a0 = a0 + 1;
    a1 = a1 + 1;
    t1 = v1 - v0;
    if (v1 != v0)
        goto loc_0000618C;
    if (a3 != 0)
    {
        v1 = *(u8*)(a0 + 0);
        goto loc_00006164;
    }

loc_00006188:
    t1 = 0;

loc_0000618C:
    v0 = t1;
    return (v1 << 32) | v0;
}

sceMesgLed_driver_9E3C79D9(...) // at 0x00006194
{
    sp = sp - 16;
    *(s32*)(sp + 8) = s2;
    t0 = -201;
    s2 = a2;
    *(s32*)(sp + 0) = s0;
    s0 = a0;
    *(s32*)(sp + 12) = ra;
    *(s32*)(sp + 4) = s1;
    if (a0 == 0)
        goto loc_00006274;
    v0 = (u32)a1 < (u32)352;
    t0 = -202;
    if (v0 != 0)
        goto loc_00006274;
    v0 = a0 & 0x3F;
    t0 = -203;
    if (v0 != 0)
        goto loc_00006274;
    v0 = 0x220000;
    v1 = (a0 >> 27) & 0x0000001F;
    v0 = v0 | 0x202;
    v0 = (s32)v0 >> v1;
    v0 = v0 & 0x1;
    t0 = -204;
    if (v0 == 0)
        goto loc_00006274;
    s1 = a0 + 208;
    a0 = 0x10000; // Data ref 0x00008808 ... 0xADF305F0 0x5E709912 0xD06C0724 0x7EFE062D
    a0 = a0 - 30712; // Data ref 0x00008808 ... 0xADF305F0 0x5E709912 0xD06C0724 0x7EFE062D
    a1 = s1;
    a2 = 4; // Data ref 0x18401856
    v0, v1 = sub_00006158(...);
    cond = (v0 == 0);
    v0 = 0x10000; // Data ref 0x00008EE0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    if (cond)
        goto loc_00006230;
    a0 = 0x10000; // Data ref 0x00008998 ... 0x279D05F0 0xB53BDC23 0xEAD682A9 0x2B6EA363
    a0 = a0 - 30312; // Data ref 0x00008998 ... 0x279D05F0 0xB53BDC23 0xEAD682A9 0x2B6EA363
    a1 = s1;
    a2 = 4;
    v0, v1 = sub_00006158(...);
    t0 = 0x80000000;
    t0 = t0 | 0x4;
    if (v0 != 0)
        goto loc_00006274;
    v0 = 0x10000; // Data ref 0x00008EE0 ... 0x00000000 0x00000000 0x00000000 0x00000000

loc_00006230:
    v0 = v0 - 28960; // Data ref 0x00008EE0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = s0 + 228;
    t0 = 0;
    a1 = 0;
    a3 = *(s8*)(v0 + 0);

loc_00006244:
    a2 = *(s8*)(v1 + 0);
    a1 = a1 + 1;
    a0 = (s32)a1 < (s32)8;
    v0 = v0 + 1;
    v1 = v1 + 1;
    if (a3 != a2)
        goto loc_000062B4;
    if (a0 != 0)
    {
        a3 = *(s8*)(v0 + 0);
        goto loc_00006244;
    }

loc_00006264:
    a1 = 0;
    if (t0 != 0)
        goto loc_00006290;
    t0 = 0x80000000;
    t0 = t0 | 0x4;

loc_00006274:
    ra = *(s32*)(sp + 12);
    s2 = *(s32*)(sp + 8);
    s1 = *(s32*)(sp + 4);
    s0 = *(s32*)(sp + 0);
    v0 = t0;
    sp = sp + 16;
    return (v1 << 32) | v0;

loc_00006290:
    v0 = s0 + a1;
    a0 = *(u8*)(v0 + 228);
    v1 = s2 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)8;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006290;
    t0 = 0;
    goto loc_00006274;

loc_000062B4:
    t0 = a3 - a2;
    goto loc_00006264;
}

sceResmgr_driver_8E6C62C8(...) // at 0x000062BC - Aliases: sceResmgr_8E6C62C8
{
    v0 = a0 + 120;
    v1 = k1 << 11;
    v0 = v0 | a0;
    sp = sp - 32;
    v0 = v0 & v1;
    *(s32*)(sp + 12) = s3;
    s3 = k1;
    k1 = v1;
    *(s32*)(sp + 4) = s1;
    s1 = -110;
    *(s32*)(sp + 0) = s0;
    s0 = a0;
    *(s32*)(sp + 28) = ra;
    *(s32*)(sp + 24) = s6;
    *(s32*)(sp + 20) = s5;
    *(s32*)(sp + 16) = s4;
    *(s32*)(sp + 8) = s2;
    if ((s32)v0 < 0)
        goto loc_00006484;
    s4 = 0x10000; // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    a2 = 0; // Data ref 0x18991AF7
    v0, v1 = sceKernelWaitSema(...);
    s1 = -108;
    if (v0 != 0)
        goto loc_00006484;
    s6 = 0x10000; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = *(s32*)(s6 - 29188); // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    s1 = -102;
    if (v0 != 0)
        goto loc_00006410;
    v0 = 0x10000; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_0000633C:
    v0 = s0 + a1;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)120;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000633C;
    s2 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = s2 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    t0 = 0x10000; // Data ref 0x00008DC0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(t0 - 29248) = v1; // Data ref 0x00008DC0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 + 4;
    a2 = v0 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006378:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)64;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006378;
    a0 = *(s32*)(t0 - 29248); // Data ref 0x00008DC0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 80;
    v1 = 0x10000; // Data ref 0x00009144 ... 0x00000000 0x00000000 0x00000000 0x00000000
    *(s32*)(a0 + 0) = v0;
    v0 = 0x00000; // Data ref 0x00007290 ... 0xEB6984AC 0xE3EE5119 0xF8935DFD 0x564797AB
    a3 = v1 - 28348; // Data ref 0x00009144 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 + 29328; // Data ref 0x00007290 ... 0xEB6984AC 0xE3EE5119 0xF8935DFD 0x564797AB
    a1 = 0;

loc_000063B4:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000063B4;
    s0 = s2 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = s0;
    a1 = 84;
    a2 = s0;
    a3 = 84;
    t0 = 11;
    v0, v1 = sceUtilsBufferCopyWithRange(...);
    s5 = 0x10000; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000
    if (v0 == 0)
        goto loc_000064B4;
    s1 = -102;
    if ((s32)v0 < 0)
        goto loc_00006410;
    a0 = v0 ^ 0xC;
    v1 = -105;

loc_00006404:
    v0 = -107;
    s1 = v0;
    if (a0 != 0)
        s1 = v1;

loc_00006410:
    v0 = 0x10000; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = v0 - 28032; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0;

loc_0000641C:
    v1 = a0 + a1;
    a0 = a0 + 1;
    v0 = (u32)a0 < (u32)16;
    *(s8*)(v1 + 0) = 0;
    if (v0 != 0)
        goto loc_0000641C;
    v0 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = v0 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0;

loc_0000643C:
    v1 = a0 + a1;
    a0 = a0 + 1;
    v0 = (u32)a0 < (u32)336;
    *(s8*)(v1 + 0) = 0;
    if (v0 != 0)
        goto loc_0000643C;
    v0 = 0x10000; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = v0 - 28952; // Data ref 0x00008EE8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0;

loc_0000645C:
    v1 = a0 + a1;
    a0 = a0 + 1;
    v0 = (u32)a0 < (u32)336;
    *(s8*)(v1 + 0) = 0;
    if (v0 != 0)
        goto loc_0000645C;
    a0 = *(s32*)(s4 - 27664); // Data ref 0x000093F0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 1;
    v0, v1 = sceKernelSignalSema(...);
    v1 = -109;
    if (v0 != 0)
        s1 = v1;

loc_00006484:
    v0 = s1;
    k1 = s3;
    ra = *(s32*)(sp + 28);
    s6 = *(s32*)(sp + 24);
    s5 = *(s32*)(sp + 20);
    s4 = *(s32*)(sp + 16);
    s3 = *(s32*)(sp + 12);
    s2 = *(s32*)(sp + 8);
    s1 = *(s32*)(sp + 4);
    s0 = *(s32*)(sp + 0);
    sp = sp + 32;
    return (v1 << 32) | v0;

loc_000064B4:
    a2 = s0;
    a1 = 0;
    a3 = s5 - 28032; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000

loc_000064C0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)20;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000064C0;
    v0 = 0x10000; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008F28 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 28396; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v1 - 28888; // Data ref 0x00008F28 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000064F0:
    v0 = a1 + a2;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a3;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000064F0;
    s0 = s2 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = s0;
    a2 = 86;
    a1 = 16;
    a3 = 0;
    v0, v1 = sub_00000000(...);
    s1 = v0;
    a2 = s5 - 28032; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0;
    if (v0 == 0)
        goto loc_00006548;

loc_00006534:
    a0 = s1 ^ 0xC;
    v1 = -106;
    if ((s32)s1 >= 0)
        goto loc_00006404;
    s1 = -103;
    goto loc_00006410;

loc_00006548:
    a1 = *(s8*)(a2 + 0);

loc_0000654C:
    v1 = *(s8*)(s0 + 0);
    a0 = a0 + 1;
    v0 = (s32)a0 < (s32)16;
    a2 = a2 + 1;
    s0 = s0 + 1;
    if (a1 != v1)
        goto loc_00006BC4;
    if (v0 != 0)
    {
        a1 = *(s8*)(a2 + 0);
        goto loc_0000654C;
    }

loc_0000656C:
    v0 = 0x00000; // Data ref 0x000072B0 ... 0x81672C2E 0x80B60DB2 0x04EB997D 0xB037C35F
    if (s1 != 0)
        goto loc_00006BBC;
    a3 = v0 + 29360; // Data ref 0x000072B0 ... 0x81672C2E 0x80B60DB2 0x04EB997D 0xB037C35F
    a2 = s2 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006580:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)40;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006580;
    v0 = 0x10000; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 28032; // Data ref 0x00009280 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = a2 + 40;
    a1 = 0;

loc_000065AC:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)20;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000065AC;
    v0 = 0x10000; // Data ref 0x00008F38 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 28872; // Data ref 0x00008F38 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = a2 + 20;
    a1 = 0;

loc_000065D8:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a2 + a1;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)40;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000065D8;
    s0 = s2 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a0 = 0;
    a1 = 0;
    a2 = s0;
    a3 = 100;
    t0 = 17; // Data ref 0x19041AFD
    v0, v1 = sceUtilsBufferCopyWithRange(...);
    s1 = -303;
    if (v0 != 0)
        goto loc_00006410;
    v0 = 0x10000; // Data ref 0x00008EF8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = s0 + 20;
    a3 = v0 - 28936; // Data ref 0x00008EF8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006628:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)48;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006628;
    v0 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x00000; // Data ref 0x000072A0 ... 0x393A7700 0x6D47FC2A 0x7E433D16 0x50C3EFEF
    s0 = v0 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 + 29344; // Data ref 0x000072A0 ... 0x393A7700 0x6D47FC2A 0x7E433D16 0x50C3EFEF
    a2 = 0;

loc_00006658:
    v0 = a2 & 0xF;
    a0 = a2 + s0;
    v0 = v0 + a3;
    a1 = *(u8*)(v0 + 0);
    v1 = *(u8*)(a0 + 20);
    a2 = a2 + 1;
    v0 = (s32)a2 < (s32)48;
    asm("xor        $v1, $v1, $a1");
    *(s8*)(a0 + 20) = v1;
    if (v0 != 0)
        goto loc_00006658;
    a0 = s0;
    a1 = 48;
    a2 = 86;
    a3 = 0;
    v0, v1 = sub_00000000(...);
    s1 = v0;
    if (v0 != 0)
        goto loc_00006534;
    v0 = 0x00000; // Data ref 0x000072A0 ... 0x393A7700 0x6D47FC2A 0x7E433D16 0x50C3EFEF
    t0 = s0;
    a3 = v0 + 29344; // Data ref 0x000072A0 ... 0x393A7700 0x6D47FC2A 0x7E433D16 0x50C3EFEF
    a2 = 0;

loc_000066AC:
    v0 = a2 & 0xF;
    a0 = a2 + t0;
    v0 = v0 + a3;
    a1 = *(u8*)(v0 + 0);
    v1 = *(u8*)(a0 + 0);
    a2 = a2 + 1;
    v0 = (s32)a2 < (s32)48;
    asm("xor        $v1, $v1, $a1");
    *(s8*)(a0 + 0) = v1;
    if (v0 != 0)
        goto loc_000066AC;
    v1 = *(u8*)(s2 - 28416); // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 240;
    if (v1 == v0)
    {
        v0 = *(u8*)(t0 + 1);
        goto loc_00006A20;
    }
    v1 = *(u8*)(s2 - 28416); // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000

loc_000066E8:
    v0 = 240;
    s0 = s2 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    if (v1 == v0)
        goto loc_00006884;
    v0 = 1;

loc_000066F8:
    *(s32*)(s6 - 29188) = v0; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a1 = 0;

loc_00006710:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006710;
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a2 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a1 = 0;

loc_00006740:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006740;
    v0 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a2 = v0 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29316; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a1 = 0;

loc_00006770:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006770;
    v0 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    a2 = v0 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29312; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    a1 = 0;

loc_000067A0:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000067A0;
    v0 = 0x10000; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    v1 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29376; // Data ref 0x00008D40 ... 0xD91611F0 0x58C0B061 0xFAD95771 0x5C0E6774
    a3 = v1 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000067D0:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000067D0;
    v0 = 0x10000; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    v1 = 0x10000; // Data ref 0x00009104 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29372; // Data ref 0x00008D44 ... 0x58C0B061 0xFAD95771 0x5C0E6774 0xB9956E7E
    a3 = v1 - 28412; // Data ref 0x00009104 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006800:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006800;
    v0 = 0x10000; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    v1 = 0x10000; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29316; // Data ref 0x00008D7C ... 0x2E5E11F0 0x43E8EB75 0xD08F87F4 0x397E7F14
    a3 = v1 - 28396; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006830:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006830;
    v0 = 0x10000; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    v1 = 0x10000; // Data ref 0x00009118 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29312; // Data ref 0x00008D80 ... 0x43E8EB75 0xD08F87F4 0x397E7F14 0x9D04AFAD
    a3 = v1 - 28392; // Data ref 0x00009118 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006860:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006860;
    s1 = 0;
    goto loc_00006410;

loc_00006884:
    v0 = *(s8*)(s0 + 1);
    cond = ((s32)v0 >= 0);
    v0 = 1;
    if (cond)
        goto loc_000066F8;
    v0 = 3;
    *(s32*)(s6 - 29188) = v0; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a1 = 0;

loc_000068AC:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000068AC;
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a2 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a1 = 0;

loc_000068DC:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000068DC;
    v0 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a2 = v0 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29296; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a1 = 0;

loc_0000690C:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000690C;
    v0 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a2 = v0 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29292; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a1 = 0;

loc_0000693C:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000693C;
    v0 = 0x10000; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    v1 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29356; // Data ref 0x00008D54 ... 0xD91680F0 0x129B222C 0x67117436 0x88D1D149
    a3 = v1 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_0000696C:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000696C;
    v0 = 0x10000; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    v1 = 0x10000; // Data ref 0x00009104 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29352; // Data ref 0x00008D58 ... 0x129B222C 0x67117436 0x88D1D149 0xD8A1F692
    a3 = v1 - 28412; // Data ref 0x00009104 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_0000699C:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_0000699C;
    v0 = 0x10000; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    v1 = 0x10000; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29296; // Data ref 0x00008D90 ... 0x2E5E80F0 0x43AF740F 0x39DACD75 0x61D95681
    a3 = v1 - 28396; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000069CC:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000069CC;
    v0 = 0x10000; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    v1 = 0x10000; // Data ref 0x00009118 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29292; // Data ref 0x00008D94 ... 0x43AF740F 0x39DACD75 0x61D95681 0x92C8163E
    a3 = v1 - 28392; // Data ref 0x00009118 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_000069FC:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_000069FC;
    s1 = 0;
    goto loc_00006410;

loc_00006A20:
    v0 = (u32)v0 < (u32)144;
    v1 = *(u8*)(s2 - 28416); // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    if (v0 != 0)
        goto loc_000066E8;
    v0 = 4;
    *(s32*)(s6 - 29188) = v0; // Data ref 0x00008DFC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a2 = v1 - 27704; // Data ref 0x000093C8 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a1 = 0;

loc_00006A48:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006A48;
    v0 = 0x10000; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a2 = v0 - 27700; // Data ref 0x000093CC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a1 = 0;

loc_00006A78:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006A78;
    v0 = 0x10000; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a2 = v0 - 27684; // Data ref 0x000093DC ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29276; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a1 = 0;

loc_00006AA8:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006AA8;
    v0 = 0x10000; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    v1 = 0x10000; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a2 = v0 - 27680; // Data ref 0x000093E0 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a3 = v1 - 29272; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a1 = 0;

loc_00006AD8:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006AD8;
    v0 = 0x10000; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    v1 = 0x10000; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29336; // Data ref 0x00008D68 ... 0xD91690F0 0x57E26142 0xB5424994 0x080D6DAA
    a3 = v1 - 28416; // Data ref 0x00009100 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006B08:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006B08;
    v0 = 0x10000; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    v1 = 0x10000; // Data ref 0x00009104 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29332; // Data ref 0x00008D6C ... 0x57E26142 0xB5424994 0x080D6DAA 0x4BF7243D
    a3 = v1 - 28412; // Data ref 0x00009104 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006B38:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006B38;
    v0 = 0x10000; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    v1 = 0x10000; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29276; // Data ref 0x00008DA4 ... 0x2E5E90F0 0x4C8FE467 0xB17DA008 0x72A7515F
    a3 = v1 - 28396; // Data ref 0x00009114 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006B68:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)4;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006B68;
    v0 = 0x10000; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    v1 = 0x10000; // Data ref 0x00009118 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a2 = v0 - 29272; // Data ref 0x00008DA8 ... 0x4C8FE467 0xB17DA008 0x72A7515F 0x7E2DA898
    a3 = v1 - 28392; // Data ref 0x00009118 ... 0x00000000 0x00000000 0x00000000 0x00000000
    a1 = 0;

loc_00006B98:
    v0 = a1 + a3;
    a0 = *(u8*)(v0 + 0);
    v1 = a1 + a2;
    a1 = a1 + 1;
    v0 = (u32)a1 < (u32)16;
    *(s8*)(v1 + 0) = a0;
    if (v0 != 0)
        goto loc_00006B98;
    s1 = 0;
    goto loc_00006410;

loc_00006BBC:
    s1 = -302;
    goto loc_00006410;

loc_00006BC4:
    s1 = a1 - v1;
    goto loc_0000656C;
}
#endif
