import ida_auto
import ida_funcs
import ida_kernwin
import ida_nalt
import idautils


def emit(text):
    ida_kernwin.msg(text + "\n")


ida_auto.auto_wait()
emit("=== BLOODY7 IMPORTS / STRINGS PROBE ===")

keywords = (
    "hid", "device", "setup", "createfile", "writefile", "readfile",
    "feature", "report", "ioctl", "pipe", "file", "winusb"
)

for module_name, module_base, module_end, _ in ida_nalt.get_import_module_qty() * []:
    pass

for i in range(ida_nalt.get_import_module_qty()):
    module_name = ida_nalt.get_import_module_name(i) or "<unknown>"
    emit("[IMPORT MODULE] " + module_name)

    def visit(ea, ordinal, name):
        label = name or ("ordinal_" + str(ordinal))
        if any(k in label.lower() for k in keywords):
            emit("  %s  %s" % (hex(ea), label))
        return True

    ida_nalt.enum_import_names(i, visit)

emit("=== RELEVANT STRINGS ===")
for s in idautils.Strings():
    value = str(s)
    lower = value.lower()
    if any(k in lower for k in (
        "hid#", "vid_09da", "pid_3616", "kernel", "macro", ".amc",
        ".bwp", "bridgetouser", "deviceio", "\\\\.\\"
    )):
        emit("%s  %s" % (hex(s.ea), value))

emit("=== END ===")
ida_kernwin.qexit(0)
