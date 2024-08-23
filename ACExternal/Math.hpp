#pragma once
#include "offset.h"
#include <array>
#include <corecrt_math.h>

struct Vector2
{
    float x{ 0 };
    float y{ 0 };

    friend Vector2 operator- (const Vector2& a, const Vector2& b) {
        return Vector2{ a.x - b.x, a.y - b.y};
    }
    
    friend Vector2 operator+ (const Vector2& a, const Vector2& b) {
        return Vector2{ a.x + b.x, a.y + b.y};
    }

    friend Vector2 operator/(const Vector2& a, const float scalar){
        return { a.x / scalar, a.y / scalar };
    }
};

struct Vector3
{
    float x{ 0 };
    float y{ 0 };
    float z{ 0 };

    friend Vector3 operator- (const Vector3& a, const Vector3& b) {
        return Vector3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }
};

struct Vector4
{
    float x{ 0 };
    float y{ 0 };
    float z{ 0 };
    float w{ 0 };
};

struct glMatrix
{
    std::array<float, 16> matrix{0};
};

class Math {
private:
    RECT client_rect{};
    int windowWidth = 0;
    int windowHeight = 0;
public:
    Math() = default;

    void init(){
        wil::unique_hwnd hwnd{ FindWindow("SDL_app", "AssaultCube") };
        GetClientRect(hwnd.get(), &client_rect);
        windowWidth = client_rect.right;
        windowHeight = client_rect.bottom;
    }

    bool WorldToScreen(Vector3 pos, Vector2& screenPos, glMatrix matrix)
    {
        Vector4 clipCoords{ 0 };
        clipCoords.x = pos.x * matrix.matrix[0] + pos.y * matrix.matrix[4] + pos.z * matrix.matrix[8] + matrix.matrix[12];
        clipCoords.y = pos.x * matrix.matrix[1] + pos.y * matrix.matrix[5] + pos.z * matrix.matrix[9] + matrix.matrix[13];
        clipCoords.z = pos.x * matrix.matrix[2] + pos.y * matrix.matrix[6] + pos.z * matrix.matrix[10] + matrix.matrix[14];
        clipCoords.w = pos.x * matrix.matrix[3] + pos.y * matrix.matrix[7] + pos.z * matrix.matrix[11] + matrix.matrix[15];

        if (clipCoords.w < 0.1f)
            return false;

        //Perspective division, dividing by clip.W = Normalized Device Coordinates
        Vector3 NDC{ 0 };
        NDC.x = clipCoords.x / clipCoords.w;
        NDC.y = clipCoords.y / clipCoords.w;
        NDC.z = clipCoords.z / clipCoords.w;

        //Transform to window coordinates
        screenPos.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
        screenPos.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
        return true;
    }


    double GetDistance(Vector2 enemy) {
        return std::sqrt(std::pow((enemy.x - windowWidth / 2), 2) + std::pow((enemy.y - windowHeight / 2), 2));
    }
};
