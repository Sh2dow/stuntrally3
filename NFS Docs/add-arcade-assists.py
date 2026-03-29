#!/usr/bin/env python3
"""
Add arcade-assists section to all .car files in SR3.

This script appends the Carbon-style arcade handling preset to all car files.
Run from the stunttrally3 root directory or specify the path as an argument.
"""

import os
import sys
from pathlib import Path

# Arcade assists section to append
ARCADE_ASSISTS_SECTION = """
# ============================================================================
# Arcade Handling Assists - Carbon Style
# ============================================================================
[ arcade-assists ]
enabled = 1
strength = 1.0

# Speed-sensitive steering
steerSpeedFactor = 1.0
steerSpeedKnee = 30.0
steerSpeedMax = 60.0
steerSpeedMinMult = 0.4

# Downforce / high-speed grip
downforceBase = 0.0
downforceSpeedMult = 0.02
downforceMax = 3000.0
gripSpeedMult = 0.15
gripSpeedKnee = 25.0

# Yaw stabilization
yawStabEnabled = 1.0
yawStabGain = 800.0
yawStabDamping = 200.0
yawStabDeadzone = 0.1
yawStabSpeedMin = 10.0

# Drift assist
driftAssistEnabled = 1.0
driftBrakeThreshold = 0.7
driftSteerMult = 1.3
driftYawTarget = 0.3
driftYawGain = 500.0
driftRearGripReduction = 0.6
driftSpeedMin = 15.0

# Nitro boost enhancement
nitroForceMult = 1.0
nitroSteerReduction = 0.5
nitroGripBoost = 0.3
nitroDuration = 0.0

# Weight transfer
weightTransferEnabled = 1.0
weightTransferLong = 0.25
weightTransferLat = 0.15
gripFrontBase = 1.0
gripRearBase = 1.0
gripLoadSensitivity = 0.3

# Air control
airControlEnabled = 1.0
airSteerMult = 0.3
airYawTorque = 300.0
airPitchTorque = 200.0
airRollDamping = 50.0

# Debug (0 = off, 1 = on)
debugOutput = 0

"""

def find_car_files(base_path):
    """Find all .car files in the carsim directory."""
    car_files = []
    carsim_path = Path(base_path) / "data" / "carsim"
    
    if not carsim_path.exists():
        print(f"Error: {carsim_path} does not exist")
        return car_files
    
    for difficulty in ["easy", "normal", "hard"]:
        diff_path = carsim_path / difficulty / "cars"
        if diff_path.exists():
            for car_file in diff_path.glob("*.car"):
                car_files.append(car_file)
    
    return car_files

def has_arcade_assists(car_file_path):
    """Check if the car file already has arcade-assists section."""
    try:
        with open(car_file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            return '[ arcade-assists ]' in content or 'arcade-assists.enabled' in content
    except Exception as e:
        print(f"  Warning: Could not read {car_file_path}: {e}")
        return False

def add_arcade_assists(car_file_path, skip_existing=True):
    """Add arcade-assists section to a car file."""
    if skip_existing and has_arcade_assists(car_file_path):
        print(f"  Skipping (already has arcade-assists): {car_file_path.name}")
        return False
    
    try:
        # Append the section
        with open(car_file_path, 'a', encoding='utf-8') as f:
            f.write(ARCADE_ASSISTS_SECTION)
        print(f"  Added: {car_file_path.name}")
        return True
    except Exception as e:
        print(f"  Error writing {car_file_path}: {e}")
        return False

def main():
    # Check for command line flags first (before path parsing)
    skip_existing = "--skip-existing" in sys.argv or "-s" in sys.argv
    force_all = "--force-all" in sys.argv or "-f" in sys.argv
    dry_run = "--dry-run" in sys.argv or "-n" in sys.argv
    
    # Remove flags from sys.argv to find path argument
    args = [a for a in sys.argv if not a.startswith('-')]
    
    # Determine base path
    if len(args) > 1:
        base_path = Path(args[1])
    else:
        # Default: look for data/carsim relative to script location
        base_path = Path(__file__).parent.parent
    
    if not force_all and not skip_existing:
        skip_existing = True  # Default behavior
    
    print(f"Searching for .car files in: {base_path / 'data' / 'carsim'}")
    print()
    
    car_files = find_car_files(base_path)
    
    if not car_files:
        print("No .car files found!")
        return 1
    
    print(f"Found {len(car_files)} .car file(s)")
    print()
    
    if dry_run:
        print("\nPreview mode - no files will be modified:\n")
    
    added = 0
    skipped = 0
    errors = 0
    
    for car_file in car_files:
        if dry_run:
            if has_arcade_assists(car_file):
                print(f"  Already has arcade-assists: {car_file.name}")
                skipped += 1
            else:
                print(f"  Would add to: {car_file.name}")
                added += 1
        else:
            result = add_arcade_assists(car_file, skip_existing=skip_existing)
            if result:
                added += 1
            elif has_arcade_assists(car_file):
                skipped += 1
            else:
                errors += 1
    
    print()
    print("=" * 60)
    if dry_run:
        print(f"Preview: {added} files would be modified, {skipped} already have arcade-assists")
        print("\nRun without -n/--dry-run to actually modify the files.")
    else:
        print(f"Done! Added arcade-assists to {added} file(s)")
        if skipped > 0:
            print(f"Skipped {skipped} file(s) that already have arcade-assists")
        if errors > 0:
            print(f"Encountered {errors} error(s)")
    
    print("\nUsage:")
    print("  python add-arcade-assists.py              # Add to files without it (default)")
    print("  python add-arcade-assists.py -f           # Force add to all files")
    print("  python add-arcade-assists.py -n           # Preview only (dry run)")
    print("  python add-arcade-assists.py -s           # Skip existing (same as default)")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
