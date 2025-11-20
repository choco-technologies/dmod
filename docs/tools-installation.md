# Instrukcja narzędzi DMOD (DMOD Tools Installation Guide)

## Przegląd (Overview)

Framework **DMOD (Dynamic Modules)** jest instalowany razem z zestawem narzędzi wiersza poleceń, które wspierają proces budowania, zarządzania i dystrybucji modułów dynamicznych. Narzędzia te są szczególnie przydatne w procesie budowania projektów z użyciem [dmod-boot](https://github.com/choco-technologies/dmod-boot).

## Dostępne narzędzia (Available Tools)

| Narzędzie | Opis | Dokumentacja |
|-----------|------|--------------|
| **dmf-get** | Menedżer pakietów DMOD do pobierania i zarządzania modułami z plików manifestu | [dmf-get-tool.md](dmf-get-tool.md) |
| **todmfc** | Narzędzie do kompresji plików DMF do formatu DMFC | [Brak dedykowanej dokumentacji](#todmfc) |
| **todmp** | Kreator pakietów DMP - tworzy pakiety zawierające wiele modułów | [Brak dedykowanej dokumentacji](#todmp) |
| **todmd** | Generator plików zależności DMD - wyodrębnia zależności z modułów | [todmd/README.md](../tools/system/todmd/README.md) |
| **whereisdmf** | Narzędzie do lokalizacji plików modułów w skonfigurowanych katalogach repozytorium | [whereisdmf/README.md](../tools/system/whereisdmf/README.md) |

## Zmienne środowiskowe (Environment Variables)

Narzędzia DMOD wykorzystują następujące zmienne środowiskowe do konfiguracji:

| Zmienna | Używane przez | Opis | Domyślna wartość |
|---------|---------------|------|------------------|
| **DMOD_TOOLS_NAME** | dmf-get | Nazwa zestawu narzędzi do podstawienia w URL manifestu (np. `arch/x86_64`, `arch/armv7/cortex-m7`) | `arch/x86_64` |
| **DMOD_DMF_DIR** | dmf-get | Katalog wyjściowy dla plików DMF | `./dmf` |
| **DMOD_DMFC_DIR** | dmf-get | Katalog wyjściowy dla plików DMFC | `./dmfc` |
| **DMOD_MANIFEST** | dmf-get | Domyślna ścieżka lub URL do pliku manifestu | - |
| **DMOD_REPO_DIR** | Wszystkie | Katalog repozytorium modułów używany podczas instalacji | Zależny od systemu |

## Budowanie projektu z narzędziami (Building the Project with Tools)

### Wymagania (Requirements)

Przed rozpoczęciem budowania upewnij się, że masz zainstalowane:

- **Kompilator GCC** lub kompatybilny
- **CMake** w wersji 3.18 lub nowszej (lub Make 4.2+)
- **libcurl** (wymagane dla dmf-get)

#### Instalacja zależności na Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev
```

#### Instalacja zależności na innych systemach Linux:

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install gcc gcc-c++ cmake libcurl-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake curl
```

### Budowanie z CMake

1. **Sklonuj repozytorium:**
```bash
git clone https://github.com/choco-technologies/dmod.git
cd dmod
git submodule update --init --recursive
```

2. **Skonfiguruj projekt w trybie SYSTEM z włączonymi narzędziami:**
```bash
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
```

3. **Zbuduj projekt:**
```bash
cmake --build build/
```

Po pomyślnym zbudowaniu, narzędzia znajdą się w katalogu `build/bin/tools/`.

### Budowanie z Make

1. **Sklonuj repozytorium:**
```bash
git clone https://github.com/choco-technologies/dmod.git
cd dmod
git submodule update --init --recursive
```

2. **Przejdź do katalogu wybranego narzędzia i zbuduj:**
```bash
cd tools/system/dmf-get
make
```

Lub zbuduj wszystkie narzędzia z głównego katalogu:
```bash
make -C tools/system/dmf-get
make -C tools/system/todmfc
make -C tools/system/todmp
make -C tools/system/todmd
make -C tools/system/whereisdmf
```

## Instalacja narzędzi w systemie Linux (Installing Tools on Linux)

### Instalacja z CMake

Po zbudowaniu projektu możesz zainstalować narzędzia w systemie:

```bash
sudo cmake --install build/ --component tools
```

Domyślnie narzędzia zostaną zainstalowane w `/usr/local/bin`. Możesz zmienić prefix instalacji:

```bash
sudo cmake --install build/ --component tools --prefix /custom/path
```

Po instalacji narzędzia będą dostępne z każdego miejsca w systemie:

```bash
dmf-get --version
todmfc --help
whereisdmf mymodule
```

### Instalacja z Make

Aby zainstalować poszczególne narzędzie:

```bash
cd tools/system/dmf-get
sudo make install
```

Instalacja do niestandardowego katalogu:

```bash
sudo make install INSTALL_PREFIX=/custom/path
```

### Dezinstalacja

Aby odinstalować narzędzie zainstalowane za pomocą Make:

```bash
cd tools/system/dmf-get
sudo make uninstall
```

Dla narzędzi zainstalowanych przez CMake, usuń pliki ręcznie:

```bash
sudo rm /usr/local/bin/dmf-get
sudo rm /usr/local/bin/todmfc
sudo rm /usr/local/bin/todmp
sudo rm /usr/local/bin/todmd
sudo rm /usr/local/bin/whereisdmf
```

## Szczegóły narzędzi (Tool Details)

### dmf-get

**dmf-get** to menedżer pakietów dla DMOD, umożliwiający pobieranie modułów z plików manifestu (.dmm) oraz zarządzanie zależnościami.

**Podstawowe użycie:**
```bash
# Pobierz najnowszą wersję modułu
dmf-get mymodule

# Pobierz konkretną wersję
dmf-get mymodule@1.0

# Pobierz wersję spełniającą warunek
dmf-get mymodule@>=1.0

# Pobierz wszystkie moduły z pliku zależności
dmf-get -d dependencies.dmd

# Użyj niestandardowego manifestu
dmf-get -m http://example.com/manifest.dmm mymodule
```

Pełna dokumentacja: [dmf-get-tool.md](dmf-get-tool.md)

### todmfc

**todmfc** kompresuje pliki DMF do formatu DMFC, zmniejszając ich rozmiar.

**Podstawowe użycie:**
```bash
# Kompresuj z domyślnymi ustawieniami (fastlz, poziom 2)
todmfc input.dmf output.dmfc

# Określ metodę kompresji i poziom
todmfc input.dmf output.dmfc fastlz 3

# Wyświetl dostępne metody kompresji
todmfc --help
```

**Parametry:**
- `path/to/file.dmf` - Plik wejściowy DMF
- `path/to/output.dmfc` - Plik wyjściowy DMFC
- `[compression_method]` - Opcjonalna metoda kompresji (domyślnie: fastlz)
- `[level]` - Opcjonalny poziom kompresji (domyślnie: 2)

### todmp

**todmp** tworzy pakiety DMP, które mogą zawierać wiele modułów DMF lub DMFC i być ładowane razem.

**Podstawowe użycie:**
```bash
# Utwórz pakiet z modułów w katalogu
todmp mypackage ./modules

# Określ plik wyjściowy i główny moduł
todmp kernel ./dmfc main-app ./out/kernel.dmp

# Wyświetl zawartość pakietu DMP
todmp -l ./mypackage.dmp
```

**Parametry:**
- `<package_name>` - Nazwa pakietu (dla nagłówka)
- `<input_dir>` - Katalog z modułami do spakowania (.dmf lub .dmfc)
- `[output_file]` - Opcjonalna ścieżka do pliku wyjściowego .dmp
- `[module_name]` - Opcjonalna nazwa głównego modułu w pakiecie

### todmd

**todmd** czyta zależności modułu i generuje plik .dmd, który może być użyty z dmf-get do pobrania wszystkich wymaganych modułów.

**Podstawowe użycie:**
```bash
# Generuj plik .dmd z domyślną nazwą
todmd myapp.dmf

# Określ niestandardową nazwę pliku wyjściowego
todmd myapp.dmf custom_deps.dmd
```

**Cechy:**
- Ładuje moduły DMF w trybie wieloplatformowym (bez wykonania)
- Wyodrębnia wymagane zależności modułu
- Automatycznie filtruje moduły systemowe
- Generuje pliki .dmd kompatybilne z dmf-get

Pełna dokumentacja: [todmd/README.md](../tools/system/todmd/README.md)

### whereisdmf

**whereisdmf** lokalizuje pliki modułów DMOD w skonfigurowanych katalogach repozytorium.

**Podstawowe użycie:**
```bash
# Znajdź moduł dla bieżącej architektury
whereisdmf mymodule

# Znajdź moduł dla konkretnej architektury
whereisdmf mymodule x86_64
whereisdmf mymodule armv7-cortex-m7
```

**Cechy:**
- Używa DMOD API (`Dmod_FindModuleFile`) do wyszukiwania
- Przydatne w skryptach i automatyzacji
- Pomaga w debugowaniu problemów z lokalizacją modułów
- Weryfikuje instalacje modułów

Pełna dokumentacja: [whereisdmf/README.md](../tools/system/whereisdmf/README.md)

## Obraz Docker (Docker Image)

Framework DMOD jest dostępny jako gotowy obraz Docker, który zawiera wszystkie narzędzia DMOD oraz środowisko kompilacji dla systemów embedded.

### Podstawowe informacje

- **Nazwa obrazu:** `chocotechnologies/dmod`
- **Aktualna wersja:** `1.0.4`
- **Platforma bazowa:** Ubuntu 20.04
- **Repozytorium:** [Docker Hub](https://hub.docker.com/r/chocotechnologies/dmod)

### Zawartość obrazu

Obraz Docker zawiera:

- **Narzędzia DMOD:** dmf-get, todmfc, todmp, todmd, whereisdmf
- **Kompilator:** GCC arm-none-eabi (wersja 10.3-2021.10)
- **System budowania:** CMake (wersja 3.31.3), Make
- **Narzędzia deweloperskie:** OpenOCD, gcovr, git, jq, zip/unzip
- **Biblioteki:** libcurl, libusb
- **Skonfigurowane zmienne środowiskowe:**
  - `DMOD_DMF_DIR=/tools/dmf`
  - `DMOD_DMFC_DIR=/tools/dmfc`
  - `PATH` zawiera `/usr/local/bin` z narzędziami DMOD

### Użycie obrazu Docker

**Pobierz obraz:**
```bash
docker pull chocotechnologies/dmod:1.0.4
```

**Uruchom kontener interaktywnie:**
```bash
docker run -it chocotechnologies/dmod:1.0.4 bash
```

**Użyj narzędzi DMOD z kontenera:**
```bash
# Uruchom dmf-get w kontenerze
docker run --rm chocotechnologies/dmod:1.0.4 dmf-get --version

# Montuj lokalny katalog i pobierz moduły
docker run --rm -v $(pwd):/workspace -w /workspace \
    chocotechnologies/dmod:1.0.4 dmf-get mymodule
```

**Zbuduj projekt w kontenerze:**
```bash
# Montuj katalog projektu i zbuduj
docker run --rm -v $(pwd):/project -w /project \
    chocotechnologies/dmod:1.0.4 bash -c "cmake -B build && cmake --build build"
```

### Dostosowanie obrazu

Możesz rozszerzyć obraz Docker o dodatkowe narzędzia:

```dockerfile
FROM chocotechnologies/dmod:1.0.4

# Dodaj własne narzędzia lub konfigurację
RUN apt-get update && apt-get install -y your-package

# Ustaw niestandardowe zmienne środowiskowe
ENV MY_CUSTOM_VAR=value

WORKDIR /workspace
```

## Typowy workflow z narzędziami (Typical Workflow)

### 1. Przygotowanie środowiska

```bash
# Opcja A: Użyj Docker
docker pull chocotechnologies/dmod:1.0.4
docker run -it -v $(pwd):/workspace -w /workspace chocotechnologies/dmod:1.0.4 bash

# Opcja B: Zainstaluj narzędzia lokalnie
git clone https://github.com/choco-technologies/dmod.git
cd dmod
cmake -DDMOD_MODE=DMOD_SYSTEM -DDMOD_BUILD_TOOLS=ON -B build -S .
cmake --build build/
sudo cmake --install build/ --component tools
```

### 2. Pobieranie modułów

```bash
# Ustaw zmienne środowiskowe
export DMOD_TOOLS_NAME=arch/armv7/cortex-m7
export DMOD_DMF_DIR=./modules/dmf
export DMOD_DMFC_DIR=./modules/dmfc

# Pobierz moduły z manifestu
dmf-get mymodule@>=1.0
dmf-get -d dependencies.dmd
```

### 3. Kompresja modułów

```bash
# Skompresuj moduły do zmniejszenia rozmiaru
todmfc ./modules/dmf/mymodule.dmf ./modules/dmfc/mymodule.dmfc
```

### 4. Tworzenie pakietów

```bash
# Utwórz pakiet DMP z wielu modułów
todmp mypackage ./modules/dmfc ./output/mypackage.dmp main_module
```

### 5. Weryfikacja

```bash
# Sprawdź lokalizację modułu
whereisdmf mymodule

# Wyświetl zawartość pakietu
todmp -l ./output/mypackage.dmp

# Wygeneruj plik zależności z modułu
todmd mymodule.dmf dependencies.dmd
```

## Rozwiązywanie problemów (Troubleshooting)

### Błąd: "libcurl not found"

**Problem:** CMake nie może znaleźć biblioteki libcurl podczas budowania.

**Rozwiązanie:**
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# Fedora/RHEL
sudo dnf install libcurl-devel
```

### Błąd: "Module not found" w whereisdmf

**Problem:** whereisdmf nie może znaleźć modułu.

**Rozwiązanie:**
1. Sprawdź czy moduł istnieje w katalogach `DMOD_DMF_DIR` lub `DMOD_DMFC_DIR`
2. Upewnij się, że zmienne środowiskowe są poprawnie ustawione
3. Sprawdź czy architektura się zgadza

### Błąd: Narzędzia nie są dostępne po instalacji

**Problem:** Po instalacji nie można uruchomić narzędzi.

**Rozwiązanie:**
1. Sprawdź czy katalog instalacji jest w PATH:
```bash
echo $PATH | grep -o "/usr/local/bin"
```

2. Jeśli nie, dodaj do PATH:
```bash
export PATH=$PATH:/usr/local/bin
# Dodaj do ~/.bashrc lub ~/.zshrc aby utrwalić
```

## Dodatkowe zasoby (Additional Resources)

- **Główne repozytorium:** [github.com/choco-technologies/dmod](https://github.com/choco-technologies/dmod)
- **DMOD Boot:** [github.com/choco-technologies/dmod-boot](https://github.com/choco-technologies/dmod-boot)
- **Dokumentacja formatu DMF:** [dmd-file-format.md](dmd-file-format.md)
- **Dokumentacja formatu DMM:** [dmm-file-format.md](dmm-file-format.md)
- **Docker Hub:** [hub.docker.com/r/chocotechnologies/dmod](https://hub.docker.com/r/chocotechnologies/dmod)

## Licencja (License)

DMOD jest dostępny na licencji MIT. Zobacz [license.md](../license.md) dla szczegółów.
