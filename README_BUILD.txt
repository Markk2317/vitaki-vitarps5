================================================================================
VitaRPS5 v0.1.768 - CODICE SORGENTE CORRETTO E OTTIMIZZATO
================================================================================

CORREZIONI APPLICATE:
✅ Thread safety: frame_ready_for_display usa _Atomic(bool) invece di volatile
✅ Validazione PSN: Controllo della lunghezza dell'indirizzo (7-45 caratteri)
✅ Include corretti: Aggiunto #include <stdatomic.h>

FILE MODIFICATI:
- vita/src/video.c (linea 28, 75)
- vita/src/psn_remote.c (linea 323-334)

================================================================================
COMPILAZIONE RAPIDA
================================================================================

OPZIONE 1: Con Docker (Consigliato)
-----------------------------------
1. Installare Docker: https://docs.docker.com/install/
2. Eseguire:
   cd vitarps5_build
   chmod +x tools/build.sh
   ./tools/build.sh --env prod

Output: build/vita/VitaRPS5.vpk

OPZIONE 2: Manuale (senza Docker)
----------------------------------
1. Installare VitaSDK: https://vitasdk.org/
2. Installare dipendenze (nanopb, json-c)
3. Eseguire:
   mkdir -p build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake \
            -DCHIAKI_ENABLE_VITA=ON \
            -DCHIAKI_ENABLE_VITA_HOLEPUNCH=ON \
            -DCMAKE_BUILD_TYPE=Release
   make -j4
   make vpk

Output: build/vita/VitaRPS5.vpk

================================================================================
DEPLOY SU PS VITA
================================================================================

METODO 1: FTP
-------------
./tools/build.sh deploy 192.168.1.100

METODO 2: USB
-------------
1. Collegare PS Vita via USB
2. Abilitare USB Connection su PS Vita
3. Copiare: build/vita/VitaRPS5.vpk → ux0:/download/
4. Su PS Vita: VitaShell → Installare VPK

METODO 3: VitaShell
-------------------
1. Copiare VPK in cartella condivisa
2. Su PS Vita: VitaShell → Navigare → Installare

================================================================================
COMANDI UTILI
================================================================================

Pulire build:
  ./tools/build.sh clean

Formattare codice:
  ./tools/build.sh format

Eseguire linter:
  ./tools/build.sh lint

Eseguire test:
  ./tools/build.sh test

Shell interattivo:
  ./tools/build.sh shell

Mostrare versione:
  ./tools/build.sh version

================================================================================
DOCUMENTAZIONE
================================================================================

Consulta i seguenti file per informazioni dettagliate:

- CODE_ANALYSIS_REPORT.md
  Analisi completa del codice, problemi trovati e soluzioni

- BUILD_INSTRUCTIONS.md
  Guida dettagliata di compilazione e deploy

- PSN_NETWORK_OPTIMIZATIONS.md
  Ottimizzazioni di rete e troubleshooting

- CRITICAL_FIXES.patch
  Patch con le correzioni critiche

================================================================================
TROUBLESHOOTING
================================================================================

Errore: "Docker is not running"
Soluzione: Installare Docker e avviare il daemon
  sudo systemctl start docker

Errore: "CMake not found"
Soluzione: Installare CMake
  sudo apt-get install cmake

Errore: "VPK non installa"
Soluzione: 
  1. Verificare spazio su PS Vita (Settings → Storage)
  2. Cancellare versione precedente
  3. Verificare integrità VPK: unzip -t build/vita/VitaRPS5.vpk

Errore: "App crasha all'avvio"
Soluzione:
  1. Verificare log: ux0:/data/vita-chiaki/vitarps5-testing.log
  2. Compilare versione debug: ./tools/build.sh debug
  3. Verificare dipendenze su PS Vita

================================================================================
VERSIONE: v0.1.768
DATA: 12 Giugno 2026
STATO: Pronto per la compilazione
================================================================================
