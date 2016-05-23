uniform float numLights;
varying vec2 texCoordVarying;
varying vec3 normalVaryingViewSpace;
uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;
attribute vec4 Position;
attribute vec3 Normal;
attribute vec3 Tangent;
attribute vec3 Bitangent;
attribute vec4 TexCoord;
void main() {
	vec4 posViewSpace = ModelViewMatrix*Position;
	float lightDistance = 0.0;
	texCoordVarying = TexCoord.st;
	normalVaryingViewSpace = mat3(ModelViewMatrix)*Normal;
	gl_Position = ProjectionMatrix*posViewSpace;
}