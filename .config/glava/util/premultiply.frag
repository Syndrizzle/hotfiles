
#if PREMULTIPLY_ALPHA == 0
#error __disablestage
#endif

#request uniform "prev" tex
uniform sampler2D tex;

out vec4 fragment;
in vec4 gl_FragCoord;

void main() {
    fragment = texelFetch(tex, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
    #if PREMULTIPLY_ALPHA > 0
    fragment.rgb *= fragment.a;
    #endif
}
