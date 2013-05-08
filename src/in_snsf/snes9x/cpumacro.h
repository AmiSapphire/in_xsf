/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2010  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),

  (c) Copyright 2002 - 2011  zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja

  (c) Copyright 2009 - 2011  BearOso,
                             OV2


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com),
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti

  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code used in 1.39-1.51
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  SPC7110 and RTC C++ emulator code used in 1.52+
  (c) Copyright 2009         byuu,
                             neviksti

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 - 2006  byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2011  BearOso

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja
  (c) Copyright 2009 - 2011  OV2

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2011  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/


#ifndef _CPUMACRO_H_
#define _CPUMACRO_H_

#define rOP8(OP, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	uint8_t	val = OpenBus = S9xGetByte(ADDR(READ)); \
	FUNC(val); \
}

#define rOP16(OP, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	uint16_t	val = S9xGetWord(ADDR(READ), WRAP); \
	OpenBus = (uint8_t) (val >> 8); \
	FUNC(val); \
}

#define rOPC(OP, COND, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	if (Check##COND()) \
	{ \
		uint8_t	val = OpenBus = S9xGetByte(ADDR(READ)); \
		FUNC(val); \
	} \
	else \
	{ \
		uint16_t	val = S9xGetWord(ADDR(READ), WRAP); \
		OpenBus = (uint8_t) (val >> 8); \
		FUNC(val); \
	} \
}

#define rOPM(OP, ADDR, WRAP, FUNC) \
rOPC(OP, Memory, ADDR, WRAP, FUNC)

#define rOPX(OP, ADDR, WRAP, FUNC) \
rOPC(OP, Index, ADDR, WRAP, FUNC)

#define wOP8(OP, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	FUNC##8(ADDR(WRITE)); \
}

#define wOP16(OP, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	FUNC##16(ADDR(WRITE), WRAP); \
}

#define wOPC(OP, COND, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	if (Check##COND()) \
		FUNC##8(ADDR(WRITE)); \
	else \
		FUNC##16(ADDR(WRITE), WRAP); \
}

#define wOPM(OP, ADDR, WRAP, FUNC) \
wOPC(OP, Memory, ADDR, WRAP, FUNC)

#define wOPX(OP, ADDR, WRAP, FUNC) \
wOPC(OP, Index, ADDR, WRAP, FUNC)

#define mOP8(OP, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	FUNC##8(ADDR(MODIFY)); \
}

#define mOP16(OP, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	FUNC##16(ADDR(MODIFY), WRAP); \
}

