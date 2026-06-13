# AGENTS.md

## Build

This is an ESP-IDF project for an ESP32-S3 target.

Preferred build command from the repo root:

```powershell
idf.py build
```

If the default ESP-IDF shell is not already configured, use the installed ESP-IDF 5.4.1 environment:

```powershell
$env:IDF_PATH='C:\Users\spwal\esp\v5.4.1\esp-idf'
$env:IDF_PYTHON_ENV_PATH='C:\Users\spwal\.espressif\python_env\idf5.4_py3.11_env'
$env:ESP_ROM_ELF_DIR='C:\Users\spwal\.espressif\tools\esp-rom-elfs\20241011\'
$env:PATH='C:\Users\spwal\.espressif\python_env\idf5.4_py3.11_env\Scripts;C:\Users\spwal\.espressif\tools\ninja\1.12.1;C:\Users\spwal\.espressif\tools\cmake\3.30.2\bin;C:\Users\spwal\.espressif\tools\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin;' + $env:PATH
& "$env:IDF_PYTHON_ENV_PATH\Scripts\python.exe" "$env:IDF_PATH\tools\idf.py" build
```

The project lock file pins ESP-IDF `5.4.1`, so prefer the `C:\Users\spwal\esp\v5.4.1\esp-idf` install over the older `C:\Users\spwal\esp\esp-idf` install.

