from __future__ import annotations

import re
import sys
from pathlib import Path

import pefile


ROOT = Path(r"C:\Program Files (x86)\Bloody7\Bloody7")
TARGETS = [
    ROOT / "Bloody7.exe",
    ROOT / "BridgeToUser.exe",
    ROOT / "Dll" / "DLL_PenSuit.dll",
    ROOT / "Dll" / "DLL_ZoomControl.dll",
]

KEYWORDS = (
    "hid", "device", "setup", "createfile", "writefile", "readfile",
    "feature", "report", "ioctl", "pipe", "winusb", "vid_09da",
    "pid_3616", "kernel", "macro", ".amc", ".bwp", "bridgetouser",
)


def strings(data: bytes):
    patterns = [
        re.compile(rb"[ -~]{5,}"),
        re.compile(rb"(?:[ -~]\x00){5,}"),
    ]
    found = set()
    for pattern in patterns:
        for match in pattern.finditer(data):
            raw = match.group(0)
            if b"\x00" in raw:
                raw = raw.decode("utf-16le", errors="ignore").encode("ascii", errors="ignore")
            try:
                value = raw.decode("ascii", errors="ignore")
            except UnicodeDecodeError:
                continue
            value = value.strip()
            if value:
                found.add(value)
    return sorted(found, key=lambda x: x.lower())


def main():
    for path in TARGETS:
        if not path.exists():
            continue
        print(f"=== {path} ===")
        pe = pefile.PE(str(path), fast_load=False)
        print(f"ImageBase=0x{pe.OPTIONAL_HEADER.ImageBase:x} EntryPoint=0x{pe.OPTIONAL_HEADER.AddressOfEntryPoint:x}")
        print("-- imports --")
        imported = []
        for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
            dll = entry.dll.decode(errors="replace")
            for item in entry.imports:
                name = item.name.decode(errors="replace") if item.name else f"ordinal_{item.ordinal}"
                imported.append((dll, name, item.address))
        for dll, name, address in imported:
            if any(word in name.lower() or word in dll.lower() for word in KEYWORDS):
                print(f"0x{address:x} {dll}!{name}")
        print("-- relevant strings --")
        for value in strings(path.read_bytes()):
            lower = value.lower()
            if any(word in lower for word in KEYWORDS):
                print(value)
        print()


if __name__ == "__main__":
    main()
