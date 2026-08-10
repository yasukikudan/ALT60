"""Parametric ALT60 hand-wired keyboard case.

Units are millimetres.  Run with CadQuery 2.x:
    python alt60_case.py

The script exports full-size STEP/STL files and Ender-3-sized left/right STL
parts.  Coordinate system: X left/right, Y front/rear, Z up.
"""

from pathlib import Path
import math
import cadquery as cq


# --- ALT60 layout (source: ALT60/public/docs/case_design.html) ----------------
U = 19.05
MX_CUTOUT = 14.0
# Standard 6.25U Cherry-style spacebar stabilizer stem centres are about
# 100 mm apart.  We use MX switch holes at those support locations.
SPACE_SUPPORT_OFFSET = 50.0
ROWS = (
    (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2),
    (1.5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5),
    (1.75, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.25),
    (2.25, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2.75),
    (1.25, 1.25, 1.25, 6.25, 1.25, 1.25, 1.25, 1.25),
)

# --- case --------------------------------------------------------------------
GRID_W, GRID_D = 15 * U, 5 * U
BEZEL = 7.0
WALL = 3.0
CASE_W = GRID_W + 2 * (BEZEL + WALL)       # 305.75
CASE_D = GRID_D + 2 * (BEZEL + WALL)       # 115.25
PLATE_T = 1.6
FLOOR_T = 2.5
FRONT_H = 14.0
REAR_H = 28.0
OUTER_R = 3.0

# All case and protoboard fasteners use M2.5.
CASE_SCREW_CLEARANCE = 2.9
CASE_INSERT_PILOT = 3.5       # tune to the selected M2.5 insert data
CASE_BOSS_OD = 10.0
PLATE_SCREW_PAD_OD = 10.0
PLATE_SCREW_PAD_EXTRA = 2.4   # 1.6 mm plate + 2.4 mm local pad = about 4 mm
BOSS_POINTS = (
    (-CASE_W / 2 + 7.0, -CASE_D / 2 + 7.0),
    ( CASE_W / 2 - 7.0, -CASE_D / 2 + 7.0),
    (-CASE_W / 2 + 7.0,  CASE_D / 2 - 7.0),
    ( CASE_W / 2 - 7.0,  CASE_D / 2 - 7.0),
    # Quarter/three-quarter positions along both long edges.
    (-CASE_W * 0.25, -CASE_D / 2 + 7.0),
    ( CASE_W * 0.25, -CASE_D / 2 + 7.0),
    (-70.0,  CASE_D / 2 - 7.0),  # shifted inward to clear the left-edge board
    ( CASE_W * 0.25,  CASE_D / 2 - 7.0),
    (-CASE_W / 2 + 7.0, 0.0),
    ( CASE_W / 2 - 7.0, 0.0),
    # Exact split-line fasteners.  Each boss is printed as two semicircles
    # and becomes a complete vertical M2.5 boss after joining left/right.
    (0.0, -CASE_D / 2 + 7.0),
    (0.0,  CASE_D / 2 - 7.0),
)

# --- controller assembly ------------------------------------------------------
PROTO_W, PROTO_D, PROTO_T = 60.0, 70.0, 1.6
PROTO_CENTER_X = -108.0    # entire 60 mm board sits near the left case edge
PROTO_REAR_GAP = 3.0
PIN_RECESS_W, PIN_RECESS_D, PIN_RECESS_DEPTH = 54.0, 64.0, 4.0
ESP_PIN_RECESS_W, ESP_PIN_RECESS_D = 30.0, 48.0
ESP_PIN_CLEARANCE = 3.0
# Photo-based initial estimate for the four large corner holes: roughly
# 5 mm in from the 60 x 70 mm board edges, giving a 50 x 60 mm pattern.
PROTO_HOLE_X, PROTO_HOLE_Y = 25.0, 30.0
PROTO_SCREW_CLEARANCE = 2.9
PROTO_PILOT = 2.0
PROTO_BOSS_OD = 7.0

ESP_W, ESP_D = 28.0, 52.0
ESP_OFFSET_X = -12.0       # move chip/pins away from the Ender-3 centre bridge
# ESP32 male pin headers are soldered directly to the universal board.
# This includes the header plastic, ESP PCB, and tallest top-side component.
ESP_CHIP_SIDE_HEIGHT = 8.0
ESP_HEIGHT_ABOVE_PROTO = ESP_CHIP_SIDE_HEIGHT
# Extra roof space also lowers the USB connector enough to retain roughly
# 2 mm of material above the large rear-wall opening.
ESP_TOP_CLEARANCE = 2.2
# Generous opening for common Micro-USB / USB-C cable plug mouldings.
USB_W, USB_H, USB_R = 18.0, 10.0, 2.0
# Estimated connector centre above the protoboard upper face; tune after test.
USB_CENTER_ABOVE_PROTO = 3.5

