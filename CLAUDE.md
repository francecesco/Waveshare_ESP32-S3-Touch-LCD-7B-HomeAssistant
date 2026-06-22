# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Obiettivo del progetto

Plancia di controllo/informativa da ingresso basata sul display **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo), con ESPHome + LVGL e integrazione Home Assistant. UI a pagine: Home (orologio, meteo, temperature, stato Tesla), Casa, Energia, Giardino.

**Stato:** ✅ Display, touch, UI a pagine navigabile, orologio e **dati live da Home Assistant** funzionanti. Fase attuale: arricchire le pagine (gauge consumi, poi Casa/Energia/Giardino).

## Comandi principali

```bash
# Flash via OTA (preferito — riavvio automatico e pulito)
esphome run test-display3.yaml --device test-plancia.local

# Flash via USB (poi serve POWER-CYCLE fisico: stacca/ricollega il cavo)
esphome run test-display3.yaml   # scegli [1] /dev/cu.usbmodem1101

# Log via rete
esphome logs test-display3.yaml --device test-plancia.local

# Erase completo (quando la board è incartata)
# Metti in download mode: tieni BOOT, premi/rilascia RST, rilascia BOOT
python3 -m esptool --chip esp32s3 --port /dev/cu.usbmodem1101 erase-flash
```

## ⚠️ LEZIONI CRITICHE (non ripetere questi errori)

### PCLK 50 MHz → rollback OTA silenzioso
Alzare il pixel clock del pannello RGB da 30 a 50 MHz rende **instabile il boot**: il pannello crasha entro 60s → il bootloader (safe_mode con rollback supportato) fa **rollback automatico** alla versione precedente. `OTA successful` **NON** garantisce che il firmware persista. Per ~10 flash il device ha eseguito sempre la vecchia versione, mascherando ogni modifica.
- **Tenere `pclk_frequency: 30MHz`.** Il refresh ~33 Hz NON dà flicker percepibile su questo pannello.
- **Verifica che un OTA si sia applicato davvero**: mettere un testo-marcatore univoco in una label LVGL e controllarlo a schermo. Il rollback è silenzioso, non appare nei log via OTA (si perde il boot).
- Siamo **solo OTA** (nessun USB collegato): NON flashare firmware che rischi un reboot loop — non c'è modo di fare l'erase di recupero.

### Flicker del backlight = PWM=247
Il registro PWM dell'IO expander a 247 è vicino alla **soglia di spegnimento** del chip → backlight instabile (sfarfallio ~2 Hz). Valore stabile e luminoso = **200**. Il registro PWM (0x05) è **diretto**: `0`=minimo, `~247`=massimo, `255`=spento (overflow).

### LVGL: niente `interval` per aggiornare le label
Un `interval: 1s` con `lvgl.label.update` **crashava il boot** (→ safe mode). Usare il trigger `on_time` del componente time, oppure `on_value` dei sensori — contesti sicuri. Nei lambda con array (giorni/mesi) mettere guardie sugli indici.

### LVGL: il widget `chart` NON esiste in ESPHome
Non c'è alcun widget `chart` né azioni `lvgl.chart.*` (e `LV_USE_CHART` è forzato a 0 in build con LVGL v9). Per i grafici: usare il widget **`meter`** (gauge/lancetta) — supportato — oppure il componente `graph` (ma disegna fuori da LVGL). I gauge sono anche fedeli alla dashboard HA che l'utente usava prima.

### Barra di navigazione: NON nel `top_layer`, e una per pagina
I widget nel `top_layer` LVGL non ricevono eventi touch in modo affidabile → la navbar va **dentro ogni pagina**. Inoltre, per evidenziare il pulsante della pagina attiva (bg blu `0x2196F3` vs grigio `0x2A2D3A`) NON si può usare un anchor condiviso (i widget LVGL non si aggiornano via anchor, e gli id non possono duplicarsi): serve una **navbar inline per pagina**, ognuna col proprio pulsante evidenziato. Le pagine non hanno titolo in alto (lo stato è dato dal pulsante attivo).

### Icone e font speciali
- **Icone Material Design**: font `font_mdi` scaricato in build (`type: web`, url del webfont Templarian/MaterialDesign-Webfont), glyphs per **codepoint** `"\U000Fxxxx"` (no supporto nativo `mdi:`). Verificare i codepoint su pictogrammers.com. Una label = un solo font.
- **Simboli `°` e `€`**: non presenti nei font LVGL di default → font custom via `gfonts://Montserrat` con `glyphs` mirati (`font_temp` per °, `font_euro` per €/kWh).
- I valori dei sensori si aggiornano con `on_value` → `lvgl.label.update` (mai `interval` grezzo).

