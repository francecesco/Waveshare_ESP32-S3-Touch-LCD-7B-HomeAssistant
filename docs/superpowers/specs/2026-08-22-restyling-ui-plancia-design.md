# Restyling UI della plancia — design

**Data:** 2026-08-22
**Stato:** approvato, da implementare
**Riferimento visivo:** tre foto della dashboard su tablet fornite dall'utente
(tema chiaro, card arrotondate, tile icona+etichetta+valore, header globale).

## Obiettivo

Rifare il look delle quattro pagine LVGL rendendolo coerente e moderno, prendendo
struttura e linguaggio visivo dagli screenshot di riferimento. **Nessuna modifica
funzionale**: stesse entita' Home Assistant, stessi comandi, stessa navigazione a
quattro pagine.

## Decisioni prese

| Tema | Scelta | Motivo |
|---|---|---|
| Palette | Linguaggio degli screenshot ma **scura** | La plancia sta in ingresso: di notte un pannello 1024x600 quasi bianco illumina troppo, e lo spegnimento automatico del backlight non e' ancora implementato. |
| Scheletro | **Header globale + navbar in basso** | Header coerente su tutte le pagine; la navbar resta perche' ha bersagli touch grandi e la navigazione non deve cambiare. |
| Letture | **Tutte tile piatte**, nessun gauge | Fedelta' al riferimento, che non ha gauge; piu' compatto, e l'header riduce lo spazio utile. |
| Esecuzione | **Una pagina alla volta** | Siamo solo OTA senza recupero USB, e un display si verifica guardandolo. Un commit per pagina rende il rollback banale. |

## Sistema di stili

Definito una volta in `style_definitions` e applicato via `styles:` sui widget,
per non ripetere le proprieta' su ~200 widget.

### Palette

```
bg       #0E1116   fondo pagina
surface  #171C24   card
chip     #232935   quadratino icona dentro la card
border   #272E3A   bordo 1 px
text     #E6EAF0   testo primario
muted    #848D9C   etichette, unita', caption
```

Accenti desaturati: `#7FB3D5` blu · `#E0908A` salmone · `#8FBF9F` salvia ·
`#D7A24C` ambra · `#9E8FC7` viola · `#D2645E` rosso.

### Tipografia

