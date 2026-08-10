"""Ender-3 derivative of the fixed ALT60 case master.

The original geometry is imported read-only from case/original.  This file
adds only print-bed splitting and left/right joining features.
"""

from pathlib import Path
import importlib.util
import cadquery as cq


HERE = Path(__file__).resolve().parent
MASTER_PATH = HERE.parents[1] / "original" / "cadquery" / "alt60_case.py"
spec = importlib.util.spec_from_file_location("alt60_case_master", MASTER_PATH)
master = importlib.util.module_from_spec(spec)
spec.loader.exec_module(master)

MAX_PRINT_DIMENSION = 200.0
SEAM_GAP = 0.30
REINFORCE_HALF_W = 20.0
REINFORCE_FLAT_HALF_W = 16.0
REINFORCED_FLOOR_T = 6.0
REINFORCED_WALL_T = 6.0
PIN_D = 3.2
SOCKET_D = 3.5
PIN_LENGTH = 8.0
SOCKET_DEPTH = 9.0
PIN_CHAMFER = 0.5
SEAM_SCREWS = (
    (-8.0, -45.0),
    (8.0, -45.0),
    (-8.0, 45.0),
    (8.0, 45.0),
)

# Remove the original bisected centre bosses and add four complete M2.5 seam
# bosses: left/right at both the front and rear of the split.
master.BOSS_POINTS = (
    tuple(p for p in master.BOSS_POINTS if abs(p[0]) > 1e-6) + SEAM_SCREWS
)


def half_tools():
    span = 600.0
    left = (
        cq.Workplane("XY")
        .box(master.CASE_W / 2 - SEAM_GAP / 2, span, span)
        .translate((-master.CASE_W / 4 - SEAM_GAP / 4, 0, span / 2))
    )
    right = (
        cq.Workplane("XY")
        .box(master.CASE_W / 2 - SEAM_GAP / 2, span, span)
        .translate((master.CASE_W / 4 + SEAM_GAP / 4, 0, span / 2))
    )
    return left, right


def floor_reinforcement():
    """Smooth internal thickening: 2.5 mm floor grows to 6 mm at the seam."""
    inner_d = master.CASE_D - 2 * master.WALL
    return (
        cq.Workplane("XZ")
        .moveTo(-REINFORCE_HALF_W, master.FLOOR_T)
        .lineTo(-REINFORCE_FLAT_HALF_W, REINFORCED_FLOOR_T)
        .lineTo(REINFORCE_FLAT_HALF_W, REINFORCED_FLOOR_T)
        .lineTo(REINFORCE_HALF_W, master.FLOOR_T)
        .close()
        .extrude(inner_d / 2, both=True)
    )


def wall_reinforcements():
    """Internal 3-to-6 mm wall thickening local to the centre split."""
    inner_edge = master.CASE_D / 2 - master.WALL
    add_depth = REINFORCED_WALL_T - master.WALL
    front_y0, front_y1 = -inner_edge, -inner_edge + add_depth
    rear_y0, rear_y1 = inner_edge - add_depth, inner_edge
    front = (
        cq.Workplane("XY")
        .moveTo(-REINFORCE_HALF_W, front_y0)
        .lineTo(-REINFORCE_FLAT_HALF_W, front_y1)
        .lineTo(REINFORCE_FLAT_HALF_W, front_y1)
        .lineTo(REINFORCE_HALF_W, front_y0)
        .close()
        .extrude(master.roof_z(-master.CASE_D / 2) - 0.8)
    )
    rear = (
        cq.Workplane("XY")
        .moveTo(-REINFORCE_FLAT_HALF_W, rear_y0)
        .lineTo(-REINFORCE_HALF_W, rear_y1)
        .lineTo(REINFORCE_HALF_W, rear_y1)
        .lineTo(REINFORCE_FLAT_HALF_W, rear_y0)
        .close()
        .extrude(master.roof_z(master.CASE_D / 2) - 0.8)
    )
    return front.union(rear)