### Home Assistant: ricaricare l'integrazione dopo ogni flash
I sensori `platform: homeassistant` restano **vuoti** finché non si ricarica l'integrazione ESPHome in HA dopo un flash (il subscribe degli stati non si ristabilisce da solo). Le entità esposte possono sparire allo stesso modo. Inoltre: le `Connection reset by peer` viste con `esphome logs` mentre HA è connesso sono il client di log che compete — NON instabilità reale.

### Controlli interattivi verso Home Assistant (pagina Casa)
Per comandare HA dalla plancia (eseguire script, toggle switch, avviare il robot, muovere le tapparelle) si usa `homeassistant.service` nell'`on_click` dei pulsanti LVGL (ESPHome lo traduce in azione `action:`). Esempi usati: `script.turn_on`, `switch.toggle`, `vacuum.start`, `cover.open_cover`/`stop_cover`/`close_cover`. L'`entity_id` è un `!secret`.
- **PREREQUISITO**: in HA, nell'integrazione ESPHome del device, abilitare **"Consenti al dispositivo di eseguire azioni di Home Assistant"** (Configura). Senza, i pulsanti non agiscono.
- **Feedback di stato** (es. switch acceso → pulsante colorato): `binary_sensor` / `text_sensor` `platform: homeassistant` con `on_state`/`on_value` → `lvgl.widget.update` (bg_color) o `lvgl.label.update`. I pulsanti da aggiornare hanno un `id`.
- **Layout pulsanti**: icona MDI + testo **affiancati in orizzontale** (`align: LEFT_MID` con `x` diversi); impilarli verticalmente in pulsanti bassi li fa sovrapporre.

## Hardware — dati certi

| Parametro | Valore |
|---|---|
| Chip | ESP32-S3 rev0.2, 8 MB PSRAM (octal), 16 MB Flash |
| Hostname | `test-plancia` (raggiungibile via mDNS `test-plancia.local`) |
| IP / MAC | dati locali — vedi `secrets.yaml` (non versionato) |
| Risoluzione | 1024×600, RGB565 |
| Touch | GT911 @ I2C `0x5D` — l'ESPHome di default usa `0x14`, va sovrascritto a `0x5D` |
| IO Expander | chip Waveshare custom @ I2C `0x24` (NON un CH422G standard) |

**Porte USB-C:** la board ne ha DUE.
- **USB** → `/dev/cu.usbmodem1101` — USB nativo S3. Usare QUESTA per flash e log.
- **UART1** → `/dev/cu.usbmodem5B5F0929241` — bridge seriale separato, NON programma l'S3.

## Architettura

### File del progetto
- `test-plancia.yaml` — firmware minimale WiFi-only (baseline sicura)
- `test-display3.yaml` — firmware attivo: `mipi_rgb` + `io_extension_ws` + LVGL (display funzionante)
- `components/io_extension_ws/` — componente ESPHome custom per l'IO expander
- `secrets.yaml` — credenziali WiFi (gitignored)

### Display RGB — CONFIG FUNZIONANTE (non cambiare senza motivo)

```yaml
display:
  - platform: mipi_rgb
    model: RPI
    color_order: BGR
    pixel_mode: 16bit
    pclk_frequency: 30MHz       # 50MHz => rollback OTA, vedi lezioni
    pclk_inverted: true
    dimensions: { width: 1024, height: 600 }
    de_pin: GPIO5
    hsync_pin: GPIO46
    vsync_pin: GPIO3
    pclk_pin: GPIO7
    hsync_pulse_width: 162
    hsync_back_porch: 152
    hsync_front_porch: 48
    vsync_pulse_width: 45
    vsync_back_porch: 13
    vsync_front_porch: 3
    # data_pins in ORDINE FISICO WAVESHARE (RGB565: data0-4=blue, 5-10=green, 11-15=red)
    data_pins:
      blue:  [GPIO14, GPIO38, GPIO18, GPIO17, GPIO10]
      green: [GPIO39, GPIO0,  GPIO45, GPIO48, GPIO47, GPIO21]
      red:   [GPIO1,  GPIO2,  GPIO42, GPIO41, GPIO40]
```

Note importanti:
- **L'ordine fisico Waveshare dei `data_pins` è la chiave dei colori.** La forma precedente (ordine diverso) dava i colori RUOTATI (rosso→blu, blu→verde).
- **Usare LVGL, non il lambda fill grezzo.** Il `lambda` del display veniva rieseguito ad ogni update senza stabilizzare il framebuffer (si vedeva rumore). `show_test_card` funzionava perché disegna una volta + `stop_poller()`. LVGL gestisce rendering/refresh correttamente. IRAM regge LVGL (niente overflow).
- I2C: SDA=GPIO8, SCL=GPIO9.

### Componente custom `io_extension_ws`

