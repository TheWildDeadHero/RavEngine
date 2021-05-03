#include "common.sh"
#include <bgfx_compute.sh>

BUFFER_WR(skinmatrix, vec4, 0);

uniform vec4 NumObjects;

NUM_THREADS(32, 16, 1)
void main()
{
	mat4 identity = mtxFromRows(
		vec4(1,0,0,0),
		vec4(0,1,0,0),
		vec4(0,0,1,0),
		vec4(0,0,0,1)
	);
	if (gl_GlobalInvocationID.y >= NumObjects.x || gl_GlobalInvocationID.x >= NumObjects.y){
		return;
	}
	int offset = gl_GlobalInvocationID.y * NumObjects.y * 4 + gl_GlobalInvocationID.x * 4;
		
	//write into buffer
	for(int i = 0; i < 4; i++){
		skinmatrix[offset + i] = identity[i];
	}
}