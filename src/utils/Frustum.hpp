#pragma once
#include <raylib.h>
#include <raymath.h>

namespace mc {
struct Frustum {
    Vector4 planes[6];

    void update(const Camera3D& cam, float aspect) {
        Matrix matView = GetCameraMatrix(cam);
        Matrix matProj = MatrixPerspective(cam.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
        Matrix m = MatrixMultiply(matView, matProj);

        // Left, Right, Bottom, Top, Near, Far
        planes[0] = { m.m3 + m.m0, m.m7 + m.m4, m.m11 + m.m8, m.m15 + m.m12 };
        planes[1] = { m.m3 - m.m0, m.m7 - m.m4, m.m11 - m.m8, m.m15 - m.m12 };
        planes[2] = { m.m3 + m.m1, m.m7 + m.m5, m.m11 + m.m9, m.m15 + m.m13 };
        planes[3] = { m.m3 - m.m1, m.m7 - m.m5, m.m11 - m.m9, m.m15 - m.m13 };
        planes[4] = { m.m3 + m.m2, m.m7 + m.m6, m.m11 + m.m10, m.m15 + m.m14 };
        planes[5] = { m.m3 - m.m2, m.m7 - m.m6, m.m11 - m.m10, m.m15 - m.m14 };

        for (int i = 0; i < 6; i++) {
            float len = sqrtf(planes[i].x*planes[i].x + planes[i].y*planes[i].y + planes[i].z*planes[i].z);
            planes[i].x /= len; planes[i].y /= len; planes[i].z /= len; planes[i].w /= len;
        }
    }

    bool isBoxVisible(Vector3 min, Vector3 max) const {
        for (int i = 0; i < 6; i++) {
            if (i == 4) continue;  // skip near plane: camera inside the AABB must stay visible
            Vector3 p = {
                planes[i].x > 0 ? max.x : min.x,
                planes[i].y > 0 ? max.y : min.y,
                planes[i].z > 0 ? max.z : min.z
            };
            if (planes[i].x * p.x + planes[i].y * p.y + planes[i].z * p.z + planes[i].w < 0) return false;
        }
        return true;
    }
};
} // namespace mc