#define mOPC(OP, COND, ADDR, WRAP, FUNC) \
static void Op##OP() \
{ \
	if (Check##COND()) \
		FUNC##8(ADDR(MODIFY)); \
	else \
		FUNC##16(ADDR(MODIFY), WRAP); \
}

#define mOPM(OP, ADDR, WRAP, FUNC) \
mOPC(OP, Memory, ADDR, WRAP, FUNC)

#define bOP(OP, REL, COND, CHK, E) \
static void Op##OP() \
{ \
	pair	newPC; \
	newPC.W = REL(JUMP); \
	if (COND) \
	{ \
		AddCycles(ONE_CYCLE); \
		if (E && Registers.PCh != newPC.B.h) \
			AddCycles(ONE_CYCLE); \
		if ((Registers.PCw & ~MEMMAP_MASK) != (newPC.W & ~MEMMAP_MASK)) \
			S9xSetPCBase(ICPU.ShiftedPB + newPC.W); \
		else \
			Registers.PCw = newPC.W; \
	} \
}


static inline void SetZN (uint16_t Work16)
{
	ICPU._Zero = Work16 != 0;
	ICPU._Negative = (uint8_t) (Work16 >> 8);
}

static inline void SetZN (uint8_t Work8)
{
	ICPU._Zero = Work8;
	ICPU._Negative = Work8;
}

static inline void ADC (uint16_t Work16)
{
	if (CheckDecimal())
	{
		uint16_t	A1 = Registers.A.W & 0x000F;
		uint16_t	A2 = Registers.A.W & 0x00F0;
		uint16_t	A3 = Registers.A.W & 0x0F00;
		uint32_t	A4 = Registers.A.W & 0xF000;
		uint16_t	W1 = Work16 & 0x000F;
		uint16_t	W2 = Work16 & 0x00F0;
		uint16_t	W3 = Work16 & 0x0F00;
		uint16_t	W4 = Work16 & 0xF000;

		A1 += W1 + CheckCarry();
		if (A1 > 0x0009)
		{
			A1 -= 0x000A;
			A1 &= 0x000F;
			A2 += 0x0010;
		}

		A2 += W2;
		if (A2 > 0x0090)
		{
			A2 -= 0x00A0;
			A2 &= 0x00F0;
			A3 += 0x0100;
		}

		A3 += W3;
		if (A3 > 0x0900)
		{
			A3 -= 0x0A00;
			A3 &= 0x0F00;
			A4 += 0x1000;
		}

		A4 += W4;
		if (A4 > 0x9000)
		{
			A4 -= 0xA000;
			A4 &= 0xF000;
			SetCarry();
		}
		else
			ClearCarry();

		uint16_t	Ans16 = A4 | A3 | A2 | A1;

		if (~(Registers.A.W ^ Work16) & (Work16 ^ Ans16) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = Ans16;
		SetZN(Registers.A.W);
	}
	else
	{
		uint32_t	Ans32 = Registers.A.W + Work16 + CheckCarry();

		ICPU._Carry = Ans32 >= 0x10000;

		if (~(Registers.A.W ^ Work16) & (Work16 ^ (uint16_t) Ans32) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = (uint16_t) Ans32;
		SetZN(Registers.A.W);
	}
}

static inline void ADC (uint8_t Work8)
{
	if (CheckDecimal())
	{
		uint8_t	A1 = Registers.A.W & 0x0F;
		uint16_t	A2 = Registers.A.W & 0xF0;
		uint8_t	W1 = Work8 & 0x0F;
		uint8_t	W2 = Work8 & 0xF0;

		A1 += W1 + CheckCarry();
		if (A1 > 0x09)
		{
			A1 -= 0x0A;
			A1 &= 0x0F;
			A2 += 0x10;
		}

		A2 += W2;
		if (A2 > 0x90)
		{
			A2 -= 0xA0;
			A2 &= 0xF0;
			SetCarry();
		}
		else
			ClearCarry();

		uint8_t	Ans8 = A2 | A1;

		if (~(Registers.AL ^ Work8) & (Work8 ^ Ans8) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = Ans8;
		SetZN(Registers.AL);
	}
	else
	{
		uint16_t	Ans16 = Registers.AL + Work8 + CheckCarry();

		ICPU._Carry = Ans16 >= 0x100;

		if (~(Registers.AL ^ Work8) & (Work8 ^ (uint8_t) Ans16) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = (uint8_t) Ans16;
		SetZN(Registers.AL);
	}
}

static inline void AND (uint16_t Work16)
{
	Registers.A.W &= Work16;
	SetZN(Registers.A.W);
}

static inline void AND (uint8_t Work8)
{
	Registers.AL &= Work8;
	SetZN(Registers.AL);
}

static inline void ASL16 (uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w);
	ICPU._Carry = (Work16 & 0x8000) != 0;
	Work16 <<= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

static inline void ASL8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress);
	ICPU._Carry = (Work8 & 0x80) != 0;
	Work8 <<= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

static inline void BIT (uint16_t Work16)
{
	ICPU._Overflow = (Work16 & 0x4000) != 0;
	ICPU._Negative = (uint8_t) (Work16 >> 8);
	ICPU._Zero = (Work16 & Registers.A.W) != 0;
}

static inline void BIT (uint8_t Work8)
{
	ICPU._Overflow = (Work8 & 0x40) != 0;
	ICPU._Negative = Work8;
	ICPU._Zero = Work8 & Registers.AL;
}

static inline void CMP (uint16_t val)
{
	int32_t	Int32 = (int32_t) Registers.A.W - (int32_t) val;
	ICPU._Carry = Int32 >= 0;
	SetZN((uint16_t) Int32);
}

static inline void CMP (uint8_t val)
{
	int16_t	Int16 = (int16_t) Registers.AL - (int16_t) val;
	ICPU._Carry = Int16 >= 0;
	SetZN((uint8_t) Int16);
}

static inline void CPX (uint16_t val)
{
	int32_t	Int32 = (int32_t) Registers.X.W - (int32_t) val;
	ICPU._Carry = Int32 >= 0;
	SetZN((uint16_t) Int32);
}

static inline void CPX (uint8_t val)
{
	int16_t	Int16 = (int16_t) Registers.XL - (int16_t) val;
	ICPU._Carry = Int16 >= 0;
	SetZN((uint8_t) Int16);
}

static inline void CPY (uint16_t val)
{
	int32_t	Int32 = (int32_t) Registers.Y.W - (int32_t) val;
	ICPU._Carry = Int32 >= 0;
	SetZN((uint16_t) Int32);
}

static inline void CPY (uint8_t val)
{
	int16_t	Int16 = (int16_t) Registers.YL - (int16_t) val;
	ICPU._Carry = Int16 >= 0;
	SetZN((uint8_t) Int16);
}

static inline void DEC16 (uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w) - 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

static inline void DEC8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress) - 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

static inline void EOR (uint16_t val)
{
	Registers.A.W ^= val;
	SetZN(Registers.A.W);
}

static inline void EOR (uint8_t val)
{
	Registers.AL ^= val;
	SetZN(Registers.AL);
}

static inline void INC16 (uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w) + 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

static inline void INC8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress) + 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

static inline void LDA (uint16_t val)
{
	Registers.A.W = val;
	SetZN(Registers.A.W);
}

static inline void LDA (uint8_t val)
{
	Registers.AL = val;
	SetZN(Registers.AL);
}

static inline void LDX (uint16_t val)
{
	Registers.X.W = val;
	SetZN(Registers.X.W);
}

static inline void LDX (uint8_t val)
{
	Registers.XL = val;
	SetZN(Registers.XL);
}

static inline void LDY (uint16_t val)
{
	Registers.Y.W = val;
	SetZN(Registers.Y.W);
}

static inline void LDY (uint8_t val)
{
	Registers.YL = val;
	SetZN(Registers.YL);
}

static inline void LSR16 (uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w);
	ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

static inline void LSR8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress);
	ICPU._Carry = Work8 & 1;
	Work8 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

static inline void ORA (uint16_t val)
{
	Registers.A.W |= val;
	SetZN(Registers.A.W);
}

static inline void ORA (uint8_t val)
{
	Registers.AL |= val;
	SetZN(Registers.AL);
}

static inline void ROL16 (uint32_t OpAddress, s9xwrap_t w)
{
	uint32_t	Work32 = (((uint32_t) S9xGetWord(OpAddress, w)) << 1) | CheckCarry();
	ICPU._Carry = Work32 >= 0x10000;
	AddCycles(ONE_CYCLE);
	S9xSetWord((uint16_t) Work32, OpAddress, w, WRITE_10);
	OpenBus = Work32 & 0xff;
	SetZN((uint16_t) Work32);
}

static inline void ROL8 (uint32_t OpAddress)
{
	uint16_t	Work16 = (((uint16_t) S9xGetByte(OpAddress)) << 1) | CheckCarry();
	ICPU._Carry = Work16 >= 0x100;
	AddCycles(ONE_CYCLE);
	S9xSetByte((uint8_t) Work16, OpAddress);
	OpenBus = Work16 & 0xff;
	SetZN((uint8_t) Work16);
}

static inline void ROR16 (uint32_t OpAddress, s9xwrap_t w)
{
	uint32_t	Work32 = ((uint32_t) S9xGetWord(OpAddress, w)) | (((uint32_t) CheckCarry()) << 16);
	ICPU._Carry = Work32 & 1;
	Work32 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord((uint16_t) Work32, OpAddress, w, WRITE_10);
	OpenBus = Work32 & 0xff;
	SetZN((uint16_t) Work32);
}

static inline void ROR8 (uint32_t OpAddress)
{
	uint16_t	Work16 = ((uint16_t) S9xGetByte(OpAddress)) | (((uint16_t) CheckCarry()) << 8);
	ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte((uint8_t) Work16, OpAddress);
	OpenBus = Work16 & 0xff;
	SetZN((uint8_t) Work16);
}

static inline void SBC (uint16_t Work16)
{
	if (CheckDecimal())
	{
		uint16_t	A1 = Registers.A.W & 0x000F;
		uint16_t	A2 = Registers.A.W & 0x00F0;
		uint16_t	A3 = Registers.A.W & 0x0F00;
		uint32_t	A4 = Registers.A.W & 0xF000;
		uint16_t	W1 = Work16 & 0x000F;
		uint16_t	W2 = Work16 & 0x00F0;
		uint16_t	W3 = Work16 & 0x0F00;
		uint16_t	W4 = Work16 & 0xF000;

		A1 -= W1 + !CheckCarry();
		A2 -= W2;
		A3 -= W3;
		A4 -= W4;

		if (A1 > 0x000F)
		{
			A1 += 0x000A;
			A1 &= 0x000F;
			A2 -= 0x0010;
		}

		if (A2 > 0x00F0)
		{
			A2 += 0x00A0;
			A2 &= 0x00F0;
			A3 -= 0x0100;
		}

		if (A3 > 0x0F00)
		{
			A3 += 0x0A00;
			A3 &= 0x0F00;
			A4 -= 0x1000;
		}

		if (A4 > 0xF000)
		{
			A4 += 0xA000;
			A4 &= 0xF000;
			ClearCarry();
		}
		else
			SetCarry();

		uint16_t	Ans16 = A4 | A3 | A2 | A1;

		if ((Registers.A.W ^ Work16) & (Registers.A.W ^ Ans16) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = Ans16;
		SetZN(Registers.A.W);
	}
	else
	{
		int32_t	Int32 = (int32_t) Registers.A.W - (int32_t) Work16 + (int32_t) CheckCarry() - 1;

		ICPU._Carry = Int32 >= 0;

		if ((Registers.A.W ^ Work16) & (Registers.A.W ^ (uint16_t) Int32) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = (uint16_t) Int32;
		SetZN(Registers.A.W);
	}
}

static inline void SBC (uint8_t Work8)
{
	if (CheckDecimal())
	{
		uint8_t	A1 = Registers.A.W & 0x0F;
		uint16_t	A2 = Registers.A.W & 0xF0;
		uint8_t	W1 = Work8 & 0x0F;
		uint8_t	W2 = Work8 & 0xF0;

		A1 -= W1 + !CheckCarry();
		A2 -= W2;

		if (A1 > 0x0F)
		{
			A1 += 0x0A;
			A1 &= 0x0F;
			A2 -= 0x10;
		}

		if (A2 > 0xF0)
		{
			A2 += 0xA0;
			A2 &= 0xF0;
			ClearCarry();
		}
		else
			SetCarry();

		uint8_t	Ans8 = A2 | A1;

		if ((Registers.AL ^ Work8) & (Registers.AL ^ Ans8) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = Ans8;
		SetZN(Registers.AL);
	}
	else
	{
		int16_t	Int16 = (int16_t) Registers.AL - (int16_t) Work8 + (int16_t) CheckCarry() - 1;

		ICPU._Carry = Int16 >= 0;

		if ((Registers.AL ^ Work8) & (Registers.AL ^ (uint8_t) Int16) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.AL = (uint8_t) Int16;
		SetZN(Registers.AL);
	}
}

static inline void STA16 (uint32_t OpAddress, enum s9xwrap_t w)
{
	S9xSetWord(Registers.A.W, OpAddress, w);
	OpenBus = Registers.AH;
}

static inline void STA8 (uint32_t OpAddress)
{
	S9xSetByte(Registers.AL, OpAddress);
	OpenBus = Registers.AL;
}

static inline void STX16 (uint32_t OpAddress, enum s9xwrap_t w)
{
	S9xSetWord(Registers.X.W, OpAddress, w);
	OpenBus = Registers.XH;
}

static inline void STX8 (uint32_t OpAddress)
{
	S9xSetByte(Registers.XL, OpAddress);
	OpenBus = Registers.XL;
}

static inline void STY16 (uint32_t OpAddress, enum s9xwrap_t w)
{
	S9xSetWord(Registers.Y.W, OpAddress, w);
	OpenBus = Registers.YH;
}

static inline void STY8 (uint32_t OpAddress)
{
	S9xSetByte(Registers.YL, OpAddress);
	OpenBus = Registers.YL;
}

static inline void STZ16 (uint32_t OpAddress, enum s9xwrap_t w)
{
	S9xSetWord(0, OpAddress, w);
	OpenBus = 0;
}

static inline void STZ8 (uint32_t OpAddress)
{
	S9xSetByte(0, OpAddress);
	OpenBus = 0;
}

static inline void TSB16 (uint32_t OpAddress, enum s9xwrap_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w);
	ICPU._Zero = (Work16 & Registers.A.W) != 0;
	Work16 |= Registers.A.W;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
}

static inline void TSB8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress);
	ICPU._Zero = Work8 & Registers.AL;
	Work8 |= Registers.AL;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
}

static inline void TRB16 (uint32_t OpAddress, enum s9xwrap_t w)
{
	uint16_t	Work16 = S9xGetWord(OpAddress, w);
	ICPU._Zero = (Work16 & Registers.A.W) != 0;
	Work16 &= ~Registers.A.W;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
}

static inline void TRB8 (uint32_t OpAddress)
{
	uint8_t	Work8 = S9xGetByte(OpAddress);
	ICPU._Zero = Work8 & Registers.AL;
	Work8 &= ~Registers.AL;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
}

#endif
