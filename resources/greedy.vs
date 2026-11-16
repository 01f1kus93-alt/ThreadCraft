#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
in vec3 vertexNormal; // <--- ДОБАВЛЕНО! Без этого шейдер не компилируется

out vec2 fragTexCoord;
out vec4 fragColor;
out vec4 fragTileBounds;
out vec2 fragRepeat;
out vec3 fragWorldPos;
flat out vec3 fragNormal;

uniform mat4 mvp;
uniform vec4 tileUVs[256]; // Массив UV границ для каждого тайла

void main() {
    fragTexCoord = vertexTexCoord;
    
    // Распаковываем данные из цветов (Raylib нормализует их в [0,1])
    int tileIdx = int(vertexColor.r * 255.0 + 0.5); // Добавлено +0.5 для точности
    float w = float(int(vertexColor.g * 255.0 + 0.5));
    float h = float(int(vertexColor.b * 255.0 + 0.5));
    float brightness = vertexColor.a;
    
    fragTileBounds = tileUVs[tileIdx];
    fragRepeat = vec2(w, h);
    fragColor = vec4(vec3(brightness), 1.0);
    
    fragWorldPos = vertexPosition;
    fragNormal = vertexNormal;
    
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}