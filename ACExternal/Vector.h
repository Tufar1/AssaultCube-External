#pragma once
#include "offset.h"
#include <array>
#include <corecrt_math.h>

RECT rctAC;
HWND hwndAC = FindWindow("SDL_app", "AssaultCube");

struct Vector2
{
    float x;
    float y;
};

struct Vector3
{
    float x;
    float y;
    float z;

    friend Vector3 operator- (const Vector3& a, const Vector3& b) {
        return Vector3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }

    float Length() const {
        return ((x * x) + (y * y) + (z * z));
    }
};

struct Vector4
{
    float x;
    float y;
    float z;
    float w;
};

struct glMatrix
{
    std::array<float, 16> matrix;
};

bool WorldToScreen(Vector3 pos, Vector2& screenPos)
{
    GetClientRect(hwndAC, &rctAC);
    int windowWidth = rctAC.right;
    int windowHeight = rctAC.bottom;

    glMatrix matrix = RPM<glMatrix>((long)GetModuleBaseAddress("ac_client.exe") + offset::viewMatrix);

    //Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
    Vector4 clipCoords;
    clipCoords.x = pos.x * matrix.matrix[0] + pos.y * matrix.matrix[4] + pos.z * matrix.matrix[8] + matrix.matrix[12];
    clipCoords.y = pos.x * matrix.matrix[1] + pos.y * matrix.matrix[5] + pos.z * matrix.matrix[9] + matrix.matrix[13];
    clipCoords.z = pos.x * matrix.matrix[2] + pos.y * matrix.matrix[6] + pos.z * matrix.matrix[10] + matrix.matrix[14];
    clipCoords.w = pos.x * matrix.matrix[3] + pos.y * matrix.matrix[7] + pos.z * matrix.matrix[11] + matrix.matrix[15];

    if (clipCoords.w < 0.1f)
        return false;

    //Perspective division, dividing by clip.W = Normalized Device Coordinates
    Vector3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    //Transform to window coordinates
    screenPos.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screenPos.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return true;
}

double GetDistance(Vector2 enemy) {
    GetClientRect(hwndAC, &rctAC);
    int windowWidth = rctAC.right;
    int windowHeight = rctAC.bottom;

    return sqrt(pow((enemy.x - windowWidth / 2), 2) + pow((enemy.y - windowHeight / 2), 2));
}