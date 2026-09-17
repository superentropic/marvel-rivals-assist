#Requires AutoHotkey v2.0
#SingleInstance Force
#Warn All, StdOut
Persistent
SendMode("Event")
SetMouseDelay(-1)

; Matches Source/TriggerControlShared.h. Fire and aim have independent
; monotonic refresh counters in the V4 packet.
kMappingName := "Local\MarvelTriggerControlStateV4"
kMagic := 0x4D544132
kStateSize := 48
kAimFovPx := 30
kAimHeightPct := 84
kAimSmoothing := 50
kFreshMs := 90
kReleaseMs := 65
kDeadZonePx := 1.5

gMapping := 0
gState := 0
gEnabled := false
gTriggerEnabled := true ; Firing armed, but master always starts OFF.
gButtonHeld := false
gPacket := 0
gLastFireRequest := 0
gLastAimUpdate := 0
gLastFireTick := 0
gLastAimTick := 0
gShowStatus := false
gTimerPeriod := false

if A_Args.Length && A_Args[1] = "--self-test" {
    RunSelfTests()
    ExitApp()
}
OnExit(ShutdownController)
InitializeController()
if gState
    WriteLong(gState + 4, 0)
gTimerPeriod := DllCall("winmm\timeBeginPeriod", "UInt", 1, "UInt") = 0
SetTimer(ControlTick, 8)
SetTimer(UpdateStatus, 200)
ShowToast("Assist OFF | Insert: toggle | Delete: firing | F6: diagnostics")
if A_Args.Length && A_Args[1] = "--startup-test" {
    gShowStatus := true
    SetTimer(() => ExitApp(), -1000)
}

Insert::ToggleAssist()
Delete::ToggleTrigger()
F7::AdjustAimHeight(-5)
F8::AdjustAimHeight(5)
F9::AdjustAimFov(-5)
F10::AdjustAimFov(5)
7::AdjustAimSmoothing(-5)
8::AdjustAimSmoothing(5)
F6::ToggleStatus()
0::ExitApp()

NowTick() => DllCall("Kernel32\GetTickCount", "UInt")
Age(tick) => (NowTick() - tick) & 0xFFFFFFFF

ToggleAssist() {
    global gEnabled, gState, gLastFireRequest, gLastAimUpdate
    global gLastFireTick, gLastAimTick, gPacket
    Critical("On")
    gEnabled := !gEnabled
    InitializeController()
    if gState
        WriteLong(gState + 4, gEnabled ? 1 : 0)
    gLastFireRequest := 0
    gLastAimUpdate := 0
    gLastFireTick := 0
    gLastAimTick := 0
    gPacket := 0
    ReleaseHeldButton()
    Critical("Off")
    ShowToast("Assist " (gEnabled ? "ON" : "OFF"))
}

ToggleTrigger() {
    global gTriggerEnabled, gLastFireTick
    Critical("On")
    gTriggerEnabled := !gTriggerEnabled
    gLastFireTick := 0
    if !gTriggerEnabled
        ReleaseHeldButton()
    Critical("Off")
    ShowToast("Firing " (gTriggerEnabled ? "ON" : "OFF") " | Insert controls master")
}

AdjustAimFov(delta) {
    global gState, kAimFovPx
    kAimFovPx := Max(5, Min(300, kAimFovPx + delta))
    if gState
        WriteLong(gState + 32, kAimFovPx)
    ShowToast("Head FOV: " kAimFovPx " px")
}

AdjustAimHeight(delta) {
    global gState, kAimHeightPct
    kAimHeightPct := Max(75, Min(100, kAimHeightPct + delta))
    if gState
        WriteLong(gState + 40, kAimHeightPct)
    ShowToast("Aim height: " kAimHeightPct "%")
}

AdjustAimSmoothing(delta) {
    global gState, kAimSmoothing
    kAimSmoothing := Max(0, Min(100, kAimSmoothing + delta))
    if gState
        WriteLong(gState + 44, kAimSmoothing)
    ShowToast("Aim smoothing: " kAimSmoothing "%")
}

ShowToast(message) {
    ToolTip(message, 12, 130, 2)
    SetTimer(() => ToolTip(,,, 2), -1600)
}

ToggleStatus() {
    global gShowStatus
    gShowStatus := !gShowStatus
    if !gShowStatus
        ToolTip()
    else
        UpdateStatus()
}

GameFocused() {
    return WinActive("ahk_exe Marvel-Win64-Shipping.exe") != 0
}

ReadPacket() {
    global gState, kMagic
    if !gState || ReadUInt(gState) != kMagic
        return 0
    ; Fire is a standalone counter. Aim uses its own counter to guard the
    ; coordinate snapshot. Retry locally if a frame lands mid-publication.
    Loop 4 {
        aimBefore := ReadUInt(gState + 36)
        packet := {
            fireRequest: ReadUInt(gState + 12), status: ReadLong(gState + 8),
            x: ReadLong(gState + 20), y: ReadLong(gState + 24),
            aim: ReadLong(gState + 28), fov: ReadLong(gState + 32),
            aimUpdate: aimBefore
        }
        if aimBefore = ReadUInt(gState + 36)
            return packet
    }
    ; Preserve firing even if aim is updating too quickly for a coherent read.
    return {fireRequest: ReadUInt(gState + 12), status: ReadLong(gState + 8),
        x: 0, y: 0, aim: 0, fov: ReadLong(gState + 32), aimUpdate: 0}
}

