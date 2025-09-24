export interface DeviceSpec {
    specName: string,
    className: string,
    sectionCode: number,
    items: any[]
}

export const programOutputSpec: DeviceSpec = {
    specName: 'programOutputSpec',
    className: "ProgramOutput",
    sectionCode: Section.PROGRAM,
    items: [
        // | general                                 | set request spec. req data length encoded in length of the spec array
        // | method name root, type spec             | item code, req data, response data type, response data length | item code, [req byte 1 (type | value), ..., req byte n (type | value) ]
        // |                   'number|min|max|step" |
        // |                   'string|max'          |
        ["Loudness", "number|0|100|1", 0x28, [], "uint8", 1, 0x20, ["uint8"]],
        ["VelocitySensitivity", "number|-100|100|1", 0x29, [], "uint8", 1, 0x21, ["int8sign", "int8abs"]],
        ["AmpMod1Source", "number|0|14|1", 0x2A, [1], "uint8", 1, 0x22, [1, "uint8"]],
        ["AmpMod2Source", "number|0|14|1", 0x2A, [2], "uint8", 1, 0x22, [2, "uint8"]],
        ["AmpMod1Value", "number|-100|100|1", 0x2B, [1], "int8", 2, 0x23, [1, "int8sign", "int8abs"]],
        ["AmpMod2Value", "number|-100|100|1", 0x2B, [2], "int8", 2, 0x23, [2, "int8sign", "int8abs"]],
        ["PanMod1Source", "number|0|14|1", 0x2C, [1], "uint8", 1, 0x24, [1, "uint8"]],
        ["PanMod2Source", "number|0|14|1", 0x2C, [2], "uint8", 1, 0x24, [2, "uint8"]],
        ["PanMod3source", "number|0|14|1", 0x2C, [3], "uint8", 1, 0x24, [3, "uint8"]],
        ["PanMod1Value", "number|-100|100|1", 0x2D, [1], "int8", 2, 0x25, [1, "int8sign", "int8abs"]],
        ["PanMod2Value", "number|-100|100|1", 0x2D, [2], "int8", 2, 0x25, [2, "int8sign", "int8abs"]],
        ["PanMod3Value", "number|-100|100|1", 0x2D, [3], "int8", 2, 0x25, [3, "int8sign", "int8abs"]],
    ]
}

export const programMidTuneSpec = {
    specName: 'programMidTuneSpec',
    className: "ProgramMidiTune",
    sectionCode: Section.PROGRAM,
    items: [
        ["SemitoneTune", "number|-36|36|1", 0x38, [], "int8", 2, 0x30, ["int8sign", "int8abs"]],
        ["FineTune", "number|-50|50|1", 0x39, [], "int8", 2, 0x31, ["int8sign", "int8abs"]],
        ["TuneTemplate", "number|0|7|1", 0x3A, [], 'uint8', 1, 0x32, ["uint8"]],
        ["Key", "number|0|11|1", 0x3C, [], 'uint8', 1, 0x34, ["uint8"]],
    ]
} as DeviceSpec

export const programPitchBendSpec = {
    specName: 'programPitchBendSpec',
    className: "ProgramPitchBend",
    sectionCode: Section.PROGRAM,
    items: [
        ["PitchBendUp", "number|0|24|1", 0x48, [], "uint8", 1, 0x40, ["uint8"]],
        ["PitchBendDown", "number|0|24|1", 0x49, [], "uint8", 1, 0x41, ["uint8"]],
        ["BendMode", "number|0|1|1", 0x4A, [], "uint8", 1, 0x42, ["uint8"]],
        ["AftertouchValue", "number|-12|12|1", 0x4B, [], "int8", 2, 0x43, ["int8sign", "int8abs"]],
        ["LegatoEnable", "number|0|1|1", 0x4C, [], "uint8", 1, 0x44, ["uint8"]],
        ["PortamentoEnable", "number|0|1|1", 0x4D, [], "uint8", 1, 0x45, ["uint8"]],
        ["PortamentoMode", "number|0|1|1", 0x4E, [], "uint8", 1, 0x46, ["uint8"]],
        ["PortamentoTime", "number|0|100|1", 0x4F, [], "uint8", 1, 0x47, ["uint8"]],
    ]
} as DeviceSpec

export const programLfosSpec = {
    specName: 'programLfosSpec',
    className: "ProgramLfos",
    sectionCode: Section.PROGRAM,
    items: [
        ["Lfo1Rate", "number|0|100|1", 0x60, [1], "uint8", 1, 0x50, [1, "uint8"]],
        ["Lfo2Rate", "number|0|100|1", 0x60, [2], "uint8", 1, 0x50, [2, "uint8"]],
        ["Lfo1Delay", "number|0|100|1", 0x61, [1], "uint8", 1, 0x51, [1, "uint8"]],
        ["Lfo2Delay", "number|0|100|1", 0x61, [2], "uint8", 1, 0x51, [2, "uint8"]],
        ["Lfo1Depth", "number|0|100|1", 0x62, [1], "uint8", 1, 0x52, [1, "uint8"]],
        ["Lfo2Depth", "number|0|100|1", 0x62, [2], "uint8", 1, 0x52, [2, "uint8"]],
        ["Lfo1Waveform", "number|0|8|1", 0x63, [1], "uint8", 1, 0x53, [1, "uint8"]],
        ["Lfo2Waveform", "number|0|8|1", 0x63, [2], "uint8", 1, 0x53, [2, "uint8"]],
        ["Lfo1Sync", "number|0|1|1", 0x64, [1], "uint8", 1, 0x54, [1, "uint8"]],
        ["Lfo2Retrigger", "number|0|1|1", 0x65, [2], "uint8", 2, 0x55, [2, "uint8"]],
        ["Lfo1RateModSource", "number|0|14|1", 0x66, [1], "uint8", 1, 0x56, [1, "uint8"]]
    ]
}

export function getDeviceSpecs(): DeviceSpec[] {
    return [
        programOutputSpec,
        programMidTuneSpec,
        programPitchBendSpec,
        programLfosSpec
    ]
}

enum Section {
    SYSEX_CONFIG = 0x00,
    SYSTEM_SETUP = 0x02,
    MIDI_CONFIG = 0x04,
    KEYGROUP_ZONE = 0x06,
    KEYGROUP = 0x08,
    PROGRAM = 0x0A,
    MULTI = 0x0C,
    SAMPLE_TOOLS = 0x0E,
    DISK_TOOLS = 0x10,
    FX = 0x12,
    SCENLIST = 0x14,
    SONGFILE = 0x16,
    FRONT_PANEL = 0x20,
    ALT_PROGRAM = 0x2A,
    ALT_MULTI = 0x2C,
    ALT_SAMPLE = 0x2E,
    ALT_FX = 0x32
}