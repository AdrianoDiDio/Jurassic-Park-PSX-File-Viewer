#version 330 core
layout (location = 0) in ivec3 inPos;
layout (location = 1) in ivec2 inTexCoord;
layout (location = 2) in ivec3 inColor;
layout (location = 3) in ivec2 inCLUTCoord;
layout (location = 4) in int   inColorMode;
layout (location = 5) in int   inTextured;

uniform mat4 MVPMatrix;
uniform bool enableLighting;
out vec3 color;
out vec2 texCoord;
out float lightingEnabled;
out vec2 CLUTCoord;
flat out int colorMode;
flat out int textured;

void main()
{
    gl_Position =  MVPMatrix * vec4(inPos, 1.0);
    color = vec3(inColor) / 255.f;
    texCoord = vec2(inTexCoord) + vec2(0.001, 0.001);
    lightingEnabled = enableLighting ? 1.0 : 0.0;
    CLUTCoord = vec2(inCLUTCoord) + vec2(0.001, 0.001);
    colorMode = inColorMode;
    textured = inTextured;
}
