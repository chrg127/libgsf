/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GBA_IO_H
#define GBA_IO_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/core/log.h>

#define GBA_REG(X) (GBA_REG_ ## X >> 1)

enum GBAIORegisters {
	// Video
	GBA_REG_DISPCNT = 0x000,
	GBA_REG_GREENSWP = 0x002,
	GBA_REG_DISPSTAT = 0x004,
	GBA_REG_VCOUNT = 0x006,
	GBA_REG_BG0CNT = 0x008,
	GBA_REG_BG1CNT = 0x00A,
	GBA_REG_BG2CNT = 0x00C,
	GBA_REG_BG3CNT = 0x00E,
	GBA_REG_BG0HOFS = 0x010,
	GBA_REG_BG0VOFS = 0x012,
	GBA_REG_BG1HOFS = 0x014,
	GBA_REG_BG1VOFS = 0x016,
	GBA_REG_BG2HOFS = 0x018,
	GBA_REG_BG2VOFS = 0x01A,
	GBA_REG_BG3HOFS = 0x01C,
	GBA_REG_BG3VOFS = 0x01E,
	GBA_REG_BG2PA = 0x020,
	GBA_REG_BG2PB = 0x022,
	GBA_REG_BG2PC = 0x024,
	GBA_REG_BG2PD = 0x026,
	GBA_REG_BG2X_LO = 0x028,
	GBA_REG_BG2X_HI = 0x02A,
	GBA_REG_BG2Y_LO = 0x02C,
	GBA_REG_BG2Y_HI = 0x02E,
	GBA_REG_BG3PA = 0x030,
	GBA_REG_BG3PB = 0x032,
	GBA_REG_BG3PC = 0x034,
	GBA_REG_BG3PD = 0x036,
	GBA_REG_BG3X_LO = 0x038,
	GBA_REG_BG3X_HI = 0x03A,
	GBA_REG_BG3Y_LO = 0x03C,
	GBA_REG_BG3Y_HI = 0x03E,
	GBA_REG_WIN0H = 0x040,
	GBA_REG_WIN1H = 0x042,
	GBA_REG_WIN0V = 0x044,
	GBA_REG_WIN1V = 0x046,
	GBA_REG_WININ = 0x048,
	GBA_REG_WINOUT = 0x04A,
	GBA_REG_MOSAIC = 0x04C,
	GBA_REG_BLDCNT = 0x050,
	GBA_REG_BLDALPHA = 0x052,
	GBA_REG_BLDY = 0x054,

	// Sound
	GBA_REG_SOUND1CNT_LO = 0x060,
	GBA_REG_SOUND1CNT_HI = 0x062,
	GBA_REG_SOUND1CNT_X = 0x064,
	GBA_REG_SOUND2CNT_LO = 0x068,
	GBA_REG_SOUND2CNT_HI = 0x06C,
	GBA_REG_SOUND3CNT_LO = 0x070,
	GBA_REG_SOUND3CNT_HI = 0x072,
	GBA_REG_SOUND3CNT_X = 0x074,
	GBA_REG_SOUND4CNT_LO = 0x078,
	GBA_REG_SOUND4CNT_HI = 0x07C,
	GBA_REG_SOUNDCNT_LO = 0x080,
	GBA_REG_SOUNDCNT_HI = 0x082,
	GBA_REG_SOUNDCNT_X = 0x084,
	GBA_REG_SOUNDBIAS = 0x088,
	GBA_REG_WAVE_RAM0_LO = 0x090,
	GBA_REG_WAVE_RAM0_HI = 0x092,
	GBA_REG_WAVE_RAM1_LO = 0x094,
	GBA_REG_WAVE_RAM1_HI = 0x096,
	GBA_REG_WAVE_RAM2_LO = 0x098,
	GBA_REG_WAVE_RAM2_HI = 0x09A,
	GBA_REG_WAVE_RAM3_LO = 0x09C,
	GBA_REG_WAVE_RAM3_HI = 0x09E,
	GBA_REG_FIFO_A_LO = 0x0A0,
	GBA_REG_FIFO_A_HI = 0x0A2,
	GBA_REG_FIFO_B_LO = 0x0A4,
	GBA_REG_FIFO_B_HI = 0x0A6,

