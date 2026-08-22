# Restyling UI della plancia — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rifare il look delle quattro pagine LVGL della plancia adottando il linguaggio visivo degli screenshot di riferimento in palette scura, senza cambiare nulla di funzionale.

**Architecture:** Un sistema di stili condivisi (`style_definitions`) applicato via `styles:` a tutti i widget, un header globale non duplicabile ospitato nel `top_layer`, e le letture rese come tile piatte uniformi invece che come gauge ad arco. Si procede una pagina alla volta, con flash e verifica a occhio dopo ogni passo.

**Tech Stack:** ESPHome 2026.5.3, LVGL 9.5, font Google via `gfonts://`, display `mipi_rgb` 1024x600, Home Assistant come sorgente dati.

**Spec:** `docs/superpowers/specs/2026-08-22-restyling-ui-plancia-design.md`

## Nota sul ciclo di verifica

Questo progetto non ha un framework di test e non ha senso inventarne uno: il
prodotto e' un firmware che disegna su un pannello. Il gate di ogni task e'
quindi questa sequenza, che sostituisce il ciclo red/green:

```bash
esphome config  test-display3.yaml     # 1. la configurazione e' valida
esphome compile test-display3.yaml     # 2. i lambda C++ compilano
esphome upload  test-display3.yaml --device test-plancia.local   # 3. flash
esphome logs    test-display3.yaml --device test-plancia.local   # 4. verifica
```

Al punto 4 si controllano tre cose: che nel dump di boot compaiano gli
`Entity ID:` attesi (prova che **non c'e' stato rollback** — `OTA successful` da
solo non lo garantisce), che non ci siano righe `[E]`/`[W]`, e che i valori
arrivino. La verifica visiva a schermo la fa l'utente e **e' parte del gate**:
non si passa al task successivo senza la sua conferma.

Dopo ogni flash puo' servire ricaricare l'integrazione ESPHome in HA perche' i
sensori ricevano di nuovo lo stato:

```bash
curl -s -X POST -H "Authorization: Bearer $(cat ~/.ha_token)" \
  -H "Content-Type: application/json" \
  -d '{"entity_id":"number.studio_test_plancia_luminosita_backlight"}' \
  http://homeassistant.local:8123/api/services/homeassistant/reload_config_entry
```

## Global Constraints

