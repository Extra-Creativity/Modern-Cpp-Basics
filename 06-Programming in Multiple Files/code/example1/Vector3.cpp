#include "Vector3.h"
#include <cmath>

int Vector3::dim_ = 3;

Vector3::Vector3(float x, float y, float z) : x_{ x }, y_{ y }, z_{ z } {}

float Vector3::GetLength() const
{
    return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_);
}