def pin_and_socket(y, z, pin_from_left):
    """Horizontal pin and hole cut directly into reinforced floor/wall plates."""
    if pin_from_left:
        pin = (
            cq.Workplane("YZ").center(y, z).circle(PIN_D / 2)
            .extrude(PIN_LENGTH).translate((-1.0, 0, 0))
            .faces(">X").edges().chamfer(PIN_CHAMFER)
        )
        socket = (
            cq.Workplane("YZ").center(y, z).circle(SOCKET_D / 2)
            .extrude(SOCKET_DEPTH).translate((-0.5, 0, 0))
        )
    else:
        pin = (
            cq.Workplane("YZ").center(y, z).circle(PIN_D / 2)
            .extrude(PIN_LENGTH).translate((-PIN_LENGTH + 1.0, 0, 0))
            .faces("<X").edges().chamfer(PIN_CHAMFER)
        )
        socket = (
            cq.Workplane("YZ").center(y, z).circle(SOCKET_D / 2)
            .extrude(SOCKET_DEPTH).translate((-SOCKET_DEPTH + 0.5, 0, 0))
        )
    return pin, socket


def split_top():
    left_tool, right_tool = half_tools()
    plate = master.top_plate()
    return plate.intersect(left_tool), plate.intersect(right_tool)


def split_bottom():
    left_tool, right_tool = half_tools()
    shell = master.bottom_case().union(floor_reinforcement()).union(wall_reinforcements())
    left, right = shell.intersect(left_tool), shell.intersect(right_tool)
    wall_y = master.CASE_D / 2 - REINFORCED_WALL_T / 2
    front_roof = master.roof_z(-wall_y)
    rear_roof = master.roof_z(wall_y)
    joints = (
        (-wall_y, master.FLOOR_T + 2.5),
        (-wall_y, front_roof - 3.0),
        (-37.5, REINFORCED_FLOOR_T / 2),
        (-22.5, REINFORCED_FLOOR_T / 2),
        (-7.5, REINFORCED_FLOOR_T / 2),
        (7.5, REINFORCED_FLOOR_T / 2),
        (22.5, REINFORCED_FLOOR_T / 2),
        (37.5, REINFORCED_FLOOR_T / 2),
        (wall_y, master.FLOOR_T + 4.5),
        (wall_y, rear_roof - 3.0),
    )
    for index, (y, z) in enumerate(joints):
        pin_from_left = index % 2 == 0
        pin, socket = pin_and_socket(y, z, pin_from_left)
        if pin_from_left:
            left = left.union(pin)
            right = right.cut(socket)
        else:
            right = right.union(pin)
            left = left.cut(socket)
    return left, right


def validate_part(name, part):
    shape = part.val()
    bb = shape.BoundingBox()
    assert shape.isValid(), f"{name} is invalid"
    assert part.solids().size() == 1, f"{name} is not one solid"
    assert max(bb.xlen, bb.ylen, bb.zlen) < MAX_PRINT_DIMENSION, (
        f"{name} exceeds {MAX_PRINT_DIMENSION} mm: "
        f"{bb.xlen:.2f} x {bb.ylen:.2f} x {bb.zlen:.2f}"
    )
    return bb


def export_all():
    root = HERE.parent
    top_dir, bottom_dir = root / "top", root / "bottom"
    for folder in (top_dir, bottom_dir):
        folder.mkdir(parents=True, exist_ok=True)

    tl, tr = split_top()
    bl, br = split_bottom()
    parts = {
        "top_left": (tl, top_dir),
        "top_right": (tr, top_dir),
        "bottom_left": (bl, bottom_dir),
        "bottom_right": (br, bottom_dir),
    }
    for name, (part, folder) in parts.items():
        bb = validate_part(name, part)
        cq.exporters.export(part, str(folder / f"{name}.step"))
        cq.exporters.export(part, str(folder / f"{name}.stl"), tolerance=0.08, angularTolerance=0.15)
        print(f"{name}: {bb.xlen:.2f} x {bb.ylen:.2f} x {bb.zlen:.2f} mm")


if "show_object" in globals():
    bl, br = split_bottom()
    exploded_gap = 24.0
    show_object(
        bl.translate((-exploded_gap / 2, 0, 0)),
        name="BOTTOM_LEFT",
        options={"color": "lightgray"},
    )
    show_object(
        br.translate((exploded_gap / 2, 0, 0)),
        name="BOTTOM_RIGHT",
        options={"color": "steelblue"},
    )


if __name__ == "__main__":
    export_all()
