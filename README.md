# open-rf-positioner

**A low-cost, open-source, 3D-printable automated positioning platform for high-resolution antenna characterization and Angle-of-Arrival (AoA) measurements.**

![Measurement platform in CAD software](/docs/images/platform_view1.png)

Reproducible over-the-air (OTA) angular measurements are a bottleneck for validating AoA, beamforming, and antenna characterization on low-cost Software-Defined Radio (SDR) platforms. Manual positioning introduces angular uncertainty and limits repeatability and dataset scalability. This project is a motorized, RF-hardware-agnostic rotational base — a moving platform plus automated sweep/calibration software that accepts user-chosen RF and antenna hardware — for repeatable antenna characterization and AoA measurements.

The platform is built entirely from a desktop 3D printer and low-cost, off-the-shelf parts, with no custom electronics or machined components. It rotates one antenna about a single vertical axis while preserving the transmitter–receiver geometry across the sweep, so the commanded angle is the only thing that changes.

> **Status:** Hardware design, firmware, and control/processing software. 3D filesa and the analytical BOM are being added — see the open items in each folder's README.

---

## Highlights

- **Low cost** — material cost on the order of **€55**, one to two orders of magnitude below commercial motorized positioners.
- **Fully 3D-printable** — segmented structure (printed dovetail joints) fits a standard **25 × 25 × 25 cm** build volume; no large-format machine needed. Assembled footprint ≈ **70 × 52 × 7.5 cm**.
- **RF-agnostic** — the base makes no assumption about the radio. A parametric payload slot carries any comparable antenna/SDR fixture; keep element spacing at λ/2 with interchangeable spacers.
- **Repeatable** — measured run-to-run repeatability **≈ 0.06°**, comfortably within a 0.1° budget, characterized in an anechoic chamber.
- **Fine resolution** — **≈ 0.011°** per microstep at the arm (NEMA 17 at 1/8 microstepping through a 20:1 reduction).
- **Open-loop but self-referencing** — limit switches at ±65° provide absolute references; an `AUTOCENTER` routine finds mechanical boresight without manual alignment.
- **Simple host interface** — a small, line-oriented serial command set over USB makes it trivial to script sweeps and synchronize acquisition to motion.

## Specifications

| Parameter | Value |
|---|---|
| Travel (automated) | ±65° (mechanical sector ±70°) |
| Angular resolution at arm | ≈ 0.01125° / microstep |
| Run-to-run repeatability | ≈ 0.06° |
| Drive | single-stage spur, 12-tooth pinion → 240-tooth sector (20:1), module 2.9 mm |
| Actuator | NEMA 17, 200 steps/rev, 1.8°/step, 2.8 kg·cm, A4988 @ 1/8 microstepping |
| Controller | ESP32-H2, USB serial @ 115200 baud |
| Payload capacity | ≈ 2 kg, up to ≈ 69 cm |
| Assembled size | ≈ 70 × 52 × 7.5 cm (W × L × H) |
| Material cost | ≈ €55 |

Values are from the platform's experimental characterization. See [`docs/`](docs/) for the measurement writeup.

## Repository layout

```
open-rf-positioner/
├── hardware/            Mechanical design and build
│   ├── cad/             Source CAD (editable parametric models)
│   ├── stl/             Print-ready meshes
│   ├── bom/             Analytical bill of materials
│   └── assembly/        Assembly guide, print settings, wiring
├── firmware/            ESP32-H2 motor-control firmware
│   └── esp32-h2/        Source, serial command interface
├── software/            Host-side control and data pipeline
│   ├── acquisition/     Sweep driver + SDR acquisition
│   ├── processing/      OFDM-enhanced monopulse / AoA processing
│   └── analysis/        Metrics, plots, repeatability analysis
├── docs/                Documentation, figures, characterization
│   ├── images/          Photos, renders, block/wiring diagrams
│   └── characterization/ Repeatability & validation results
└── data/                Datasets
    └── sample/          Small example capture for a quick start
```

## How it works

A triangular arm with an extended arc gear carries the antenna and pivots at its center. A pinion on the stepper meshes with a large printed toothed sector; turning the pinion turns the sector, changing the angle the antenna sees. Because the antenna sits on the pivot axis, it turns in place and the transmitter–receiver distance and geometry are preserved across the sweep.

The ESP32-H2 runs the motor open-loop but keeps a drift-free signed **step count** as the reference, deriving the angle from it. Large moves use a trapezoidal velocity profile so no steps are lost; limit switches at ±65° re-reference the position on contact. The host drives the whole sweep over a request–reply serial handshake: command a move, wait for completion, trigger the receiver, record the reported angle as ground truth, advance.

## Serial command interface

The firmware exposes a small, line-oriented command set over USB serial (115200 baud):

| Command | Reply | Description |
|---|---|---|
| `GOTO <deg>` | `OK DONE <ang>` | Move to an absolute angle (rejected outside ±65°) |
| `MOVE <deg>` | `OK DONE <ang>` | Move relative to the current angle |
| `POS?` | `POS <ang> steps=<n>` | Report current angle and step count |
| `HOME` | `OK HOMED` | Set the current position as 0° |
| `AUTOCENTER` | `OK AUTOCENTERED` | Drive to both limits, stop at the midpoint (boresight) |
| `STOP` | `OK STOPPED <ang>` | Abort the current move |
| `FLASH LED` | `OK FLASHED` | Blink the onboard LED for identification |
| `HELP` | command list | List available commands |

A move first returns `OK MOVING`; invalid input returns `ERROR OUT_OF_RANGE`, `ERROR BUSY`, or `ERROR UNKNOWN_CMD`.

## Getting started

> Detailed build and run instructions live in [`hardware/assembly/`](hardware/assembly/), [`firmware/`](firmware/), and [`software/`](software/). Quick outline:

1. **Print the parts** — meshes in [`hardware/stl/`](hardware/stl/); recommended settings in [`hardware/assembly/`](hardware/assembly/) (PLA, 0.2 mm layer, 3 perimeters, 40% infill).
2. **Source the components** — see the [`BOM`](hardware/bom/).
3. **Assemble** — follow the [assembly guide](hardware/assembly/).
4. **Flash the firmware** — [`firmware/esp32-h2/`](firmware/esp32-h2/).
5. **Run a sweep** — [`software/acquisition/`](software/acquisition/) commands the base and acquires from your SDR; [`software/processing/`](software/processing/) and [`software/analysis/`](software/analysis/) turn captures into AoA estimates and plots.

## Reference application: OFDM-enhanced monopulse AoA

The base was validated using an **OFDM-enhanced phase-comparison monopulse** AoA estimator as a representative payload: a modified dual-channel ADALM-Pluto SDR (AD9361-compatible 2×2 mode) at 2.3 GHz feeding two directional log-periodic antennas at λ/2 = 65 mm. This is only the reference instantiation — the platform is not specific to it and extends naturally to radiation-pattern measurement and beamforming/MIMO array characterization as a function of angle. The processing pipeline is included under [`software/`](software/).

## Citing this work

If you use this platform in academic work, please cite the platform paper (and the prior algorithm paper where relevant). See [`CITATION.cff`](CITATION.cff). BibTeX entries will be added once the paper is published.

## License

Hardware is released under **CERN-OHL-S v2** and software/firmware under the **MIT License** — see [`LICENSE`](LICENSE) and [`hardware/LICENSE`](hardware/LICENSE). *(Confirm these choices before first release — see note in [`LICENSE`](LICENSE).)*

## Acknowledgment

This work was supported by the European Union's Horizon Europe research and innovation program under grant agreement **HE-MSCA-SE-6G-TERAFIT-101131501**.
