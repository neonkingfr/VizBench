//
// GLSL Sandbox example 1
//
//
//	Duelling Mandelbulbs
//
// http://glsl.herokuapp.com/e#1293.0
// http://www.thealphablenders.com/
// 2012 by Andrew Caudwell
//

// This has to be removed for stringify
// #ifdef GL_ES
// precision mediump float;
// #endif

/*
uniform vec2  resolution;
uniform vec2  mouse;
uniform float time;

struct Ray {
   vec3 o;
   vec3 d;
};

// dueling mandelbulbs
// @acaudwell

// http://www.fractalforums.com/mandelbulb-implementation/realtime-renderingoptimisations/

float mandelbulb(in vec3 p, float power) {
	
    float dr = 1.0;
    float r  = length(p);

    vec3 c = p;
	
    for(int i=0; i<2; i++) {
		    
        float zo0 = asin(p.z / r);
        float zi0 = atan(p.y, p.x);
        float zr  = pow(r, power-1.0);
        float zo  = (zo0) * power;
        float zi  = (zi0) * power;
        float czo = cos(zo);

        dr = zr * dr * power + 1.0;
        zr *= r;

        p = zr * vec3(czo*cos(zi), czo*sin(zi), sin(zo));
	        
	p += c;
	    
        r = length(p);
    }

    return 0.5 * r * log(r) / r;	
}

void main() {

    vec2 p = ((gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0) * 3.5;
	
    float t = time;

    Ray ray1;
    ray1.o = vec3(0.0);
    ray1.d = normalize( vec3((p - 1.5*vec2(sin(t-2.0), cos(t+1.0))) * vec2(resolution.x/resolution.y, 1.0), 1.0 ) );

    Ray ray2;
    ray2.o = vec3(0.0);
    ray2.d = normalize( vec3((p - 1.5*vec2(cos(-t),sin(t))) * vec2(resolution.x/resolution.y, 1.0), 1.0 ) );
		
    ray1.d.xy = vec2( ray1.d.x * cos(t) - ray1.d.y * sin(t), ray1.d.x * sin(t) + ray1.d.y * cos(t)); 
    ray2.d.xy = vec2( ray2.d.x * cos(t) - ray2.d.y * sin(t), ray2.d.x * sin(t) + ray2.d.y * cos(t)); 
	
    float m1 =  mandelbulb(ray1.o + ray1.d, abs(cos(t)*13.0));
    float m2 =  mandelbulb(ray2.o + ray2.d, abs(sin(t)*13.0));
	
    float f = pow(max(m1,m2) , abs(m1-m2));
    vec3  c = m1 > m2 ? vec3(0.0, 0.05, 0.2) : vec3(0.2, 0.05, 0.0);
	
    gl_FragColor = vec4(c*f, 1.0);
}
