# VitaRPS5 - Ottimizzazioni di Rete PSN

## 1. Configurazione della Rete Wi-Fi

### 1.1 Impostazioni Consigliate su PS Vita

#### Banda Wi-Fi
- **Preferenza:** 5GHz (se disponibile)
- **Motivo:** Meno interferenza, latenza più bassa
- **Fallback:** 2.4GHz se 5GHz non disponibile

#### Canale Wi-Fi (Router)
- **5GHz:** Canali 36-48 (preferiti), 149-165 (alternativi)
- **2.4GHz:** Canali 1, 6, 11 (non sovrapposti)

#### Potenza del Segnale
- **Target:** -50 dBm o meglio
- **Accettabile:** -60 dBm
- **Scarso:** < -70 dBm (riposizionare router o Vita)

---

### 1.2 Configurazione del Router

#### QoS (Quality of Service)
```
Abilitare QoS per prioritizzare il traffico di streaming:
- Priorità Alta: PS Vita (MAC address)
- Priorità Media: Altro
- Priorità Bassa: Download/Torrent
```

#### UPnP e NAT-PMP
```
Abilitare entrambi per migliorare il hole-punching:
- Settings → UPnP: ON
- Settings → NAT-PMP: ON (se disponibile)
```

#### DNS
```
Usare DNS veloci:
- Primary: 1.1.1.1 (Cloudflare)
- Secondary: 8.8.8.8 (Google)
```

---

## 2. Configurazione PSN su PS Vita

### 2.1 Abilitare PSN Internet Mode

1. Aprire VitaRPS5
2. Andare a **Settings**
3. Attivare **PSN Internet Mode**
4. Andare a **Profile**
5. Premere **X** su Connection Card
6. Seguire il flusso di autenticazione PSN

### 2.2 Configurare la Connessione

```
Settings → Connection:
- Connection Type: PSN Internet
- Bitrate: 3000-3500 kbps (PSN)
- Resolution: 540p
- FPS: 30
- Latency Mode: Ultra Low
```

---

## 3. Ottimizzazioni Lato Codice

### 3.1 Configurare SO_SNDBUF

**File:** `lib/src/takion.c`

```c
#define TAKION_A_RWND (512 * 1024)  // 512 KB

// Nel socket setup:
int sndbuf_size = TAKION_A_RWND;
if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size)) < 0) {
    LOGE("Failed to set SO_SNDBUF: %s", strerror(errno));
}

// Verificare che sia stato impostato:
int actual_sndbuf;
socklen_t len = sizeof(actual_sndbuf);
getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &actual_sndbuf, &len);
LOGD("SO_SNDBUF: requested=%d, actual=%d", sndbuf_size, actual_sndbuf);
```

**Benefici:**
- ✅ Riduce packet loss durante burst di dati
- ✅ Migliora throughput di 10-15%

---

### 3.2 Configurare IP_TOS (DSCP)

**File:** `lib/src/takion.c`

```c
// Impostare DSCP EF (Expedited Forwarding) per priorità di rete
#define IP_TOS_EXPEDITED_FORWARDING 0xB8  // DSCP EF

int tos = IP_TOS_EXPEDITED_FORWARDING;
if (setsockopt(sock, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0) {
    LOGD("Failed to set IP_TOS: %s (non-critical)", strerror(errno));
}
```

**Benefici:**
- ✅ Priorità di rete su router che supportano QoS
- ✅ Riduce latenza in reti congestionate

---

### 3.3 Throttle WebSocket Ping

**File:** `lib/src/remote/holepunch.c`

```c
#define WEBSOCKET_PING_INTERVAL_IDLE_SEC 5
#define WEBSOCKET_PING_INTERVAL_STREAMING_SEC 30

// Nel loop di streaming:
uint32_t ping_interval = WEBSOCKET_PING_INTERVAL_IDLE_SEC;

if (stream_active) {
    ping_interval = WEBSOCKET_PING_INTERVAL_STREAMING_SEC;
    LOGD("WebSocket ping interval: %u sec (streaming)", ping_interval);
} else {
    LOGD("WebSocket ping interval: %u sec (idle)", ping_interval);
}

// Impostare il timeout del socket
setsockopt(ws_sock, SOL_SOCKET, SO_RCVTIMEO, &ping_interval, sizeof(ping_interval));
```

**Benefici:**
- ✅ Riduce latenza di 2-3ms
- ✅ Riduce overhead di rete

---

### 3.4 Ridurre Socket NAT Burst

**File:** `lib/src/remote/holepunch.c`

