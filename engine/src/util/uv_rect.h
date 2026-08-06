#pragma once

struct UVRect
{
    float x0, x1, y0, y1;

    UVRect(float x0, float x1, float y0, float y1)
    {
        this->x0 = x0;
        this->x1 = x1;
        this->y0 = y0;
        this->y1 = y1;
    }

    bool operator==(const UVRect &other)
    {
        if (x0 == other.x0 && x1 == other.x1 && y0 == other.y0 && y1 == other.y1)
            return true;
        else
            return false;
    }
};