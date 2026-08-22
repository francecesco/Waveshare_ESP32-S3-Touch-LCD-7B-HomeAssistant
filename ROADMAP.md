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
- [x] **Restyling UI**: palette scura ispirata agli screenshot di riferimento, header
      globale (ora/data/meteo/temperature) nel `top_layer`, tile piatte al posto dei
      gauge, transizioni in dissolvenza fra le pagine, tipografia Poppins (ritagliata
      per taglia) ingrandita su richiesta — a parità di funzionalità e con il flash
      sceso invece di crescere (vedi CLAUDE.md, sezione "Icone e font speciali")

## In programma

### 1. Gestione accensione display

- [ ] Accensione al tocco, spegnimento del backlight dopo **30 s** di inutilizzo
      (LVGL `on_idle` → backlight off via `io_extension_ws`, riaccensione su touch)

### 2. Rifiniture

- [ ] (opzionale) sincronizzazione inversa del `select` pagina verso Home Assistant
- [ ] IP statico: riservare il DHCP per il MAC del dispositivo nel router
