#!/usr/bin/env python3
"""
Generate concrete per-event XML files used by the runtime EventManager.

The grouped events_example/events_expanded files are useful as design docs,
but the current runtime EventManager loads one file per event from data/events.
"""

from __future__ import annotations

from pathlib import Path
import xml.etree.ElementTree as ET


EVENTS = [
    dict(id="sprint_01", type="Sprint", name="Highway Sprint", desc="Point-to-point race through the city", track="Test1-Flat", difficulty=2, laps=1, rewardCash=500, rewardRep=100, requiredRep=0, opponents=["FN", "LF", "MO"]),
    dict(id="circuit_01", type="Circuit", name="City Circuit", desc="Multi-lap circuit race", track="Jng1-Curly", difficulty=3, laps=3, rewardCash=1000, rewardRep=250, requiredRep=200, opponents=["ES", "FR4", "H1", "H2", "HR"]),
    dict(id="drift_01", type="Drift", name="Drift Battle", desc="Score points by drifting through zones", track="Test1-Flat", difficulty=3, laps=0, rewardCash=750, rewardRep=200, requiredRep=150, driftTarget=5000),
    dict(id="sprint_02", type="Sprint", name="Factory Sprint", desc="Sprint through the factory district", track="Jng1-Curly", difficulty=3, laps=1, rewardCash=700, rewardRep=180, requiredRep=500, opponents=["LF", "MO", "ES"]),
    dict(id="circuit_02", type="Circuit", name="Industrial Circuit", desc="Three laps through industrial roads", track="Jng1-Curly", difficulty=3, laps=3, rewardCash=1200, rewardRep=280, requiredRep=700, opponents=["MO", "ES", "FR4", "H1"]),
    dict(id="canyon_01", type="Canyon Duel", name="Factory Canyon", desc="Industrial canyon duel", track="Jng1-Curly", difficulty=4, laps=0, rewardCash=1500, rewardRep=400, requiredRep=1000, canyonTarget=75, opponents=["FR4"]),
    dict(id="sprint_03", type="Sprint", name="Harbor Sprint", desc="Sprint along the waterfront", track="Test1-Flat", difficulty=3, laps=1, rewardCash=900, rewardRep=220, requiredRep=1500, opponents=["MO", "ES", "FR4", "H1"]),
    dict(id="drift_02", type="Drift", name="Pier Drift", desc="Drift along the harbor roads", track="Test1-Flat", difficulty=4, laps=0, rewardCash=1100, rewardRep=260, requiredRep=1800, driftTarget=6500),
    dict(id="canyon_02", type="Canyon Duel", name="Harbor Duel", desc="Canyon-style duel by the waterfront", track="Test1-Flat", difficulty=4, laps=0, rewardCash=1700, rewardRep=420, requiredRep=2200, canyonTarget=82, opponents=["H1"]),
    dict(id="sprint_04", type="Sprint", name="Runway Sprint", desc="High-speed airport sprint", track="Jng1-Curly", difficulty=4, laps=1, rewardCash=1200, rewardRep=280, requiredRep=3000, opponents=["ES", "FR4", "H1", "H2", "HR"]),
    dict(id="circuit_03", type="Circuit", name="Terminal Loop", desc="Airport multi-lap circuit", track="Jng1-Curly", difficulty=4, laps=4, rewardCash=1500, rewardRep=350, requiredRep=3500, opponents=["FR4", "H1", "H2", "HR", "R1"]),
    dict(id="canyon_run_01", type="Canyon Run", name="Airport Time Attack", desc="Single canyon-style run through the airport district", track="Jng1-Curly", difficulty=4, laps=1, rewardCash=1300, rewardRep=300, requiredRep=3800, canyonTarget=70),
    dict(id="canyon_03", type="Canyon Duel", name="West Coast Duel", desc="Long-form duel on the Carbon world port", track="W2C", difficulty=5, laps=0, rewardCash=2500, rewardRep=700, requiredRep=5000, canyonTarget=88, opponents=["S8"]),
    dict(id="canyon_run_02", type="Canyon Run", name="West Coast Run", desc="Full-map canyon run on the Carbon world port", track="W2C", difficulty=5, laps=1, rewardCash=2200, rewardRep=600, requiredRep=5500, canyonTarget=74),
    dict(id="drift_03", type="Drift", name="West Coast Drift", desc="High-speed drift challenge on W2C", track="W2C", difficulty=5, laps=0, rewardCash=1800, rewardRep=500, requiredRep=5200, driftTarget=12000),
    dict(id="boss_downtown", type="Boss", name="Boss: Wolf", desc="Defeat Wolf downtown", track="Test1-Flat", difficulty=3, laps=1, rewardCash=2000, rewardRep=500, requiredRep=0, opponents=["R3"]),
    dict(id="boss_industrial", type="Boss", name="Boss: Razor", desc="Defeat Razor in the industrial district", track="Jng1-Curly", difficulty=4, laps=1, rewardCash=3000, rewardRep=750, requiredRep=500, opponents=["FR4"]),
    dict(id="boss_harbor", type="Boss", name="Boss: Kaze", desc="Defeat Kaze at the harbor", track="Test1-Flat", difficulty=4, laps=1, rewardCash=4000, rewardRep=1000, requiredRep=1500, opponents=["HI"]),
    dict(id="boss_airport", type="Boss", name="Boss: Jax", desc="Defeat Jax at the airport", track="Jng1-Curly", difficulty=5, laps=1, rewardCash=5000, rewardRep=1500, requiredRep=3000, opponents=["H2"]),
    dict(id="boss_hills", type="Boss", name="Boss: Darius", desc="Final showdown on West Coast Carbon", track="W2C", difficulty=5, laps=1, rewardCash=10000, rewardRep=5000, requiredRep=5000, opponents=["S8"]),
]


def write_event(base: Path, cfg: dict) -> None:
    root = ET.Element("event")
    root.set("type", cfg["type"])
    root.set("id", cfg["id"])
    root.set("name", cfg["name"])
    root.set("desc", cfg["desc"])
    root.set("track", cfg["track"])
    root.set("reversed", "false")
    root.set("laps", str(cfg.get("laps", 1)))
    root.set("timeLimit", "0")
    root.set("difficulty", str(cfg["difficulty"]))
    root.set("rewardCash", str(cfg["rewardCash"]))
    root.set("rewardRep", str(cfg["rewardRep"]))
    root.set("requiredRep", str(cfg["requiredRep"]))

    if "driftTarget" in cfg:
        root.set("driftTarget", str(cfg["driftTarget"]))
    if "canyonTarget" in cfg:
        root.set("canyonTarget", str(cfg["canyonTarget"]))
    if "pursuitHeat" in cfg:
        root.set("pursuitHeat", str(cfg["pursuitHeat"]))

    opponents = ET.SubElement(root, "opponents")
    for car in cfg.get("opponents", []):
        car_elem = ET.SubElement(opponents, "car")
        car_elem.text = car

    ET.indent(root)
    tree = ET.ElementTree(root)
    out = base / f"{cfg['id']}.xml"
    tree.write(out, encoding="utf-8", xml_declaration=True)


def main() -> int:
    base = Path(__file__).resolve().parents[1] / "data" / "events"
    base.mkdir(parents=True, exist_ok=True)
    for cfg in EVENTS:
        write_event(base, cfg)
    print(f"Generated {len(EVENTS)} event files in {base}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
