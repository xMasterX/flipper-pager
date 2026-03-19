# POCSAG Pager TRX — Receive & Transmit for Flipper Zero

Fork of [xMasterX/flipper-pager](https://github.com/xMasterX/flipper-pager) with **POCSAG transmit** capability.

The original application only supports receiving POCSAG messages. This fork adds a full transmit feature while preserving all original receive functionality.

## What's New

- **Transmit message** — send POCSAG messages to pagers and other receivers
- On-screen keyboard for entering RIC (address) and message text
- Configurable TX frequency (439.9875, 439.9800, 433.9200, 433.8750, 446.0000 MHz)
- Transmission at 1200 baud with 2-FSK modulation
- Visual TX indicator with animation
- Improved message detail view — RIC info line no longer overlaps message text
- Memory safety fix — `memset` zeroing after `malloc` for radio state structs

## Compatibility

- **Firmware**: [Unleashed](https://github.com/DarkFlippers/unleashed-firmware) (tested on unlshd-085e and newer)
- **Hardware**: Flipper Zero with internal CC1101 or external CC1101 module
- **Protocols**: POCSAG 512 / 1200 / 2400 (receive), POCSAG 1200 (transmit)

## Building

### Prerequisites

- Python 3
- [ufbt](https://pypi.org/project/ufbt/) (micro Flipper Build Tool)

### Build Steps

```bash
# Install ufbt
python3 -m venv ~/ufbt-env
~/ufbt-env/bin/pip install ufbt

# Set SDK to match your firmware
# For Unleashed:
~/ufbt-env/bin/ufbt update -c release --index-url https://up.unleashedflip.com/directory.json

# For Official firmware:
# ~/ufbt-env/bin/ufbt update -c release

# Clone and build
git clone https://github.com/shchuchkin-pkims/flipper-pager-trx.git
cd flipper-pager-trx/pocsag_pager
~/ufbt-env/bin/ufbt

# Install on Flipper (connected via USB)
~/ufbt-env/bin/ufbt launch
```

The compiled `.fap` file is placed in `dist/pocsag_pager_trx.fap`. It can also be manually copied to the Flipper's SD card at `/ext/apps/Sub-GHz/`.

## Usage

### Receiving Messages

1. Open **POCSAG Pager TRX** from Applications → Sub-GHz
2. Select **Receive messages**
3. Press Left to open **Config** — set frequency and modulation
4. Received messages appear in a scrollable list
5. Press OK on a message to view full text

### Transmitting Messages

1. Select **Transmit message**
2. Set **RIC** — tap to enter the pager address (0–2097151)
3. Set **Message** — tap to enter text (ASCII)
4. Set **Freq MHz** — scroll left/right to select frequency
5. Select **>> SEND <<** to transmit

### Adding Custom Frequencies

Create `/ext/pocsag/settings.txt` on the Flipper's SD card:

```
Filetype: Flipper SubGhz Setting File
Version: 1
Add_standard_frequencies: true
Frequency: 439987500
```

Additional `Frequency:` lines can be added for more custom frequencies.

## Technical Details

### Transmit Implementation

- POCSAG encoding: 576-bit preamble, BCH(31,21) error correction, 7-bit reversed ASCII
- Modulation: 2-FSK via Flipper's `subghz_devices_start_async_tx` API
- Preset: `FuriHalSubGhzPreset2FSKDev238Async` (built-in, ~2.38 kHz deviation)
- Polarity: inverted for CC1101 compatibility with standard POCSAG receivers
- Multi-batch support for long messages

### Changes from Original

All changes are additive — no existing functionality was modified or removed.

**New files:**

| File | Purpose |
|------|---------|
| `pocsag_pager_tx.c` | POCSAG protocol encoder (BCH, codewords, batches) |
| `pocsag_pager_tx.h` | Encoder API header |
| `scenes/pocsag_pager_scene_transmit.c` | Transmit configuration scene |
| `scenes/pocsag_pager_scene_transmit_input.c` | Text input scene for RIC/message |

**Modified files:**

| File | Change |
|------|--------|
| `application.fam` | Updated app name, version, description |
| `pocsag_pager_app.c` | Added TextInput view, TX state init, memory safety |
| `pocsag_pager_app_i.h` | Added TextInput and TX fields to app struct |
| `helpers/pocsag_pager_types.h` | Added TextInput view enum, updated credits |
| `helpers/pocsag_pager_event.h` | Added TX custom events |
| `scenes/pocsag_pager_scene_config.h` | Registered Transmit and TransmitInput scenes |
| `scenes/pocsag_pager_scene_start.c` | Added "Transmit message" menu item |
| `scenes/pocsag_pager_scene_about.c` | Added TX description and contributor info |
| `protocols/pocsag.c` | Shortened RIC display to fit screen width |
| `views/pocsag_pager_receiver_info.c` | Adjusted message text offset to prevent overlap |
| `views/pocsag_pager_receiver.c` | Updated icon header reference |

### Memory Safety Fix

This fork adds `memset` zeroing for `POCSAGPagerTxRx` and `SubGhzRadioPreset` structures after `malloc`. The original code relies on `malloc` returning zeroed memory, which is not guaranteed by the C standard. Without this fix, uninitialized fields (`rx_key_state`, `txrx_state`, `hopper_state`) may cause the receiver to silently fail on some firmware versions or memory layouts.

## Credits

- **@xMasterX** & **@Shmuma** — original POCSAG Pager application
- **@htotoo** — protocol improvements (512/2400 baud support)
- **@Svaarich** — icons
- **Shchuchkin Evgenii Yurievich** — transmit feature

## License

GPL-3.0 — same as the [original project](https://github.com/xMasterX/flipper-pager).