Il chip Waveshare @ I2C `0x24` **non è un CH422G standard** (risponde solo a 0x24). Protocollo: **registro singolo** — ogni scrittura è `[registro_interno][valore]` (2 byte).

| Registro | Indirizzo | Funzione |
|---|---|---|
| MODE | `0x02` | `0xFF` = tutti i pin in output |
| OUTPUT | `0x03` | bitmask stato pin |
| PWM | `0x05` | backlight: 0=min, ~247=max, 255=spento (default usato: 200) |

Assegnazione pin sull'expander: **IO1=reset touch GT911, IO2=backlight, IO3=reset LCD, IO4=SD CS** (bit-position 0-indexed; IO2=bit2=0x04, IO3=bit3=0x08).

Sequenza `setup()`: MODE=0xFF → OUTPUT=0xFF (backlight ON via IO2) → PWM=200 (luminoso stabile), ribadito dopo 300 ms. Il chip **conserva il valore PWM tra i riavvii dell'ESP32** (alimentazione separata), quindi va impostato esplicitamente.

API chiamabile da lambda YAML:
```cpp
id(io_ext).set_output_pin(uint8_t pin, bool value);  // pin 0-7
id(io_ext).set_pwm(uint8_t value);                   // 0=min, ~247=max, 255=off
```

Il componente espone anche un **GPIOPin** (`io_extension_ws: <id>` + `number:`), usato come `reset_pin` del touch. Implementazione: classe `IoExtensionWSGPIOPin` con tutti i metodi virtuali puri di `GPIOPin` (incluso **`get_flags`**, facile da dimenticare); in `__init__.py` `pins.gpio_base_schema(...)` SENZA `modes=[...]` (altrimenti `KeyError: 'input'`) + `PIN_SCHEMA_REGISTRY.register`.

Riferimento driver Waveshare ufficiale: [waveshareteam/ESP32-S3-Touch-LCD-7B](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-7B) (`io_extension.c`, `rgb_lcd_port.c`). Il `bl_on()` nativo usa solo il pin digitale IO2.

### Touch GT911 — CONFIG FUNZIONANTE
INT su **GPIO4** (diretto ESP32), address **0x5D**, bus I2C condiviso. Reset HW via IO1 dell'expander gestito dal driver — **necessario** per init affidabile ad ogni boot (senza, il touch si rompe dopo un riavvio):
```yaml
touchscreen:
  - platform: gt911
    id: my_touch
    interrupt_pin: GPIO4
    address: 0x5D
    reset_pin:
      io_extension_ws: io_ext
      number: 1
      mode:
        output: true
```
Collegamento a LVGL: `touchscreens: [my_touch]`. Touch allineato (X 0→1024 sx→dx, Y 0→600 alto→basso).

## Entità Home Assistant

Gli `entity_id` reali stanno in `secrets.yaml` (non versionato), riferiti nel YAML via `!secret`. Mappa dei secret usati:

- **Umidità**: `ent_um_giardino`, `ent_um_sala`, `ent_um_camera` (gauge 0–100 %)
- **Temperature**: `ent_temp_giardino`, `ent_temp_sala`, `ent_temp_camera` (gauge −10..50 °C)
- **Energia**: `ent_potenza` (potenza attuale, kW, 0–3.5), `ent_bolletta` (bolletta mese, 22–150)
- **Meteo**: `ent_meteo` (entità `weather.*`, attributo `temperature` per la temp esterna)
- **Persone**: `ent_persona1`, `ent_persona2` + nomi visualizzati `nome_persona1`, `nome_persona2`

Vedi `secrets.yaml.example` per il modello da compilare.

## Roadmap

1. ✅ Display, touch, UI a pagine, orologio, dati live HA sulla Home (gauge temp/umidità, potenza, bolletta, persone)
2. ✅ Controllo pagine da HA (entità `select`)
3. ✅ Pagina **Energia**: 6 tile con icone MDI (spesa oggi/previsione/mese scorso, consumo, Tesla €/kWh)
4. ✅ Navbar con pulsante attivo evidenziato; titoli rimossi
5. ✅ Pagina **Casa**: Routine (script Esco/Buongiorno/Buonanotte), Cucina (switch Caffè/Bollitore/Deum. con feedback colore), Pulizie (robot Igor), Tapparelle (su/stop/giù)
6. ✅ Pagina **Giardino**: luce cucina esterna (toggle), consumo cucina, temperatura/umidità giardino, portafinestra (icona aperta/chiusa)
7. **Gestione accensione display**: al tocco si accende, dopo **30 s di inutilizzo** si spegne il backlight (LVGL `on_idle` → backlight off via `io_extension_ws`; riaccensione su evento touch)
8. (opzionale) icone anche sulla Home; sync inversa del select pagina
9. IP statico DHCP per il MAC del dispositivo (riservarlo nel router)
