#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec4 fragTileBounds; // (u0, v0, u1, v1)
in vec2 fragRepeat;

uniform sampler2D texture0;
out vec4 finalColor;

void main() {
    // Вычисляем локальные UV внутри тайла (от 0 до 1) — в пределах одного тайла
    vec2 tileSize = fragTileBounds.zw - fragTileBounds.xy;
    vec2 localUV = (fragTexCoord - fragTileBounds.xy) / tileSize;
    
    // Зацикливаем UV (fract) умножая на размер слияния (w × h)
    localUV = fract(localUV * fragRepeat);
    
    // Возвращаем UV обратно в координаты атласа
    vec2 finalUV = fragTileBounds.xy + localUV * tileSize;
    
    vec4 texColor = texture(texture0, finalUV);
    finalColor = texColor * fragColor;
}