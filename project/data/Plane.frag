precision mediump float;
uniform float numLights;
varying vec2 texCoordVarying;
varying vec3 normalVaryingViewSpace;
uniform sampler2D DiffuseMap;
uniform sampler2D NormalMap;
uniform sampler2D SpecularMap;
uniform vec3 ambientColor;
uniform sampler2D CausticsTexture;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;
varying mediump vec4 posVarying; // pos in world space
varying mediump vec3 normalVarying;     // normal in world space


void main() {
    vec4 diffuse = vec4(1.0,1.0,1.0,1.0);
    vec3 surfaceNormal = normalize(normalVaryingViewSpace);
    float intensity = 0.0;
    diffuse = diffuse * vec4(Kd,1.0) * texture2D(DiffuseMap, texCoordVarying);
    
    // Texture coordinates for caustics
    float caustX = posVarying.x / 100.0;
    float caustZ = posVarying.z / 100.0;
    
    vec4 caust = vec4(vec3(0.0), 1.0);
    vec3 up = vec3(0.0, 1.0, 0.0);
    float upAngle = dot(up, normalVarying);
    if (upAngle > 0.0) {
        // sharp edge:
        // caust = texture2D(CausticsTexture, vec2(caustX, caustZ))
        
        // softer edge
        vec4 caustTexCol = texture2D(CausticsTexture, vec2(caustX, caustZ));
        caust = upAngle * upAngle * upAngle * caustTexCol * caustTexCol;
    }
    
    
    gl_FragColor = clamp(diffuse, 0.0, 1.0);
    gl_FragColor = vec4(vec3(0.5), 1.0);
    gl_FragColor = clamp(diffuse + 0.3 * caust, 0.0, 1.0);
    // gl_FragColor = mix(diffuse, caust, 0.5);
}