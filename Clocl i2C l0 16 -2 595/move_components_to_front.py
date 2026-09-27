import pcbnew


BOARD = r"Clock i2c l0.kicad_pcb"
KEEP_ON_BACK = {"Q1", "BZ1", "BT1", "J1", "J2", "J3", "J4"}

board = pcbnew.LoadBoard(BOARD)
moved = []

for footprint in board.GetFootprints():
    reference = footprint.GetReference()
    desired_layer = pcbnew.B_Cu if reference in KEEP_ON_BACK else pcbnew.F_Cu
    if footprint.GetLayer() != desired_layer:
        footprint.Flip(footprint.GetPosition(), False)
        moved.append(reference)

pcbnew.SaveBoard(BOARD, board)
print(f"Moved {len(moved)} footprints: {', '.join(sorted(moved))}")
