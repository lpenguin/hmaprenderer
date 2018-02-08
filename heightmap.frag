#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform usampler2D myTextureSampler;
uniform sampler1D paletteSampler;

void main(){
//    color = texture(paletteSampler, UV.x).rgb;
	// Output color = color of the texture at the specified UV

        float color_id = texture( myTextureSampler, UV ).x;

////		vec3 r = t;
//		float color_id = t.x;
		color_id = color_id/256;
		color = texture(paletteSampler, color_id).rgb * 4;

//    	color =  vec3(color_id, color_id, color_id);
//    	color =  texture(paletteSampler, color_id).rgb;
}