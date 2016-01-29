//
// Another Shadertoy example WITH MOUSE
//
// https://www.shadertoy.com/view/4dj3zh
//
// #ifdef GL_ES
// precision mediump float;
// #endif

//uniform float time;
//uniform vec2 iMouse;
//uniform vec2 iResolution;

This should cause the shader fail, right?


vec4 col;
float pixelPower;
const float powerTreshold = 2.5;
const int numberOfMetaballs = 3;
const float lineSize = 1000.0;
float norm;

vec3 ColorOfMetaball(int metaballNumber)
{
	vec3 metaColor = vec3(0.0);

	if (metaballNumber == 0)
	{
		metaColor = vec3(0.0, 1.0, 0.0);
	}
	else if (metaballNumber == 1)
	{
		metaColor = vec3(0.0, 0.0, 1.0);
	}
	else if (metaballNumber == 2)
	{
		metaColor = vec3(1.0, 0.0, 0.0);
	}

	return metaColor;
}

vec2 PositionOfMetaball(int metaballNumber)
{
	vec2 metaPos = vec2(0.0);

	if (metaballNumber == 0)
	{
		metaPos = vec2(0.5);
	}
	else if (metaballNumber == 1)
	{
		metaPos = iMouse.xy / iResolution.xy;
	}
	else if (metaballNumber == 2)
	{
		metaPos = iMouse.xy / iResolution.xy;
		metaPos /= 1.5;
	}

	metaPos.x = metaPos.x * (iResolution.x / iResolution.y);

	return metaPos;
}

float RadiusOfMetaball(int metaballNumber)
{
	float radius = 0.0;

	if (metaballNumber == 0)
	{
		radius = 0.25;
	}
	else if (metaballNumber == 1)
	{
		radius = 0.25;
	}
	else if (metaballNumber == 2)
	{
		radius = 0.25;
	}

	return radius;
}

//float norm;

float Norm(float num)
{
	//float norm = 0.9;
	float res = pow(num, norm);
	return res;
}

float SquareDistanceToMetaball(int metaballNumber)
{
	vec2 metaPos = PositionOfMetaball(metaballNumber);
	vec2 pixelPos = gl_FragCoord.xy / iResolution.xy;
	pixelPos.x = pixelPos.x * iResolution.x / iResolution.y;
	vec2 distanceVector = pixelPos - PositionOfMetaball(metaballNumber);
	distanceVector = vec2(abs(distanceVector.x), abs(distanceVector.y));
	float normDistance = Norm(distanceVector.x) + Norm(distanceVector.y);

	return normDistance;
}

float PowerOfMetaball(int metaballNumber)
{
	float power = 0.0;

	float radius = RadiusOfMetaball(metaballNumber);
	float squareDistance = SquareDistanceToMetaball(metaballNumber);


	power = Norm(radius) / squareDistance;

	return power;
}

vec3 CalculateColor(float maxPower)
{
	vec3 val = vec3(0.0);

	for (int i = 0; i < numberOfMetaballs; i++)
	{
		val += ColorOfMetaball(i) * (PowerOfMetaball(i) / maxPower);
	}

	return val;
}

void Metaballs()
{
	vec3 val;
	pixelPower = 0.0;
	col = vec4(0.0);
	int powerMeta = 0;
	float maxPower = 0.0;
	for (int i = 0; i < numberOfMetaballs; i++)
	{
		float power = PowerOfMetaball(i);
		pixelPower += power;
		/*
		if(power > 1.0)
		{
		power = 1.0;
		}
		*/
		if (maxPower < power)
		{
			maxPower = power;
			powerMeta = i;
		}
		power *= RadiusOfMetaball(i);

		//val += ColorOfMetaball(i) * power;
	}

	val = CalculateColor(maxPower);

	if (pixelPower < powerTreshold || pixelPower > powerTreshold + Norm(lineSize))
	{
		val = vec3(0.0);
	}

	col = vec4(val, 1.0);
}

void main(void)
{
	norm = 2.0;//mod(iGlobalTime, 5.0);
	Metaballs();
	//col = vec4(iMouse.xy / iResolution.xy, 0.0, 1.0);
	gl_FragColor = col;
}

