//
// Shadertoy example 1
//
//
// https://www.shadertoy.com/view/MdfSzn
//
// MetaTunnel
//
// Created by Anatole Duprat - XT95/2014 - http://www.aduprat.com/portfolio/?page=shaders
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// http://www.pouet.net/prod.php?which=52777
// By Frequency - http://www.frequency.fr/
// 1st place at Numerica Artparty in march 2009 in France
//

float time = iGlobalTime*.5;

const float s=0.4; //Density threshold

// The scene define by density
float obj(vec3 p)
{
    float d = 1.0;
    d *= distance(p, vec3(cos(time)+sin(time*0.2),0.3,2.0+cos(time*0.5)*0.5) );
    d  = distance(p,vec3(-cos(time*0.7),0.3,2.0+sin(time*0.5)));
    d *= distance(p,vec3(-sin(time*0.2)*0.5,sin(time),2.0));
    d *=cos(p.y)*cos(p.x)-0.1-cos(p.z*7.+time*7.)*cos(p.x*3.)*cos(p.y*4.)*0.1;
    return d;
}

void main()
{
    vec2 q = gl_FragCoord.xy/iResolution.xy;
