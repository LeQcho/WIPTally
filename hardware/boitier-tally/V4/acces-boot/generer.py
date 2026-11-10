"""Add a side tool passage for the LOLIN S2 Mini BOOT switch to the V4 base."""
import json
from pathlib import Path

import cadquery as cq
import trimesh

ROOT = Path(__file__).resolve().parent
PARAMETERS = json.loads((ROOT.parent / "sources/parametres.json").read_text())
NAME = "Socle-V4-master-acces-BOOT"

# Component side up, USB at negative X. From above with USB toward the
# observer, Wemos' photo shows the side-actuated switch marked 0 on the right.
# Offset estimated from the official photo, since it is not on the dimension drawing.
BUTTON_FROM_USB_PCB_EDGE = 3.3
BUTTON_ABOVE_PCB = 0.9
tool_x = PARAMETERS["pcb_xmin"] + BUTTON_FROM_USB_PCB_EDGE
tool_z = PARAMETERS["pcb_bottom"] + PARAMETERS["pcb_thickness"] + BUTTON_ABOVE_PCB
half_width = 1.7
outside_y = -PARAMETERS["depth"] / 2 - 0.5
inside_y = -13.2

# XZ workplane faces negative Y. A pointed roof keeps the small opening
# printable in the same orientation as the rest of the base.
opening = (
    cq.Workplane("XZ", origin=(tool_x, inside_y, tool_z))
    .polyline([
        (-half_width, -1.7),
        (half_width, -1.7),
        (half_width, 1.6),
        (0, 1.6 + half_width),
        (-half_width, 1.6),
    ])
    .close()
    .extrude(inside_y - outside_y)
)
original = cq.importers.importStep(str(ROOT.parent / "STEP/01_socle_noir_4_vis.step"))
base = original.cut(opening).clean()
assert len(base.solids().vals()) == 1 and base.val().isValid()
removed = original.cut(base)
assert removed.val().Volume() > 0

# The channel stays below the screw pilots and outside the PCB footprint.
assert removed.val().BoundingBox().zmax < 20.0
assert removed.val().BoundingBox().ymax < -PARAMETERS["pcb_width"] / 2
probe = cq.Solid.makeCylinder(
    0.75, 13.1, cq.Vector(tool_x, outside_y, tool_z), cq.Vector(0, 1, 0)
)
assert base.val().intersect(probe).Volume() < 0.001

step_path = ROOT / f"{NAME}.step"
stl_path = ROOT / f"{NAME}.stl"
cq.exporters.export(base, str(step_path))
cq.exporters.export(base, str(stl_path), tolerance=0.025, angularTolerance=0.08)
readback = cq.importers.importStep(str(step_path))
assert len(readback.solids().vals()) == 1 and readback.val().isValid()
mesh = trimesh.load_mesh(stl_path)
assert mesh.is_watertight and mesh.is_volume
neighbors = [[] for _ in mesh.faces]
for first, second in mesh.face_adjacency:
    neighbors[first].append(second)
    neighbors[second].append(first)
seen = {0}
pending = [0]
while pending:
    for face in neighbors[pending.pop()]:
        if face not in seen:
            seen.add(face)
            pending.append(face)
assert len(seen) == len(mesh.faces)
assert abs(mesh.bounds[0, 2]) < 0.01
assert all(abs(a - b) < 0.01 for a, b in zip(
    mesh.extents, (PARAMETERS["width"], PARAMETERS["depth"], PARAMETERS["base_height"])
))

report = {
    "physical_fit_validated": False,
    "board_reference": "LOLIN S2 Mini V1.0.0, component face up",
    "button_position": "estimated from the official Wemos board photograph",
    "axis_from_usb_side_mm": round(tool_x + PARAMETERS["width"] / 2, 2),
    "axis_above_bottom_mm": round(tool_z, 2),
    "opening_width_mm": 2 * half_width,
    "probe_diameter_checked_mm": 1.5,
    "removed_volume_mm3": round(removed.val().Volume(), 3),
    "step_valid": True,
    "stl_watertight": True,
    "four_lid_screw_pilots_preserved": True,
    "pcb_mounts_preserved": True,
}
(ROOT / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
