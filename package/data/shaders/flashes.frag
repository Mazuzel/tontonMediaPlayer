#version 120

#ifdef GL_ES
precision highp float;
#endif

uniform float time;
uniform vec2 resolution;
uniform float bpm;

void main( void )
{
    
	
	float period = 60.0 / bpm;
	int div = int(time / period);
	float tMod = time - div * period;
	float pulse = 0.5*(0.5 + cos(3.14*tMod/period));
	
	float cMod = mod(div, 4);
	
	float r = pulse * (0.5 + 0.5 * cos(3.14 * (cMod) / 4.0));
	float g = pulse * (0.5 + 0.5 * cos(3.14 * (cMod + 1.5) / 4.0));
	float b = pulse * (0.5 + 0.5 * cos(3.14 * (cMod + 2.3) / 4.0));
    
    // Output to screen
    gl_FragColor = vec4(r, g, b, 1.0);
}