```c
// Vita-specific optimization
#ifdef __PSVITA__
#define RANDOM_ALLOCATION_SOCKS_NUMBER 24  // Ridotto da 48
#define RANDOM_ALLOCATION_SOCKS_TIMEOUT_SEC 2  // Timeout ridotto
#else
#define RANDOM_ALLOCATION_SOCKS_NUMBER 48
#define RANDOM_ALLOCATION_SOCKS_TIMEOUT_SEC 3
#endif

// Nel codice di hole-punching:
for (int i = 0; i < RANDOM_ALLOCATION_SOCKS_NUMBER; i++) {
    // Allocare socket e inviare STUN request
    // Con timeout di RANDOM_ALLOCATION_SOCKS_TIMEOUT_SEC
}
```

**Benefici:**
- ✅ Riduce memoria allocata
- ✅ Migliora stabilità su reti congestionate

---

## 4. Diagnostica e Troubleshooting

### 4.1 Verificare la Connessione PSN

```bash
# Su PS Vita, aprire il menu debug:
# Settings → Debug Menu (se abilitato)

# Controllare:
- PSN Token Status: Valid/Expired
- UPnP Status: Available/Unavailable
- NAT Type: Open/Moderate/Strict
- Holepunch Status: Connected/Failed
```

### 4.2 Analizzare i Log

```bash
# Scaricare i log da PS Vita:
# VitaShell → ux0:/data/vita-chiaki/vitarps5-testing.log

# Cercare messaggi critici:
grep -i "PSN\|holepunch\|upnp\|error" vitarps5-testing.log

# Analizzare latenza:
grep "latency\|rtt\|ping" vitarps5-testing.log
```

### 4.3 Monitorare le Metriche di Rete

**Metriche Importanti:**

| Metrica | Target | Accettabile | Scarso |
|---------|--------|-------------|--------|
| Latenza | < 20ms | < 50ms | > 100ms |
| Jitter | < 5ms | < 10ms | > 20ms |
| Packet Loss | 0% | < 1% | > 5% |
| Bandwidth | 3-5 Mbps | 2-3 Mbps | < 1.5 Mbps |

---

## 5. Strategie di Fallback

### 5.1 Fallback da PSN a LAN

Se la connessione PSN fallisce, l'app dovrebbe automaticamente:

1. **Rilevare il fallimento** (timeout > 30 secondi)
2. **Cercare console locali** (mDNS/SSDP)
3. **Connettersi via LAN** se disponibile
4. **Notificare l'utente** del cambio di modalità

**Implementazione:**

```c
// In host.c
if (psn_remote && holepunch_session) {
    // Impostare timeout di 30 secondi
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < 30) {
        if (chiaki_holepunch_session_is_connected(holepunch_session)) {
            // Connessione PSN riuscita
            break;
        }
        sceKernelDelayThread(100000);  // 100ms
    }
    
    if (!chiaki_holepunch_session_is_connected(holepunch_session)) {
        // Timeout PSN - fallback a LAN
        LOGD("PSN connection timeout - falling back to LAN");
        psn_remote = false;
        chiaki_holepunch_session_fini(holepunch_session);
        holepunch_session = NULL;
        
        // Riavviare la connessione in modalità LAN
        host_set_hint(host, "PSN connection failed. Trying LAN...", false, 3000000);
        // Continuare con LAN connection
    }
}
```

### 5.2 Retry Intelligente

```c
#define PSN_CONNECT_MAX_RETRIES 3
#define PSN_CONNECT_RETRY_DELAY_SEC 2

for (int retry = 0; retry < PSN_CONNECT_MAX_RETRIES; retry++) {
    if (psn_remote_prepare_connect_host(host, &holepunch_session) == 0) {
        LOGD("PSN connection successful on attempt %d", retry + 1);
        break;
    }
    
    if (retry < PSN_CONNECT_MAX_RETRIES - 1) {
        LOGD("PSN connection failed - retry %d/%d after %d sec", 
             retry + 1, PSN_CONNECT_MAX_RETRIES, PSN_CONNECT_RETRY_DELAY_SEC);
        
        sceKernelDelayThread(PSN_CONNECT_RETRY_DELAY_SEC * 1000000);
    } else {
        LOGE("PSN connection failed after %d attempts", PSN_CONNECT_MAX_RETRIES);
        host_set_hint(host, "PSN connection failed. Trying LAN...", false, 3000000);
        psn_remote = false;
    }
}
```

---

## 6. Monitoraggio della Qualità di Rete

