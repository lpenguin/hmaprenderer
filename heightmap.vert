#version 330 core


uniform vec4 u_CamPos;
uniform vec4 u_ScreenSize;
uniform vec4 u_TextureScale;
uniform mat4 u_ViewProj;
uniform mat4 u_InvViewProj;

in ivec4 a_Pos;
//layout(location = 1) in vec2 vertexUV;

out vec2 UV;

void main() {
    gl_Position = u_ViewProj * vec4(a_Pos);
//    UV = vertexUV;
}