Poppins via `gfonts://` (geometrica arrotondata, e' quella che il riferimento
ricorda di piu'). Scala e destinazione d'uso:

| px | usata per |
|---|---|
| 40 | orologio nell'header |
| 30 | valore delle tile |
| 24 | titolo delle card-sezione (Casa, Giardino) |
| 20 | pill del nome pagina, pulsanti navbar |
| 16 | etichetta della tile, nomi persone |
| 14 | data, caption ESTERNO/INTERNO, unita' di misura |

- Sostituisce i `montserrat_XX` **builtin** di LVGL con asset generati: costo
  stimato **150–250 KB** di flash, su ~6.7 MB liberi.
- I glyph includono accenti, `°` e `€`: spariscono i font separati `font_temp` e
  `font_euro`, e i nomi dei giorni tornano accentati ("Mercoledi" -> "Mercoledì",
  oggi senza accento solo perche' il Montserrat builtin non ha `ì`).
- `font_mdi` resta invariato.

### Geometria

Card `radius: 16`, bordo 1 px, `pad_all: 14` · chip icona 44x44 `radius: 12` ·
pill `radius: 999` · griglia 3 colonne, gutter 16, margine pagina 20.

### Ombre

**Nessuna.** Su fondo scuro un'ombra si vede poco pagando comunque rendering; la
separazione la fanno bordo e contrasto card/fondo. Si valuta di aggiungerle solo
se il risultato appare piatto.

## Layout

### Bande fisse

```
  0–76    header (top_layer)
  76      riga separatrice 1 px
  84–500  contenuto (416 px utili, margine laterale 20 -> larghezza 984)
508–600   navbar (inline in ogni pagina)
```

### Header

Nel **`top_layer`**, non duplicato nelle pagine.

> **Vincolo strutturale.** Gli id LVGL devono essere unici e orologio, meteo e
> temperature sono label aggiornate dai sensori: quattro copie di `lbl_clock` non
> sono ammesse. Il `top_layer` e' la sede corretta, ed e' sicuro qui perche'
> l'avvertenza nota (i widget nel `top_layer` non ricevono il touch in modo
> affidabile) riguarda i widget interattivi, e l'header non ne ha nessuno.

- **Sinistra**: ora 40, data 14 sotto, poi la **sola icona della condizione
  meteo**, senza numero.
- **Centro**: pill col nome della pagina, aggiornata dal trigger `on_load` della
  pagina — un punto solo invece di 20 fra pulsanti navbar e rami del `select`.
- **Destra**: due letture `valore 20` + `caption 12 maiuscoletto`, **ESTERNO**
  (sensore giardino, piu' attendibile dell'OpenWeatherMap) e **INTERNO** (sala).

La temperatura meteo oggi accanto all'orologio si sposta nel campo ESTERNO: e'
l'impaginazione del riferimento e toglie il doppione fra meteo e sensore giardino.

### Anatomia della tile

Uguale su tutte le pagine: chip icona 44x44 a sinistra, etichetta 16 `muted` in
alto, valore 30 `text` sotto.

### Home — griglia 3x3 uniforme, celle 317x128

```
┌ Temp Giardino ┬ Temp Sala ─────┬ Temp Camera ───┐
├ Um. Giardino ─┼ Um. Sala ──────┼ Um. Camera ────┤
├ Potenza kW ───┼ Bolletta € ────┼ Persone ───────┤
└───────────────┴────────────────┴────────────────┘
```

Le persone diventano la terza cella dell'ultima riga, liberando l'angolo in alto a
destra ora occupato dall'header. La cella contiene due righe, una per persona:
nome a 16 `muted` a sinistra, stato ("Casa" / "Fuori") a 16 `text` a destra,
colorato salvia quando in casa e `muted` quando fuori.

### Casa — 4 card-sezione 2x2, 484x200

Routine (Esco / Buongiorno / Buonanotte) · Cucina (Caffe / Bollitore / Deum., con
accento colorato quando attivi) · Pulizie (stato Igor + Avvia) · Tapparelle
(su / stop / giu).

### Energia — 6 tile in griglia 3x2, 317x200

Spesa oggi · Previsione mese · Mese scorso · Consumo mese · Tesla € · Tesla kWh.

### Giardino

Due righe da 200, come Energia:

- Riga 1, tre celle 317: Luce esterna (toggle) · Consumo cucina · Portafinestra
- Riga 2, due celle 484: Temperatura giardino · Umidita' giardino

### Transizioni

`lvgl.page.show` con `animation: FADE_IN`, `time: 150ms`. Fade e non scorrimento
perche' quest'ultimo vorrebbe conoscere la direzione dello spostamento. Il
pannello gira a ~33 Hz, quindi 150 ms sono 5 frame: se risulta scattoso si passa a
`NONE` senza perdere nulla.

## Ordine di implementazione

1. Font Poppins e `style_definitions` (nessun cambio visibile finche' non usati)
2. Header nel `top_layer` + pill via `on_load` + navbar restilizzata
3. Home
4. Casa
5. Energia
6. Giardino
7. Transizioni di pagina

Flash e verifica a occhio dopo ogni passo, un commit per passo. Durante la
transizione convivono due estetiche: e' accettato.

## Vincoli da non violare

- `pclk_frequency` resta a **30 MHz**: alzarlo causa rollback silenzioso dell'OTA.
- Il widget `chart` **non esiste** in ESPHome.
- La **navbar resta dentro ogni pagina**, mai nel `top_layer`.
- Una label = **un solo font**: icone MDI e testo in widget separati.
- Niente `interval` per aggiornare le label: solo `on_value` / `on_time`.
- Siamo **solo OTA**: mai flashare firmware che rischi un reboot loop.

## Fuori scope

Restano voci separate della roadmap, da non mescolare a questo lavoro:

- spegnimento del backlight dopo 30 s di inattivita'
- sincronizzazione inversa del `select` pagina verso Home Assistant
- IP statico riservato nel router

## Criteri di completamento

- Le quattro pagine condividono palette, tipografia, raggi e spaziature.
- Ogni valore mostrato oggi e' ancora mostrato, con la stessa entita' HA.
- Ogni comando (script, switch, robot, tapparelle, luce) funziona come prima.
- Nessun `[E]`/`[W]` nei log e nessun rollback OTA dopo l'ultimo flash.
