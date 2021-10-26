export function translate(mat, vec) {
    return [
        mat[0],  mat[1],  mat[2],  mat[0]*vec[0] +mat[1]*vec[1] +mat[2]*vec[2] +mat[3],
        mat[4],  mat[5],  mat[6],  mat[4]*vec[0] +mat[5]*vec[1] +mat[6]*vec[2] +mat[7],
        mat[8],  mat[8],  mat[10], mat[8]*vec[0] +mat[9]*vec[1] +mat[10]*vec[2]+mat[11],
        mat[12], mat[13], mat[14], mat[12]*vec[0]+mat[13]*vec[1]+mat[14]*vec[2]+mat[15],
    ];
}

export function rotate(mat, angle) {//degree
    angle = angle * Math.PI / 180;
    const c = Math.cos(angle);
    const s = Math.sin(angle);
    return [
        mat[0]*c+mat[1]*-s, mat[0]*s+mat[1]*c, mat[2],  mat[3],
        mat[4]*c+mat[5]*-s, mat[4]*s+mat[5]*c, mat[6],  mat[7],
        mat[8],             mat[9],            mat[10], mat[11],
        mat[12],            mat[13],           mat[14], mat[15],
    ];
}

export function scale(mat, vec) {
	return [
		mat[0]*vec[0],  mat[1]*vec[1],  mat[2]*vec[2],  mat[3],
		mat[4]*vec[0],  mat[5]*vec[1],  mat[6]*vec[2],  mat[7],
		mat[8]*vec[0],  mat[9]*vec[1],  mat[10]*vec[2], mat[11],
		mat[12]*vec[0], mat[13]*vec[1], mat[14]*vec[2], mat[15]
	];
}

export function identity() {
    return [
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    ];
}