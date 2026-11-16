#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec4 fragTileBounds;
in vec2 fragRepeat;
in vec3 fragWorldPos;
flat in vec3 fragNormal;

uniform sampler2D texture0;
out vec4 finalColor;

void main() {
    vec2 tileSize = fragTileBounds.zw - fragTileBounds.xy;
    
    // Защита от деления на ноль (если tileUVs не переданы)
    if (tileSize.x <= 0.0 || tileSize.y <= 0.0) {
        discard;
    }
    
    vec2 localUV = (fragTexCoord - fragTileBounds.xy) / tileSize;
    localUV = fract(localUV * fragRepeat);
    vec2 finalUV = fragTileBounds.xy + localUV * tileSize;
    
    vec4 texColor = texture(texture0, finalUV);
    
    // Отбрасываем прозрачные пиксели (дырки в листве)
    if (texColor.a < 0.1) {
        discard;
    }

    // fragColor.r содержит яркость (0.0 - 1.0). Применяем к RGB.
    finalColor = vec4(texColor.rgb * fragColor.r, texColor.a);
}