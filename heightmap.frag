#version 150 core
//
uniform vec4 u_CameraPos;
uniform vec4 u_ScreenSize;
uniform vec4 u_TextureScale;
uniform	mat4 u_ViewProj;
uniform	mat4 u_InvViewProj;
//	vec4 u_LightPos;
//	vec4 u_LightColor;
//};
//
////uniform c_Locals {
////	vec4 u_ScreenSize;		// XY = size
////	vec4 u_TextureScale;	// XY = size, Z = height scale, w = number of layers
////};
//
uniform sampler1D t_Palette;
uniform usampler2D t_Height;
uniform usampler2D t_Color;

const float c_HorFactor = 0.5; //H_CORRECTION
const uint c_DoubleLevelMask = 1U<<6, c_ShadowMask = 1U<<7;
const uint c_TerrainShift = 3U, c_NumTerrains = 8U;
const uint c_NumBinarySteps = 8U, c_NumForwardSteps = 0U;

out vec4 Target0;


struct Surface {
	float alt;
//	uint low_type, high_type;
	vec3 tex_coord;
};

uint get_terrain_type(uint meta) {
	return (meta >> c_TerrainShift) & (c_NumTerrains - 1U);
}

Surface get_surface(vec2 pos) {
	vec3 tc = vec3(pos / u_TextureScale.xy, 0.0);
	tc.z = trunc(mod(tc.y, u_TextureScale.w));
	Surface suf;
	suf.tex_coord = tc;
//	suf.high_alt = suf.delta = 0.0;
//	uint meta = texture(t_Meta, tc).x;
//	suf.low_type = get_terrain_type(meta);
//	suf.high_type = 0U;
//	if ((meta & c_DoubleLevelMask) != 0U) {
//		if (mod(pos.x, 2.0) >= 1.0) {
//			tc.x -= 1.0 / u_TextureScale.x;
//			suf.high_type = suf.low_type;
//			uint meta_low = texture(t_Meta, tc).x;
//			suf.low_type = get_terrain_type(meta_low);
//		}else {
//			uint meta_high = textureOffset(t_Meta, tc, ivec2(1, 0)).x;
//			suf.high_type = get_terrain_type(meta_high);
//		}
//		suf.low_alt = texture(t_Height, tc).x * u_TextureScale.z;
//		suf.high_alt = textureOffset(t_Height, tc, ivec2(1, 0)).x * u_TextureScale.z;
//	}else {
		suf.alt = texture(t_Height, tc.xy).x * u_TextureScale.z;
//	}
	return suf;
}


vec3 cast_ray_to_plane(float level, vec3 base, vec3 dir) {
	float t = (level - base.z) / dir.z;
	return t * dir + base;
}

vec4 cast_ray_with_latitude(float level, vec3 base, vec3 dir) {
	vec3 pos = cast_ray_to_plane(level, base, dir);
	Surface suf = get_surface(pos.xy);
	return vec4(pos, suf.alt);
}

vec3 cast_ray_to_map(vec3 base, vec3 dir) {
	vec4 a = cast_ray_with_latitude(u_TextureScale.z, base, dir);
	vec4 b = cast_ray_with_latitude(0.0, base, dir);
	vec4 step = (1.0 / float(c_NumForwardSteps + 1U)) * (b - a);
	for (uint i=0U; i<c_NumForwardSteps; ++i) {
		vec4 c = a + step;
		Surface suf = get_surface(c.xy);
		c.w = suf.alt;
		if (c.z < c.w) {
			b = c;
			break;
		}else {
			a = c;
		}
	}
	for (uint i=0U; i<c_NumBinarySteps; ++i) {
		vec4 c = 0.5 * (a + b);
		Surface suf = get_surface(c.xy);
		c.w = suf.alt;
		if (c.z < c.w) {
			b = c;
		}else {
			a = c;
		}
	}
	//float t = a.z > a.w + 0.1 ? (b.w - a.w - b.z + a.z) / (a.z - a.w) : 0.5;
	float t = 0.5;
	return mix(a.xyz, b.xyz, t);
}

//
//void main() {
////    float cid = texture(t_Color, UV).r/256.0f;
////    color = vec3(cid, cid, cid);
//    Target0 = vec4(1f, 1f, 1f, 1f);
//}
void main() {
	vec4 sp_ndc = vec4((gl_FragCoord.xy / u_ScreenSize.xy) * 2.0 - 1.0, 0.0, 1.0);
    	vec4 sp_world = u_InvViewProj * sp_ndc;
    	vec3 view = normalize(sp_world.xyz / sp_world.w - u_CameraPos.xyz);

    	//vec3 pos = cast_ray_with_latitude(0.0, u_CamPos.xyz, view).xyw;
    	//Target0 = texture(t_Palette, pos.z / u_TextureScale.z);
    	vec3 pos = cast_ray_to_map(u_CameraPos.xyz, view);
    	{
    		Surface suf = get_surface(pos.xy);
//    		float terrain = float(suf.low_type) + 0.5;
//    		float diff = textureOffset(t_Height, suf.tex_coord, ivec2(1, 0)).x - textureOffset(t_Height, suf.tex_coord, ivec2(-1, 0)).x;
//    		float light_clr = texture(t_Table, vec3(0.5 * diff + 0.5, 0.25, terrain)).x;
//    		float tmp = light_clr - c_HorFactor * (1.0 - pos.z / u_TextureScale.z);
//    		float color_id = texture(t_Table, vec3(0.5 * tmp + 0.5, 0.75, terrain)).x;
    		float color_id = texture(t_Color, suf.tex_coord.xy).x/256f;
//    		Target0 = vec4(color_id, color_id, color_id, 1);
    		Target0 =  texture(t_Palette, color_id) * 4;
    	}
//    	vec4 target_ndc = u_ViewProj * vec4(pos, 1.0);
//    	gl_FragDepth = vec4(1, 1, 1.0f, 1.0f);//target_ndc.z / target_ndc.w * 0.5 + 0.5;
}