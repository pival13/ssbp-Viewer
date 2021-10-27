export async function loadSsbp(file) {
    file = await (new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onloadend = () => resolve(reader.result);
        reader.readAsArrayBuffer(file);
    }));
    const data = new Uint8Array(file);
    const reader = new DataView(file);
    function readInt(off) { return reader.getInt32(off, true); }
    function readUInt(off) { return reader.getUint32(off, true); }
    function readFloat(off) { return reader.getFloat32(off, true); }
    function readShort(off) { return reader.getInt16(off, true); }
    function readUShort(off) { return reader.getUint16(off, true); }
    function readStringSize(off, size) { return String.fromCharCode.apply(null, data.slice(off, off+size)); };
    function readString(off) { off = readUInt(off); return off === 0 ? "" : readStringSize(off, data.indexOf(0, off)-off); };
    function readVec2(off) { return {x: readShort(off), y: readShort(off+2)}; };
    function readVec2f(off) { return {x: readFloat(off), y: readFloat(off+4)}; };
    function readVec3(off) { return {x: readShort(off), y: readShort(off+2), z: readShort(off+4)}; };
    function readVec3f(off) { return {x: readFloat(off), y: readFloat(off+4), z: readFloat(off+8)}; };
    function arrayOf(count, generator, off, size) { return Array.from({length: count}, (_,i) => generator(off+i*size)) };

    if (readStringSize(0,4) != "SSPB" || readUInt(4) != 3)
        throw TypeError("Invalid SSBP file")
    const obj = {
        magickWord: readStringSize(0, 4),
        version: readUInt(4),
        flags: readUInt(8),
        imageBaseDir: readString(12),
        cells: arrayOf(readShort(28), (off) => Object({
            name: readString(off+0),
            texture: {
                name: readString(readInt(off+4)+0),
                path: readString(readInt(off+4)+4),
                index: readShort(readInt(off+4)+8),
                wrapMode: readShort(readInt(off+4)+10),
                filterMode: readShort(readInt(off+4)+12),
                _padding: readShort(readInt(off+4)+14)
            },
            spriteIndex: readShort(off+8), // index on the texture
            pos: readVec2(off+10),
            size: readVec2(off+14),
            _padding: readShort(off+18),
            pivot: readVec2f(off+20)
        }), readShort(16), 28),
        animationPacks: arrayOf(readShort(30), (offPack) => Object({
            name: readString(offPack),
            parts: arrayOf(readShort(offPack+12), (off) => Object({
                name: readString(off),
                index: readShort(off+4),
                parentIndex: readShort(off+6),
                type: readShort(off+8),
                boundsType: readShort(off+10),
                blending: readShort(off+12),
                _padding: readShort(off+14),
                animation: readString(off+16),
                effect: readString(off+20),
                colorLabel: readString(off+24),
            }), readUInt(offPack+4), 28),
            animations: arrayOf(readShort(offPack+14), (off) => Object({
                name: readString(off),
                initialDatas: arrayOf(readShort(offPack+12), (off) => Object({
                    index: readShort(off),
                    _padding1: readShort(off+2),
                    flags: readInt(off+4),
                    cellIndex: readShort(off+8),
                    pos: readVec3(off+10),
                    opacity: readShort(off+16),
                    _padding2: readShort(off+18),
                    pivot: readVec2f(off+20),
                    rotation: readVec3f(off+28),
                    scale: readVec2f(off+40),
                    size: readVec2f(off+48),
                    textureShift: readVec2f(off+56),
                    textureRotation: readFloat(off+64),
                    textureScale: readVec2f(off+68),
                    boundingRadius: readFloat(off+76)
                }), readUInt(off+4), 80),
                frames: arrayOf(readShort(off+20), (off) => {
                    off = readUInt(off);
                    const readU8 = () => {const v=reader.getUint8(off);off+=1;return v};
                    const readS16 = () => {const v=readShort(off);off+=2;return v};
                    const readU16 = () => {const v=readUShort(off);off+=2;return v};
                    const readU32 = () => {const v=readUInt(off);off+=4;return v};
                    const readF32 = () => {const v=readFloat(off);off+=4;return v};
                    const read2U16 = () => {const v=readVec2(off);off+=4;return v};
                    const read4U8 = () => {return{a:readU8(),r:readU8(),g:readU8(),b:readU8()}};
                    const parts = [];
                    for (let i = 0; i < readShort(offPack+12); ++i) {
                        const part = {};
                        part.index = readS16();
                        part.flags = readU32();
                        part.invisible = part.flags & (1 << 0);
                        part.flipX = part.flags & (1 << 1);
                        part.flipY = part.flags & (1 << 2);
                        if (part.flags & (1 << 3)) part.cellIndex = readS16();
                        if (part.flags & (1 << 4)) part.posX = readS16();
                        if (part.flags & (1 << 5)) part.posY = readS16();
                        if (part.flags & (1 << 6)) part.posZ = readS16();
                        if (part.flags & (1 << 7)) part.pivotX = readF32();
                        if (part.flags & (1 << 8)) part.pivotY = readF32();
                        if (part.flags & (1 << 9)) part.rotationX = readF32();
                        if (part.flags & (1 << 10)) part.rotationY = readF32();
                        if (part.flags & (1 << 11)) part.rotationZ = readF32();
                        if (part.flags & (1 << 12)) part.scaleX = readF32();
                        if (part.flags & (1 << 13)) part.scaleY = readF32();
                        if (part.flags & (1 << 14)) part.opacity = readU16();
                        if (part.flags & (1 << 17)) part.sizeX = readF32();
                        if (part.flags & (1 << 18)) part.sizeY = readF32();
                        if (part.flags & (1 << 19)) part.textureShiftX = readF32();
                        if (part.flags & (1 << 20)) part.textureShiftY = readF32();
                        if (part.flags & (1 << 21)) part.textureRotation = readF32();
                        if (part.flags & (1 << 22)) part.textureScaleX = readF32();
                        if (part.flags & (1 << 23)) part.textureScaleY = readF32();
                        if (part.flags & (1 << 24)) part.boundingRadius = readF32();
                        if (part.flags & (1 << 16)) {
                            const flag = readU16();
                            if (flag & (1 << 0)) part.vertexTransformTL = read2U16();
                            if (flag & (1 << 1)) part.vertexTransformTR = read2U16();
                            if (flag & (1 << 2)) part.vertexTransformBL = read2U16();
                            if (flag & (1 << 3)) part.vertexTransformBR = read2U16();
                        }
                        if (part.flags & (1 << 15)) {
                            part.blending = readU8();
                            const flag = readU8();
                            if (flag & (1 << 4)) {
                                part.blendRate = readF32();
                                part.vertexColorTL = part.vertexColorTR = part.vertexColorBL = part.vertexColorBR = read4U8();
                            } else {
                                if (flag & (1 << 0)) part.vertexColorTL = read4U8();
                                if (flag & (1 << 1)) part.vertexColorTR = read4U8();
                                if (flag & (1 << 2)) part.vertexColorBL = read4U8();
                                if (flag & (1 << 3)) part.vertexColorBR = read4U8();
                            }
                        }
                        if (part.flags & (1 << 25)) part.animationKeyframe = readS16();
                        if (part.flags & (1 << 26)) part.animationStart = readS16();
                        if (part.flags & (1 << 27)) part.animationEnd = readS16();
                        if (part.flags & (1 << 28)) part.animationSpeed = readF32();
                        if (part.flags & (1 << 29)) part.animationNbLoop = readS16();
                        if (part.flags & (1 << 30)) {
                            const flag = readS16();
                            part.animationInfinity = flag & (1 << 0);
                            part.animationReverse = flag & (1 << 1);
                            part.animationPingpong = flag & (1 << 2);
                            part.animationIndependent = flag & (1 << 3);
                        }
                        parts[i] = part;
                    }
                    return parts;
                }, readUInt(off+8), 4),
                userDatas: arrayOf(/*readShort(off+20)*/0, (off) => "TODO: UserData", readUInt(off+12), 0),
                labels: arrayOf(readShort(off+24), off => Object({
                    name: readString(readUInt(off)),
                    frame: readUShort(readUInt(off)+4)
                }), readUInt(off+16), 4),
                FPS: readShort(off+22),
                canvasSize: readVec2(off+26),
                _padding: readShort(off+30)
            }), readUInt(offPack+8), 32)
        }), readUInt(20), 16),
        effects: arrayOf(readShort(32), (off) => "TODO: effect", readUInt(24), 0),
    };
    return obj;
}