# Split export clearance keeps each half below an Ender-3's 220 mm bed width.
SPLIT_GAP = 0.30

# CQ-editor preview: "separate" shows only the two printable master parts.
# Change to "both" when the assembled controller/plate relationship is needed.
CQ_VIEW_MODE = "separate"


SLOPE = (REAR_H - FRONT_H) / CASE_D
ANGLE_DEG = math.degrees(math.atan(SLOPE))
PROTO_CENTER_Y = CASE_D / 2 - WALL - PROTO_REAR_GAP - PROTO_D / 2
ESP_CENTER_X = PROTO_CENTER_X + ESP_OFFSET_X


def proto_underside_z(y: float) -> float:
    """Tilted board underside, parallel to the roof/typing plate."""
    return roof_z(y) - PLATE_T - PROTO_T - ESP_HEIGHT_ABOVE_PROTO - ESP_TOP_CLEARANCE


def roof_z(y: float) -> float:
    return FRONT_H + (y + CASE_D / 2) * SLOPE


def switch_centres():
    """Return MX centres; the 6.25U spacebar has centre + two support holes."""
    pts = []
    for row_i, widths in enumerate(ROWS):
        cursor = -GRID_W / 2
        y = GRID_D / 2 - U / 2 - row_i * U
        for width in widths:
            cx = cursor + width * U / 2
            if row_i == 4 and abs(width - 6.25) < 1e-6:
                pts.extend(((cx - SPACE_SUPPORT_OFFSET, y), (cx, y), (cx + SPACE_SUPPORT_OFFSET, y)))
            else:
                pts.append((cx, y))
            cursor += width * U
        assert abs(cursor - GRID_W / 2) < 1e-6
    return pts


def wedge(width, depth, front_z, rear_z, x0=0.0, y0=0.0):
    """Solid under a sloped roof and above Z=0."""
    section = (
        cq.Workplane("YZ")
        .moveTo(y0 - depth / 2, 0)
        .lineTo(y0 + depth / 2, 0)
        .lineTo(y0 + depth / 2, rear_z)
        .lineTo(y0 - depth / 2, front_z)
        .close()
        .extrude(width / 2, both=True)
    )
    return section.translate((x0, 0, 0))


def _flat_plate_without_case_screw_holes():
    plate = cq.Workplane("XY").box(CASE_W, CASE_D, PLATE_T, centered=(True, True, False))
    plate = plate.edges("|Z").fillet(OUTER_R)
    holes = switch_centres()
    plate = plate.faces(">Z").workplane().pushPoints(holes).rect(MX_CUTOUT, MX_CUTOUT).cutThruAll()
    return plate


def _top_plate_assembled_geometry():
    """Sloped plate with globally vertical screw bores and reinforced pads."""
    z_mid = (FRONT_H + REAR_H) / 2
    plate = (
        _flat_plate_without_case_screw_holes()
        .rotate((0, 0, 0), (1, 0, 0), ANGLE_DEG)
        .translate((0, 0, z_mid))
    )
    for x, y in BOSS_POINTS:
        plate_bottom_z = roof_z(y)
        pad = (
            cq.Workplane("XY")
            .center(x, y)
            .circle(PLATE_SCREW_PAD_OD / 2)
            .extrude(PLATE_T + PLATE_SCREW_PAD_EXTRA)
            .translate((0, 0, plate_bottom_z - PLATE_SCREW_PAD_EXTRA))
        )
        vertical_bore = (
            cq.Workplane("XY")
            .center(x, y)
            .circle(CASE_SCREW_CLEARANCE / 2)
            .extrude(PLATE_T + PLATE_SCREW_PAD_EXTRA + 4.0)
            .translate((0, 0, plate_bottom_z - PLATE_SCREW_PAD_EXTRA - 2.0))
        )
        plate = plate.union(pad).cut(vertical_bore)
    return plate


def top_plate():
    """Return the reinforced plate in a flat, bed-ready export orientation."""
    z_mid = (FRONT_H + REAR_H) / 2
    flat = (
        _top_plate_assembled_geometry()
        .translate((0, 0, -z_mid))
        .rotate((0, 0, 0), (1, 0, 0), -ANGLE_DEG)
    )
    return flat.translate((0, 0, -flat.val().BoundingBox().zmin))


