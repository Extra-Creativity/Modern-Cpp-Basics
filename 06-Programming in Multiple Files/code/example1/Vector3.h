#pragma once
class Vector3
{
public:
    Vector3(float x, float y, float z);
    float GetLength() const;
    static auto GetDim() { return dim_; }

private:
    float x_, y_, z_;
    static int dim_;
};
