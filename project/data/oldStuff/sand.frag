uniform float numLights;
varying vec2 texCoordVarying;
varying vec3 normalVaryingViewSpace;
uniform sampler2D DiffuseMap;
uniform sampler2D NormalMap;
uniform sampler2D SpecularMap;
uniform vec3 ambientColor;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;
void main() {
vec4 diffuse = vec4(1.0,1.0,1.0,1.0);
vec3 surfaceNormal = normalize(normalVaryingViewSpace);
float intensity = 0.0;
diffuse = diffuse * vec4(Kd,1.0) * texture2D(DiffuseMap, texCoordVarying);
gl_FragColor = clamp(diffuse, 0.0, 1.0);
gl_FragColor = texture2D(DiffuseMap, texCoordVarying);
}