	// DMA
	GBA_REG_DMA0SAD_LO = 0x0B0,
	GBA_REG_DMA0SAD_HI = 0x0B2,
	GBA_REG_DMA0DAD_LO = 0x0B4,
	GBA_REG_DMA0DAD_HI = 0x0B6,
	GBA_REG_DMA0CNT_LO = 0x0B8,
	GBA_REG_DMA0CNT_HI = 0x0BA,
	GBA_REG_DMA1SAD_LO = 0x0BC,
	GBA_REG_DMA1SAD_HI = 0x0BE,
	GBA_REG_DMA1DAD_LO = 0x0C0,
	GBA_REG_DMA1DAD_HI = 0x0C2,
	GBA_REG_DMA1CNT_LO = 0x0C4,
	GBA_REG_DMA1CNT_HI = 0x0C6,
	GBA_REG_DMA2SAD_LO = 0x0C8,
	GBA_REG_DMA2SAD_HI = 0x0CA,
	GBA_REG_DMA2DAD_LO = 0x0CC,
	GBA_REG_DMA2DAD_HI = 0x0CE,
	GBA_REG_DMA2CNT_LO = 0x0D0,
	GBA_REG_DMA2CNT_HI = 0x0D2,
	GBA_REG_DMA3SAD_LO = 0x0D4,
	GBA_REG_DMA3SAD_HI = 0x0D6,
	GBA_REG_DMA3DAD_LO = 0x0D8,
	GBA_REG_DMA3DAD_HI = 0x0DA,
	GBA_REG_DMA3CNT_LO = 0x0DC,
	GBA_REG_DMA3CNT_HI = 0x0DE,

	// Timers
	GBA_REG_TM0CNT_LO = 0x100,
	GBA_REG_TM0CNT_HI = 0x102,
	GBA_REG_TM1CNT_LO = 0x104,
	GBA_REG_TM1CNT_HI = 0x106,
	GBA_REG_TM2CNT_LO = 0x108,
	GBA_REG_TM2CNT_HI = 0x10A,
	GBA_REG_TM3CNT_LO = 0x10C,
	GBA_REG_TM3CNT_HI = 0x10E,

	// SIO (note: some of these are repeated)
	GBA_REG_SIODATA32_LO = 0x120,
	GBA_REG_SIOMULTI0 = 0x120,
	GBA_REG_SIODATA32_HI = 0x122,
	GBA_REG_SIOMULTI1 = 0x122,
	GBA_REG_SIOMULTI2 = 0x124,
	GBA_REG_SIOMULTI3 = 0x126,
	GBA_REG_SIOCNT = 0x128,
	GBA_REG_SIOMLT_SEND = 0x12A,
	GBA_REG_SIODATA8 = 0x12A,
	GBA_REG_RCNT = 0x134,
	GBA_REG_JOYCNT = 0x140,
	GBA_REG_JOY_RECV_LO = 0x150,
	GBA_REG_JOY_RECV_HI = 0x152,
	GBA_REG_JOY_TRANS_LO = 0x154,
	GBA_REG_JOY_TRANS_HI = 0x156,
	GBA_REG_JOYSTAT = 0x158,

	// Keypad
	GBA_REG_KEYINPUT = 0x130,
	GBA_REG_KEYCNT = 0x132,

	// Interrupts, etc
	GBA_REG_IE = 0x200,
	GBA_REG_IF = 0x202,
	GBA_REG_WAITCNT = 0x204,
	GBA_REG_IME = 0x208,

	GBA_REG_MAX = 0x20A,

	GBA_REG_POSTFLG = 0x300,
	GBA_REG_HALTCNT = 0x301,

	GBA_REG_EXWAITCNT_LO = 0x800,
	GBA_REG_EXWAITCNT_HI = 0x802,

	GBA_REG_INTERNAL_EXWAITCNT_LO = 0x210,
	GBA_REG_INTERNAL_EXWAITCNT_HI = 0x212,
	GBA_REG_INTERNAL_MAX = 0x214,

	GBA_REG_DEBUG_STRING = 0xFFF600,
	GBA_REG_DEBUG_FLAGS = 0xFFF700,
	GBA_REG_DEBUG_ENABLE = 0xFFF780,
};

mLOG_DECLARE_CATEGORY(GBA_IO);

extern MGBA_EXPORT const char* const GBAIORegisterNames[];

struct GBA;
void GBAIOInit(struct GBA* gba);
void GBAIOWrite(struct GBA* gba, uint32_t address, uint16_t value);
void GBAIOWrite8(struct GBA* gba, uint32_t address, uint8_t value);
void GBAIOWrite32(struct GBA* gba, uint32_t address, uint32_t value);
uint16_t GBAIORead(struct GBA* gba, uint32_t address);

bool GBAIOIsReadConstant(uint32_t address);

struct GBASerializedState;
void GBAIOSerialize(struct GBA* gba, struct GBASerializedState* state);
void GBAIODeserialize(struct GBA* gba, const struct GBASerializedState* state);

CXX_GUARD_END

#endif