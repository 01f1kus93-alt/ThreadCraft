#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec4 fragTileBounds;
in vec2 fragRepeat;
in vec3 fragWorldPos;
flat in vec3 fragNormal;
flat in int fragTileIndex;
in float fragLeafDepth;

uniform sampler2D texture0;
uniform vec3 camPos;
uniform float fogStart;
uniform float fogEnd;
uniform float leavesLodDistance;
uniform int visibleLeafLayers;
uniform vec3 fogColor;
out vec4 finalColor;

// Атласные индексы листвы (см. BlockRegistry.cpp / TextureAtlas.cpp).
const int  TILE_OAK_LEAVES   = 10;
const int  TILE_BIRCH_LEAVES = 15;

// Дальняя листва больше не использует кислотно-зелёный (0x77AB2F), а берёт
// приглушённый тёмно-зелёный, близкий к среднему цвету кроны дуба/берёзы.
const vec3 OAK_LEAF_LOD_COLOR   = vec3(0.26, 0.38, 0.12);
const vec3 BIRCH_LEAF_LOD_COLOR = vec3(0.36, 0.46, 0.18);

// Внутренний «фон» кроны: непрозрачный и чуть темнее тёмно-зелёной текстуры.
const vec3 OAK_LEAF_INNER   = vec3(0.14, 0.22, 0.05);
const vec3 BIRCH_LEAF_INNER = vec3(0.22, 0.30, 0.09);
const float LEAF_INNER_DARKEN = 0.72;

void main() {
    vec2 tileSize = fragTileBounds.zw - fragTileBounds.xy;
    
    // Защита от деления на ноль (если tileUVs не переданы)
    if (tileSize.x <= 0.0 || tileSize.y <= 0.0) {
        discard;
    }
    
    float dist = length(fragWorldPos - camPos);
    bool isLeaf = (fragTileIndex == TILE_OAK_LEAVES || fragTileIndex == TILE_BIRCH_LEAVES);

    // Далёкая листва (> leavesLodDistance) рисуется плоским непрозрачным зелёным
    // вместо текстуры с дырками: без texture() и discard дальние чанки не
    // тормозят из-за прозрачности листвы.
    if (isLeaf && dist > leavesLodDistance) {
        vec3 lodColor = (fragTileIndex == TILE_BIRCH_LEAVES) ? BIRCH_LEAF_LOD_COLOR : OAK_LEAF_LOD_COLOR;
        // Внутренние слои на расстоянии тоже затемняем — слайдер работает везде.
        if (fragLeafDepth >= float(visibleLeafLayers)) {
            lodColor *= LEAF_INNER_DARKEN;
        }
        finalColor = vec4(lodColor * fragColor.r, 1.0);
    } else {
        vec2 localUV = (fragTexCoord - fragTileBounds.xy) / tileSize;
        localUV = fract(localUV * fragRepeat);
        vec2 finalUV = fragTileBounds.xy + localUV * tileSize;
        
        vec4 texColor = texture(texture0, finalUV);

        if (isLeaf) {
            // Внешние слои (до visibleLeafLayers) остаются «дырявыми»,
            // а внутренние рисуются непрозрачным тёмным фоном кроны.
            if (fragLeafDepth >= float(visibleLeafLayers)) {
                vec3 base = (fragTileIndex == TILE_BIRCH_LEAVES) ? BIRCH_LEAF_INNER : OAK_LEAF_INNER;
                vec3 rgb = (texColor.a < 0.1)
                    ? (base * fragColor.r)
                    : (texColor.rgb * fragColor.r * LEAF_INNER_DARKEN);
                finalColor = vec4(rgb, 1.0);
            } else {
                if (texColor.a < 0.1) discard;
                finalColor = vec4(texColor.rgb * fragColor.r, texColor.a);
            }
        } else {
            // Обычные прозрачные блоки (вода, стекло) и непрозрачные блоки.
            if (texColor.a < 0.1) discard;
            finalColor = vec4(texColor.rgb * fragColor.r, texColor.a);
        }
    }

    // Туман, зависящий от дальности прорисовки (линейный, затухает к краю мира).
    float fog = clamp((dist - fogStart) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);
    finalColor.rgb = mix(finalColor.rgb, fogColor, fog);
}