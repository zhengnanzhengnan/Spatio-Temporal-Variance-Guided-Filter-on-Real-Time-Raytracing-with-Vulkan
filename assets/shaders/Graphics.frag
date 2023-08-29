#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "Material.glsl"
#include "UniformBufferObject.glsl"

layout(binding = 0) readonly uniform UniformBufferObjectStruct { UniformBufferObject Camera; };
layout(binding = 1) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 2) uniform sampler2D[] TextureSamplers;
layout (binding = 3) uniform sampler2D prevDepthSampler;

layout(location = 0) in vec3 FragColor;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragTexCoord;
layout(location = 3) in flat int FragMaterialIndex;
layout(location = 4) in vec4 FragClipPosition;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec2 MotionVector;

void main() 
{
	//¼ÆËãmotion vector----------------------------------------
	vec2 screenCoord = FragClipPosition.xy / FragClipPosition.w;

	float depth = texture(prevDepthSampler, 0.5 * (screenCoord + 1.0)).r;
	vec4 clipCoord = vec4(screenCoord * depth * 2.0 - vec2(depth), depth, 1.0);
	vec4 viewCoord = Camera.ProjectionInverse * clipCoord;
	viewCoord /= viewCoord.w;

	vec4 lastClipCoord = Camera.LastFrameProjection * Camera.LastFrameModelView * viewCoord;
	vec2 lastScreenCoord = lastClipCoord.xy / lastClipCoord.w;

	vec2 motionVector = screenCoord - lastScreenCoord;
	//----------------------------------------------------------

	const int textureId = Materials[FragMaterialIndex].DiffuseTextureId;
	const vec3 lightVector = normalize(vec3(5, 4, 3));
	const float d = max(dot(lightVector, normalize(FragNormal)), 0.2);
	
	vec3 c = FragColor * d;
	if (textureId >= 0)
	{
		c *= texture(TextureSamplers[textureId], FragTexCoord).rgb;
	}

    OutColor = vec4(c, 1);
	MotionVector = motionVector;
}