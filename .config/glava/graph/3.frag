
in vec4 gl_FragCoord;

#request uniform "screen" screen
uniform ivec2 screen; /* screen dimensions */

#request uniform "prev" tex
uniform sampler2D tex;    /* screen texture    */

out vec4 fragment; /* output */

#include "@graph.glsl"
#include ":graph.glsl"

#if ANTI_ALIAS == 0
#error __disablestage
#endif

/* Moves toward the border of the graph, gives the
   y coordinate of the last colored pixel */
float get_col_height_up(float x, float oy) {
    float y = oy;
    #if INVERT > 0
    while (y >= 0) {
    #else
    while (y < screen.y) {
    #endif
        vec4 f = texelFetch(tex, ivec2(x, y), 0);
        if (f.a <= 0) {
            #if INVERT > 0
            y += 1;
            #else
            y -= 1;
            #endif
            break;
        }
        #if INVERT > 0
        y -= 1;
        #else
        y += 1;
        #endif
    }

    return y;
}

/* Moves toward the base of the graph, gives the
   y coordinate of the first colored pixel */
float get_col_height_down(float x, float oy) {
    float y = oy;
    #if INVERT > 0
    while (y < screen.y) {
    #else
    while (y >= 0) {
    #endif
        vec4 f = texelFetch(tex, ivec2(x, y), 0);
        if (f.a > 0) {
            break;
        }
        #if INVERT > 0
        y += 1;
        #else
        y -= 1;
        #endif
    }

    return y;
}

void main() {
    fragment = texelFetch(tex, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);

    #if ANTI_ALIAS > 0

    if (fragment.a <= 0) {
        bool left_done = false;
        float h2;
        float a_fact = 0;

        if (texelFetch(tex, ivec2(gl_FragCoord.x - 1, gl_FragCoord.y), 0).a > 0) {
            float h1 = get_col_height_up(gl_FragCoord.x - 1, gl_FragCoord.y);
            h2 = get_col_height_down(gl_FragCoord.x, gl_FragCoord.y);
            fragment = texelFetch(tex, ivec2(gl_FragCoord.x, h2), 0);

            a_fact = clamp(abs((h1 - gl_FragCoord.y) / (h2 - h1)), 0.0, 1.0);

            left_done = true;
        }
        if (texelFetch(tex, ivec2(gl_FragCoord.x + 1, gl_FragCoord.y), 0).a > 0) {
            if (!left_done) {
                h2 = get_col_height_down(gl_FragCoord.x, gl_FragCoord.y);
                fragment = texelFetch(tex, ivec2(gl_FragCoord.x, h2), 0);
            }
            float h3 = get_col_height_up(gl_FragCoord.x + 1, gl_FragCoord.y);

            a_fact = max(a_fact, clamp(abs((h3 - gl_FragCoord.y) / (h2 - h3)), 0.0, 1.0));
        }
        
        fragment.a *= a_fact;

    }

    #endif
}
