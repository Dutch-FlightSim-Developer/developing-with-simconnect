# Cleanup script for the PMDG 737/777 SDK headers

This Python script reads a PMDG SDK header file and extracts its EVT_*
(and related THIRD_PARTY_EVENT_ID_MIN / CDU_EVT_OFFSET_*) constants. It
supports two output formats:

- **txt** (default): flat `NAME value` lines, decimal — the format 8-2's
  own `main.cpp` loads via `--names` (`loadEventNames`). This keeps the
  8-2 example working unchanged.
- **json**: an msfs-events-style catalog (same shape as
  `msfs-events/2020/*.json` and `msfs-events/2024/*.json`), so PMDG's
  custom events sit in the same name -> eventId -> params -> description
  structure as the MSFS ones. `eventId` is emitted as `"#<number>"`
  (SimConnect's numeric-event-name convention), not a `KEY_*` string.

Format is picked automatically from the `-o` extension (`.json` -> json,
anything else -> txt), or forced with `--format`.

Header files come from the aircraft's Community folder and are not
guaranteed identical between the 2020 and 2024 SDK/aircraft, so for JSON
output keep files split into the matching `msfs-events/2020/` or
`msfs-events/2024/` folder, one file per aircraft (e.g. `pmdg-737.json`).

The header does not reliably declare parameter types or descriptions, so
JSON output always has `"params": []` and `"description": ""` — extend
those by hand afterward. Where a param exists, its value is always an
unsigned int (`DWORD`) on the wire.

## Usage:

```
# Flat txt for 8-2's --names loader (UTF-8, no BOM)
python cleanup.py PMDG_NG3_SDK.h -o PMDG-737_events.txt
python cleanup.py PMDG_777X_SDK.h -o PMDG-777_events.txt

# JSON catalog, into the matching SDK-year folder
python cleanup.py PMDG_NG3_SDK.h -o ../../../msfs-events/2024/pmdg-737.json
python cleanup.py PMDG_777X_SDK.h -o ../../../msfs-events/2024/pmdg-777.json

# Override the "source"/"category" fields explicitly
python cleanup.py PMDG_NG3_SDK.h -o pmdg-737.json --source PMDG_737_SDK --category pmdg-737

# Force a format regardless of extension
python cleanup.py PMDG_NG3_SDK.h --format json

# Or print to stdout (default txt)
python cleanup.py PMDG_NG3_SDK.h

# Show help
python cleanup.py --help
```
