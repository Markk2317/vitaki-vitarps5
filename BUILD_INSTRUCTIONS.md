# VitaRPS5 - Istruzioni di Compilazione e Deploy

## Prerequisiti

### Sistema Host
- **OS:** Linux (Ubuntu 20.04+) o macOS
- **Docker:** Installato e in esecuzione
- **Spazio Disco:** Almeno 10GB liberi
- **RAM:** Almeno 4GB disponibili

### Software Richiesto
```bash
# Ubuntu/Debian
sudo apt-get install -y docker.io git

# macOS
brew install docker git
```

---

## Opzione 1: Build con Docker (Consigliato)

### 1.1 Preparare l'Ambiente

```bash
cd /path/to/vitarps5_build

# Rendere eseguibile lo script di build
chmod +x tools/build.sh

# Verificare Docker
docker --version
```

### 1.2 Build Release VPK

```bash
# Build standard (production)
./tools/build.sh --env prod

# Build con debug symbols
./tools/build.sh --env testing debug

# Build e test
./tools/build.sh test
```

### 1.3 Output

Il VPK compilato sarà disponibile in:
```
build/vita/VitaRPS5.vpk
```

---

## Opzione 2: Build Manuale (Senza Docker)

### 2.1 Installare VitaSDK

```bash
# Seguire le istruzioni su https://vitasdk.org/
# Oppure usare Docker per evitare dipendenze

# Su Ubuntu:
sudo apt-get install -y vitasdk-toolchain

# Su macOS:
brew tap vitasdk/vitasdk
brew install vitasdk
```

### 2.2 Installare Dipendenze

```bash
# Nanopb
cd /tmp
wget https://github.com/nanopb/nanopb/archive/refs/tags/0.4.8.tar.gz
tar -xzf 0.4.8.tar.gz
cd nanopb-0.4.8
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
make && make install

# JSON-C
cd /tmp
wget https://github.com/json-c/json-c/archive/refs/tags/json-c-0.17-20230812.tar.gz
tar -xzf json-c-0.17-20230812.tar.gz
cd json-c-json-c-0.17-20230812
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
         -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF
make && make install
```

### 2.3 Compilare VitaRPS5

```bash
cd /path/to/vitarps5_build

# Creare directory di build
mkdir -p build
cd build

# Configurare CMake
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
  -DCHIAKI_ENABLE_VITA=ON \
  -DCHIAKI_ENABLE_VITA_HOLEPUNCH=ON \
  -DCMAKE_BUILD_TYPE=Release

# Compilare
make -j4

# Creare VPK
make vpk
```

---

## Opzione 3: Deploy su PS Vita

### 3.1 Via FTP

```bash
# Usare lo script di deploy
./tools/build.sh deploy 192.168.1.100

# Oppure manualmente:
# 1. Abilitare FTP su PS Vita (VitaShell)
# 2. Trasferire il VPK:
ftp -u ftp://vita_user:password@192.168.1.100:1337 build/vita/VitaRPS5.vpk
```

### 3.2 Via USB

```bash
# 1. Collegare PS Vita al PC via USB
# 2. Montare la memoria:
#    - Su PS Vita: Settings → USB Connection
# 3. Copiare il VPK:
cp build/vita/VitaRPS5.vpk /media/vita_memory/ux0:/download/

# 4. Su PS Vita:
#    - Aprire VitaShell
#    - Navigare a ux0:/download/
#    - Premere X su VitaRPS5.vpk
#    - Selezionare "Install"
```

### 3.3 Via VitaShell Direttamente

```bash
# Copiare il VPK in una cartella condivisa
cp build/vita/VitaRPS5.vpk ~/shared_folder/

# Su PS Vita:
# 1. Aprire VitaShell
# 2. Navigare alla cartella condivisa
# 3. Premere X su VitaRPS5.vpk
# 4. Selezionare "Install"
```

---

## Opzione 4: Build Personalizzato con Ottimizzazioni

### 4.1 Applicare le Correzioni Critiche

```bash
# Applicare il patch con le correzioni
patch -p1 < CRITICAL_FIXES.patch

# Ricompilare
./tools/build.sh --env prod
```

### 4.2 Configurare le Ottimizzazioni

Modificare `.env.prod`:

```bash
# Abilita menu debug
VITARPS5_DEBUG_MENU=1

# Profilo di log
VITARPS5_LOG_PROFILE=verbose

# Abilita PSN holepunch
CHIAKI_ENABLE_VITA_HOLEPUNCH=1

# Bitrate PSN adattivo (se implementato)
PSN_ADAPTIVE_BITRATE=1
```

### 4.3 Build Ottimizzato

