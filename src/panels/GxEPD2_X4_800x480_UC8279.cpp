// See GxEPD2_X4_800x480_UC8279.h for provenance and licence.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../DisplayDrivers.h"

#if defined(USE_GXEPD2)

#include "GxEPD2_X4_800x480_UC8279.h"

namespace {
// Register values, from the FreeInk SDK's uc8279X4DefaultConfig().
constexpr uint8_t PSR0 = 0x37;       // REG=1 at init (the AA path uses it as is)
constexpr uint8_t PSR1 = 0x4D;       //
constexpr uint8_t PSR0_OTP = 0x17;   // PSR0 & 0xDF — REG=0, run the OTP waveforms
constexpr uint8_t PFS = 0x20;        // 0x03 power-off sequence
constexpr uint8_t PLL = 0x0E;        // 0x30 frame rate
constexpr uint8_t GATE_SCAN = 0x02;  // 0xE1
constexpr uint8_t CCSET = 0x02;      // 0xE0
constexpr uint8_t TSSET_FULL = 0x1E; // 0xE5 forced temperature, full refresh
constexpr uint8_t TSSET_FAST = 0x5A; // 0xE5, fast/partial
} // namespace

GxEPD2_X4_800x480_UC8279::GxEPD2_X4_800x480_UC8279(int16_t cs, int16_t dc, int16_t rst, int16_t busy)
    : GxEPD2_Xteink_UC(
          cs, dc, rst, busy, 10000000, WIDTH, HEIGHT, GATE_OFFSET, GATE_TOTAL, panel
      ) {}

void GxEPD2_X4_800x480_UC8279::_initController() {
    _writeCommand(0x00); // PSR panel setting
    _writeData(PSR0);
    _writeData(PSR1);

    _writeCommand(0x61); // TRES resolution
    _writeData((uint8_t)(WIDTH >> 8));
    _writeData((uint8_t)(WIDTH & 0xFF));
    _writeData((uint8_t)(GATE_TOTAL >> 8));
    _writeData((uint8_t)(GATE_TOTAL & 0xFF));

    _writeCommand(0x65); // GSST gate/source start
    _writeData(0x00);
    _writeData(0x00);
    _writeData(0x00);
    _writeData(0x00);

    _writeCommand(0x03); // PFS power-off sequence
    _writeData(PFS);

    _writeCommand(0x30); // PLL — this part programs the frame rate, not the booster
    _writeData(PLL);

    _writeCommand(0xE1); // gate scan selection
    _writeData(GATE_SCAN);
}

void GxEPD2_X4_800x480_UC8279::_refreshFrame(bool full_sync) {
    // The plane writes pad 120 white rows before the image and none after
    // (120 + 480 = 600); the base class does that from GATE_OFFSET.
    _sendPlane(0x13, false);

    if (full_sync) _fillPlane(0x10, 0xFF);
    // Otherwise 0x10 still holds the previously displayed frame.

    // No CDI here on purpose: 0x50 belongs to this part's grayscale pass, and
    // the B/W path in the OEM stream never touches it.
    _writeCommand(0xE0); // CCSET
    _writeData(CCSET);
    _writeCommand(0xE5); // TSSET forced temperature
    _writeData(full_sync ? TSSET_FULL : TSSET_FAST);
    _writeCommand(0x00); // PSR again, REG cleared so the OTP waveforms run
    _writeData(PSR0_OTP);
    _writeData(PSR1);

    if (!full_sync) {
        _writeCommand(0x03);
        _writeData(PFS);
        _writeCommand(0xE1);
        _writeData(GATE_SCAN);
    }

    _PowerOn();

    if (!full_sync) _writeCommand(0x91); // PTIN
    _writeCommand(0x12);                 // DRF
    _waitWhileBusy("_Update", full_sync ? full_refresh_time : partial_refresh_time);
    if (!full_sync) _writeCommand(0x92); // PTOUT

    // Sync OLD with what is now on the glass, so the next partial diffs against
    // the real screen contents.
    _sendPlane(0x10, false);
}

#endif // USE_GXEPD2