ControlTick() {
    global gState, gEnabled, gTriggerEnabled, gPacket
    global gLastFireRequest, gLastAimUpdate, gLastFireTick, gLastAimTick
    global gButtonHeld, kFreshMs, kReleaseMs
    Critical("On")
    try {
        if !InitializeController() {
            ReleaseHeldButton()
            return
        }
        latest := ReadPacket()
        if IsObject(latest)
            gPacket := latest
        now := NowTick()
        if !gEnabled || !IsObject(gPacket) || !GameFocused() {
            ReleaseHeldButton()
            return
        }
        if gPacket.fireRequest != gLastFireRequest {
            gLastFireRequest := gPacket.fireRequest
            gLastFireTick := now
        }
        if gPacket.aim && gPacket.aimUpdate != gLastAimUpdate {
            gLastAimUpdate := gPacket.aimUpdate
            gLastAimTick := now
        }
        fireFresh := gLastFireTick && Age(gLastFireTick) <= kFreshMs
        freshAim := gPacket.aim && gLastAimTick && Age(gLastAimTick) <= kFreshMs
        ; Firing and movement are intentionally independent. A validated
        ; trigger refresh controls LButton; selected-head refresh controls aim.
        if gTriggerEnabled && fireFresh {
            if !gButtonHeld {
                SendEvent("{LButton down}")
                gButtonHeld := true
            }
        } else if !gTriggerEnabled || !gLastFireTick || Age(gLastFireTick) >= kReleaseMs {
            ReleaseHeldButton()
        }
        if !freshAim {
            return
        }
        viewport := GetGameViewport()
        if !IsObject(viewport) || gPacket.x < 0 || gPacket.y < 0
            || gPacket.x >= viewport.width || gPacket.y >= viewport.height {
            return
        }
        ; ProjectWorldLocationToScreen returns client coordinates. Never use
        ; these as desktop coordinates: move relative to the viewport centre.
        dx := gPacket.x - viewport.width / 2.0
        dy := gPacket.y - viewport.height / 2.0
        step := ComputeStep(dx, dy, gPacket.fov)
        if step.x || step.y
            MouseMove(step.x, step.y, 0, "R")
    } catch as err {
        gEnabled := false
        if gState
            WriteLong(gState + 4, 0)
        ReleaseHeldButton()
        ShowToast("Assist stopped: " err.Message)
    } finally {
        Critical("Off")
    }
}

GetGameViewport() {
    hwnd := WinExist("ahk_exe Marvel-Win64-Shipping.exe")
    if !hwnd
        return 0
    rect := Buffer(16, 0)
    if !DllCall("User32\GetClientRect", "Ptr", hwnd, "Ptr", rect.Ptr, "Int")
        return 0
    width := NumGet(rect, 8, "Int")
    height := NumGet(rect, 12, "Int")
    return width > 0 && height > 0 ? {width: width, height: height} : 0
}

ComputeStep(dx, dy, aimFovPx) {
    global kDeadZonePx, kAimSmoothing
    distance := Sqrt(dx * dx + dy * dy)
    if distance <= kDeadZonePx
        return {x: 0, y: 0}
    ; A proportional/eased controller: targets farther from centre (relative
    ; to the configured FOV) gain speed, but each update remains bounded.
    normalized := Min(1.0, distance / Max(5, aimFovPx))
    speed := 1.35 - Max(0, Min(100, kAimSmoothing)) / 100.0
    variation := 0.96 + Mod(DllCall("Kernel32\GetTickCount64", "UInt64"), 9) / 100.0
    gain := (0.12 + 0.08 * normalized) * speed * variation
    maxStep := Min(14, Max(1, Round((4 + 10 * normalized) * speed)))
    moveX := Max(-maxStep, Min(maxStep, Round(dx * gain)))
    moveY := Max(-maxStep, Min(maxStep, Round(dy * gain)))
    return {x: moveX, y: moveY}
}

ReleaseHeldButton() {
    global gButtonHeld
    if gButtonHeld {
        SendEvent("{LButton up}")
        gButtonHeld := false
    }
}

