# MechaTrellis serialosc extension

The RGB and 8-bit OSC commands require a matched host build. The patches in
this directory extend upstream libmonome v1.4.10 and serialosc v1.4.7 without
vendoring either project.

Build into `host/dist`:

```sh
./host/build_serialosc.sh
```

The build is isolated and does not replace the Homebrew installation or
restart its service. The resulting executables are under `host/dist/bin`.
See `../docs/mechatrellis-osc.md` for the protocol and OSC paths.

Run the OSC-to-serial integration test against that build:

```sh
python3 host/test_osc_serial.py
```

## macOS service activation

After building and testing, activate the custom executables as a user
LaunchAgent:

```sh
./host/activate_macos.sh
```

This stops the Homebrew `serialosc` service but does not overwrite or remove
it. The custom service uses the existing serialosc device configuration,
including its OSC prefix and port.

Return to the stock Homebrew service at any time:

```sh
./host/rollback_macos.sh
```

The Fruit Jam upload helper detects which serialosc service is active, releases
the serial port for the upload, and restores the same service afterward.
