# Roadmap — Plancia Ingresso

Stato dei lavori e prossimi passi. Le note tecniche di dettaglio stanno in
[CLAUDE.md](CLAUDE.md).

## Fatto

- [x] Pannello RGB 1024×600 con colori corretti e backlight stabile
- [x] Touch capacitivo GT911 con reset hardware affidabile ad ogni boot
- [x] UI LVGL a 4 pagine con barra di navigazione (pulsante pagina attiva evidenziato)
- [x] **Home**: ora/data, icona meteo, temperature esterna/interna nell'header, tile
      temperatura e umidità (×3 stanze), potenza, bolletta, presenza persone —
      tutti live da Home Assistant
- [x] Controllo della pagina visualizzata da Home Assistant (entità `select`)
- [x] Sincronizzazione **bidirezionale** del `select` pagina: la plancia annuncia
      a HA la pagina su cui si trova, comunque ci si sia arrivati
- [x] Pagina **Energia**: tile con icone MDI (spesa oggi / previsione / mese scorso,
      consumo, Tesla €/kWh)
- [x] Pagina **Casa**: quattro macro tile — Igor (avvia / rimanda alla base a
      seconda dello stato), tapparelle (su/stop/giù), routine Esco e Buonanotte
- [x] Pagina **Calendario**: un riquadro per calendario Home Assistant (raccolta differenziata, whereveroffice, pole pole) col prossimo impegno e quando — "Oggi" / "Domani" / "24 ago", con l'orario solo se non e' un evento di giornata intera
- [x] **Restyling UI**: palette scura ispirata agli screenshot di riferimento, header
      globale (ora/data/meteo/temperature) nel `top_layer`, tile piatte al posto dei
      gauge, transizioni in dissolvenza fra le pagine, tipografia Poppins (ritagliata
      per taglia) ingrandita su richiesta — a parità di funzionalità e con il flash
      sceso invece di crescere (vedi CLAUDE.md, sezione "Icone e font speciali")

## In programma

- [ ] IP statico: riservare il DHCP per il MAC del dispositivo nel router

## Non si fa

- **Spegnimento automatico del display.** Gestito da un'automazione lato Home
  Assistant, non serve implementarlo nel firmware.
