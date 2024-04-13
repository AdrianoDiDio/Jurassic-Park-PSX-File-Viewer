#version 330 core
out vec4 fragColor;
  
in vec3 color;
in vec2 texCoord;
in float lightingEnabled;
in vec2 CLUTCoord;
flat in int colorMode;
flat in int STPMode;
flat in int textured;

uniform usampler2D indexTexture;
uniform sampler2D paletteTexture;
        

uint InternalToPsxColor(vec4 c) {
    uint a = uint(floor(c.a + 0.5));
    uint r = uint(floor(c.r * 31.0 + 0.5));
    uint g = uint(floor(c.g * 31.0 + 0.5));
    uint b = uint(floor(c.b * 31.0 + 0.5));
    return (a << 15) | (b << 10) | (g << 5) | r;
}

void main()
{
    uvec4 texColor;
    uint CLUTIndex;
    vec4 CLUTTexel;
    uint CLUTX;
    uint CLUTY;

    if( textured == 1 ) {
        //NOTE(Adriano):16-bpp mode textures are encoded directly into CLUT.
        if( colorMode == 2 ) {
            CLUTTexel = texelFetch(paletteTexture, ivec2(texCoord), 0);
        } else {
            texColor = texelFetch(indexTexture, ivec2(texCoord), 0);
            CLUTIndex = texColor.r;
            CLUTX = uint(CLUTCoord.x) + CLUTIndex;
            CLUTY = uint(CLUTCoord.y);
            CLUTTexel = texelFetch(paletteTexture, ivec2(CLUTX,CLUTY), 0);
        }
        if( InternalToPsxColor(CLUTTexel) == 0x0000u) {
            discard;
        }

        if( lightingEnabled > 0.5 ) {
            CLUTTexel.r = clamp(CLUTTexel.r * color.r * 2.f, 0.f, 1.f);
            CLUTTexel.g = clamp(CLUTTexel.g * color.g * 2.f, 0.f, 1.f);
            CLUTTexel.b = clamp(CLUTTexel.b * color.b * 2.f, 0.f, 1.f);
        }
    } else {
        CLUTTexel = vec4(color.rgb,1);
    }

    fragColor = vec4(CLUTTexel.rgb,1);
}