def bottom_case():
    outer = wedge(CASE_W, CASE_D, FRONT_H, REAR_H).edges("|Z").fillet(OUTER_R)

    # Open cavity: flat 3 mm floor, 3 mm perimeter wall, sloped open top.
    inner_w, inner_d = CASE_W - 2 * WALL, CASE_D - 2 * WALL
    cavity = wedge(
        inner_w,
        inner_d,
        roof_z(-CASE_D / 2 + WALL) + 2.0,
        roof_z(CASE_D / 2 - WALL) + 2.0,
    ).translate((0, 0, FLOOR_T))
    shell = outer.cut(cavity)

    # Case bosses rise to just below the sloped plate. Holes are blind pilots.
    for x, y in BOSS_POINTS:
        h = roof_z(y) - FLOOR_T - 0.8
        boss = cq.Workplane("XY").center(x, y).circle(CASE_BOSS_OD / 2).extrude(h).translate((0, 0, FLOOR_T))
        pilot = cq.Workplane("XY").center(x, y).circle(CASE_INSERT_PILOT / 2).extrude(7.0).translate((0, 0, roof_z(y) - 7.0))
        shell = shell.union(boss).cut(pilot)

    # Four M2.5 bosses ending on the tilted 60 x 70 mm protoboard plane.
    for dx in (-PROTO_HOLE_X, PROTO_HOLE_X):
        for dy in (-PROTO_HOLE_Y, PROTO_HOLE_Y):
            x, y = PROTO_CENTER_X + dx, PROTO_CENTER_Y + dy
            board_z = proto_underside_z(y)
            boss_h = board_z - FLOOR_T
            boss = cq.Workplane("XY").center(x, y).circle(PROTO_BOSS_OD / 2).extrude(boss_h).translate((0, 0, FLOOR_T))
            pilot_depth = min(5.0, boss_h - 0.8)
            pilot = cq.Workplane("XY").center(x, y).circle(PROTO_PILOT / 2).extrude(pilot_depth).translate((0, 0, board_z - pilot_depth))
            shell = shell.union(boss).cut(pilot)

    # A shallow pin relief immediately below the board.  Only the portion
    # intersecting the existing floor is removed, leaving the outer bottom intact.
    pin_recess = (
        cq.Workplane("XY")
        .box(PIN_RECESS_W, PIN_RECESS_D, PIN_RECESS_DEPTH, centered=(True, True, False))
        .translate((0, 0, -PIN_RECESS_DEPTH))
        .rotate((0, 0, 0), (1, 0, 0), ANGLE_DEG)
        .translate((PROTO_CENTER_X, PROTO_CENTER_Y, proto_underside_z(PROTO_CENTER_Y)))
    )
    shell = shell.cut(pin_recess)

    # Deeper local relief only where the ESP32 pin rows project below the
    # universal board.  This replaces the earlier tall dummy-spacer envelope.
    esp_pin_recess = (
        cq.Workplane("XY")
        .box(ESP_PIN_RECESS_W, ESP_PIN_RECESS_D, ESP_PIN_CLEARANCE, centered=(True, True, False))
        .translate((0, 0, -ESP_PIN_CLEARANCE))
        .rotate((0, 0, 0), (1, 0, 0), ANGLE_DEG)
        .translate((ESP_CENTER_X, PROTO_CENTER_Y, proto_underside_z(PROTO_CENTER_Y)))
    )
    shell = shell.cut(esp_pin_recess)

    # Rounded rear-wall USB opening, centred on the ESP32 connector.
    usb_z = proto_underside_z(CASE_D / 2 - WALL - PROTO_REAR_GAP) + PROTO_T + USB_CENTER_ABOVE_PROTO
    usb = (
        cq.Workplane("XZ")
        .center(ESP_CENTER_X, usb_z)
        .rect(USB_W, USB_H)
        .extrude(WALL + 4.0, both=True)
        .edges("|Y")
        .fillet(USB_R)
        .translate((0, CASE_D / 2, 0))
    )
    return shell.cut(usb)


def validate():
    assert abs(CASE_W - 305.75) < 0.01 and abs(CASE_D - 115.25) < 0.01
    assert WALL >= 3.0 and FLOOR_T >= 2.5
    assert PIN_RECESS_DEPTH == 4.0
    assert ESP_PIN_CLEARANCE == 3.0
    y_front = PROTO_CENTER_Y - PROTO_D / 2
    minimum_clearance = proto_underside_z(y_front) - FLOOR_T
    if minimum_clearance <= 0:
        raise ValueError("The tilted protoboard intersects the case floor")


