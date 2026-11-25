# 1_basics_framework

PlatformIO project for the ESP32 "basics framework" assignment. It uses the
[pioarduino](https://github.com/pioarduino/platform-espressif32) fork of the
`espressif32` platform (Arduino-ESP32 3.x / ESP-IDF 5.x) instead of the
official platform, because only this generation builds FreeRTOS from source
via the ESP-IDF component system. That is required for `sdkconfig.defaults`
to actually enable the trace facility
(`uxTaskGetSystemState`/`vTaskList`), since the official platform's
precompiled 2.0.x FreeRTOS library never exports those symbols.

## Troubleshooting

### `IncompatiblePlatform` error when building

```
IncompatiblePlatform: Development platform 'espressif32' is not compatible with
PlatformIO Core v6.2.0-post.1 and depends on PlatformIO Core >=6.2.0.
```

**Cause:** This is not an issue in `platformio.ini`. The `platform-espressif32`
package (both the pioarduino fork and the official platform) declares a
dependency on `PlatformIO Core >= 6.2.0`. PlatformIO's VS Code extension runs
its own Core install in a dedicated virtual environment
(`%USERPROFILE%\.platformio\penv` on Windows). In this environment the
installed `platformio` package can end up patched to report itself as
`6.2.0.post1` (visible via `pio --version`), even though `pip show platformio`
still lists `6.2.0`. PlatformIO treats the `-post1` suffix as a *pre-release*
tag, so `6.2.0.post1` compares as **lower** than `6.2.0`, which fails the
`>=6.2.0` platform requirement — this is what produces the
`IncompatiblePlatform` error.

A plain `pip install -U platformio==6.2.0` (see the comment in
`platformio.ini`) does **not** fix this, because pip sees the installed
version metadata already matches `6.2.0` and skips reinstalling, leaving the
patched `6.2.0.post1` runtime version in place.

**Fix:** Force a clean reinstall of the exact `platformio` package from PyPI
in PlatformIO's own virtual environment, bypassing the stale cached files:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m pip install --force-reinstall --no-cache-dir --no-deps platformio==6.2.0
```

Verify the fix:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" --version
# Expect: PlatformIO Core, version 6.2.0  (no ".post1" suffix)
```

Then rebuild the project:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e basics_framework
```

### Linux/macOS equivalent

```bash
~/.platformio/penv/bin/python -m pip install --force-reinstall --no-cache-dir --no-deps platformio==6.2.0

# Verify the fix
~/.platformio/penv/bin/pio --version
# Expect: PlatformIO Core, version 6.2.0  (no ".post1" suffix)

# Rebuild the project
~/.platformio/penv/bin/pio run -e basics_framework
```