- `pclk_frequency` resta **30MHz**: alzarlo causa crash al boot e rollback silenzioso dell'OTA.
- Siamo **solo OTA**, nessun USB collegato: mai flashare firmware che rischi un reboot loop, non c'e' erase di recupero.
- Il widget `chart` **non esiste** in ESPHome e `LV_USE_CHART` e' forzato a 0.
- La **navbar resta inline in ogni pagina**, mai nel `top_layer`: li' i widget non ricevono il touch in modo affidabile.
- Nel `top_layer` va **solo roba non interattiva** (l'header).
- Una label usa **un solo font**: icona MDI e testo sono widget separati.
- Mai `interval` per aggiornare le label: solo `on_value` / `on_time`, gli altri contesti crashano il boot.
- Gli **id LVGL sono unici**: nessun widget puo' esistere in due pagine.
- Palette, in esadecimale LVGL: `bg 0x0E1116` · `surface 0x171C24` · `chip 0x232935` · `border 0x272E3A` · `text 0xE6EAF0` · `muted 0x848D9C`. Accenti: `blu 0x7FB3D5` · `salmone 0xE0908A` · `salvia 0x8FBF9F` · `ambra 0xD7A24C` · `viola 0x9E8FC7` · `rosso 0xD2645E`.
- Bande fisse: header 0–76 · separatore a 76 · contenuto 84–500 (margine laterale 20, larghezza 984) · navbar 508–600.

## File Structure

Un solo file, come oggi. Lo split in piu' file con `packages:` e' stato
**valutato e scartato** in fase di design: e' un cambio strutturale in aggiunta al
restyle, e gli id LVGL condivisi fra pagine lo complicano.

`test-display3.yaml` (1526 righe alla stesura del piano), sezioni toccate:

| Righe | Sezione | Cosa cambia |
|---|---|---|
| 63–120 | `font:` | Task 1 aggiunge i Poppins e `font_mdi_24`; Task 9 rimuove `font_temp`/`font_euro` |
| 121–297 | `sensor:` | le azioni `lvgl.arc.update` spariscono con gli archi; nuovi target per ESTERNO/INTERNO |
| 298–371 | `text_sensor:` | il sensore meteo perde l'aggiornamento della temperatura |
| 508–514 | `lvgl:` | Task 1 aggiunge `style_definitions`, Task 2 aggiunge `top_layer` |
| 515–906 | `page_home` | Task 4 |
| 907–1103 | `page_casa` | Task 5 |
| 1104–1337 | `page_energia` | Task 6 |
| 1338–1468 | `page_giardino` | Task 7 |

---

## Task 1: Font Poppins, font MDI piccolo e stili condivisi

Nessun cambiamento visibile: si aggiungono risorse che i task successivi useranno.
I font vecchi **restano** perche' i widget attuali li usano ancora; si rimuovono
nel Task 9, quando l'ultimo utilizzatore e' sparito.

**Files:**
- Modify: `test-display3.yaml` — sezione `font:` (dopo la riga 120, in coda ai font esistenti)
- Modify: `test-display3.yaml` — blocco `lvgl:` (inserire `style_definitions:` dopo `touchscreens:`, riga 513)

**Interfaces:**
- Produces: i font `f_40 f_30 f_24 f_20 f_16 f_14 font_mdi_24`
- Produces: gli stili `st_card st_chip st_pill st_label st_value st_caption st_nav st_nav_active`
- Consumes: niente

**Dimensionamento dei glyphset — motivo.** Il glyphset di default `GF_Latin_Kernel`
ha `°` e `€` ma **non** gli accenti italiani (`ì à è é ò ù`); `GF_Latin_Core` li ha
tutti ma sono 319 glifi. Applicarlo a tutte le taglie costerebbe ~367 KB. Quindi:
`GF_Latin_Core` solo alle taglie che disegnano testo (24/20/16/14, ~133 KB) e una
lista `glyphs` esplicita alle due taglie che disegnano solo numeri (40/30, ~12 KB).
Totale atteso **~145 KB**.

- [ ] **Step 1: Aggiungere i font**

In coda alla sezione `font:`, dopo il blocco `font_mdi`:

```yaml
  # --- Poppins: tipografia del restyling (vedi spec) ---
  # Taglie "numeriche": lista glyphs esplicita, non serve tutto il Latin Core.
  - file: "gfonts://Poppins@500"
    id: f_40
    size: 40
    bpp: 4
    glyphs: "0123456789:.- "
  - file: "gfonts://Poppins@500"
    id: f_30
    size: 30
    bpp: 4
    glyphs: "0123456789.,:-+ °€%kWhVA"
  # Taglie "testuali": Latin Core, che include gli accenti italiani.
  - file: "gfonts://Poppins@400"
    id: f_24
    size: 24
    bpp: 4
    glyphsets: [GF_Latin_Core]
  - file: "gfonts://Poppins@400"
    id: f_20
    size: 20
    bpp: 4
    glyphsets: [GF_Latin_Core]
  - file: "gfonts://Poppins@400"
    id: f_16
    size: 16
    bpp: 4
    glyphsets: [GF_Latin_Core]
  - file: "gfonts://Poppins@400"
    id: f_14
    size: 14
    bpp: 4
    glyphsets: [GF_Latin_Core]
```

Poi un secondo font MDI a 24 px, perche' `font_mdi` e' a 40 e non entra nel chip
icona da 44x44. Stessa identica lista di glifi di `font_mdi` (copiarla dal blocco
esistente, inclusi i 15 glifi meteo), cambiano solo `id` e `size`:

```yaml
  - file:
      type: web
      url: "https://github.com/Templarian/MaterialDesign-Webfont/raw/master/fonts/materialdesignicons-webfont.ttf"
    id: font_mdi_24
    size: 24
    bpp: 4
    glyphs:
      - "\U000F0114"  # cash (spesa oggi)
      - "\U000F1A91"  # cash-clock (previsione mese)
      - "\U000F02DA"  # history (mese scorso)
      - "\U000F140B"  # lightning-bolt (consumo)
      - "\U000F0B6C"  # car-electric (Tesla)
      - "\U000F0A48"  # exit-run (Esco)
      - "\U000F059C"  # weather-sunset-up (Buongiorno)
      - "\U000F0594"  # weather-night (Buonanotte)
      - "\U000F0176"  # coffee (Caffe)
      - "\U000F05FA"  # kettle (Bollitore)
      - "\U000F1099"  # air-humidifier (Deumidificatore)
      - "\U000F070D"  # robot-vacuum (Igor)
      - "\U000F111C"  # window-shutter (tapparelle)
      - "\U000F0143"  # chevron-up (tapparelle su)
      - "\U000F0140"  # chevron-down (tapparelle giu)
      - "\U000F04DB"  # stop (tapparelle stop)
      - "\U000F050F"  # thermometer (temp giardino)
      - "\U000F058E"  # water-percent (umidita giardino)
      - "\U000F0335"  # lightbulb (luce cucina esterna)
      - "\U000F0241"  # flash (consumo cucina)
      - "\U000F11DC"  # window-open-variant (portafinestra aperta)
      - "\U000F11DB"  # window-closed-variant (portafinestra chiusa)
      # --- meteo (stati weather.* di Home Assistant) ---
      - "\U000F0599"  # weather-sunny (sunny)
      # clear-night riusa il glifo weather-night gia' dichiarato sopra (Buonanotte)
      - "\U000F0595"  # weather-partly-cloudy (partlycloudy)
      - "\U000F0F31"  # weather-night-partly-cloudy
      - "\U000F0590"  # weather-cloudy (cloudy / fallback)
      - "\U000F0591"  # weather-fog (fog)
      - "\U000F0592"  # weather-hail (hail)
      - "\U000F0593"  # weather-lightning (lightning)
      - "\U000F067E"  # weather-lightning-rainy
      - "\U000F0596"  # weather-pouring (pouring)
      - "\U000F0597"  # weather-rainy (rainy)
      - "\U000F0598"  # weather-snowy (snowy)
      - "\U000F067F"  # weather-snowy-rainy
      - "\U000F059D"  # weather-windy (windy)
      - "\U000F059E"  # weather-windy-variant
      - "\U000F05D6"  # alert-circle-outline (exceptional)
```

- [ ] **Step 2: Aggiungere gli stili condivisi**

Nel blocco `lvgl:`, subito dopo `touchscreens:` e prima di `pages:`:

```yaml
  style_definitions:
    - id: st_card
      bg_color: 0x171C24
      bg_opa: COVER
      radius: 16
      border_width: 1
      border_color: 0x272E3A
      border_opa: COVER
      pad_all: 14
    - id: st_chip
      bg_color: 0x232935
      bg_opa: COVER
      radius: 12
      border_width: 0
      pad_all: 0
    - id: st_pill
      bg_color: 0x171C24
      bg_opa: COVER
      radius: 40
      border_width: 1
      border_color: 0x272E3A
      border_opa: COVER
      pad_all: 0
    - id: st_label
      text_color: 0x848D9C
      text_font: f_16
    - id: st_value
      text_color: 0xE6EAF0
      text_font: f_30
    - id: st_caption
      text_color: 0x848D9C
      text_font: f_14
    - id: st_nav
      bg_color: 0x171C24
      bg_opa: COVER
      radius: 14
      border_width: 0
      text_color: 0x848D9C
      text_font: f_20
    - id: st_nav_active
      bg_color: 0x232935
      bg_opa: COVER
      radius: 14
      border_width: 1
      border_color: 0x7FB3D5
      border_opa: COVER
      text_color: 0xE6EAF0
      text_font: f_20
```

Nota su `st_pill`: LVGL rende un raggio maggiore di meta' altezza come pillola;
40 su un'altezza di 40 basta, non serve un valore sentinella.

- [ ] **Step 3: Validare la configurazione**

Run: `esphome config test-display3.yaml | tail -3`
Expected: `INFO Configuration is valid!`

Se fallisce con `Found duplicate glyph`, un codepoint compare due volte nella
lista di `font_mdi_24`: e' successo gia' una volta con `F0594`, che serve sia a
`weather-night` sia al pulsante Buonanotte.

- [ ] **Step 4: Compilare e misurare il costo reale in flash**

Run: `esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"`
Expected: `Successfully compiled`, e `Flash:` cresciuto di **~145 KB** rispetto a
1359635 byte, quindi circa 1.50 MB (18–19%). Se e' cresciuto di piu' di 400 KB,
un font testuale sta usando un glyphset troppo largo: ricontrollare lo Step 1.

Non si flasha: questo task non cambia nulla a schermo.

- [ ] **Step 5: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 1/9: font Poppins, font MDI a 24 px e stili condivisi"
```

---

## Task 2: Header globale nel top_layer

**Files:**
- Modify: `test-display3.yaml` — blocco `lvgl:`, aggiungere `top_layer:` dopo `style_definitions:`
- Modify: `test-display3.yaml` — `page_home`, rimuovere i widget `lbl_clock`, `lbl_date`, `lbl_meteo_icon`, `lbl_meteo_temp`
- Modify: `test-display3.yaml` — sensori `temp_giardino` e `temp_sala`, aggiungere l'aggiornamento di ESTERNO/INTERNO
- Modify: `test-display3.yaml` — sensore `meteo_temp`, da rimuovere

**Interfaces:**
- Consumes: `f_40 f_20 f_16 f_14 font_mdi st_pill st_caption` dal Task 1
- Produces: `lbl_clock lbl_date lbl_meteo_icon lbl_page_name lbl_ext lbl_int` nel `top_layer`
- Produces: la convenzione per cui ogni pagina aggiorna `lbl_page_name` dal proprio `on_load`

**Perche' nel `top_layer`.** Gli id LVGL sono unici e queste label sono aggiornate
dai sensori: quattro copie di `lbl_clock`, una per pagina, non sono ammesse. Il
`top_layer` e' l'unica sede possibile, ed e' sicuro qui perche' il problema noto
del `top_layer` riguarda i widget che devono **ricevere il touch**, e l'header non
ne ha nessuno.

- [ ] **Step 1: Spostare orologio, data e icona meteo nel top_layer**

Rimuovere da `page_home` i widget con id `lbl_clock`, `lbl_date`, `lbl_meteo_icon`
e `lbl_meteo_temp`.

> **Non toccare i due riquadri delle persone in questo task.** I text_sensor
> `pers_1` e `pers_2` aggiornano `lbl_fra` e `lbl_cos`: rimuovendo i riquadri qui,
> quelle azioni punterebbero a id inesistenti e `esphome config` fallirebbe con
> `Couldn't find ID 'lbl_fra'`. I riquadri spariscono e rinascono come cella della
> griglia nel Task 4, in un colpo solo. Per un task resteranno nascosti sotto
> l'header, che essendo nel `top_layer` disegna sopra: e' accettato.

Poi, nel blocco `lvgl:` dopo `style_definitions:`, aggiungere:

```yaml
  top_layer:
    widgets:
      - label:
          id: lbl_clock
          align: TOP_LEFT
          x: 24
          y: 6
          text: "--:--"
          text_font: f_40
          text_color: 0xE6EAF0
      - label:
          id: lbl_date
          align: TOP_LEFT
          x: 26
          y: 54
          text: "sincronizzazione..."
          styles: [st_caption]
      - label:
          id: lbl_meteo_icon
          align: TOP_LEFT
          x: 190
          y: 18
          text: "\U000F0590"
          text_font: font_mdi
          text_color: 0x848D9C
      - obj:
          styles: [st_pill]
          align: TOP_MID
          x: 0
          y: 18
          width: 200
          height: 40
          widgets:
            - label:
                id: lbl_page_name
                align: CENTER
                text: "Home"
                text_font: f_20
                text_color: 0xE6EAF0
      - label:
          id: lbl_ext
          align: TOP_RIGHT
          x: -150
          y: 16
          text: "--°"
          text_font: f_20
          text_color: 0xE6EAF0
      - label:
          align: TOP_RIGHT
          x: -150
          y: 44
          text: "ESTERNO"
          styles: [st_caption]
      - label:
          id: lbl_int
          align: TOP_RIGHT
          x: -24
          y: 16
          text: "--°"
          text_font: f_20
          text_color: 0xE6EAF0
      - label:
          align: TOP_RIGHT
          x: -24
          y: 44
          text: "INTERNO"
          styles: [st_caption]
      - obj:
          align: TOP_MID
          x: 0
          y: 76
          width: 1024
          height: 1
          bg_color: 0x272E3A
          bg_opa: COVER
          border_width: 0
          radius: 0
          pad_all: 0
```

- [ ] **Step 2: Alimentare ESTERNO e INTERNO**

Nel sensore `temp_giardino` (riga ~205), aggiungere in coda alle azioni `on_value`:

```yaml
      - lvgl.label.update:
          id: lbl_ext
          text: !lambda 'return str_sprintf("%.1f°", x);'
```

Nel sensore `temp_sala`, aggiungere la stessa azione con `id: lbl_int`.

- [ ] **Step 3: Rimuovere il sensore della temperatura meteo**

Cancellare l'intero blocco `- platform: homeassistant / id: meteo_temp` (sensore
con `attribute: temperature`). Dopo lo spostamento la sua unica label,
`lbl_meteo_temp`, non esiste piu': ESTERNO usa il sensore fisico del giardino, che
in fase di design e' stato giudicato piu' attendibile dell'OpenWeatherMap.
Lasciarlo darebbe `Couldn't find ID 'lbl_meteo_temp'` in validazione.

