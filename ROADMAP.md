# Roadmap — Plancia Ingresso

Stato dei lavori e prossimi passi. Le note tecniche di dettaglio stanno in
[CLAUDE.md](CLAUDE.md).

## Fatto

- [x] Pannello RGB 1024×600 con colori corretti e backlight stabile
- [x] Touch capacitivo GT911 con reset hardware affidabile ad ogni boot
- [x] UI LVGL a 4 pagine con barra di navigazione (pulsante pagina attiva evidenziato)
- [x] **Home**: ora/data, meteo (icona condizione + temperatura esterna), gauge
      temperatura e umidità (×3 stanze), potenza, bolletta, presenza persone —
      tutti live da Home Assistant
- [x] Controllo della pagina visualizzata da Home Assistant (entità `select`)
- [x] Pagina **Energia**: tile con icone MDI (spesa oggi / previsione / mese scorso,
      consumo, Tesla €/kWh)
- [x] Pagina **Casa**: routine (script), controlli cucina (switch con feedback colore),
      robot aspirapolvere, tapparelle
- [x] Pagina **Giardino**: luce e consumo cucina esterna, temperatura/umidità giardino,
      stato portafinestra

## In programma

### 1. Restyling UI — interfaccia più moderna e accattivante

**Intento (agosto 2026):** l'interfaccia attuale è funzionale ma grafica­mente
essenziale. Voglio rifarne il look rendendolo più moderno e accattivante,
sfruttando meglio le possibilità di LVGL: stili e temi, gradienti, angoli
arrotondati e ombre, transizioni/animazioni tra le pagine, tipografia più curata,
gerarchia visiva più chiara nelle tile.

Da fare quando c'è tempo — non urgente, nessuna modifica funzionale richiesta.

Riferimenti da studiare prima di mettere mano al layout:

- **LVGL in ESPHome** — componenti, widget, stili e azioni disponibili lato YAML:
  <https://esphome.io/components/lvgl/>
- **LVGL (repository upstream)** — la libreria grafica per microcontrollori su cui
  si basa tutto; utile per capire cosa è realmente supportato (stili, temi,
  animazioni, widget) e vedere esempi/demo:
  <https://github.com/lvgl/lvgl>

Vincoli da non dimenticare durante il restyling:

- Il widget `chart` **non esiste** in ESPHome: per i grafici si usa `meter`/`arc`.
- La navbar deve restare **dentro ogni pagina** (non nel `top_layer`, che non riceve
  il touch in modo affidabile).
- Ogni label usa **un solo font**: icone MDI e testo vanno in widget separati.
- Attenzione al peso di ciò che si aggiunge: LVGL sta in IRAM, ma niente `interval`
  grezzi per aggiornare le label (crashano il boot) — usare `on_value` / `on_time`.
- Il `pclk_frequency` resta a **30 MHz**: alzarlo fa rollback silenzioso dell'OTA.

### 2. Gestione accensione display

- [ ] Accensione al tocco, spegnimento del backlight dopo **30 s** di inutilizzo
      (LVGL `on_idle` → backlight off via `io_extension_ws`, riaccensione su touch)

### 3. Rifiniture

- [ ] (opzionale) icone MDI anche sulla Home
- [ ] (opzionale) sincronizzazione inversa del `select` pagina verso Home Assistant
- [ ] IP statico: riservare il DHCP per il MAC del dispositivo nel router
