# Hardware

Mechanical design and build files for the positioning base.

## Contents
- **Source CAD** — the parametric design lives on [Onshape](https://cad.onshape.com/documents/38afda30efdd53e54d52e6ab/w/fc69859d114c8133178aba7d/e/cd36117da42823404910bc80); copy it to your own workspace to modify, extend, or re-export parts.
- **[`stl/`](stl/)** — print-ready meshes exported from the CAD. One file per printable part; name parts clearly (e.g. `sector_segment_1.stl`, `arm.stl`, `pinion.stl`, `pinion_cover.stl`, `base_plate.stl`).
- **[`bom/`](bom/)** — analytical bill of materials (parts, quantities, unit costs, sources).
- **[`assembly/`](assembly/)** — assembly guide, print settings, and wiring.

## Design summary

- Single-stage spur reduction: 12-tooth pinion on the stepper → 240-tooth printed sector gear = **20:1**, module 2.9 mm, sector pitch diameter ≈ 696 mm.
- Sector spans **140°** (mechanical ±70°); limit switches at **±65°** bound usable travel.
- Structure is **segmented** and joined with printed dovetails so no single part exceeds a **25 × 25 × 25 cm** build volume; full stage ≈ 70 cm assembles without a large-format printer.
- Printed in **PLA**, 0.2 mm layer height, 3 perimeters, 40% infill. Printed sector teeth mesh directly with the pinion.
- Arm pivots on a **22 × 7 × 7 mm** bearing at the central axis; same-type bearings act as wheels/guides between the moving sector and the base plate.
- A centerline **5.15 mm slot** on the arm is the generic payload interface — accepts 5 mm bolts/pins, slides to accommodate different payloads while keeping the phase center on the rotation axis.



Copyright 2026 Alogdianakis Georgios

This source describes Open Hardware and is licensed under the CERN-OHL-P v2.
You may redistribute and modify this documentation and make products using it
under the terms of the CERN-OHL-P v2 (https://ohwr.org/cern_ohl_p_v2.txt).
This documentation is distributed WITHOUT ANY EXPRESS OR IMPLIED WARRANTY,
INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS FOR A PARTICULAR
PURPOSE. Please see the CERN-OHL-P v2 for applicable conditions.

