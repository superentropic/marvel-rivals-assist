from __future__ import annotations

import json
import sys
import time
from pathlib import Path

import frida


PROCESS = "Bloody7.exe"

JS = r'''
function dumpBytes(ptr, length) {
    try {
        const n = Math.min(Number(length), 256);
        const raw = ptr.readByteArray(n);
        if (raw === null) return "<null>";
        return Array.from(new Uint8Array(raw))
            .map(x => x.toString(16).padStart(2, "0"))
            .join("");
    } catch (e) {
        return "<unreadable:" + e + ">";
    }
}

function hookExport(moduleName, symbol, bufferArg, lengthArg) {
    let address = null;
    try {
        const module = Process.getModuleByName(moduleName);
        address = module.findExportByName(symbol);
    } catch (e) {
        try {
            address = Module.getExportByName(moduleName, symbol);
        } catch (ignored) {
            address = null;
        }
    }
    if (!address) {
        send({event: "missing", module: moduleName, symbol: symbol});
        return;
    }
    Interceptor.attach(address, {
        onEnter(args) {
            const length = args[lengthArg].toUInt32();
            send({
                event: "call",
                module: moduleName,
                symbol: symbol,
                length: length,
                data: dumpBytes(args[bufferArg], length)
            });
        }
    });
    send({event: "hooked", module: moduleName, symbol: symbol});
}

hookExport("hid.dll", "HidD_SetFeature", 1, 2);
'''


REPORT_LOG = Path(__file__).with_name("bloody_hid_setfeature.jsonl")


def on_message(message, data):
    if message.get("type") == "send":
        payload = message.get("payload")
        line = json.dumps(payload, sort_keys=True)
        with REPORT_LOG.open("a", encoding="utf-8") as log:
            log.write(line + "\n")
        print(line, flush=True)
    elif message.get("type") == "error":
        print(json.dumps(message, sort_keys=True), flush=True)


def main():
    try:
        session = frida.attach(PROCESS)
    except Exception as exc:
        print(f"ATTACH_FAILED: {exc}", flush=True)
        return 1

    script = session.create_script(JS)
    script.on("message", on_message)
    script.load()
    print("TRACE_ATTACHED", flush=True)
    try:
        time.sleep(120)
    finally:
        session.detach()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
