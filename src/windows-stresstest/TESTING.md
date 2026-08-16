# Font upload test runbook

Purpose: decide whether the PFP font failures reported in the field come from
`Font::ResizeCellHeight` (the 23x32 rebuild) or from the hardware/transport.

Menu option 7 runs the plugin's real pipeline (`Font::GlyphData` ->
`Font::ResizeCellHeight` -> upload -> screen position) and lets you pick the
geometry. The packets stay addressed to device 0x32, so PFP geometry can be
pushed to an MCDU. The only difference from a real PFP upload is which device
the packets are addressed to, so a font that fails here fails in the rebuild,
not on PFP hardware.

After each upload the harness draws a glyph sheet: all printable ASCII, in
order. Every cell should show a character. A blank cell is a glyph the device
did not store.

## Build and deploy

```
cd src/windows-stresstest
./build.sh
```

`build.sh` builds and then copies the exe plus a fresh `fonts/` into
`/Volumes/New folder/stresstest`, skipping the copy when the volume is not
mounted. **The exe needs `fonts/` beside it**: the font loader reads
`<exe dir>/fonts`, so copying the exe on its own leaves the font list empty and
nothing can be uploaded.

## One upload per USB session

The first diagnostic run (2026-08-14) established this: **only the first font
upload after a USB connect takes effect.** In that session the 23x29 upload
landed and changed the display; the two 23x32 uploads that followed changed
nothing, and neither did a final upload back to 23x29. So a geometry that
"fails" as upload #2 tells you nothing.

Consequences, both now built into the harness:

- `connect()` no longer uploads a font. The device keeps its factory font until
  you upload one, so a test upload is genuinely #1.
- Option 9 performs exactly **one** upload per run and logs
  `upload #N since connect`. Replug the device between cases.

This also means the plugin's repeated `setFont` calls were never merely
wasteful: on any device where the first upload is the one that counts, the
later ones were silently dropped by the hardware. The dedupe fix in
`ProductFMC::setFont` matters more than it looked.

## Fastest path: option 9

Option 9 asks which geometry to test, asks what is on screen before and after,
and appends everything to `stresstest-log.txt` next to the exe: device info,
font discovery, the resize outcome (`rebuilt` vs `no-op` vs `PARSE FAILED`,
derived from the packet count rather than the return value), packet count,
drain time, upload number, and the answers.

## Scenario

Each case is a separate run. Replug the device, start the exe, press 9, pick the
case. The log appends, so all runs end up in one file.

| Run | Case | What it establishes |
|---|---|---|
| A | Option 9 -> case 3 (no upload) | Baseline: the factory font renders every glyph |
| B | Replug, option 9 -> case 1 (MCDU 23x29) | The upload path works when the resize no-ops |
| C | Replug, option 9 -> case 2 (PFP 3N 23x32) | **The decisive one.** Whether the rebuilt font is accepted as upload #1 |
| D | Replug, option 9 -> case 2 again | Confirms C, since a single observation of a hardware commit is thin |

## What the outcomes mean

- B works, C fails -> the 23x32 rebuild output is rejected by the hardware,
  reproduced without PFP hardware. The defect is in `Font::ResizeCellHeight`.
- B and C both work -> the rebuild is fine and the field failure needs a
  PFP-specific explanation: the device address byte, the `0x18` origin block, or
  the PFP firmware. Note `ScreenLayoutForHardware` returns Y=4 for PFP where SAP
  uses 12, which is worth checking on real hardware either way.
- B fails too -> the problem is upstream of the resize entirely.
