# Naprawienie błędu "argument list too long" dla ESP32

## Problem

Podczas budowania modułów dla ESP32, może pojawić się błąd:

```
xtensa-esp-elf-gcc: fatal error: cannot execute '.../cc1': posix_spawn: Argument list too long
```

### Przyczyna

Błąd ten pojawia się, gdy lista argumentów przekazywanych do kompilatora (flagi `-I` dla include directories) przekracza limit systemu operacyjnego `ARG_MAX`. Wcześniej, kod CMake rekursywnie zbierał **setki** katalogów include z całego ESP-IDF i dodawał je do linii komend kompilatora.

## Rozwiązanie

### Zmiany zastosowane w DMOD:

1. **Ograniczenie include directories** w `configs/arch/xtensa/esp32s3/tools-cfg.cmake`:
   - Zamiast zbierać ALL include directories z ESP-IDF, teraz dodajemy tylko **niezbędne** katalogi
   - Zmniejsza to liczbę flag `-I` z setków do zaledwie ~25

2. **Włączenie Response Files** w `CMakeLists.txt`:
   - CMake automatycznie używa response files (`@...txt`) dla bardzo długich linii komend
   - Pozwala to na bezpieczne przekazanie nawet bardzo długich list argumentów

## Weryfikacja poprawki

Aby sprawdzić, czy poprawka działa:

```bash
cd /path/to/dmod
cmake -S . -B build_esp32s3 \
  -DDMOD_TOOLS=/path/to/configs/arch/xtensa/esp32s3/tools-cfg.cmake \
  -DDMOD_MODE=DMOD_MODULE \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build_esp32s3
```

Jeśli budowanie powiodło się bez błędu "argument list too long", poprawka działa!

## Dodawanie nowych include directories

Jeśli w przyszłości będziesz potrzebować dodać więcej include directories do ESP-IDF:

1. Otwórz `configs/arch/xtensa/esp32s3/tools-cfg.cmake`
2. Dodaj katalog do listy `ESP_IDF_INCLUDE_DIRS`:

```cmake
set(ESP_IDF_INCLUDE_DIRS
    # ... istniejące katalogi ...
    "${IDF_PATH}/components/my_component/include"
)
```

3. Pamiętaj aby dodawać **tylko katalogi `/include`**, nie całe ścieżki komponentu

## Kompatybilność

Ta poprawka jest kompatybilna z:
- ESP-IDF v5.2.2 i nowszymi
- Xtensa ESP toolchain 14.2.0 i nowszymi
- CMake 3.18 i nowszymi