def split_halves(solid):
    span = 500.0
    left_tool = cq.Workplane("XY").box(CASE_W / 2 - SPLIT_GAP / 2, span, span).translate((-CASE_W / 4 - SPLIT_GAP / 4, 0, span / 2))
    right_tool = cq.Workplane("XY").box(CASE_W / 2 - SPLIT_GAP / 2, span, span).translate((CASE_W / 4 + SPLIT_GAP / 4, 0, span / 2))
    return solid.intersect(left_tool), solid.intersect(right_tool)


def export_all(out_dir=None):
    validate()
    out = Path(out_dir or Path(__file__).resolve().parent)
    out.mkdir(parents=True, exist_ok=True)
    plate, shell = top_plate(), bottom_case()
    parts = {"top_plate": plate, "bottom_case": shell}
    for name, part in parts.items():
        cq.exporters.export(part, str(out / f"{name}.step"))
        cq.exporters.export(part, str(out / f"{name}.stl"), tolerance=0.08, angularTolerance=0.15)
        left, right = split_halves(part)
        cq.exporters.export(left, str(out / f"{name}_left_ender3.stl"), tolerance=0.08, angularTolerance=0.15)
        cq.exporters.export(right, str(out / f"{name}_right_ender3.stl"), tolerance=0.08, angularTolerance=0.15)
    print(f"Exported to {out}")
    print(f"Case {CASE_W:.2f} x {CASE_D:.2f} mm, front/rear {FRONT_H:.1f}/{REAR_H:.1f} mm, slope {ANGLE_DEG:.2f} deg")
    y_front = PROTO_CENTER_Y - PROTO_D / 2
    y_rear = PROTO_CENTER_Y + PROTO_D / 2
    print(f"MX holes: {len(switch_centres())}; protoboard clearance front/rear: "
          f"{proto_underside_z(y_front)-FLOOR_T:.1f}/{proto_underside_z(y_rear)-FLOOR_T:.1f} mm")


def assembled_top_plate():
    """Return the plate in its installed position with vertical screw holes."""
    return _top_plate_assembled_geometry()


def controller_visuals():
    """Non-exported board/ESP32 reference solids for CQ-editor inspection."""
    board = cq.Workplane("XY").box(PROTO_W, PROTO_D, PROTO_T, centered=(True, True, False))
    esp = (
        cq.Workplane("XY")
        .box(ESP_W, ESP_D, ESP_HEIGHT_ABOVE_PROTO, centered=(True, True, False))
        .translate((0, 0, PROTO_T))
    )
    z0 = proto_underside_z(PROTO_CENTER_Y)
    board = board.rotate((0, 0, 0), (1, 0, 0), ANGLE_DEG).translate((PROTO_CENTER_X, PROTO_CENTER_Y, z0))
    esp = esp.rotate((0, 0, 0), (1, 0, 0), ANGLE_DEG).translate((ESP_CENTER_X, PROTO_CENTER_Y, z0))
    return board, esp


# CQ-editor injects show_object into the script's global namespace.
if "show_object" in globals():
    validate()
    shell, plate = bottom_case(), assembled_top_plate()
    board, esp = controller_visuals()
    if CQ_VIEW_MODE == "separate":
        # Printable orientations, side by side with no overlapping geometry.
        offset = CASE_W * 0.56
        show_object(shell.translate((-offset, 0, 0)), name="BOTTOM_CASE", options={"color": "lightgray"})
        show_object(top_plate().translate((offset, 0, 0)), name="TOP_PLATE", options={"color": "orange"})
    else:
        # Left: assembled. Right: exploded, including non-printable references.
        left, right = -CASE_W * 0.58, CASE_W * 0.58
        show_object(shell.translate((left, 0, 0)), name="ASSEMBLED_bottom", options={"color": "lightgray"})
        show_object(plate.translate((left, 0, 0)), name="ASSEMBLED_top", options={"color": "orange", "alpha": 0.82})
        show_object(board.translate((left, 0, 0)), name="ASSEMBLED_protoboard", options={"color": "green"})
        show_object(esp.translate((left, 0, 0)), name="ASSEMBLED_ESP32", options={"color": "blue"})
        show_object(shell.translate((right, 0, 0)), name="SEPARATE_bottom", options={"color": "lightgray"})
        show_object(plate.translate((right, 0, 45)), name="SEPARATE_top", options={"color": "orange"})
        show_object(board.translate((right, 0, 18)), name="SEPARATE_protoboard", options={"color": "green"})
        show_object(esp.translate((right, 0, 18)), name="SEPARATE_ESP32", options={"color": "blue"})


if __name__ == "__main__":
    export_all()
