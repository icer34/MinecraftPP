#include "spline.h"

#include <algorithm>
#include <iostream>

Spline::Spline(float xMin, float xMax, float yMin, float yMax)
{
    m_xBounds = glm::vec2(xMin, xMax);
    m_yBounds = glm::vec2(yMin, yMax);
    addPoint(xMin, (yMin + yMax) / 2.0f);
    addPoint(xMax, (yMin + yMax) / 2.0f);
}

void Spline::addPoint(float x, float y)
{
    // you cannot add 2 points that overlap on the x axis
    if (std::find(m_xVal.begin(), m_xVal.end(), x) != m_xVal.end())
    {
        std::cout << "SPLINE_ERROR::Cannot add 2 points that overlap on the x axis" << std::endl;
        return;
    }

    m_xVal.push_back(x);

    // sort the vector to maintain order
    std::sort(m_xVal.begin(), m_xVal.end());

    // get the index of x in m_xVal to add y in the correct place
    int index = std::find(m_xVal.begin(), m_xVal.end(), x) - m_xVal.begin();

    m_yVal.insert(m_yVal.begin() + index, y);
}

void Spline::removePoint(size_t index)
{
    m_xVal.erase(m_xVal.begin() + index);
    m_yVal.erase(m_yVal.begin() + index);
}

void Spline::setPoint(size_t index, float x, float y)
{
    // refuse to move a point onto another point's x -- would break the interpolation
    // (division by zero in get()) and make the two points unrecoverably ambiguous
    for (size_t i = 0; i < m_xVal.size(); i++)
    {
        if (i != index && m_xVal[i] == x)
        {
            m_yVal[index] = y; // still allow the height to move
            return;
        }
    }

    m_xVal[index] = x;
    m_yVal[index] = y;

    // re-sort by x, keeping x/y pairs together (dragging a point past a neighbor
    // changes its relative order)
    std::vector<size_t> order(m_xVal.size());
    for (size_t i = 0; i < order.size(); i++)
        order[i] = i;

    std::sort(
        order.begin(), order.end(), [this](size_t a, size_t b) { return m_xVal[a] < m_xVal[b]; });

    std::vector<float> sortedX(m_xVal.size()), sortedY(m_yVal.size());
    for (size_t i = 0; i < order.size(); i++)
    {
        sortedX[i] = m_xVal[order[i]];
        sortedY[i] = m_yVal[order[i]];
    }

    m_xVal = std::move(sortedX);
    m_yVal = std::move(sortedY);
}

float Spline::get(float x) const
{
    if (x <= m_xVal.front())
        return m_yVal.front();
    if (x >= m_xVal.back())
        return m_yVal.back();

    // find the index of the first element that is not less than x
    int index = std::lower_bound(m_xVal.begin(), m_xVal.end(), x) - m_xVal.begin();

    float x0 = m_xVal.at(index - 1);
    float x1 = m_xVal.at(index);

    float y0 = m_yVal.at(index - 1);
    float y1 = m_yVal.at(index);

    float t = abs(x - x0) / abs(x1 - x0);

    return y0 + (y1 - y0) * t;
}