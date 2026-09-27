from pathlib import Path
import re
import pcbnew


ROOT = Path(__file__).resolve().parent
BOARD_PATH = ROOT / "Clock i2c l0.kicad_pcb"
SCHEMATIC_PATH = ROOT / "Clock i2c l0.kicad_sch"
LIB_ROOT = Path(r"C:\Program Files\KiCad\10.0\share\kicad\footprints")

R_0805 = "Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder"
C_0805 = "Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder"

MAPPING = {f"R{i}": R_0805 for i in range(1, 33)}
MAPPING.update({
    "C1": C_0805,
    "C2": C_0805,
    "C3": "Capacitor_SMD:CP_Elec_8x10",
    "C4": C_0805,
    "C5": "Capacitor_SMD:CP_Elec_4x5.3",
    "C6": C_0805,
    "C7": C_0805,
    "C8": C_0805,
    "D1": "Diode_SMD:D_SMA_Handsoldering",
    "D2": "Diode_SMD:D_SMA_Handsoldering",
    "D3": "Diode_SMD:D_SMA_Handsoldering",
    "D4": "Diode_SMD:D_SMA_Handsoldering",
    "U3": "Package_SO:SOIC-16_3.9x9.9mm_P1.27mm",
    "U4": "Package_SO:SOIC-16_3.9x9.9mm_P1.27mm",
    "Y1": "Crystal:Crystal_SMD_3215-2Pin_3.2x1.5mm",
    "BZ1": "Buzzer_Beeper:MagneticBuzzer_CUI_CMT-8504-100-SMT",
    "J1": "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
    "J2": "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical",
    "J3": "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical",
    "J4": "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical",
})


def load_footprint(identifier):
    library, name = identifier.split(":", 1)
    footprint = pcbnew.FootprintLoad(str(LIB_ROOT / f"{library}.pretty"), name)
    if footprint is None:
        raise RuntimeError(f"Cannot load footprint {identifier}")
    footprint.SetFPID(pcbnew.LIB_ID(library, name))
    return footprint


def replace_board_footprints():
    board = pcbnew.LoadBoard(str(BOARD_PATH))
    existing = {fp.GetReference(): fp for fp in board.GetFootprints()}
    for reference, identifier in MAPPING.items():
        old = existing[reference]
        new = load_footprint(identifier)
        new.SetReference(reference)
        new.SetValue(old.GetValue())
        new.SetPosition(old.GetPosition())
        new.SetOrientation(old.GetOrientation())
        if hasattr(new, "SetPath") and hasattr(old, "GetPath"):
            new.SetPath(old.GetPath())
        old_pads = {pad.GetNumber(): pad for pad in old.Pads()}
        for pad in new.Pads():
            source = old_pads.get(pad.GetNumber())
            if source is not None:
                pad.SetNet(source.GetNet())
                pad.SetPinFunction(source.GetPinFunction())
                pad.SetPinType(source.GetPinType())
        board.Remove(old)
        board.Add(new)
    pcbnew.SaveBoard(str(BOARD_PATH), board)


def replace_schematic_footprints():
    lines = SCHEMATIC_PATH.read_text(encoding="utf-8").splitlines(keepends=True)
    changed = set()
    for index, line in enumerate(lines):
        match = re.search(r'\(property "Reference" "([A-Z]+\d+)"', line)
        if not match or match.group(1) not in MAPPING:
            continue
        reference = match.group(1)
        for property_index in range(index + 1, min(index + 80, len(lines))):
            if '(property "Footprint" ' in lines[property_index]:
                identifier = MAPPING[reference]
                lines[property_index] = re.sub(
                    r'\(property "Footprint" "[^"]*"',
                    f'(property "Footprint" "{identifier}"',
                    lines[property_index],
                    count=1,
                )
                changed.add(reference)
                break
    missing = set(MAPPING) - changed
    if missing:
        raise RuntimeError(f"Footprints not found in schematic: {sorted(missing)}")
    SCHEMATIC_PATH.write_text("".join(lines), encoding="utf-8", newline="")


if __name__ == "__main__":
    replace_board_footprints()
    replace_schematic_footprints()
    print(f"Converted {len(MAPPING)} footprints")
