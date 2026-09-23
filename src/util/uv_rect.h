/**
 * @file uv_rect.h
 * @brief Rectangle of texture coordinates inside an atlas.
 */

#pragma once

/**
 * @brief Axis-aligned rectangle in UV space, typically one sprite inside a texture atlas.
 */
struct UVRect
{
    float x0; ///< Left U coordinate.
    float x1; ///< Right U coordinate.
    float y0; ///< First V coordinate.
    float y1; ///< Second V coordinate.

    /**
     * @brief Builds a UV rectangle from its bounds.
     */
    UVRect(float x0, float x1, float y0, float y1)
    {
        this->x0 = x0;
        this->x1 = x1;
        this->y0 = y0;
        this->y1 = y1;
    }

    /**
     * @brief Exact (non-epsilon) comparison of the four bounds.
     */
    bool operator==(const UVRect &other)
    {
        if (x0 == other.x0 && x1 == other.x1 && y0 == other.y0 && y1 == other.y1)
            return true;
        else
            return false;
    }
};