- [ ] **Step 4: Far aggiornare la pill dal caricamento pagina**

Su **ognuna** delle quattro pagine, come primo figlio del blocco della pagina
(allo stesso livello di `widgets:`), aggiungere il trigger, cambiando la stringa:

```yaml
      on_load:
        - lvgl.label.update:
            id: lbl_page_name
            text: "Home"        # "Casa" / "Energia" / "Giardino" sulle altre
```

- [ ] **Step 5: Validare, compilare, flashare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
```
Expected: `Configuration is valid!`, `Successfully compiled`, `OTA successful`.

- [ ] **Step 6: Verificare che non ci sia stato rollback**

Run: `esphome logs test-display3.yaml --device test-plancia.local`
Expected: nessuna riga `[E]`/`[W]`; ricaricare l'integrazione HA (comando in cima
al piano) e vedere arrivare `sensor.sensore_giardino_temperatura` e
`sensor.hub_sala_temperatura`.

**Verifica visiva, da chiedere all'utente:** l'header appare su tutte e quattro le
pagine, la pill cambia nome quando si naviga, ESTERNO e INTERNO mostrano valori
sensati, e la Home in questo momento e' volutamente monca in alto.

- [ ] **Step 7: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 2/9: header globale nel top_layer, pill del nome pagina via on_load"
```

