import { rotate, scale, translate, multMatVec4 } from './math.js';
import { getTexture, quad, setBlending } from './webgl.js';

export async function drawAnimation(ssbp, packName, animName, frame, initmatrix) {
    const pack = ssbp.animationPacks.find(pack => pack.name == packName);
    if (!pack) return;
    const animation = pack.animations.find(anim => anim.name == animName);
    if (!animation) return;
    const parts = pack.parts;
    const initdatas = animation.initialDatas;
    const framedatas = animation.frames[frame % animation.frames.length];
    
    const matrices = [];
    for (let i = 0; i < parts.length; ++i) {
        const initdata = initdatas.find(initdata => initdata.index == i);
        const framedata = framedatas.find(framedata => framedata.index == i);
        let mat = parts[i].parentIndex >= 0 ? matrices[parts[i].parentIndex] : initmatrix;
        mat = translate(mat, [
            ('posX' in framedata ? framedata.posX : initdata.pos.x) / 10,
            ('posY' in framedata ? framedata.posY : initdata.pos.y) / 10,
            0,
        ]);
        mat = rotate(mat, 'rotationZ' in framedata ? framedata.rotationZ : initdata.rotation.z);
        mat = scale(mat, [
            'scaleX' in framedata ? framedata.scaleX : initdata.scale.x,
            'scaleY' in framedata ? framedata.scaleY : initdata.scale.y,
            1
        ]);
        matrices.push(mat);
    }
    for (let i = 0; i < framedatas.length; ++i) {
        const framedata = framedatas[i];
        const part = parts[framedata.index];
        const initdata = initdatas[framedata.index];
        const matrix = matrices[framedata.index];
        if (part.type == 0 || framedata.invisible || ('opacity' in framedata ? framedata.opacity : initdata.opacity) == 0)
            continue;
        else if (part.type == 1) {
            const cellindex = 'cellIndex' in framedata ? framedata.cellIndex : initdata.cellIndex;
            if (cellindex == -1) continue;
            const cell = ssbp.cells[cellindex];
            const texture = await getTexture(cell.texture);
            if (!texture.loaded) continue;
            quad.setTexture('u_Texture', texture);
            quad.setInt('u_UseTexture', true);
            const blending = 'blending' in framedata ? framedata.blending : part.blending;
            if (blending != 0) {
                quad.setInt('u_BlendType', blending);
                setBlending(blending);
            }
            if ('textureShiftX' in framedata || initdata.textureShift.x != 0 ||
                'textureShiftY' in framedata || initdata.textureShift.y != 0 ||
                'textureRotation' in framedata || initdata.textureRotation != 0 ||
                'textureScaleX' in framedata || initdata.textureScale.x != 1 ||
                'textureScaleY' in framedata || initdata.textureScale.y != 1)
                    console.error('Unsupported operation on texture');
            const width = 'sizeX' in framedata ? framedata.sizeX : initdata.size.x;
            const height = 'sizeY' in framedata ? framedata.sizeY : initdata.size.y;
            const offsetX = (('pivotX' in framedata ? framedata.pivotX : initdata.pivot.x) + cell.pivot.x * (framedata.flipX ? -1 : 1)) * width;
            const offsetY = (('pivotY' in framedata ? framedata.pivotY : initdata.pivot.y) + cell.pivot.y * (framedata.flipY ? -1 : 1)) * height;
            quad.draw([].concat(
                multMatVec4(matrix, [-width/2 - offsetX + ('vertexTransformTL' in framedata ? framedata.vertexTransformTL.x : 0), height/2 + offsetY + ('vertexTransformTL' in framedata ? framedata.vertexTransformTL.y : 0), 0, 1]).slice(0,-1),
                multMatVec4(matrix, [-width/2 - offsetX + ('vertexTransformBL' in framedata ? framedata.vertexTransformBL.x : 0), -height/2 + offsetY + ('vertexTransformBL' in framedata ? framedata.vertexTransformBL.y : 0), 0, 1]).slice(0,-1),
                multMatVec4(matrix, [width/2 - offsetX + ('vertexTransformTR' in framedata ? framedata.vertexTransformTR.x : 0), height/2 + offsetY + ('vertexTransformTR' in framedata ? framedata.vertexTransformTR.y : 0), 0, 1]).slice(0,-1),
                multMatVec4(matrix, [width/2 - offsetX + ('vertexTransformBR' in framedata ? framedata.vertexTransformBR.x : 0), -height/2 + offsetY + ('vertexTransformBR' in framedata ? framedata.vertexTransformBR.y : 0), 0, 1]).slice(0,-1),
            ), [
                (cell.pos.x + (framedata.flipX ? cell.size.x : 0)) / texture.width,
                (cell.pos.y + (framedata.flipY ? cell.size.y : 0)) / texture.height,
                
                (cell.pos.x + (framedata.flipX ? cell.size.x : 0)) / texture.width,
                (cell.pos.y + (framedata.flipY ? 0 : cell.size.y)) / texture.height,
                
                (cell.pos.x + (framedata.flipX ? 0 : cell.size.x)) / texture.width,
                (cell.pos.y + (framedata.flipY ? cell.size.y : 0)) / texture.height,
                
                (cell.pos.x + (framedata.flipX ? 0 : cell.size.x)) / texture.width,
                (cell.pos.y + (framedata.flipY ? 0 : cell.size.y)) / texture.height,
            ], [
                1, 1, 1, ('opacity' in framedata ? framedata.opacity : initdata.opacity) / 0xFF,
                1, 1, 1, ('opacity' in framedata ? framedata.opacity : initdata.opacity) / 0xFF,
                1, 1, 1, ('opacity' in framedata ? framedata.opacity : initdata.opacity) / 0xFF,
                1, 1, 1, ('opacity' in framedata ? framedata.opacity : initdata.opacity) / 0xFF,
                //TODO
            ]);
            if (blending != 0)
                setBlending(0);
        } else if (part.type == 3) {
            // TODO
        } else {
            throw "Unsupported type";
        }
    };
}