#include "spline.h"

#include <algorithm>
#include <iostream>

using glm::vec2;

Spline::Spline(float xMin, float xMax, float yMin, float yMax)
{
    _xBounds = vec2(xMin, xMax);
    _yBounds = vec2(yMin, yMax);
    addPoint(xMin, (yMin + yMax) / 2.0f);
    addPoint(xMax, (yMin + yMax) / 2.0f);
}

void Spline::addPoint(float x, float y)
{
    // you cannot add 2 points that overlap on the x axis
    if (std::find(_xVal.begin(), _xVal.end(), x) != _xVal.end())
    {
        std::cout << "SPLINE_ERROR::Cannot add 2 points that overlap on the x axis" << std::endl;
        return;
    }

    _xVal.push_back(x);

    // sort the vector to maintain order
    std::sort(_xVal.begin(), _xVal.end());

    // get the index of x in _xVal to add y in the correct place
    int index = std::find(_xVal.begin(), _xVal.end(), x) - _xVal.begin();

    _yVal.insert(_yVal.begin() + index, y);
}

void Spline::removePoint(size_t index)
{
    if (_xVal.size() <= 2)
    {
        std::cout << "Remove not allowed: not enough points" << std::endl;
        return;
    }

    _xVal.erase(_xVal.begin() + index);
    _yVal.erase(_yVal.begin() + index);
}

void Spline::setPoint(size_t index, float x, float y)
{
    // refuse to move a point onto another point's x -- would break the interpolation
    // (division by zero in get()) and make the two points unrecoverably ambiguous
    for (size_t i = 0; i < _xVal.size(); i++)
    {
        if (i != index && _xVal[i] == x)
        {
            _yVal[index] = y; // still allow the height to move
            return;
        }
    }

    _xVal[index] = x;
    _yVal[index] = y;

    // re-sort by x, keeping x/y pairs together (dragging a point past a neighbor
    // changes its relative order)
    std::vector<size_t> order(_xVal.size());
    for (size_t i = 0; i < order.size(); i++)
        order[i] = i;

    std::sort(
        order.begin(), order.end(), [this](size_t a, size_t b) { return _xVal[a] < _xVal[b]; });

    std::vector<float> sortedX(_xVal.size()), sortedY(_yVal.size());
    for (size_t i = 0; i < order.size(); i++)
    {
        sortedX[i] = _xVal[order[i]];
        sortedY[i] = _yVal[order[i]];
    }

    _xVal = std::move(sortedX);
    _yVal = std::move(sortedY);
}

float Spline::get(float x) const
{
    if (x <= _xVal.front())
        return _yVal.front();
    if (x >= _xVal.back())
        return _yVal.back();

    // find the index of the first element that is not less than x
    int index = std::lower_bound(_xVal.begin(), _xVal.end(), x) - _xVal.begin();

    float x0 = _xVal.at(index - 1);
    float x1 = _xVal.at(index);

    float y0 = _yVal.at(index - 1);
    float y1 = _yVal.at(index);

    float t = std::abs(x - x0) / std::abs(x1 - x0);

    return y0 + (y1 - y0) * t;
}