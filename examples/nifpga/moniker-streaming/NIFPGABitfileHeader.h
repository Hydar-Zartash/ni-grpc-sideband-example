/*
 * Generated with the FPGA Interface C API Generator 24.3
 * for NI-RIO 24.3 or later.
 */
#ifndef __NiFpga_TopLevel_h__
#define __NiFpga_TopLevel_h__

#ifndef NiFpga_Version
   #define NiFpga_Version 243
#endif

#include "NiFpga.h"

/**
 * The filename of the FPGA bitfile.
 *
 * This is a #define to allow for string literal concatenation. For example:
 *
 *    static const char* const Bitfile = "C:\\" NiFpga_TopLevel_Bitfile;
 */
#define NiFpga_TopLevel_Bitfile "NiFpga_TopLevel.lvbitx"

/**
 * The signature of the FPGA bitfile.
 */
static const char* const NiFpga_TopLevel_Signature = "B87E020B835175EAFCABCC9638852794";

#if NiFpga_Cpp
extern "C"
{
#endif

typedef enum
{
   NiFpga_TopLevel_IndicatorBool_sidebandce_out = 0x18002
} NiFpga_TopLevel_IndicatorBool;

typedef enum
{
   NiFpga_TopLevel_IndicatorU32_sidebandArrayOut1 = 0x18004,
   NiFpga_TopLevel_IndicatorU32_sidebandArrayOut2 = 0x18008
} NiFpga_TopLevel_IndicatorU32;

typedef enum
{
   NiFpga_TopLevel_ControlBool_sidebandclk_enable = 0x1800E
} NiFpga_TopLevel_ControlBool;

typedef enum
{
   NiFpga_TopLevel_ControlU32_sidebandArrayIn1 = 0x18010,
   NiFpga_TopLevel_ControlU32_sidebandArrayIn2 = 0x18014
} NiFpga_TopLevel_ControlU32;


#if NiFpga_Cpp
}
#endif

#endif
