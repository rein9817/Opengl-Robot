#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N;
    vec3 L; 
    vec3 H; 
    vec2 texcoord;
} vertexData;

uniform sampler2D tex;

void main()
{
    vec3 texColor = texture(tex,vertexData.texcoord).rgb;
    fragColor = vec4(texColor, 1.0);
}