---

## Task 3: Navbar restilizzata

**Files:**
- Modify: `test-display3.yaml` — il blocco navbar in fondo a ognuna delle quattro pagine

**Interfaces:**
- Consumes: `st_nav st_nav_active` dal Task 1
- Produces: niente di nuovo

- [ ] **Step 1: Riscrivere la navbar in tutte e quattro le pagine**

Sostituire il blocco navbar esistente. Sotto la versione per `page_home`: sulle
altre pagine cambia **solo** quale pulsante usa `st_nav_active` invece di `st_nav`.

```yaml
        - obj:
            align: BOTTOM_MID
            width: 1024
            height: 92
            bg_color: 0x0B0D12
            bg_opa: COVER
            border_width: 0
            radius: 0
            pad_all: 0
            widgets:
              - button:
                  styles: [st_nav_active]     # Home: attivo
                  align: LEFT_MID
                  x: 20
                  width: 231
                  height: 60
                  widgets:
                    - label:
                        align: CENTER
                        text: "Home"
                  on_click:
                    - lvgl.page.show: page_home
              - button:
                  styles: [st_nav]
                  align: LEFT_MID
                  x: 267
                  width: 231
                  height: 60
                  widgets:
                    - label:
                        align: CENTER
                        text: "Casa"
                  on_click:
                    - lvgl.page.show: page_casa
              - button:
                  styles: [st_nav]
                  align: LEFT_MID
                  x: 514
                  width: 231
                  height: 60
                  widgets:
                    - label:
                        align: CENTER
                        text: "Energia"
                  on_click:
                    - lvgl.page.show: page_energia
              - button:
                  styles: [st_nav]
                  align: LEFT_MID
                  x: 761
                  width: 231
                  height: 60
                  widgets:
                    - label:
                        align: CENTER
                        text: "Giardino"
                  on_click:
                    - lvgl.page.show: page_giardino
```

