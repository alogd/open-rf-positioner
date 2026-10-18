# STL (print-ready meshes)

Print-ready meshes, one file per printable part.

## Recommended print settings

PLA, 0.2 mm layer height, 3 perimeters, 40% infill. No single part exceeds a
25 × 25 × 25 cm build volume.

The limit switch actuators need a M5x30mm bolt and two M5 nuts, one goes on to the limit switch actuator and the other at the end of the bolt with the cap. Together they help with fine-tuning the range of the platform. Instant (cyanoacrylate) glue can be used for the placement of the actuators on to the edges of the arc. Calibration is needed when building the platform.

## Optional support parts

If the moving section needs extra rigidity, the following parts can be added:

- **Support bracket with bearing** — mounts onto the motor base and holds the
  arc at height. Requires 2 bearings and 2 bearing pins.
- **Support rib (side)** — glued to the side of the moving section to add rigidity.
- **Support rib (middle)** — glued beneath the centerline slot of the moving part.

Instant (cyanoacrylate) glue provides sufficient adhesion between the parts.

## Printed bearings

For the wheels (`Moving_section_3` and `Moving_section_4`) and the
`Support_bracket_with_bearing`, `Bearing.stl` can be printed in place of an
actual bearing. In that case, lubricate between the mating parts — preferably
with a PTFE lubricant.