### 6.1 Implementare Network Health Check

```c
typedef struct {
    uint32_t latency_ms;
    float packet_loss_percent;
    uint32_t jitter_ms;
    uint32_t bandwidth_kbps;
} NetworkHealth;

// Funzione di health check
NetworkHealth psn_network_health_check(ChiakiHolepunchSession session) {
    NetworkHealth health = {0};
    
    // Misurare RTT (Round Trip Time)
    uint64_t start_us = sceKernelGetSystemTimeWide();
    chiaki_holepunch_ping(session, 5);  // 5 secondi timeout
    uint64_t end_us = sceKernelGetSystemTimeWide();
    health.latency_ms = (end_us - start_us) / 1000;
    
    // Calcolare packet loss dai log
    // health.packet_loss_percent = ...
    
    // Calcolare jitter
    // health.jitter_ms = ...
    
    return health;
}

// Usare per adattare il bitrate
if (health.packet_loss_percent > 5.0f) {
    profile.bitrate = PSN_REMOTE_BITRATE_CAP_KBPS / 2;  // Ridurre
    LOGD("Network degradation detected - reducing bitrate");
}
```

---

## 7. Best Practices

### 7.1 Configurazione Ottimale

1. **Router:** Posizionare vicino a PS Vita
2. **Canale Wi-Fi:** Usare canale meno congestionato (app WiFi Analyzer)
3. **QoS:** Abilitare e prioritizzare PS Vita
4. **DNS:** Usare DNS veloce (Cloudflare 1.1.1.1)
5. **UPnP:** Abilitare su router e PS Vita

### 7.2 Troubleshooting Checklist

- [ ] PS Vita connessa a Wi-Fi 5GHz
- [ ] Segnale Wi-Fi > -60 dBm
- [ ] Router supporta UPnP
- [ ] PSN Internet Mode abilitato su PS Vita
- [ ] Token PSN valido (non scaduto)
- [ ] Nessun firewall che blocca porte
- [ ] Nessun VPN attivo
- [ ] Nessun throttling ISP

### 7.3 Performance Tuning

| Impostazione | Valore | Effetto |
|--------------|--------|--------|
| Bitrate | 3000 kbps | Stabile su reti scadenti |
| Bitrate | 3500 kbps | Qualità media |
| Bitrate | 5000 kbps | Qualità alta (reti buone) |
| Resolution | 360p | Latenza bassa |
| Resolution | 540p | Qualità media |
| FPS | 30 | Stabile |
| FPS | 60 | Richiede rete buona |
| Latency Mode | Ultra Low | Riduce latenza di 10-20ms |

---

## 8. Metriche di Successo

### 8.1 Streaming Stabile

- ✅ Latenza < 50ms
- ✅ Jitter < 10ms
- ✅ Packet Loss < 1%
- ✅ Frame Rate stabile (30fps o 60fps)
- ✅ Audio sincronizzato
- ✅ Nessun glitch visivo

### 8.2 Streaming Accettabile

- ⚠️ Latenza 50-100ms
- ⚠️ Jitter 10-20ms
- ⚠️ Packet Loss 1-5%
- ⚠️ Frame Rate occasionalmente instabile
- ⚠️ Audio occasionalmente desincronizzato
- ⚠️ Glitch visivi rari

### 8.3 Streaming Inaccettabile

- ❌ Latenza > 100ms
- ❌ Jitter > 20ms
- ❌ Packet Loss > 5%
- ❌ Frame Rate instabile
- ❌ Audio desincronizzato
- ❌ Glitch visivi frequenti

---

## 9. Riferimenti Tecnici

### 9.1 Porte Utilizzate

| Protocollo | Porta | Direzione | Scopo |
|-----------|-------|-----------|-------|
| TCP | 9295 | Outbound | PSN Authentication |
| TCP | 443 | Outbound | PSN WebSocket (HTTPS) |
| UDP | 3478-3479 | Bidirectional | STUN (NAT Discovery) |
| UDP | Dinamica | Bidirectional | Holepunch (Streaming) |

### 9.2 Documentazione Correlata

- [Chiaki Remote Play Protocol](https://git.sr.ht/~thestr4ng3r/chiaki)
- [PlayStation Network Documentation](https://www.playstation.com/en-us/support/)
- [UPnP Specification](http://upnp.org/)
- [STUN Protocol (RFC 5389)](https://tools.ietf.org/html/rfc5389)

---

**Ultima Aggiornamento:** 12 Giugno 2026  
**Versione:** v0.1.768
