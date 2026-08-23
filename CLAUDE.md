# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Obiettivo del progetto

Plancia di controllo/informativa da ingresso basata sul display **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo), con ESPHome + LVGL e integrazione Home Assistant. UI a pagine: Home, Energia, Casa, Calendario.

**Stato:** ✅ Display, touch, UI a pagine navigabile, orologio, **dati live da Home Assistant** e **restyling UI** completati (le quattro pagine sono tutte popolate). Resta una voce opzionale: IP statico — vedi [ROADMAP.md](ROADMAP.md). Lo spegnimento automatico del display **non** si fa nel firmware: e' gestito da un'automazione lato Home Assistant.

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
Non c'è alcun widget `chart` né azioni `lvgl.chart.*` (e `LV_USE_CHART` è forzato a 0 in build con LVGL v9). Per i grafici: usare il widget **`meter`** (gauge/lancetta) — supportato — oppure il componente `graph` (ma disegna fuori da LVGL). Nota: **nel progetto non c'è più alcun gauge**, il restyling li ha sostituiti con tile piatte (icona + etichetta + valore numerico grande), più leggibili a distanza; non reintrodurli.

### Barra di navigazione: NON nel `top_layer`, e una per pagina
I widget nel `top_layer` LVGL non ricevono eventi touch in modo affidabile → la navbar va **dentro ogni pagina**. Inoltre, per evidenziare il pulsante della pagina attiva (stile `st_nav_active`: bg `0x232935` + bordo `0x7FB3D5`, contro `st_nav`: bg `0x171C24` senza bordo) NON si può usare un anchor condiviso (i widget LVGL non si aggiornano via anchor, e gli id non possono duplicarsi): serve una **navbar inline per pagina**, ognuna col proprio pulsante evidenziato. Il nome della pagina corrente si legge nella pill `lbl_page_name` del `top_layer`, aggiornata dal trigger `on_load` di ogni pagina.

- **`st_nav` è solo per la navbar**, dove sta sulla fascia `0x0B0D12`. Dentro una card (`st_card`, bg `0x171C24`) avrebbe lo stesso colore del fondo → pulsante invisibile: per quelli c'è **`st_btn`** (bg `0x232935`, bordo `0x272E3A`).

### Icone e font speciali
- **Icone Material Design**: font `font_mdi` scaricato in build (`type: web`, url del webfont Templarian/MaterialDesign-Webfont), glyphs per **codepoint** `"\U000Fxxxx"` (no supporto nativo `mdi:`). Verificare i codepoint su pictogrammers.com. Una label = un solo font.
- **Tipografia Poppins via `gfonts://`**, ritagliata per taglia. Le taglie testuali (`f_24 f_20 f_16 f_14`) usano `glyphsets: [GF_Latin_Core]`, che copre anche gli accenti italiani (`ì à è é ò ù`); le taglie numeriche (`f_44`, `f_40`), che disegnano solo cifre/orologio, usano invece una lista `glyphs` esplicita.
- **Il glyphset di default `GF_Latin_Kernel` ha `°` e `€` ma non gli accenti italiani.** Serve `GF_Latin_Core` per quelli, ma sono 319 glifi: applicarlo indiscriminatamente a tutte le taglie costerebbe ~367 KB invece di ~145 KB, da cui la divisione fra taglie testuali (Latin Core) e numeriche (glyphs mirati).
- **I font builtin di LVGL (`montserrat_XX`) non sono gratuiti**: ogni taglia usata viene compilata con l'intero charset (`montserrat_48` da solo pesava ~95 KB). Sostituendoli con Poppins ritagliata sui soli glifi disegnati, il firmware è **sceso** dal 16.7% al 14.8% di flash (1359635 → 1205127 B su 8126464) pur aggiungendo sei taglie tipografiche e sedici icone meteo — nel progetto non compare più alcun `montserrat_*`.
- **Serve un secondo font MDI a 24 px** (`font_mdi_24`) oltre a `font_mdi` (40 px), perché quest'ultimo non entra nel chip icona da 44×44; occhio a non disallineare le due liste di glifi (rischio duplicati).
- **Gli stili condivisi stanno in `lvgl: style_definitions:`** e si applicano ai widget con `styles: [nome]` — è quanto ha reso l'ingrandimento dei caratteri una modifica di due righe invece che di venti tile.
- **L'header (ora/data/meteo/temperature) sta nel `top_layer`**: è sicuro perché non ha widget interattivi (l'avvertenza nota sul `top_layer` riguarda solo il touch), ed è l'unica sede possibile perché gli id LVGL sono unici e queste label non possono esistere in quattro copie (una per pagina).
- **`text_color` accetta un lambda** purché restituisca un `lv_color_t` (es. `lv_color_hex(...)`), utile per colorare dinamicamente senza catene di `if`.
- I valori dei sensori si aggiornano con `on_value` → `lvgl.label.update` (mai `interval` grezzo).