UpdateStatus() {
    global gShowStatus, gEnabled, gTriggerEnabled, gPacket, kAimFovPx, kAimHeightPct, kAimSmoothing, kFreshMs
    global gLastFireTick, gLastAimTick
    if !gShowStatus
        return
    title := "Assist: " (gEnabled ? "ON" : "OFF") " | Firing: "
        (gTriggerEnabled ? "ON" : "OFF") " | FOV: " kAimFovPx " px | Height: " kAimHeightPct "% | Smooth: " kAimSmoothing "%"
    if !IsObject(gPacket) {
        ToolTip(title "`nNo V4 DLL data. Reload the rebuilt DLL.", 12, 12)
        return
    }
    names := Map(0, "Waiting", 1, "Assist off", 2, "Engine unavailable",
        3, "Viewport unavailable", 4, "World unavailable", 5, "Controller unavailable",
        6, "Camera unavailable", 7, "Character class unavailable", 12, "Pawn unavailable",
        21, "No live enemies", 22, "Exact Head socket unavailable",
        23, "No visible head", 24, "Visible head outside FOV", 25, "Head acquired",
        26, "Head projection failed",
        27, "Actor query unavailable", 28, "Actor query returned invalid array",
        29, "Detection exception", 30, "No skeletal components returned",
        31, "Mesh/socket function unavailable", 32, "Mesh/socket lookup exception",
        33, "Target area acquired")
    reason := names.Has(gPacket.status) ? names[gPacket.status] : "Status " gPacket.status
    fireAge := gLastFireTick ? Age(gLastFireTick) : "none"
    refreshAge := gLastAimTick ? Age(gLastAimTick) : "none"
    ToolTip(title "`n" reason " | fire/aim age: " fireAge "/" refreshAge " ms"
        "`nGame focused: " (GameFocused() ? "yes" : "no"), 12, 12)
}

InitializeController() {
    global gMapping, gState, kMappingName, kMagic, kStateSize, kAimFovPx, gEnabled
    if gState
        return true
    gMapping := DllCall("Kernel32\CreateFileMappingW", "Ptr", -1, "Ptr", 0,
        "UInt", 0x04, "UInt", 0, "UInt", kStateSize, "WStr", kMappingName, "Ptr")
    if !gMapping
        return false
    gState := DllCall("Kernel32\MapViewOfFile", "Ptr", gMapping, "UInt", 0x0006,
        "UInt", 0, "UInt", 0, "UPtr", kStateSize, "Ptr")
    if !gState {
        DllCall("Kernel32\CloseHandle", "Ptr", gMapping)
        gMapping := 0
        return false
    }
    if ReadUInt(gState) != kMagic {
        Loop kStateSize
            NumPut("UChar", 0, gState + A_Index - 1)
        WriteLong(gState + 32, kAimFovPx)
        WriteLong(gState + 40, kAimHeightPct)
        WriteLong(gState + 44, kAimSmoothing)
        WriteLong(gState, kMagic)
    }
    WriteLong(gState + 4, gEnabled ? 1 : 0)
    WriteLong(gState + 32, kAimFovPx)
    WriteLong(gState + 40, kAimHeightPct)
    WriteLong(gState + 44, kAimSmoothing)
    return true
}

ReadLong(address) => NumGet(address, 0, "Int")
ReadUInt(address) => NumGet(address, 0, "UInt")
WriteLong(address, value) => NumPut("Int", value, address, 0)

ShutdownController(*) {
    global gMapping, gState, gTimerPeriod
    if gState
        WriteLong(gState + 4, 0)
    ReleaseHeldButton()
    if gState {
        DllCall("Kernel32\UnmapViewOfFile", "Ptr", gState)
        gState := 0
    }
    if gMapping {
        DllCall("Kernel32\CloseHandle", "Ptr", gMapping)
        gMapping := 0
    }
    if gTimerPeriod
        DllCall("winmm\timeEndPeriod", "UInt", 1)
}

RunSelfTests() {
    global gState, kMagic, kStateSize, kAimSmoothing
    ; No mapping, mouse output, timers, or game access in this mode.
    if kStateSize != 48
        throw Error("Shared state allocation must be 48 bytes")
    testBuffer := Buffer(kStateSize, 0)
    gState := testBuffer.Ptr
    WriteLong(gState, kMagic)
    WriteLong(gState + 20, 123)
    WriteLong(gState + 24, 456)
    WriteLong(gState + 28, 1)
    WriteLong(gState + 32, 30)
    WriteLong(gState + 12, 1)
    WriteLong(gState + 36, 7)
    WriteLong(gState + 40, 84)
    WriteLong(gState + 44, 50)
    packet := ReadPacket()
    if !IsObject(packet) || packet.x != 123 || packet.y != 456 || packet.aim != 1
        || packet.fov != 30 || packet.fireRequest != 1 || packet.aimUpdate != 7
        throw Error("Packet layout test failed")
    gState := 0
    for fov in [5, 30, 300] {
        step := ComputeStep(500, -500, fov)
        if Abs(step.x) > 14 || Abs(step.y) > 14
            throw Error("Step cap test failed")
        step := ComputeStep(0, 0, fov)
        if step.x || step.y
            throw Error("Dead-zone drift test failed")
    }
    kAimSmoothing := 0
    fast := ComputeStep(100, 0, 125)
    kAimSmoothing := 100
    slow := ComputeStep(100, 0, 125)
    if slow.x >= fast.x || slow.x < 0
        throw Error("Smoothing control must reduce movement without reversing it")
    kAimSmoothing := 50
    FileAppend("PASS: 48-byte allocation, independent counters, step bounds, dead-zone stop, smoothing control", "*")
}
