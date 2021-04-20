/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "snes9x.h"
#include "memmap.h"
#include "dma.h"
#include "apu/apu.h"

static inline void ADD_CYCLES(int32_t n) { CPU.Cycles += n; }

extern uint8_t *HDMAMemPointers[8];
extern int HDMA_ModeByteCounts[8];

static uint8_t sdd1_decode_buffer[0x10000];

static inline bool addCyclesInDMA(uint8_t dma_channel)
{
	// Add 8 cycles per byte, sync APU, and do HC related events.
	// If HDMA was done in S9xDoHEventProcessing(), check if it used the same channel as DMA.
	ADD_CYCLES(SLOW_ONE_CYCLE);
	while (CPU.Cycles >= CPU.NextEvent)
		S9xDoHEventProcessing();

	if (CPU.HDMARanInDMA & (1 << dma_channel))
	{
		CPU.HDMARanInDMA = 0;
		// If HDMA triggers in the middle of DMA transfer and it uses the same channel,
		// it kills the DMA transfer immediately. $43x2 and $43x5 stop updating.
		return false;
	}

	CPU.HDMARanInDMA = 0;
	return true;
}

bool S9xDoDMA(uint8_t Channel)
{
	CPU.InDMA = CPU.InDMAorHDMA = true;
	CPU.CurrentDMAorHDMAChannel = Channel;

	SDMA *d = &DMA[Channel];

	// Check invalid DMA first
	if ((d->ABank == 0x7E || d->ABank == 0x7F) && d->BAddress == 0x80 && !d->ReverseTransfer)
	{
		// Attempting a DMA from WRAM to $2180 will not work, WRAM will not be written.
		// Attempting a DMA from $2180 to WRAM will similarly not work,
		// the value written is (initially) the OpenBus value.
		// In either case, the address in $2181-3 is not incremented.

		// Does an invalid DMA actually take time?
		// I'd say yes, since 'invalid' is probably just the WRAM chip
		// not being able to read and write itself at the same time
		// And no, PPU.WRAM should not be updated.

		int32_t c = d->DMACount_Or_HDMAIndirectAddress;
		// Writing $0000 to $43x5 actually results in a transfer of $10000 bytes, not 0.
		if (!c)
			c = 0x10000;

		// 8 cycles per channel
		ADD_CYCLES(SLOW_ONE_CYCLE);
		// 8 cycles per byte
		while (c)
		{
			--d->DMACount_Or_HDMAIndirectAddress;
			++d->AAddress;
			--c;
			if (!addCyclesInDMA(Channel))
			{
				CPU.InDMA = false;
				CPU.InDMAorHDMA = false;
				CPU.CurrentDMAorHDMAChannel = -1;
				return false;
			}
		}

		CPU.InDMA = false;
		CPU.InDMAorHDMA = false;
		CPU.CurrentDMAorHDMAChannel = -1;
		return true;
	}

	// Prepare for accessing $2118-2119

	int32_t inc = d->AAddressFixed ? 0 : (!d->AAddressDecrement ? 1 : -1);
	int32_t count = d->DMACount_Or_HDMAIndirectAddress;
	// Writing $0000 to $43x5 actually results in a transfer of $10000 bytes, not 0.
	if (!count)
		count = 0x10000;

	// Prepare for custom chip DMA

	// S-DD1

	uint8_t *in_sdd1_dma = nullptr;

	if (Settings.SDD1)
	{
		if (d->AAddressFixed && Memory.FillRAM[0x4801] > 0)
		{
			// XXX: Should probably verify that we're DMAing from ROM?
			// And somewhere we should make sure we're not running across a mapping boundary too.
			// Hacky support for pre-decompressed S-DD1 data
			inc = !d->AAddressDecrement ? 1 : -1;

			uint8_t *in_ptr = S9xGetBasePointer((d->ABank << 16) | d->AAddress);
			if (in_ptr)
				in_ptr += d->AAddress;

			in_sdd1_dma = sdd1_decode_buffer;
		}

		Memory.FillRAM[0x4801] = 0;
	}

	// Do Transfer

	uint8_t Work;

	// 8 cycles per channel
	ADD_CYCLES(SLOW_ONE_CYCLE);

	if (!d->ReverseTransfer)
	{
		// CPU -> PPU
		int32_t b = 0;
		uint16_t p = d->AAddress;
		uint8_t *base = S9xGetBasePointer((d->ABank << 16) + d->AAddress);

		int32_t rem = count;
		// Transfer per block if d->AAdressFixed is false
		count = d->AAddressFixed ? rem : (d->AAddressDecrement ? ((p & MEMMAP_MASK) + 1) : (MEMMAP_BLOCK_SIZE - (p & MEMMAP_MASK)));

		// Settings for custom chip DMA
		if (in_sdd1_dma)
		{
			base = in_sdd1_dma;
			p = 0;
			count = rem;
		}

		bool inWRAM_DMA = !in_sdd1_dma && (d->ABank == 0x7e || d->ABank == 0x7f || (!(d->ABank & 0x40) && d->AAddress < 0x2000));

		// 8 cycles per byte
		auto UPDATE_COUNTERS = [&]() -> bool
		{
			--d->DMACount_Or_HDMAIndirectAddress;
			d->AAddress += inc;
			p += inc;
			if (!addCyclesInDMA(Channel))
			{
				CPU.InDMA = CPU.InDMAorHDMA = CPU.InWRAMDMAorHDMA = false;
				CPU.CurrentDMAorHDMAChannel = -1;
				return false;
			}
			return true;
		};

		while (1)
		{
			if (count > rem)
				count = rem;
			rem -= count;

			CPU.InWRAMDMAorHDMA = inWRAM_DMA;

			if (!base)
			{
				// DMA SLOW PATH
				if (!d->TransferMode || d->TransferMode == 2 || d->TransferMode == 6)
				{
					do
					{
						Work = S9xGetByte((d->ABank << 16) + p);
						S9xSetPPU(Work, 0x2100 + d->BAddress);
						if (!UPDATE_COUNTERS())
							return false;
					} while (--count > 0);
				}
				else if (d->TransferMode == 1 || d->TransferMode == 5)
				{
					// This is a variation on Duff's Device. It is legal C/C++.
					switch (b)
					{
						default:
						while (count > 1)
						{
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						// Fall through
						case 1:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						}
					}

					if (count == 1)
					{
						Work = S9xGetByte((d->ABank << 16) + p);
						S9xSetPPU(Work, 0x2100 + d->BAddress);
						if (!UPDATE_COUNTERS())
							return false;
						b = 1;
					}
					else
						b = 0;
				}
				else if (d->TransferMode == 3 || d->TransferMode == 7)
				{
					switch (b)
					{
						default:
						do
						{
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}
						// Fall through
						case 1:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}
						// Fall through
						case 2:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}
						// Fall through
						case 3:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
				else if (d->TransferMode == 4)
				{
					switch (b)
					{
						default:
						do
						{
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}
						// Fall through
						case 1:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}
						// Fall through
						case 2:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2102 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}
						// Fall through
						case 3:
							Work = S9xGetByte((d->ABank << 16) + p);
							S9xSetPPU(Work, 0x2103 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
			}
			else
			{
				// DMA FAST PATH
				if (!d->TransferMode || d->TransferMode == 2 || d->TransferMode == 6)
				{
					switch (d->BAddress)
					{
						case 0x04: // OAMDATA
							do
							{
								Work = *(base + p);
								REGISTER_2104(Work);
								if (!UPDATE_COUNTERS())
									return false;
							} while (--count > 0);

							break;

						case 0x18: // VMDATAL
							if (!PPU.VMA.FullGraphicCount)
							{
								do
								{
									Work = *(base + p);
									REGISTER_2118_linear(Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}
							else
							{
								do
								{
									Work = *(base + p);
									REGISTER_2118_tile(Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}

							break;

						case 0x19: // VMDATAH
							if (!PPU.VMA.FullGraphicCount)
							{
								do
								{
									Work = *(base + p);
									REGISTER_2119_linear(Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}
							else
							{
								do
								{
									Work = *(base + p);
									REGISTER_2119_tile(Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}

							break;

						case 0x22: // CGDATA
							do
							{
								Work = *(base + p);
								if (!UPDATE_COUNTERS())
									return false;
							} while (--count > 0);

							break;

						case 0x80: // WMDATA
							if (!CPU.InWRAMDMAorHDMA)
							{
								do
								{
									Work = *(base + p);
									REGISTER_2180(Work);
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}
							else
							{
								do
								{
									if (!UPDATE_COUNTERS())
										return false;
								} while (--count > 0);
							}

							break;

						default:
							do
							{
								Work = *(base + p);
								S9xSetPPU(Work, 0x2100 + d->BAddress);
								if (!UPDATE_COUNTERS())
									return false;
							} while (--count > 0);

							break;
					}
				}
				else if (d->TransferMode == 1 || d->TransferMode == 5)
				{
					if (d->BAddress == 0x18)
					{
						// VMDATAL
						if (!PPU.VMA.FullGraphicCount)
						{
							switch (b)
							{
								default:
								while (count > 1)
								{
									Work = *(base + p);
									REGISTER_2118_linear(Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;
								// Fall through
								case 1:
									OpenBus = *(base + p);
									REGISTER_2119_linear(OpenBus);
									if (!UPDATE_COUNTERS())
										return false;
									--count;
								}
							}

							if (count == 1)
							{
								Work = *(base + p);
								REGISTER_2118_linear(Work);
								if (!UPDATE_COUNTERS())
									return false;
								b = 1;
							}
							else
								b = 0;
						}
						else
						{
							switch (b)
							{
								default:
								while (count > 1)
								{
									Work = *(base + p);
									REGISTER_2118_tile(Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;
								// Fall through
								case 1:
									Work = *(base + p);
									REGISTER_2119_tile(Work);
									if (!UPDATE_COUNTERS())
										return false;
									--count;
								}
							}

							if (count == 1)
							{
								Work = *(base + p);
								REGISTER_2118_tile(Work);
								if (!UPDATE_COUNTERS())
									return false;
								b = 1;
							}
							else
								b = 0;
						}
					}
					else
					{
						// DMA mode 1 general case
						switch (b)
						{
							default:
							while (count > 1)
							{
								Work = *(base + p);
								S9xSetPPU(Work, 0x2100 + d->BAddress);
								if (!UPDATE_COUNTERS())
									return false;
								--count;
							// Fall through
							case 1:
								Work = *(base + p);
								S9xSetPPU(Work, 0x2101 + d->BAddress);
								if (!UPDATE_COUNTERS())
									return false;
								--count;
							}
						}

						if (count == 1)
						{
							Work = *(base + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							b = 1;
						}
						else
							b = 0;
					}
				}
				else if (d->TransferMode == 3 || d->TransferMode == 7)
				{
					switch (b)
					{
						default:
						do
						{
							Work = *(base + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}
						// Fall through
						case 1:
							Work = *(base + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}
						// Fall through
						case 2:
							Work = *(base + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}
						// Fall through
						case 3:
							Work = *(base + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
				else if (d->TransferMode == 4)
				{
					switch (b)
					{
						default:
						do
						{
							Work = *(base + p);
							S9xSetPPU(Work, 0x2100 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 1;
								break;
							}
						// Fall through
						case 1:
							Work = *(base + p);
							S9xSetPPU(Work, 0x2101 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 2;
								break;
							}
						// Fall through
						case 2:
							Work = *(base + p);
							S9xSetPPU(Work, 0x2102 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 3;
								break;
							}
						// Fall through
						case 3:
							Work = *(base + p);
							S9xSetPPU(Work, 0x2103 + d->BAddress);
							if (!UPDATE_COUNTERS())
								return false;
							if (--count <= 0)
							{
								b = 0;
								break;
							}
						} while (1);
					}
				}
			}

			if (rem <= 0)
				break;

			base = S9xGetBasePointer((d->ABank << 16) + d->AAddress);
			count = MEMMAP_BLOCK_SIZE;
			inWRAM_DMA = !in_sdd1_dma && (d->ABank == 0x7e || d->ABank == 0x7f || (!(d->ABank & 0x40) && d->AAddress < 0x2000));
		}
	}
	else
	{
		// PPU -> CPU

		// 8 cycles per byte
		auto UPDATE_COUNTERS = [&]() -> bool
		{
			--d->DMACount_Or_HDMAIndirectAddress;
			d->AAddress += inc;
			if (!addCyclesInDMA(Channel))
			{
				CPU.InDMA = CPU.InDMAorHDMA = CPU.InWRAMDMAorHDMA = false;
				CPU.CurrentDMAorHDMAChannel = -1;
				return false;
			}
			return true;
		};

		if (d->BAddress > 0x80 - 4 && d->BAddress <= 0x83 && !(d->ABank & 0x40))
		{
			// REVERSE-DMA REALLY-SLOW PATH
			do
			{
				switch (d->TransferMode)
				{
					case 0:
					case 2:
					case 6:
						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 1:
					case 5:
						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 3:
					case 7:
						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 4:
						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2102 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						CPU.InWRAMDMAorHDMA = d->AAddress < 0x2000;
						Work = S9xGetPPU(0x2103 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					default:
						while (count)
						{
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						}
				}
			} while (count);
		}
		else
		{
			// REVERSE-DMA FASTER PATH
			CPU.InWRAMDMAorHDMA = d->ABank == 0x7e || d->ABank == 0x7f;
			do
			{
				switch (d->TransferMode)
				{
					case 0:
					case 2:
					case 6:
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 1:
					case 5:
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 3:
					case 7:
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					case 4:
						Work = S9xGetPPU(0x2100 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2101 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2102 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						if (!--count)
							break;

						Work = S9xGetPPU(0x2103 + d->BAddress);
						S9xSetByte(Work, (d->ABank << 16) + d->AAddress);
						if (!UPDATE_COUNTERS())
							return false;
						--count;

						break;

					default:
						while (count)
						{
							if (!UPDATE_COUNTERS())
								return false;
							--count;
						}
				}
			} while (count);
		}
	}

	if (CPU.NMIPending && Timings.NMITriggerPos != 0xffff)
		Timings.NMITriggerPos = CPU.Cycles + Timings.NMIDMADelay;

	CPU.InDMA = CPU.InDMAorHDMA = CPU.InWRAMDMAorHDMA = false;
	CPU.CurrentDMAorHDMAChannel = -1;

	return true;
}

static inline bool HDMAReadLineCount(int d)
{
	// CPU.InDMA is set, so S9xGetXXX() / S9xSetXXX() incur no charges.

	uint8_t line = S9xGetByte((DMA[d].ABank << 16) + DMA[d].Address);
	ADD_CYCLES(SLOW_ONE_CYCLE);

	if (!line)
	{
		DMA[d].Repeat = false;
		DMA[d].LineCount = 128;

		if (DMA[d].HDMAIndirectAddressing)
		{
			if (PPU.HDMA & (0xfe << d))
			{
				++DMA[d].Address;
				ADD_CYCLES(SLOW_ONE_CYCLE << 1);
			}
			else
				ADD_CYCLES(SLOW_ONE_CYCLE);

			DMA[d].DMACount_Or_HDMAIndirectAddress = S9xGetWord((DMA[d].ABank << 16) + DMA[d].Address);
			++DMA[d].Address;
		}

		++DMA[d].Address;
		HDMAMemPointers[d] = nullptr;

		return false;
	}
	else if (line == 0x80)
	{
		DMA[d].Repeat = true;
		DMA[d].LineCount = 128;
	}
	else
	{
		DMA[d].Repeat = !(line & 0x80);
		DMA[d].LineCount = line & 0x7f;
	}

	++DMA[d].Address;
	DMA[d].DoTransfer = true;

	if (DMA[d].HDMAIndirectAddressing)
	{
		ADD_CYCLES(SLOW_ONE_CYCLE << 1);
		DMA[d].DMACount_Or_HDMAIndirectAddress = S9xGetWord((DMA[d].ABank << 16) + DMA[d].Address);
		DMA[d].Address += 2;
		HDMAMemPointers[d] = S9xGetMemPointer((DMA[d].IndirectBank << 16) + DMA[d].DMACount_Or_HDMAIndirectAddress);
	}
	else
		HDMAMemPointers[d] = S9xGetMemPointer((DMA[d].ABank << 16) + DMA[d].Address);

	return true;
}

void S9xStartHDMA()
{
	PPU.HDMA = Memory.FillRAM[0x420c];

	PPU.HDMAEnded = 0;

	CPU.InHDMA = CPU.InDMAorHDMA = true;
	int32_t tmpch = CPU.CurrentDMAorHDMAChannel;

	// XXX: Not quite right...
	if (PPU.HDMA)
		ADD_CYCLES(Timings.DMACPUSync);

	for (uint8_t i = 0; i < 8; ++i)
	{
		if (PPU.HDMA & (1 << i))
		{
			CPU.CurrentDMAorHDMAChannel = i;

			DMA[i].Address = DMA[i].AAddress;

			if (!HDMAReadLineCount(i))
			{
				PPU.HDMA &= ~(1 << i);
				PPU.HDMAEnded |= 1 << i;
			}
		}
		else
			DMA[i].DoTransfer = false;
	}

	CPU.InHDMA = false;
	CPU.InDMAorHDMA = CPU.InDMA;
	CPU.HDMARanInDMA = CPU.InDMA ? PPU.HDMA : 0;
	CPU.CurrentDMAorHDMAChannel = tmpch;
}

uint8_t S9xDoHDMA(uint8_t byte)
{
	SDMA *p;

	int d;
	uint8_t mask;

	CPU.InHDMA = CPU.InDMAorHDMA = true;
	CPU.HDMARanInDMA = CPU.InDMA ? byte : 0;
	bool temp = CPU.InWRAMDMAorHDMA;
	int32_t tmpch = CPU.CurrentDMAorHDMAChannel;

	// XXX: Not quite right...
	ADD_CYCLES(Timings.DMACPUSync);

	for (mask = 1, p = &DMA[0], d = 0; mask; mask <<= 1, ++p, ++d)
	{
		if (byte & mask)
		{
			CPU.InWRAMDMAorHDMA = false;
			CPU.CurrentDMAorHDMAChannel = d;

			uint32_t ShiftedIBank;
			uint16_t IAddr;
			if (p->HDMAIndirectAddressing)
			{
				ShiftedIBank = p->IndirectBank << 16;
				IAddr = p->DMACount_Or_HDMAIndirectAddress;
			}
			else
			{
				ShiftedIBank = p->ABank << 16;
				IAddr = p->Address;
			}

			if (!HDMAMemPointers[d])
				HDMAMemPointers[d] = S9xGetMemPointer(ShiftedIBank + IAddr);

			if (p->DoTransfer)
			{
				// XXX: Hack for Uniracers, because we don't understand
				// OAM Address Invalidation
				if (p->BAddress == 0x04)
				{
					if (SNESGameFixes.Uniracers)
					{
						PPU.OAMAddr = 0x10c;
						PPU.OAMFlip = 0;
					}
				}

				if (!p->ReverseTransfer)
				{
					if ((IAddr & MEMMAP_MASK) + HDMA_ModeByteCounts[p->TransferMode] >= MEMMAP_BLOCK_SIZE)
					{
						// HDMA REALLY-SLOW PATH
						HDMAMemPointers[d] = nullptr;

						auto DOBYTE = [&](uint16_t Addr, uint16_t RegOff)
						{
							CPU.InWRAMDMAorHDMA = ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || (!(ShiftedIBank & 0x400000) && Addr < 0x2000);
							S9xSetPPU(S9xGetByte(ShiftedIBank + Addr), 0x2100 + p->BAddress + RegOff);
						};

						switch (p->TransferMode)
						{
							case 0:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								break;

							case 5:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 1);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 2, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 3, 1);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								break;

							case 1:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 1);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								break;

							case 2:
							case 6:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								break;

							case 3:
							case 7:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 2, 1);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 3, 1);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								break;

							case 4:
								DOBYTE(IAddr, 0);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 1, 1);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 2, 2);
								ADD_CYCLES(SLOW_ONE_CYCLE);
								DOBYTE(IAddr + 3, 3);
								ADD_CYCLES(SLOW_ONE_CYCLE);
						}
					}
					else
					{
						CPU.InWRAMDMAorHDMA = ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || (!(ShiftedIBank & 0x400000) && IAddr < 0x2000);

						if (!HDMAMemPointers[d])
						{
							// HDMA SLOW PATH
							uint32_t Addr = ShiftedIBank + IAddr;

							switch (p->TransferMode)
							{
								case 0:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									break;

								case 5:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									Addr += 2;
									/* fall through */
								case 1:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									break;

								case 2:
								case 6:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									break;

								case 3:
								case 7:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 2), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 3), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									break;

								case 4:
									S9xSetPPU(S9xGetByte(Addr), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 2), 0x2102 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(S9xGetByte(Addr + 3), 0x2103 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
							}
						}
						else
						{
							// HDMA FAST PATH
							switch (p->TransferMode)
							{
								case 0:
									S9xSetPPU(*HDMAMemPointers[d]++, 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									break;

								case 5:
									S9xSetPPU(*HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									HDMAMemPointers[d] += 2;
									/* fall through */
								case 1:
									S9xSetPPU(*HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									// XXX: All HDMA should read to MDR first. This one just
									// happens to fix Speedy Gonzales.
									OpenBus = *(HDMAMemPointers[d] + 1);
									S9xSetPPU(OpenBus, 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									HDMAMemPointers[d] += 2;
									break;

								case 2:
								case 6:
									S9xSetPPU(*HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									HDMAMemPointers[d] += 2;
									break;

								case 3:
								case 7:
									S9xSetPPU(*HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 2), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 3), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									HDMAMemPointers[d] += 4;
									break;

								case 4:
									S9xSetPPU(*HDMAMemPointers[d], 0x2100 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 1), 0x2101 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 2), 0x2102 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									S9xSetPPU(*(HDMAMemPointers[d] + 3), 0x2103 + p->BAddress);
									ADD_CYCLES(SLOW_ONE_CYCLE);
									HDMAMemPointers[d] += 4;
							}
						}
					}
				}
				else
				{
					// REVERSE HDMA REALLY-SLOW PATH
					// anomie says: Since this is apparently never used
					// (otherwise we would have noticed before now), let's not bother with faster paths.
					HDMAMemPointers[d] = nullptr;

					auto DOBYTE = [&](uint16_t Addr, uint16_t RegOff)
					{
						CPU.InWRAMDMAorHDMA = ShiftedIBank == 0x7e0000 || ShiftedIBank == 0x7f0000 || (!(ShiftedIBank & 0x400000) && Addr < 0x2000);
						S9xSetByte(S9xGetPPU(0x2100 + p->BAddress + RegOff), ShiftedIBank + Addr);
					};

					switch (p->TransferMode)
					{
						case 0:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							break;

						case 5:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 1);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 2, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 3, 1);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							break;

						case 1:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 1);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							break;

						case 2:
						case 6:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							break;

						case 3:
						case 7:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 2, 1);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 3, 1);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							break;

						case 4:
							DOBYTE(IAddr, 0);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 1, 1);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 2, 2);
							ADD_CYCLES(SLOW_ONE_CYCLE);
							DOBYTE(IAddr + 3, 3);
							ADD_CYCLES(SLOW_ONE_CYCLE);
					}
				}
			}
		}
	}

	for (mask = 1, p = &DMA[0], d = 0; mask; mask <<= 1, ++p, ++d)
	{
		if (byte & mask)
		{
			if (p->DoTransfer)
			{
				if (p->HDMAIndirectAddressing)
					p->DMACount_Or_HDMAIndirectAddress += HDMA_ModeByteCounts[p->TransferMode];
				else
					p->Address += HDMA_ModeByteCounts[p->TransferMode];
			}

			p->DoTransfer = !p->Repeat;

			if (!--p->LineCount)
			{
				if (!HDMAReadLineCount(d))
				{
					byte &= ~mask;
					PPU.HDMAEnded |= mask;
					p->DoTransfer = false;
				}
			}
			else
				ADD_CYCLES(SLOW_ONE_CYCLE);
		}
	}

	CPU.InHDMA = false;
	CPU.InDMAorHDMA = CPU.InDMA;
	CPU.InWRAMDMAorHDMA = temp;
	CPU.CurrentDMAorHDMAChannel = tmpch;

	return byte;
}

void S9xResetDMA()
{
	for (int d = 0; d < 8; ++d)
	{
		DMA[d].ReverseTransfer = DMA[d].HDMAIndirectAddressing = DMA[d].AAddressFixed = DMA[d].AAddressDecrement = true;
		DMA[d].TransferMode = 7;
		DMA[d].BAddress = 0xff;
		DMA[d].AAddress = 0xffff;
		DMA[d].ABank = 0xff;
		DMA[d].DMACount_Or_HDMAIndirectAddress = 0xffff;
		DMA[d].IndirectBank = 0xff;
		DMA[d].Address = 0xffff;
		DMA[d].Repeat = false;
		DMA[d].LineCount = 0x7f;
		DMA[d].UnknownByte = 0xff;
		DMA[d].DoTransfer = false;
		DMA[d].UnusedBit43x0 = true;
	}
}
