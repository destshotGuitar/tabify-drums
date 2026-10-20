#!/usr/bin/env bash
# Rebuild resources/kits/MuldjordKit from DrumGizmo's MuldjordKit 3
# (https://drumgizmo.org/kits/MuldjordKit/MuldjordKit3.zip, CC BY 4.0).
# usage: scripts/import_muldjord.sh <path to extracted MuldjordKit3 folder>
set -euo pipefail

src="$1"
root="$(cd "$(dirname "$0")/.." && pwd)"
out="$root/resources/kits/MuldjordKit"
mkdir -p "$out"

room="OHL=0.35,OHR=0.35,AmbL=0.25,AmbR=0.25"
cymbal_room="OHL=1.0,OHR=1.0,AmbL=0.4,AmbR=0.4"

import() { # instrument prefix mix max-seconds [extra args]
    local instrument="$1" prefix="$2" mix="$3" seconds="$4"
    shift 4
    python3 -P "$root/tools/import_drumgizmo.py" "$src/$instrument/$instrument.xml" \
        --mix "$mix" --prefix "Muldjord-$prefix" --out "$out" \
        --max-seconds "$seconds" --format flac "$@"
}

import KdrumL      Kick         "KdrumL=1.0,OHL=0.3,OHR=0.3,AmbL=0.2,AmbR=0.2" 2.5
import Snare       Snare        "Snare_top=1.0,Snare_bottom=-0.5,$room" 2.5
import SnareRest   SideStick    "Snare_top=1.0,Snare_bottom=-0.5,$room" 1.5
import HihatClosed HatClosed    "Hihat=1.0,$room" 1.5
import HihatOpen   HatOpen      "Hihat=1.0,$room" 4.0
import Tom1        Tom1         "Tom1=1.0,$room" 3.0
import Tom2        Tom2         "Tom2=1.0,$room" 3.0
import Tom4        TomFloor     "Tom4=1.0,$room" 3.5
import CrashL      Crash        "$cymbal_room" 6.0 --peak 0.8
import RideR       Ride         "RideR=0.7,$cymbal_room" 6.0 --peak 0.8
import RideRBell   Bell         "RideR=0.7,$cymbal_room" 6.0 --peak 0.8
import RideL       24Ride       "RideL=0.7,$cymbal_room" 6.0 --peak 0.8 --naming numbered