### Entità HA rinominate = valori fermi a `--`, in totale silenzio
Se un `entity_id` sottoscritto non esiste più in HA (rinominato, integrazione sostituita), ESPHome **non logga alcun errore**: si sottoscrive e non riceve mai uno stato, quindi la label LVGL resta al placeholder. Nei log si vedono solo i `Got state` delle entità *vive*, e l'assenza di una riga non distingue "entità morta" da "valore non ancora cambiato".
- **Diagnosi rapida** — confrontare gli entity_id del firmware con la verità di HA via API REST (serve un long-lived token):
  ```bash
  curl -s -H "Authorization: Bearer $TOKEN" http://homeassistant.local:8123/api/states > states.json
  # poi incrociare con le chiavi ent_* di secrets.yaml
  ```
  L'elenco degli entity_id realmente compilati nel firmware in esecuzione si legge nel dump di boot: `esphome logs ... | grep "Entity ID:"`.
- Dopo aver cambiato entità in HA, ricontrollare **tutte** le chiavi `ent_*`, non solo quella che si è notata a schermo.

### Home Assistant: ricaricare l'integrazione dopo ogni flash
I sensori `platform: homeassistant` restano **vuoti** finché non si ricarica l'integrazione ESPHome in HA dopo un flash (il subscribe degli stati non si ristabilisce da solo). Le entità esposte possono sparire allo stesso modo. Inoltre: le `Connection reset by peer` viste con `esphome logs` mentre HA è connesso sono il client di log che compete — NON instabilità reale.

### Controlli interattivi verso Home Assistant (pagina Casa)
Per comandare HA dalla plancia (eseguire script, toggle switch, avviare il robot, muovere le tapparelle) si usa `homeassistant.service` nell'`on_click` dei pulsanti LVGL (ESPHome lo traduce in azione `action:`). Esempi usati: `script.turn_on`, `switch.toggle`, `vacuum.start`, `cover.open_cover`/`stop_cover`/`close_cover`. L'`entity_id` è un `!secret`.
- **PREREQUISITO**: in HA, nell'integrazione ESPHome del device, abilitare **"Consenti al dispositivo di eseguire azioni di Home Assistant"** (Configura). Senza, i pulsanti non agiscono.
- **Feedback di stato** (es. switch acceso → pulsante colorato): `binary_sensor` / `text_sensor` `platform: homeassistant` con `on_state`/`on_value` → `lvgl.widget.update` (bg_color) o `lvgl.label.update`. I pulsanti da aggiornare hanno un `id`.
- **Layout pulsanti**: icona MDI + testo **affiancati in orizzontale** (`align: LEFT_MID` con `x` diversi); impilarli verticalmente in pulsanti bassi li fa sovrapporre.

### Agenda: leggere i calendari (`calendar.get_events` con risposta)

Gli attributi di un'entita' `calendar.*` **non bastano** e non basteranno mai:

- espongono al massimo il **prossimo** evento (`message`, `start_time`, `all_day`);
- le agende Google condivise hanno `supported_features: 1` e non espongono
  **nemmeno quello** — `calendar.pole_pole` restava a "Nessun impegno" pur avendo
  quattro eventi. Nessun `text_sensor` + `attribute:` potra' leggerli;
- un template Jinja lato HA non e' una via d'uscita: **i template non possono
  chiamare azioni**, quindi nemmeno `calendar.get_events`.

La via giusta e' che il **dispositivo chiami l'azione e ne riceva la risposta**.
L'API nativa lo sa fare: `homeassistant.action` + `capture_response: true` +
`on_success`, dove il lambda riceve un `JsonObjectConst response`. Serve ESPHome
>= 2025.8 e HA recente (qui 2026.5.3 / 2026.8.3).

- **`response_template` e' il pezzo che rende la cosa pratica.** Senza, arriva
  il JSON grezzo di tutti gli eventi da parsare a bordo. Con, si manda a HA il
  Jinja che unisce/ordina/formatta e torna **una stringa piccola**. HA lo esegue
  ma non lo conserva: il template vive in `test-display3.yaml`, nel repo, e
  **`configuration.yaml` non si tocca**.
- **La risposta arriva sempre incartata**: `{"response": "<testo reso>"}`, anche
  con un `response_template`. Sul device: `const char *raw = response["response"]`.
- Formato scelto: 15 campi separati da `|` (5 righe x sorgente/quando/cosa),
  sempre 15 anche se vuoti — il parsing a bordo e' un `find('|')` in ciclo.
  Le righe senza evento nascondono la propria card (`lvgl.widget.hide`); se non
  c'e' proprio nulla resta la sola label `lbl_cal_vuoto` al centro.
