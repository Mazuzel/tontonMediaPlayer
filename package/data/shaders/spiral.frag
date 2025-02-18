// fragment shader
#version 120


#ifdef GL_ES
precision mediump float;
#endif


uniform float time;
uniform float bpm;
uniform vec2 resolution;

void main( void ) {

	vec2 p = 2.0*( gl_FragCoord.xy / resolution.xy )-1.0;
	vec3 col = vec3(0.2, 0.5, 1.0);

	float ang = 3.1415926*sin(time)*(1.-1.5*length(gl_FragCoord.xy / resolution.xy - 0.5));
	p = mat2(cos(ang),-sin(ang),sin(ang),cos(ang))*p;
	float d = -time*1.5+ abs(p.x)+abs(p.y);
	d = mod(d+10.10,0.2)-0.1;
	if (abs(d) < 0.05 + 0.04 * sin(3. * time + 20.*length(gl_FragCoord.xy / resolution.xy - 0.5)) + 0.04 * sin(40.*atan(p.y / p.x))) col=  vec3(1.0, 0.7, 0.2);
	gl_FragColor = vec4(col, 1.0);
}
