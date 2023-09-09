/* https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/
	"We recommend using up to 64 vertices and 126 primitives"
*/
//#define SCREEN_X 1920
//#define SCREEN_Y 1080
#define VERTEX_DIMENSION_COUNT 8
#define QUAD_DIMENSION_COUNT 7 // 8 -1
#define VERTEX_DIMENSION_STEP 0.0001
#define VERTEX_COUNT 64 // 8 * 8
#define PRIMITIVE_COUNT 98 // 7 * 2 * 2
#define HALF_PRIMITIVE_COUNT 49
#define SCALE 2