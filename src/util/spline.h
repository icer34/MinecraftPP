/**
 * @file spline.h
 * @brief Piecewise-linear curve used to remap noise values in terrain generation.
 */

#pragma once

#include <glm/glm.hpp>
#include <vector>

/**
 * @brief Piecewise-linear function defined by control points sorted by x.
 *
 * The curve always has at least two points, and no two points can share the same x.
 * Outside the range of its control points, the curve is clamped to the first/last y value.
 * The bounds only describe the editable range (e.g. for the settings UI): they are not
 * enforced on the control points.
 */
class Spline
{
public:
    /**
     * @brief Creates a flat spline with one point at each end of the x range, both at the
     * middle of the y range.
     *
     * @param xMin lower bound of the x range
     * @param xMax upper bound of the x range
     * @param yMin lower bound of the y range
     * @param yMax upper bound of the y range
     */
    Spline(float xMin, float xMax, float yMin, float yMax);

    /**
     * @brief Inserts a control point, keeping the points sorted by x.
     *
     * Ignored (with an error message) if a point already exists at this x.
     */
    void addPoint(float x, float y);

    /**
     * @brief Removes the control point at `index`.
     *
     * Ignored if the spline only has two points left.
     */
    void removePoint(size_t index);

    /**
     * @brief Moves the control point at `index`.
     *
     * If another point already uses `x`, only the y value is updated.
     */
    void setPoint(size_t index, float x, float y);

    /**
     * @brief Evaluates the curve at `x` using linear interpolation.
     *
     * @return the interpolated y value, clamped to the first/last point outside their range
     */
    float get(float x) const;

    /** @brief X values of the control points, in increasing order. */
    const std::vector<float> &getXValues() const { return _xVal; }
    /** @brief Y values of the control points, in the same order as getXValues(). */
    const std::vector<float> &getYValues() const { return _yVal; }
    /** @brief Editable x range, as (min, max). */
    glm::vec2 getXBounds() const { return _xBounds; }
    /** @brief Editable y range, as (min, max). */
    glm::vec2 getYBounds() const { return _yBounds; }

private:
    glm::vec2 _xBounds;
    glm::vec2 _yBounds;
    std::vector<float> _xVal{};
    std::vector<float> _yVal{};
};