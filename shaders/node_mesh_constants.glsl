/* https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/
	"We recommend using up to 64 vertices and 126 primitives"
*/
//#define VERTEX_DIMENSION_COUNT 10
//#define QUAD_DIMENSION_COUNT 9 // 10 - 1
//#define VERTEX_COUNT 100 // 10 * 10
//#define PRIMITIVE_COUNT 162 // 9 * 9 * 2
//#define HALF_PRIMITIVE_COUNT 81 // 9 * 9

#define VERTEX_DIMENSION_COUNT 8
#define QUAD_DIMENSION_COUNT 7 // 8 -1
#define VERTEX_COUNT 64 // 8 * 8
#define PRIMITIVE_COUNT 98 // 7 * 7 * 2
#define HALF_PRIMITIVE_COUNT 49

//#define VERTEX_DIMENSION_COUNT 4
//#define QUAD_DIMENSION_COUNT 3 // 4 - 1
//#define VERTEX_COUNT 16 // 4 * 4
//#define PRIMITIVE_COUNT 18 // 3 * 3 * 2
//#define HALF_PRIMITIVE_COUNT 9

#define SCALE 1

struct MeshTaskPayload
{
    vec4 ulClipPos, urClipPos, lrClipPos, llClipPos;
    vec3 ulNDC, urNDC, lrNDC, llNDC;
};