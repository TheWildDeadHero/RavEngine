$input a_position
$output v_color

#include <bgfx_shader.sh>
#include <bgfx_compute.sh>

BUFFER_RO(vertbuffer,float,12);
BUFFER_RO(indbuffer,int,13);
BUFFER_RO(light_databuffer,float,14);	
uniform vec4 NumObjects;		// x = start index into light_databuffer, y = number of total instances, z = number of lights shadows are being calculated for

vec3 calcNormal(vec3 u, vec3 v){

	return normalize(vec3(
		u.y * v.z - u.z * v.y,
		u.z * v.x - u.x * v.z,
		u.x * v.y - u.y * v.x
	));
}

void main()
{
	// get the assigned triangle
	int index = indbuffer[gl_InstanceID * 3];
	vec3 points[3];

	points[0] = vec3(vertbuffer[index*3],vertbuffer[index*3+1],vertbuffer[index*3+2]);
	index = indbuffer[gl_InstanceID * 3 + 1];
	points[1] = vec3(vertbuffer[index*3],vertbuffer[index*3+1],vertbuffer[index*3+2]);
	index = indbuffer[gl_InstanceID * 3 + 2];
	points[2] = vec3(vertbuffer[index*3],vertbuffer[index*3+1],vertbuffer[index*3+2]);

	// TODO: calculate the vector to translate the higher-order vertices along
	// for directional light, this is just the light's forward vector
	// for spot and point lights, this is the vector from the center of the light to the vertex being processed

	vec3 normal = calcNormal(points[0] - points[1], points[0] - points[2]);

	//TODO: this is the directional light implementation, add ifdefs for the other light types
	index = (gl_InstanceID / (int)NumObjects.y) * 3;
	vec3 toLight = vec3(light_databuffer[index],light_databuffer[index+1],light_databuffer[index+2]);
	vec3 dirvec = toLight * -30;

	// if the triangle is facing the wrong way, we don't want to have it cast shadows (reduce the number of volumes generated)
	float nDotL = max(dot(normal, toLight), 0);
	if (nDotL < 0.2){
		gl_Position = vec4(0,0,0,1);	// don't shade this by placing it at the origin
	}
	else{
		// inset the triangle by the normal a small amount, then extend it along the light ray
		gl_Position = mul(u_viewProj, vec4((points[gl_VertexID % 3] + normal * -0.02) + dirvec * (gl_VertexID < 3 ? 0 : 1),1));
	}


	// debugging color
	v_color = gl_VertexID < 3 ? vec3(1,0,0) : vec3(0,1,1);		// red = base, blue = cap
}