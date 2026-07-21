# MechaTrellis RGB and 8-bit OSC extension

This is a private extension shared by the dinkii NeoTrellis firmware, the
patched libmonome host library, and the patched serialosc host process. It is
not part of the monome OSC or mext standards.

## OSC commands

All arguments are OSC integers. Color, level, and intensity inputs are clamped
to the range 0 through 255 by serialosc.

| OSC path | Arguments | Description |
| --- | --- | --- |
| `/<prefix>/grid/led/rgb/set` | `x y r g b` | Set one cell to an RGB color. |
| `/<prefix>/grid/led/rgb/all` | `r g b` | Set every cell to an RGB color. |
| `/<prefix>/grid/led/level8/set` | `x y level` | Set one cell to an 8-bit level using the configured legacy color. |
| `/<prefix>/grid/led/level8/all` | `level` | Set every cell to an 8-bit level using the configured legacy color. |
| `/<prefix>/grid/led/intensity8` | `intensity` | Set the global 8-bit intensity multiplier. |
| `/<prefix>/grid/led/color/set` | `x y r g b` | Set one cell's persistent base color without changing its current level. |
| `/<prefix>/grid/led/color/all` | `r g b` | Set every cell's persistent base color without changing current levels. |
| `/<prefix>/grid/led/color/preset/store` | `slot` | Store the current base-color layer in RAM slot 0–7. |
| `/<prefix>/grid/led/color/preset/recall` | `slot` | Recall RAM slot 0–7 without changing LED levels. |

The standard monome OSC paths remain supported. A standard
`grid/led/intensity` value of 0 through 15 maps to 0 through 255 in steps of 17.

Examples for a device using the `/monome` prefix and port 12345:

```sh
oscsend localhost 12345 /monome/grid/led/rgb/set iiiii 3 4 255 64 0
oscsend localhost 12345 /monome/grid/led/level8/all i 128
oscsend localhost 12345 /monome/grid/led/intensity8 i 96
```

The prefix and device port come from serialosc's existing configuration. For
example, a device configured with the `/box` prefix must use
`/box/grid/led/rgb/set`, not `/monome/grid/led/rgb/set`.

### Using colors with an unchanged legacy application

Use `color/set` or `color/all` to initialize the grid's color layer, then let
the legacy application continue sending its normal monome LED commands. Legacy
on/off, map, row, column, intensity, and 4-bit level messages change brightness
without erasing the assigned colors. For example:

```sh
oscsend localhost 12237 /box/grid/led/color/all iii 0 80 255
oscsend localhost 12237 /box/grid/led/color/set iiiii 0 0 255 0 80
```

Color assignments remain in RAM through legacy clear commands, but reset to
the firmware's configured default color after a device reset or power cycle.

### Color presets

Eight firmware-side slots avoid retransmitting a full 256-cell palette on
every application page change. Build a color layer with `color/set` and
`color/all`, store it, then recall it with one OSC message:

```sh
oscsend localhost 12237 /box/grid/led/color/preset/store i 0
oscsend localhost 12237 /box/grid/led/color/preset/recall i 0
```

Presets contain RGB base colors only. Recall does not change cell levels,
on/off state, LED modes, animations, or global intensity. Slots are volatile:
all eight become invalid after reset or power loss and must be populated again.
Recalling an invalid slot has no effect.

## Serial packets

The extension reserves mext subsystem `0xA`, which is unused by mext 1.x.
Every field below is one unsigned byte and packets have no reply.

| ID | Packet bytes | Description |
| --- | --- | --- |
| `0xA0` | `A0 x y r g b` | RGB set |
| `0xA1` | `A1 r g b` | RGB all |
| `0xA2` | `A2 x y level` | 8-bit level set |
| `0xA3` | `A3 level` | 8-bit level all |
| `0xA4` | `A4 intensity` | 8-bit global intensity |
| `0xA5` | `A5 x y r g b` | Persistent base color set |
| `0xA6` | `A6 r g b` | Persistent base color all |
| `0xA7` | `A7 slot` | Store base colors in RAM slot 0–7 |
| `0xA8` | `A8 slot` | Recall base colors from RAM slot 0–7 |

## Brightness and compatibility

Eight-bit values provide additional resolution. They do not bypass the
firmware's `BRIGHTNESS` limit, which remains the final power-safety ceiling.
Sending an RGB command also updates the affected cell's base color. Subsequent
legacy commands use that color while controlling the cell's 4-bit brightness.
The color-only commands are preferable when an existing application should
retain complete control of LED state and brightness.

Bulk RGB and level8 map commands are intentionally deferred until animation
throughput has been measured on hardware.