Le label non dichiarano font o colore: li ereditano dallo stile del pulsante
padre, `st_nav`/`st_nav_active`. **Se dopo il flash i testi della navbar
risultassero col font di default invece che Poppins**, l'ereditarieta' non ha
funzionato: rimediare aggiungendo `text_font: f_20` e `text_color:` esplicito su
ciascuna delle sedici label. Verificarlo a schermo prima di dare per buono il task.
Geometria: margine 20, quattro pulsanti da 231 con gutter 16 (20 + 4*231 + 3*16 = 992,
piu' 20 di margine destro = 1012, dentro i 1024 con 12 px di aria).

- [ ] **Step 2: Validare, compilare, flashare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
```

- [ ] **Step 3: Verifica**

**Da chiedere all'utente, ed e' il punto critico di questo task:** i quattro
pulsanti rispondono ancora al tocco su tutte le pagine, e il pulsante della pagina
corrente e' quello evidenziato. Se il touch non risponde, il sospetto e' la
geometria dei pulsanti, non lo stile.

- [ ] **Step 4: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 3/9: navbar con stili condivisi"
```

---

## Task 4: Home — griglia 3x3 di tile

**Files:**
- Modify: `test-display3.yaml` — `page_home`, sostituire tutti gli archi e le label
- Modify: `test-display3.yaml` — sensori `um_giardino um_sala um_camera potenza bolletta temp_giardino temp_sala temp_camera`: rimuovere le azioni `lvgl.arc.update`

**Interfaces:**
- Consumes: `st_card st_chip st_label st_value f_16 f_30 font_mdi_24` dai Task 1
- Produces: nessun id nuovo. Gli id delle label **restano quelli di oggi**
  (`lbl_t_g lbl_t_s lbl_t_c lbl_um_g lbl_um_s lbl_um_c lbl_pot lbl_bol lbl_fra lbl_cos`)
  cosi' le azioni dei sensori continuano a funzionare senza toccarle.

**Griglia.** Celle 317x128, margine 20, gutter 16.
Colonne x = 20 / 353 / 686. Righe y = 84 / 228 / 372.

| Cella | x, y | icona MDI | colore chip | etichetta | id valore |
|---|---|---|---|---|---|
| Temp giardino | 20, 84 | `\U000F050F` thermometer | 0x8FBF9F salvia | Giardino | `lbl_t_g` |
| Temp sala | 353, 84 | `\U000F050F` thermometer | 0x8FBF9F salvia | Sala | `lbl_t_s` |
| Temp camera | 686, 84 | `\U000F050F` thermometer | 0x8FBF9F salvia | Camera | `lbl_t_c` |
| Um. giardino | 20, 228 | `\U000F058E` water-percent | 0x7FB3D5 blu | Um. Giardino | `lbl_um_g` |
| Um. sala | 353, 228 | `\U000F058E` water-percent | 0x7FB3D5 blu | Um. Sala | `lbl_um_s` |
| Um. camera | 686, 228 | `\U000F058E` water-percent | 0x7FB3D5 blu | Um. Camera | `lbl_um_c` |
| Potenza | 20, 372 | `\U000F140B` lightning-bolt | 0xD7A24C ambra | Potenza kW | `lbl_pot` |
| Bolletta | 353, 372 | `\U000F0114` cash | 0x9E8FC7 viola | Bolletta € | `lbl_bol` |
| Persone | 686, 372 | — | — | — | `lbl_fra`, `lbl_cos` |

- [ ] **Step 1: Scrivere la tile canonica**

Questo e' il blocco completo della prima cella. Le altre sette tile sono **lo
stesso identico blocco** con `x`, `y`, glifo, colore del chip, testo
dell'etichetta e `id` del valore presi dalla riga corrispondente della tabella
qui sopra. Non c'e' nient'altro da inventare.

```yaml
        - obj:
            styles: [st_card]
            align: TOP_LEFT
            x: 20
            y: 84
            width: 317
            height: 128
            widgets:
              - obj:
                  styles: [st_chip]
                  align: TOP_LEFT
                  x: 0
                  y: 0
                  width: 44
                  height: 44
                  widgets:
                    - label:
                        align: CENTER
                        text: "\U000F050F"
                        text_font: font_mdi_24
                        text_color: 0x8FBF9F
              - label:
                  align: TOP_LEFT
                  x: 58
                  y: 2
                  text: "Giardino"
                  styles: [st_label]
              - label:
                  id: lbl_t_g
                  align: TOP_LEFT
                  x: 58
                  y: 26
                  text: "--"
                  styles: [st_value]
```

- [ ] **Step 2: Scrivere la cella Persone**

Non ha chip icona: due righe nome/stato.

```yaml
        - obj:
            styles: [st_card]
            align: TOP_LEFT
            x: 686
            y: 372
            width: 317
            height: 128
            widgets:
              - label:
                  align: TOP_LEFT
                  x: 0
                  y: 8
                  text: !secret nome_persona1
                  styles: [st_label]
              - label:
                  id: lbl_fra
                  align: TOP_RIGHT
                  x: 0
                  y: 8
                  text: "--"
                  text_font: f_16
                  text_color: 0xE6EAF0
              - label:
                  align: TOP_LEFT
                  x: 0
                  y: 56
                  text: !secret nome_persona2
                  styles: [st_label]
              - label:
                  id: lbl_cos
                  align: TOP_RIGHT
                  x: 0
                  y: 56
                  text: "--"
                  text_font: f_16
                  text_color: 0xE6EAF0
```

- [ ] **Step 3: Colorare lo stato delle persone**

Nei text_sensor `pers_1` e `pers_2` la label si aggiorna gia'. Aggiungere il
colore, salvia in casa e muted fuori, sostituendo l'azione esistente con:

```yaml
      - lvgl.label.update:
          id: lbl_fra          # lbl_cos per pers_2
          text: !lambda 'return x == "home" ? std::string("Casa") : std::string("Fuori");'
          text_color: !lambda 'return x == "home" ? lv_color_hex(0x8FBF9F) : lv_color_hex(0x848D9C);'
```

`text_color` accetta un lambda purche' torni un `lv_color_t`, quindi
`lv_color_hex()`.

- [ ] **Step 4: Rimuovere le azioni sugli archi**

Gli archi non esistono piu'. In ognuno degli otto sensori della Home cancellare
l'azione `- lvgl.arc.update:` con il suo `id:` e il suo `value:`. Restano solo le
`lvgl.label.update`. Se ne resta anche una sola, la validazione fallisce con
`Couldn't find ID 'arc_...'`.

Controllo: `grep -n "lvgl.arc.update" test-display3.yaml` deve non restituire nulla.

- [ ] **Step 5: Validare, compilare, flashare, verificare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
esphome logs    test-display3.yaml --device test-plancia.local
```
Poi ricaricare l'integrazione HA e verificare che tutte e otto le letture arrivino.

**Verifica visiva:** nove celle allineate, valori corretti, niente testo tagliato.

- [ ] **Step 6: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 4/9: Home a griglia 3x3 di tile, via i gauge ad arco"
```

---

## Task 5: Casa — quattro card-sezione

**Files:**
- Modify: `test-display3.yaml` — `page_casa`

**Interfaces:**
- Consumes: `st_card st_chip st_label f_24 f_16 font_mdi_24` dal Task 1
- Produces: nessun id nuovo; restano `btn_caffe btn_bollitore btn_deum lbl_igor`,
  perche' i binary_sensor `st_caffe st_bollitore st_deum` e il text_sensor
  `st_vacuum` li aggiornano.

**Griglia:** quattro card 484x200. x = 20 / 520. y = 84 / 300.

**Card Routine** (20, 84) — tre pulsanti a tutta larghezza, y 44 / 90 / 136:

| Testo | icona | colore icona | servizio | entity_id |
|---|---|---|---|---|
| Esco | `\U000F0A48` | 0x8FBF9F | `script.turn_on` | `!secret ent_script_esco` |
| Buongiorno | `\U000F059C` | 0xD7A24C | `script.turn_on` | `!secret ent_script_buongiorno` |
| Buonanotte | `\U000F0594` | 0x9E8FC7 | `script.turn_on` | `!secret ent_script_buonanotte` |

**Card Cucina** (520, 84) — tre pulsanti a tutta larghezza, y 44 / 90 / 136:

| Testo | id | icona | servizio | entity_id |
|---|---|---|---|---|
| Caffe | `btn_caffe` | `\U000F0176` | `switch.toggle` | `!secret ent_sw_caffe` |
| Bollitore | `btn_bollitore` | `\U000F05FA` | `switch.toggle` | `!secret ent_sw_bollitore` |
| Deum. | `btn_deum` | `\U000F1099` | `switch.toggle` | `!secret ent_sw_deum` |

Il colore dell'icona di questi tre lo gestisce il feedback di stato (Step 2), a
riposo `0x848D9C`.

**Card Pulizie** (20, 300) — chip icona `\U000F070D` colore 0x7FB3D5, titolo
"Igor", la label `lbl_igor` con lo stato allineata a destra, e sotto un pulsante a
tutta larghezza "Avvia" che chiama `vacuum.start` su `!secret ent_vacuum`.

**Card Tapparelle** (520, 300) — tre pulsanti affiancati da **144**x60 a y 100,
x 0 / 156 / 312, con la sola icona centrata. La larghezza interna della card e'
456 (484 meno `pad_all: 14` per lato): 144x3 + 12x2 = 456 esatto. Con 148, come
prescriveva la prima stesura, l'ultimo pulsante sforerebbe di 4 px.

| icona | colore | servizio | entity_id |
|---|---|---|---|
| `\U000F0143` su | 0x8FBF9F | `cover.open_cover` | `!secret ent_cover_tapparelle` |
| `\U000F04DB` stop | 0xD2645E | `cover.stop_cover` | `!secret ent_cover_tapparelle` |
| `\U000F0140` giu | 0xD7A24C | `cover.close_cover` | `!secret ent_cover_tapparelle` |

- [ ] **Step 1: Scrivere la card canonica con i suoi pulsanti**

Card Routine, completa. Le altre tre hanno la stessa struttura — contenitore
`st_card`, titolo a `f_24`, pulsanti `st_nav` — e prendono testi, icone, colori,
servizi ed entity_id dalle tabelle qui sopra, che li riportano tutti. Servizi ed
entity_id sono **gli stessi di oggi**: questo task cambia l'aspetto, non il
comportamento.

```yaml
        - obj:
            styles: [st_card]
            align: TOP_LEFT
            x: 20
            y: 84
            width: 484
            height: 200
            widgets:
              - label:
                  align: TOP_LEFT
                  x: 0
                  y: 0
                  text: "Routine"
                  text_font: f_24
                  text_color: 0xE6EAF0
              - button:
                  styles: [st_nav]
                  align: TOP_LEFT
                  x: 0
                  y: 44
                  width: 456
                  height: 34
                  widgets:
                    - label:
                        align: LEFT_MID
                        x: 12
                        text: "\U000F0A48"
                        text_font: font_mdi_24
                        text_color: 0x7FB3D5
                    - label:
                        align: LEFT_MID
                        x: 48
                        text: "Esco"
                  on_click:
                    - homeassistant.service:
                        service: script.turn_on
                        data:
                          entity_id: !secret ent_script_esco
```

Icona e testo sono due label affiancate con `align: LEFT_MID` e `x` diversi:
impilarle in verticale dentro un pulsante basso le fa sovrapporre, ed e' un errore
gia' fatto in passato su questa pagina.

- [ ] **Step 2: Mantenere il feedback di colore degli switch cucina**

I binary_sensor `st_caffe`, `st_bollitore`, `st_deum` fanno gia'
`lvgl.widget.update: { id: btn_caffe, bg_color: ... }`. Aggiornare **solo** i
colori alla nuova palette: acceso `0xE0908A` per caffe e bollitore, `0x7FB3D5`
per il deumidificatore; spento `0x171C24` per tutti e tre.

- [ ] **Step 3: Validare, compilare, flashare, verificare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
```

**Verifica visiva e funzionale, da chiedere all'utente:** questa e' la pagina con i
comandi reali. Vanno provati **tutti**: le tre routine partono, i tre switch
cucina commutano e il pulsante si colora, il robot parte, le tapparelle si muovono.

- [ ] **Step 4: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 5/9: pagina Casa a card-sezione"
```

---

## Task 6: Energia — sei tile 3x2

**Files:**
- Modify: `test-display3.yaml` — `page_energia`

**Interfaces:**
- Consumes: la tile canonica del Task 4
- Produces: nessun id nuovo; restano `lbl_e_oggi lbl_e_previsione lbl_e_prec lbl_e_consumo lbl_e_tesla_eur lbl_e_tesla_kwh`

**Griglia:** celle 317x200. Colonne x = 20 / 353 / 686. Righe y = 84 / 300.

| Cella | x, y | icona MDI | colore chip | etichetta | id valore |
|---|---|---|---|---|---|
| Spesa oggi | 20, 84 | `\U000F0114` cash | 0x8FBF9F salvia | Spesa oggi | `lbl_e_oggi` |
| Previsione mese | 353, 84 | `\U000F1A91` cash-clock | 0xD7A24C ambra | Previsione mese | `lbl_e_previsione` |
| Mese scorso | 686, 84 | `\U000F02DA` history | 0x848D9C muted | Mese scorso | `lbl_e_prec` |
| Consumo mese | 20, 300 | `\U000F140B` lightning-bolt | 0x7FB3D5 blu | Consumo mese | `lbl_e_consumo` |
| Tesla € | 353, 300 | `\U000F0B6C` car-electric | 0x9E8FC7 viola | Tesla mese | `lbl_e_tesla_eur` |
| Tesla kWh | 686, 300 | `\U000F0B6C` car-electric | 0x9E8FC7 viola | Tesla kWh mese | `lbl_e_tesla_kwh` |

- [ ] **Step 1: Scrivere le sei tile**

Stesso blocco della tile canonica del Task 4 (Step 1), con `height: 200` invece di
128 e i valori della tabella qui sopra. Con 200 px di altezza etichetta e valore
restano in alto: va bene, la tile respira.

- [ ] **Step 2: Validare, compilare, flashare, verificare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
```
Ricaricare l'integrazione HA. Attenzione al valore Tesla kWh: e' la **somma delle
tre fasce**, deve leggere ~126 kWh e non 0.

- [ ] **Step 3: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 6/9: pagina Energia a griglia di tile"
```

---

## Task 7: Giardino

**Files:**
- Modify: `test-display3.yaml` — `page_giardino`

**Interfaces:**
- Consumes: la tile canonica del Task 4
- Produces: nessun id nuovo; restano `btn_luce_cucina lbl_g_consumo lbl_g_temp lbl_g_um lbl_porta lbl_porta_icon`

**Griglia:** due righe da 200, y = 84 / 300.
Riga 1 tre celle da 317 (x = 20 / 353 / 686), riga 2 due celle da 484 (x = 20 / 520).

| Cella | x, y | w | icona MDI | colore chip | etichetta | id |
|---|---|---|---|---|---|---|
| Luce esterna | 20, 84 | 317 | `\U000F0335` lightbulb | 0xD7A24C ambra | Luce esterna | `btn_luce_cucina` |
| Consumo cucina | 353, 84 | 317 | `\U000F0241` flash | 0x7FB3D5 blu | Consumo cucina | `lbl_g_consumo` |
| Portafinestra | 686, 84 | 317 | `lbl_porta_icon` | dinamico | Portafinestra | `lbl_porta` |
| Temperatura | 20, 300 | 484 | `\U000F050F` thermometer | 0x8FBF9F salvia | Temperatura | `lbl_g_temp` |
| Umidita | 520, 300 | 484 | `\U000F058E` water-percent | 0x7FB3D5 blu | Umidita | `lbl_g_um` |

- [ ] **Step 1: Scrivere le cinque celle**

Le due della riga 2 e quella del consumo sono la tile canonica del Task 4. La cella
Luce esterna e' una tile il cui contenitore e' un `button` con
`id: btn_luce_cucina`, e il cui `on_click` e':

```yaml
                  on_click:
                    - homeassistant.service:
                        service: light.toggle
                        data:
                          entity_id: !secret ent_luce_cucina_est
``` La cella Portafinestra ha, al posto del glifo fisso nel chip, la
label `lbl_porta_icon`, che il binary_sensor `st_porta` gia' commuta fra
`\U000F11DC` aperta e `\U000F11DB` chiusa.

- [ ] **Step 2: Aggiornare i colori del feedback portafinestra e luce**

Nel binary_sensor `st_porta` sostituire i colori: aperta `0xD2645E`, chiusa
`0x8FBF9F`. Nel binary_sensor `st_luce_cucina`: accesa `0xD7A24C`, spenta
`0x171C24`.

- [ ] **Step 3: Validare, compilare, flashare, verificare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
```

**Verifica funzionale:** il toggle della luce esterna accende davvero, e l'icona
della portafinestra cambia aprendola.

- [ ] **Step 4: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 7/9: pagina Giardino"
```

---

## Task 8: Transizioni di pagina

**Files:**
- Modify: `test-display3.yaml` — i 16 `on_click` della navbar e i 4 rami del `select` (righe ~1469-1501)

**Interfaces:**
- Consumes: le pagine dei Task 4-7

- [ ] **Step 1: Aggiungere l'animazione a ogni cambio pagina**

Sostituire ogni `- lvgl.page.show: page_xxx` con la forma estesa:

```yaml
                  - lvgl.page.show:
                      id: page_xxx
                      animation: FADE_IN
                      time: 150ms
```

Vale per tutti e 16 i pulsanti navbar e per i quattro rami del `select`
`sel_pagina`: se se ne dimentica uno, quel passaggio scatta senza dissolvenza e
si nota.

Controllo: `grep -c "animation: FADE_IN" test-display3.yaml` deve dare **20**.

- [ ] **Step 2: Validare, compilare, flashare**

```bash
esphome config  test-display3.yaml | tail -2
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
```

- [ ] **Step 3: Verifica**

**Da chiedere all'utente:** il pannello gira a ~33 Hz, quindi 150 ms sono 5 frame.
Se la dissolvenza risulta scattosa invece che fluida, si toglie mettendo
`animation: NONE` e non si perde nulla di sostanziale. Questa e' una decisione da
prendere guardando, non ragionando.

- [ ] **Step 4: Commit**

```bash
git add test-display3.yaml
git commit -m "Restyling 8/9: dissolvenza nel cambio pagina"
```

---

## Task 9: Pulizia dei font non piu' usati e documentazione

**Files:**
- Modify: `test-display3.yaml` — sezione `font:`, rimuovere `font_temp` e `font_euro`
- Modify: `CLAUDE.md` — sezione icone e font
- Modify: `ROADMAP.md` — spuntare il restyling

**Interfaces:**
- Consumes: tutti i task precedenti

- [ ] **Step 1: Verificare che i font vecchi siano orfani**

Run: `grep -n "font_temp\|font_euro" test-display3.yaml`
Expected: solo le due dichiarazioni nella sezione `font:`. Se compare un
`text_font: font_temp` in una pagina, quella pagina non e' stata convertita: **non
proseguire**, tornare al task corrispondente.

- [ ] **Step 2: Rimuoverli e validare**

Cancellare i due blocchi, poi:

Run: `esphome config test-display3.yaml | tail -2`
Expected: `INFO Configuration is valid!`

- [ ] **Step 3: Aggiornare la documentazione**

In `CLAUDE.md`, nella sezione "Icone e font speciali", sostituire la riga sui
simboli `°` e `€` con: la tipografia e' Poppins via `gfonts://`, le taglie
testuali usano `glyphsets: [GF_Latin_Core]` (che include gli accenti italiani,
assenti dal default `GF_Latin_Kernel`) e le taglie numeriche una lista `glyphs`
esplicita per non pagare 319 glifi a 40 px. Aggiungere che gli stili condivisi
stanno in `lvgl: style_definitions:` e che l'header e' nel `top_layer`, sicuro
perche' non interattivo.

In `ROADMAP.md`, spostare "Restyling UI" fra le voci fatte.

- [ ] **Step 4: Compilare, flashare, verificare l'ultima volta**

```bash
esphome compile test-display3.yaml 2>&1 | grep -E "Flash:|error"
esphome upload  test-display3.yaml --device test-plancia.local
esphome logs    test-display3.yaml --device test-plancia.local
```
Expected: nessun `[E]`/`[W]`, entity attese nel dump di boot, nessun rollback.

- [ ] **Step 5: Commit**

```bash
git add test-display3.yaml CLAUDE.md ROADMAP.md
git commit -m "Restyling 9/9: rimossi i font orfani, documentazione aggiornata"
```

---

## Criteri di completamento

- Le quattro pagine condividono palette, tipografia, raggi e spaziature.
- Ogni valore mostrato prima e' ancora mostrato, con la stessa entita' HA.
- Ogni comando funziona: script, switch cucina, robot, tapparelle, luce esterna.
- `grep -n "lvgl.arc.update\|font_temp\|font_euro" test-display3.yaml` non trova nulla fuori dalle dichiarazioni rimosse.
- Nessun `[E]`/`[W]` nei log e nessun rollback OTA dopo l'ultimo flash.
