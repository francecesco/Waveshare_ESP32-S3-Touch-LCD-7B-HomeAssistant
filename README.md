# Plancia Ingresso — Waveshare ESP32-S3-Touch-LCD-7B

Plancia di controllo/informativa da ingresso basata sul display **Waveshare
ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo), realizzata con **ESPHome +
LVGL** e integrata con **Home Assistant**.

UI a pagine: **Home** (orologio, temperature, umidità, consumi, persone), **Casa**,
**Energia**, **Giardino**.

## Funziona

- ✅ Pannello RGB 1024×600 con colori corretti, backlight stabile
- ✅ Touch capacitivo GT911 (reset hardware affidabile ad ogni boot)
- ✅ UI LVGL a 4 pagine con barra di navigazione
- ✅ **Home**: ora/data, gauge temperatura e umidità (×3 stanze), potenza, bolletta,
  presenza persone — tutti alimentati in tempo reale da Home Assistant
- ✅ Controllo della pagina visualizzata **da Home Assistant** (entità `select`),
  per scorrere le schermate via automazioni

## Hardware

| Parametro | Valore |
|---|---|
| Scheda | Waveshare ESP32-S3-Touch-LCD-7B |
| Chip | ESP32-S3, 8 MB PSRAM (octal), 16 MB Flash |
| Display | 1024×600 RGB565, pannello RGB parallelo |
| Touch | GT911 capacitivo (I2C `0x5D`) |
| IO Expander | chip Waveshare custom (I2C `0x24`) — gestito dal componente `io_extension_ws` |

L'IO expander pilota: backlight (IO2), reset LCD (IO3), reset touch (IO1).

## Struttura del progetto

```
.
├── test-display3.yaml          # firmware principale: display + touch + LVGL + HA
├── test-plancia.yaml           # firmware minimale WiFi-only (base di test)
├── secrets.yaml.example        # modello dei secret (copialo in secrets.yaml)
├── components/
│   └── io_extension_ws/        # componente ESPHome custom per l'IO expander Waveshare
│       ├── __init__.py
│       ├── io_extension_ws.h
│       └── io_extension_ws.cpp
├── CLAUDE.md                   # note tecniche dettagliate e lezioni apprese
└── README.md
```

## Setup

1. **Installa ESPHome** (consigliato 2026.5+):
   ```bash
   pip install esphome
   ```

2. **Configura i secret**: copia il modello e compila con i tuoi valori (WiFi ed
   `entity_id` di Home Assistant):
   ```bash
   cp secrets.yaml.example secrets.yaml
   # poi modifica secrets.yaml
   ```
   `secrets.yaml` è in `.gitignore` e non viene versionato.

3. **Flash** (via OTA, dopo il primo flash USB):
   ```bash
   esphome run test-display3.yaml --device test-plancia.local
   ```

4. In **Home Assistant**, aggiungi il dispositivo tramite l'integrazione ESPHome.
   Dopo ogni flash, **ricarica l'integrazione** affinché i sensori importati si
   ripopolino.

## Note tecniche chiave

Dettagli completi in [CLAUDE.md](CLAUDE.md). In sintesi:

- **Pixel clock a 30 MHz**: a 50 MHz il boot del pannello è instabile e il
  bootloader fa rollback dell'OTA (silenzioso). Il refresh ~33 Hz non dà flicker.
- **Backlight via PWM = 200**: il registro PWM dell'expander a 247 è sulla soglia di
  spegnimento e fa sfarfallare la retroilluminazione.
- **`data_pins` in ordine fisico Waveshare**: è la chiave per avere i colori giusti.
- **Touch**: il reset del GT911 va pilotato dal driver tramite un `GPIOPin` esposto
  dal componente `io_extension_ws`, altrimenti l'init è intermittente.
- **Niente widget `chart` in LVGL ESPHome**: per i grafici si usa `meter`/`arc`.
- **Il simbolo `°`** non è nei font LVGL di default: serve un font custom (gfonts).

## Componente custom `io_extension_ws`

L'IO expander Waveshare a I2C `0x24` non è un CH422G standard (il driver `ch422g`
fallisce). È un chip a **registro singolo**: ogni scrittura è `[registro][valore]`.
Il componente espone un'API per i pin di output e il PWM del backlight, e un
`GPIOPin` usabile come `reset_pin` di altri componenti (es. il touch).

## Roadmap

- [x] Display, touch, UI a pagine, Home con dati live HA
- [x] Controllo pagine da HA (select)
- [ ] Pagina **Casa**: serratura, scene "Esco"/"Torno"
- [ ] Pagina **Energia**: storico consumi, bolletta, auto
- [ ] Pagina **Giardino**: sensori giardino, irrigazione