```bash
./tools/build.sh --env prod
```

---

## Troubleshooting

### Problema: Docker non trovato

**Soluzione:**
```bash
# Installare Docker
sudo apt-get install docker.io

# Avviare il daemon
sudo systemctl start docker

# Aggiungere utente al gruppo docker
sudo usermod -aG docker $USER
newgrp docker
```

### Problema: Errore di compilazione CMake

**Soluzione:**
```bash
# Pulire la build precedente
./tools/build.sh clean

# Ricompilare
./tools/build.sh --env prod
```

### Problema: VPK non installa su PS Vita

**Soluzione:**
1. Verificare che il VPK sia valido:
   ```bash
   unzip -t build/vita/VitaRPS5.vpk
   ```

2. Verificare lo spazio disponibile su PS Vita:
   - Andare a Settings → Storage
   - Assicurarsi di avere almeno 100MB liberi

3. Provare a cancellare la versione precedente:
   - VitaShell → ux0:/app/VITARPS5
   - Premere Triangle → Delete

### Problema: App crasha all'avvio

**Soluzione:**
1. Verificare i log:
   ```bash
   # Su PS Vita, VitaShell:
   # Navigare a ux0:/data/vita-chiaki/
   # Leggere vitarps5-testing.log
   ```

2. Provare la build di debug:
   ```bash
   ./tools/build.sh --env testing debug
   ```

3. Verificare le dipendenze:
   ```bash
   # Assicurarsi che libvita2d, libchiaki, etc. siano installati
   ```

---

## Variabili di Ambiente Importanti

### .env.prod (Production)

```bash
# Versione
VERSION_PHASE="0.1"
VERSION_ITERATION="768"

# Logging
VITARPS5_LOG_ENABLED=0
VITARPS5_LOG_PROFILE=standard

# Features
VITARPS5_DEBUG_MENU=0
CHIAKI_ENABLE_VITA_HOLEPUNCH=1
```

### .env.testing (Testing)

```bash
# Versione
VERSION_PHASE="0.1"
VERSION_ITERATION="768"

# Logging
VITARPS5_LOG_ENABLED=1
VITARPS5_LOG_PROFILE=verbose

# Features
VITARPS5_DEBUG_MENU=1
CHIAKI_ENABLE_VITA_HOLEPUNCH=1
```

---

## Comandi Utili

### Pulire la Build

```bash
./tools/build.sh clean
```

### Formattare il Codice

```bash
./tools/build.sh format
```

### Eseguire Linter

```bash
./tools/build.sh lint
```

### Eseguire Test

```bash
./tools/build.sh test
```

### Mostrare la Versione

```bash
./tools/build.sh version
```

### Shell Interattivo

```bash
./tools/build.sh shell
```

---

## Performance Tuning

### Per Ridurre la Latenza

1. Applicare le correzioni critiche:
   ```bash
   patch -p1 < CRITICAL_FIXES.patch
   ```

2. Abilitare il profilo di latenza basso:
   - Su PS Vita: Settings → Video → Latency Mode → Ultra Low

3. Usare una connessione Wi-Fi 5GHz (se disponibile)

### Per Migliorare la Stabilità

1. Ridurre il bitrate:
   - Su PS Vita: Settings → Video → Bitrate → 3 Mbps

2. Usare la risoluzione 360p:
   - Su PS Vita: Settings → Video → Resolution → 360p

3. Disabilitare 60fps:
   - Su PS Vita: Settings → Video → Force 30 FPS → ON

---

## Distribuzione

### Creare una Release

```bash
# Build release
./tools/build.sh --env prod

# Creare un archivio
mkdir -p releases/v0.1.768
cp build/vita/VitaRPS5.vpk releases/v0.1.768/
cd releases/v0.1.768
sha256sum VitaRPS5.vpk > VitaRPS5.vpk.sha256
cd ../..

# Comprimere
tar -czf vitarps5-v0.1.768.tar.gz releases/v0.1.768/
```

### Pubblicare su GitHub

```bash
# Creare un tag
git tag -a v0.1.768 -m "VitaRPS5 v0.1.768 Release"
git push origin v0.1.768

# Caricare il VPK come release asset
# (Usare GitHub CLI o web interface)
```

---

## Supporto e Feedback

- **GitHub Issues:** https://github.com/mauricio-gg/vitaki-vitarps5/issues
- **Discussioni:** https://github.com/mauricio-gg/vitaki-vitarps5/discussions
- **Wiki:** https://github.com/mauricio-gg/vitaki-vitarps5/wiki

---

**Ultima Aggiornamento:** 12 Giugno 2026  
**Versione:** v0.1.768
