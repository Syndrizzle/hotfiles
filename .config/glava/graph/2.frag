
in vec4 gl_FragCoord;

#request uniform "prev" tex
uniform sampler2D tex;    /* screen texture    */

out vec4 fragment; /* output */

#include "@graph.glsl"
#include ":graph.glsl"

#if DRAW_OUTLINE == 0 && DRAW_HIGHLIGHT == 0
#error __disablestage
#endif

void main() {
    fragment = texelFetch(tex, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
    
    vec4
        a0 = texelFetch(tex, ivec2((gl_FragCoord.x + 1), (gl_FragCoord.y + 0)), 0),
        a1 = texelFetch(tex, ivec2((gl_FragCoord.x + 1), (gl_FragCoord.y + 1)), 0),
        a2 = texelFetch(tex, ivec2((gl_FragCoord.x + 0), (gl_FragCoord.y + 1)), 0),
        a3 = texelFetch(tex, ivec2((gl_FragCoord.x + 1), (gl_FragCoord.y + 0)), 0),
    
        a4 = texelFetch(tex, ivec2((gl_FragCoord.x - 1), (gl_FragCoord.y - 0)), 0),
        a5 = texelFetch(tex, ivec2((gl_FragCoord.x - 1), (gl_FragCoord.y - 1)), 0),
        a6 = texelFetch(tex, ivec2((gl_FragCoord.x - 0), (gl_FragCoord.y - 1)), 0),
        a7 = texelFetch(tex, ivec2((gl_FragCoord.x - 1), (gl_FragCoord.y - 0)), 0);

    vec4 avg = (a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7) / 8.0;
    if (avg.a > 0){
        if (fragment.a <= 0) {
            /* outline */
            #if DRAW_OUTLINE > 0
            fragment = OUTLINE;
            #endif
        } else if (avg.a < 1) {
            /* creates a highlight along the edge of the spectrum */
            #if DRAW_HIGHLIGHT > 0
            fragment.rgb *= avg.a * 2;
            #endif
        }
    }
}