- **Gli `entity_id` servono dentro il Jinja**, dove `!secret` non e' ammesso
  (e' un tag YAML, non si puo' mettere a meta' di uno scalare). Si passano con
  le **`substitutions:`**, che valgono `!secret` e vengono sostituite anche
  dentro i block scalar e i lambda.
- **Prerequisito**: lo stesso della pagina Casa — "Consenti al dispositivo di
  eseguire azioni di Home Assistant" abilitato nell'integrazione.
- **Il template si valida senza flashare**: `POST /api/template` con il Jinja
  preceduto da `{%- set response = <JSON reale> -%}`, dove il JSON reale si
  ottiene da `POST /api/services/calendar/get_events?return_response=true`.
  Quattro minuti di compilazione risparmiati ad ogni giro.
- **`on_client_connected` dell'`api:` scatta per ogni client**, incluso
  `esphome logs`, non solo per HA. Va bene come innesco (l'agenda si ricarica
  quando HA ricompare) ma non e' un segnale di "HA connesso".
- La chiamata dell'azione **non viene loggata**: se va bene non si vede nulla.
  L'unico riscontro nei log e' il `logger.log` dentro `on_error`.

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

- **Umidità**: `ent_um_giardino`, `ent_um_sala`, `ent_um_camera` (tile Home, `%`)
- **Temperature**: `ent_temp_giardino`, `ent_temp_sala`, `ent_temp_camera` (tile Home, `°`). `ent_temp_giardino` alimenta anche la temperatura **esterna** dell'header, `ent_temp_sala` quella **interna**
- **Energia**: `ent_potenza` (potenza attuale, kW), `ent_bolletta` (bolletta mese, €)
- **Meteo**: `ent_meteo` (entità `weather.*`) — solo `text_sensor`: lo **stato** diventa glifo MDI + colore dell'icona nell'header. Il `sensor` con `attribute: temperature` è stato rimosso (la temperatura esterna viene dal sensore del giardino)
- **Tesla (mese)**: `ent_tesla_costo` (€) e `ent_tesla_kwh_f1/f2/f3` — in HA non esiste un totale mensile, i kWh si **sommano per fascia** in un lambda (le fasce non ancora ricevute sono NaN → contate 0)
- **Calendario**: `ent_cal_rifiuti`, `ent_cal_office`, `ent_cal_pole` — entita' `calendar.*`. Non sono lette come sensori: alimentano l'azione `calendar.get_events` dello script `aggiorna_agenda` (vedi "Agenda: leggere i calendari"). Sono anche esposte come **substitutions** (`cal_rifiuti`, `cal_office`, `cal_pole`), perche' servono dentro il template Jinja dove `!secret` non arriva
- **Persone**: `ent_persona1`, `ent_persona2` + nomi visualizzati `nome_persona1`, `nome_persona2`

Vedi `secrets.yaml.example` per il modello da compilare.

## Roadmap

Dettaglio e riferimenti in [ROADMAP.md](ROADMAP.md).

1. ✅ Display, touch, UI a pagine, orologio, dati live HA sulla Home (tile temp/umidità, potenza, bolletta, persone)
2. ✅ Controllo pagine da HA (entità `select`)
3. ✅ Pagina **Energia**: 6 tile con icone MDI (spesa oggi/previsione/mese scorso, consumo, Tesla €/kWh)
4. ✅ Navbar con pulsante attivo evidenziato; titoli rimossi
5. ✅ Pagina **Casa**: quattro macro tile 484x200 — Igor (l'`on_click` sceglie fra `vacuum.start` e `vacuum.return_to_base` con un `if` sullo stato di `st_vacuum`), Tapparelle (su/stop/giù dentro un'unica tile), routine Esco e Buonanotte. I controlli cucina e la routine Buongiorno sono stati rimossi: troppi bersagli e troppo piccoli. Gli `entity_id` restano in `secrets.yaml`.
6. ✅ Pagina **Calendario**: **agenda cronologica** che fonde i tre calendari HA (raccolta differenziata, whereveroffice, pole pole) — cinque righe in ordine di tempo, "Oggi" / "Domani" / "Gio 27", l'orario solo se non e' un evento di giornata intera, e un'icona colorata per calendario di provenienza

   La pagina **Giardino** e' stata rimossa: le sue letture di temperatura e umidita' erano gia' sulla Home, e luce esterna, consumo cucina e portafinestra sono state giudicate non necessarie sulla plancia. Rimossi con essa i sensori `consumo_cucina`, `st_luce_cucina` e `st_porta`; gli `entity_id` restano in `secrets.yaml`.

   Il perche' della meccanica insolita e' nella sezione "Agenda: leggere i calendari".
7. ✅ **Restyling UI**: palette scura, header globale nel `top_layer`, tile piatte al posto dei gauge, dissolvenza fra le pagine, tipografia Poppins ritagliata per taglia
9. (opzionale) sync inversa del `select` pagina verso Home Assistant
10. IP statico DHCP per il MAC del dispositivo (riservarlo nel router)
