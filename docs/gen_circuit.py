"""Genera firmware/docs/circuito.mmd: un nodo por pin, una arista por cable."""
import sys

COL = {
    "3v3": "#d62728", "5v": "#ff7f0e", "12v": "#8c564b", "gnd": "#222222",
    "spi": "#1f77b4", "i2c": "#2ca02c", "sig": "#9467bd", "pwm": "#e377c2",
    "row": "#17becf",
}

lines = []
edges = []  # (a, b, label, color)


def L(s=""):
    lines.append(s)


def wire(a, b, label, kind):
    if a in NET or b in NET:
        label = ""  # the net label already names the rail
    a = net(a) if a in NET else a
    b = net(b) if b in NET else b
    edges.append((a, b, label, kind))


NET = {"R3V3": ("+3V3", "3v3"), "R5V": ("+5V", "5v"), "RGND": ("GND", "gnd")}
net_nodes = []
pending = []  # labels declared lazily inside the open subgraph


def net(rail):
    txt, _ = NET[rail]
    nid = f"N{len(net_nodes)}"
    net_nodes.append(nid)
    node = f'    {nid}[/"{txt}"\\]' if rail == "RGND" else f'    {nid}(["{txt}"])'
    # the label goes inside the most recently closed subgraph (the component it powers)
    last_end = max(i for i, x in enumerate(lines) if x == "  end")
    lines.insert(last_end, node)
    return nid


L("---")
L("title: Edifika - Circuito del nodo ESP32 (firmware/). Etiquetas +3V3 / +5V / GND con el mismo nombre son la misma red")
L("config:")
L("  layout: elk")
L("  elk:")
L("    mergeEdges: false")
L("    nodePlacementStrategy: NETWORK_SIMPLEX")
L("  flowchart:")
L("    curve: linear")
L("---")
L("flowchart LR")

# ---------- ESP32 ----------
L('  subgraph ESP["ESP32 DevKit (ESP32 Dev Module)"]')
L("    direction TB")
esp_pins = [
    ("E3V3", "3V3"), ("E5V", "5V / VIN"), ("EGND", "GND"),
    ("E4", "GPIO4"), ("E5", "GPIO5 (SS)"), ("E18", "GPIO18 (SCK)"),
    ("E19", "GPIO19 (MISO)"), ("E23", "GPIO23 (MOSI)"), ("E27", "GPIO27 (RST)"),
    ("E21", "GPIO21 (SDA)"), ("E22", "GPIO22 (SCL)"),
    ("E25", "GPIO25 (tono)"), ("E26", "GPIO26 (rele)"),
    ("E13", "GPIO13 (PWM R)"), ("E14", "GPIO14 (PWM G)"), ("E15", "GPIO15 (PWM B)"),
    ("E16", "GPIO16 (fila 1)"), ("E17", "GPIO17 (fila 2)"), ("E2", "GPIO2 (fila 3)"),
]
for nid, txt in esp_pins:
    L(f'    {nid}["{txt}"]')
L("  end")

# ---------- rieles ----------
L('  subgraph ESPPWR["Salidas de alimentacion del ESP32"]')
L("    direction TB")
L("  end")
wire("E3V3", "R3V3", "3V3", "3v3")
wire("E5V", "R5V", "5V", "5v")
wire("EGND", "RGND", "GND", "gnd")

# ---------- DHT11 ----------
L('  subgraph DHT["DHT11 (modulo 3 pines)"]')
L('    DVCC["VCC"]'); L('    DDAT["DATA"]'); L('    DGND["GND"]')
L("  end")
wire("R3V3", "DVCC", "3V3", "3v3")
wire("DDAT", "E4", "DATA", "sig")
wire("DGND", "RGND", "GND", "gnd")

# ---------- RC522 ----------
L('  subgraph RFID["Lector RFID RC522 (SPI)"]')
for p in ["3.3V", "RST", "GND", "MISO", "MOSI", "SCK", "SDA (SS)"]:
    L(f'    RF{p.split()[0].replace(".", "")}["{p}"]')
L("  end")
wire("R3V3", "RF33V", "3V3", "3v3")
wire("RFGND", "RGND", "GND", "gnd")
wire("RFSDA", "E5", "SS", "spi")
wire("RFSCK", "E18", "SCK", "spi")
wire("RFMISO", "E19", "MISO", "spi")
wire("RFMOSI", "E23", "MOSI", "spi")
wire("RFRST", "E27", "RST", "spi")

# ---------- OLED ----------
L('  subgraph OLED["OLED SSD1306 128x64 (I2C 0x3C)"]')
L('    OVCC["VCC"]'); L('    OGND["GND"]'); L('    OSCL["SCL"]'); L('    OSDA["SDA"]')
L("  end")
wire("R3V3", "OVCC", "3V3", "3v3")
wire("OGND", "RGND", "GND", "gnd")
wire("OSDA", "E21", "SDA", "i2c")
wire("OSCL", "E22", "SCL", "i2c")

