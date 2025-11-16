# Cleanup script for the PMDG 737/777 SDK headers

This Python script will read the PMDG SDK header file and generate an eventnames file so you can send their custom events by name.

## Usage:

```
# Write to UTF-8 file without BOM
python cleanup.py PMDG_777X_SDK.h -o pmdg-777x-events.txt
python cleanup.py PMDG_NG3_SDK.h -o pmdg-ng3-events.txt

# Or print to stdout (original behavior)
python cleanup.py PMDG_777X_SDK.h

# Show help
python cleanup.py --help
```