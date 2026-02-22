#version 330 core

in float v_near; //0.01
in float v_far; //100
in vec3 nearPoint;
in vec3 farPoint;
in mat4 fragView;
in mat4 fragProj;

out vec4 outColor;

vec4 makeGrid(vec3 fragPos3D, float scale)
{
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minZ = min(derivative.y, 1);
    float minX = min(derivative.x, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));

    //z-axis
    if (fragPos3D.x > -0.2 * minX && fragPos3D.x < 0.2 * minX)
        color.z = 1.0;

    // x axis
    if (fragPos3D.z > -0.2 * minZ && fragPos3D.z < 0.2 * minZ)
        color.x = 1.0;

    return color;
}

float computeDepth(vec3 pos)
{
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    float ndc_depth = clip_space_pos.z / clip_space_pos.w;
    return ndc_depth * 0.5 + 0.5; // NDC [-1,1] -> window depth [0,1]
}

float computeLinearDepth(vec3 pos)
{
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = clip_space_pos.z / clip_space_pos.w; // OpenGL NDC is already [-1, 1]
    float linearDepth = (2.0 * v_near * v_far) / (v_far + v_near - clip_space_depth * (v_far - v_near));
    return linearDepth / v_far; // normalize
}

void main()
{
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));

    outColor = (makeGrid(fragPos3D, 2) + makeGrid(fragPos3D, 1)) * float(t > 0); // adding multiple resolution for the grid
    outColor.a *= fading;
}