# ---------- Buzzer ----------
L('  subgraph BZ["Buzzer pasivo"]')
L('    BZP["+"]'); L('    BZN["-"]')
L("  end")
wire("BZP", "E25", "tono 2 kHz", "sig")
wire("BZN", "RGND", "GND", "gnd")

# ---------- Rele + cerradura ----------
L('  subgraph RLY["Modulo rele SRD-05VDC-SL-C (canal 1, activo en LOW)"]')
L('    RYVCC["VCC"]'); L('    RYGND["GND"]'); L('    RYIN["IN1"]')
L('    RYCOM["COM"]'); L('    RYNO["NO"]'); L('    RYNC["NC (libre)"]')
L("  end")
wire("R5V", "RYVCC", "5V", "5v")
wire("RYGND", "RGND", "GND", "gnd")
wire("RYIN", "E26", "IN1", "sig")

L('  subgraph LOCK["Cerradura electrica 12 V (fuente externa)"]')
L('    PSUP(["Fuente 12 V +"])'); L('    PSUN(["Fuente 12 V -"])')
L('    LKP["Cerradura +"]'); L('    LKN["Cerradura -"]')
L('    DK["1N4007 catodo"]'); L('    DA["1N4007 anodo"]')
L("  end")
wire("PSUP", "RYCOM", "+12V", "12v")
wire("RYNO", "LKP", "+12V conmutado", "12v")
wire("LKN", "PSUN", "retorno 12V", "gnd")
wire("DK", "LKP", "diodo antiparalelo", "12v")
wire("DA", "LKN", "diodo antiparalelo", "gnd")

# ---------- LED RGB anodo comun ----------
L('  subgraph RGB["LED RGB 4 patas (anodo comun)"]')
L('    RGBA["Pata larga (anodo)"]')
L('    RGBR["R"]'); L('    RGBG["G"]'); L('    RGBB["B"]')
L('    RR["220 ohm (R)"]'); L('    RG["220 ohm (G)"]'); L('    RB["220 ohm (B)"]')
L("  end")
wire("R3V3", "RGBA", "3V3", "3v3")
for c, gpio in (("R", "E13"), ("G", "E14"), ("B", "E15")):
    wire(f"RGB{c}", f"R{c}", c, "pwm")
    wire(f"R{c}", gpio, f"PWM {c}", "pwm")

# ---------- Malla 3x3 ----------
for row, gpio in ((1, "E16"), (2, "E17"), (3, "E2")):
    L(f'  subgraph M{row}["Malla 3x3 - fila {row}"]')
    L(f'    M{row}RB["1k (base)"]')
    L(f'    M{row}QB["Q{row} NPN - base"]'); L(f'    M{row}QC["Q{row} NPN - colector"]'); L(f'    M{row}QE["Q{row} NPN - emisor"]')
    for k in (1, 2, 3):
        L(f'    M{row}R{k}["220 ohm L{row}{k}"]')
        L(f'    M{row}L{k}["LED rojo L{row}{k}"]')
    L("  end")
    wire(gpio, f"M{row}RB", f"fila {row}", "row")
    wire(f"M{row}RB", f"M{row}QB", "base", "row")
    wire(f"M{row}QE", "RGND", "GND", "gnd")
    for k in (1, 2, 3):
        wire("R5V", f"M{row}R{k}", "5V", "5v")
        wire(f"M{row}R{k}", f"M{row}L{k}", "anodo", "5v")
        wire(f"M{row}L{k}", f"M{row}QC", "catodo", "row")

# ---------- aristas + estilos ----------
for a, b, label, _ in edges:
    L(f'  {a} ---|"{label}"| {b}' if label else f"  {a} --- {b}")
for i, (_, _, _, kind) in enumerate(edges):
    L(f"  linkStyle {i} stroke:{COL[kind]},stroke-width:2.5px,color:{COL[kind]}")

L("  classDef esp fill:#1b2a41,color:#ffffff,stroke:#0d1520")
L("  classDef rail fill:#fff4d6,stroke:#b8860b,color:#222")
L("  classDef pin fill:#f5f7fa,stroke:#8899aa,color:#111")
L("  classDef part fill:#eef6ee,stroke:#5a8f5a,color:#111")
L("  class " + ",".join(n for n, _ in esp_pins) + " esp")
L("  class " + ",".join(net_nodes + ["PSUP", "PSUN"]) + " rail")

with open(sys.argv[1], "w") as f:
    f.write("\n".join(lines) + "\n")
print(len(edges), "cables")
