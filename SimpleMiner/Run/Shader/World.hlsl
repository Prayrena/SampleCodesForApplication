

float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	float disp = rangeEnd - rangeStart;
	float proportion = (value - rangeStart) / disp;
	return proportion;
}

float Interpolate(float start, float end, float fractionTowardEnd)
{
	float disp = end - start;
	float distWithinRange = disp * fractionTowardEnd;
	float interpolatedPosition = start + distWithinRange;
	return interpolatedPosition;
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float proportion = GetFractionWithinRange(inValue, inStart, inEnd);

	float outValue = Interpolate(outStart, outEnd, proportion);
	return outValue;
}

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

	cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

	cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

cbuffer SimpleMinerWorldConstant : register(b8)
{
    float4 c_cameraWorldPos; //
}

struct vs_input_t
	{
		float3 localPosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};

	struct v2p_t
	{
		float4 position : SV_Position;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
		float3 worldPosition : WorldPosition;
	};


	v2p_t VertexMain(vs_input_t input)
	{
		float4 localPosition = float4(input.localPosition, 1);
		float4 worldPosition = mul(ModelMatrix, localPosition);
		float4 renderPosition = mul(ViewMatrix, worldPosition);
		float4 clipPosition = mul(ProjectionMatrix, renderPosition);

		v2p_t v2p;
		v2p.position = clipPosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		v2p.worldPosition = worldPosition.xyz;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
		textureColor *= ModelColor;
		textureColor *= input.color;

		float4 rgbEncodedData = input.color;
		float indoorLightBrightness = rgbEncodedData.g;
		float indoorTint = c_indoorLightColor * indoorLightBrightness;

		float outDoorLightBrightness = rgbEncodedData.r;
		float outdoorTint = c_outdoorLightColor * outdoorLightBrightness;

		clip(textureColor.a - 0.01f);

		float distanceWorldToCamera;
		float fogFraction;		

		float4 finalColor = lerp();

		return float4(textureColor);
	}
