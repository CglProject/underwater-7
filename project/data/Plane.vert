precision mediump float;
uniform float numLights;
varying vec2 texCoordVarying;
varying vec3 normalVaryingViewSpace;
uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;

uniform mediump mat4 ModelMatrix;
uniform mediump mat3 NormalMatrix;
varying mediump vec4 posVarying; // pos in world space
varying mediump vec3 normalVarying;     // normal in world space

attribute vec4 Position;
attribute vec3 Normal;
attribute vec3 Tangent;
attribute vec3 Bitangent;
attribute vec4 TexCoord;
void main() {
    posVarying = ModelMatrix * Position;
    normalVarying = normalize(NormalMatrix * Normal);
    
    vec4 posViewSpace = ModelViewMatrix*Position;
    float lightDistance = 0.0;
    texCoordVarying = TexCoord.st;
    normalVaryingViewSpace = mat3(ModelViewMatrix)*Normal;
    gl_Position = ProjectionMatrix*posViewSpace;
}