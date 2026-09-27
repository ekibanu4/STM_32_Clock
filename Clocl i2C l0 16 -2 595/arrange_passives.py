import pcbnew


BOARD = r"Clock i2c l0.kicad_pcb"

# Coordinates are in millimetres. Only passives and the crystal are moved.
PLACEMENT = {
    # LED current-limiting resistors: three visually aligned rows.
    "R11": (86.35, 31.50, 0),
    "R12": (99.05, 31.50, 0),
    "R13": (111.75, 31.50, 0),
    "R14": (48.25, 55.00, 0),
    "R15": (60.95, 43.50, 0),
    "R16": (73.65, 43.50, 0),
    "R17": (86.35, 43.50, 0),
    "R18": (99.05, 43.50, 0),
    "R19": (111.75, 43.50, 0),
    "R20": (52.00, 72.50, 0),
    "R21": (60.95, 73.50, 0),
    "R22": (73.65, 73.50, 0),
    "R23": (86.35, 73.50, 0),
    "R24": (99.05, 73.50, 0),
    "R25": (111.75, 73.50, 0),
    "R26": (39.00, 75.20, 0),

    # Button ladder: one resistor beside each fixed button.
    "R28": (36.50, 40.17, 0),
    "R29": (36.50, 62.00, 0),
    "R30": (122.00, 50.80, 0),
    "R31": (122.00, 73.66, 0),
    "R32": (36.50, 85.89, 0),
    "R27": (69.50, 54.00, 0),

    # MCU support and signal conditioning.
    "R2": (64.50, 52.00, 0),
    "R3": (67.00, 65.00, 0),
    "R4": (53.00, 43.00, 0),
    "R5": (67.00, 67.50, 0),
    "R6": (55.00, 64.00, 0),
    "R7": (50.00, 53.00, 0),
    "R8": (67.00, 70.00, 0),
    "R9": (59.00, 64.00, 0),
    "R10": (63.00, 28.00, 0),
    "R1": (50.00, 90.00, 0),

    # Decoupling and clock cluster around the fixed ICs.
    "C1": (66.50, 56.00, 90),
    "C2": (66.50, 60.80, 90),
    "C3": (72.00, 26.00, 0),
    "C4": (73.00, 58.35, 90),
    "C5": (120.00, 35.00, 0),
    "C6": (98.50, 58.42, 90),
    "C7": (55.00, 56.20, 0),
    "C8": (55.00, 60.60, 0),
    "Y1": (52.00, 58.40, 90),
}


board = pcbnew.LoadBoard(BOARD)
footprints = {fp.GetReference(): fp for fp in board.GetFootprints()}

missing = set(PLACEMENT) - set(footprints)
if missing:
    raise RuntimeError(f"Missing footprints: {sorted(missing)}")

for reference, (x, y, angle) in PLACEMENT.items():
    footprint = footprints[reference]
    if footprint.GetLayer() != pcbnew.B_Cu:
        footprint.Flip(footprint.GetPosition(), False)
    footprint.SetPosition(pcbnew.VECTOR2I_MM(x, y))
    footprint.SetOrientationDegrees(angle)

pcbnew.SaveBoard(BOARD, board)
print(f"Arranged {len(PLACEMENT)} passive components")
