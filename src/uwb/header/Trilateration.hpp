#pragma once

#include "UwbDevice.hpp"

#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace uwbp::uwb
{

struct AnchorDistance
{
    Vec3 position;
    double distance;
};

// linearized least-squares trilateration
// needs at least 3 anchors. returns position + residual (RMS error).
// if anchors are coplanar (same z) we solve 2D and keep z = 0
inline std::optional<std::pair<Vec3, double>>
trilaterate(const std::vector<AnchorDistance>& anchors)
{
    const auto n = anchors.size();
    if (n < 3) return std::nullopt;

    // we subtract the first sphere equation from all others
    // to get linear equations:
    //   2*(x1-x0)*x + 2*(y1-y0)*y + 2*(z1-z0)*z = (d0^2 - d1^2) - (x0^2-x1^2) - (y0^2-y1^2) - (z0^2-z1^2)
    // simplifies to: A * p = b

    const auto& p0 = anchors[0].position;
    double d0sq = anchors[0].distance * anchors[0].distance;

    // check if all anchors are at roughly the same height (coplanar)
    bool coplanar = true;
    for (std::size_t i = 1; i < n; ++i)
    {
        if (std::abs(anchors[i].position.z - p0.z) > 0.01)
        {
            coplanar = false;
            break;
        }
    }

    // number of rows and columns
    int rows = static_cast<int>(n - 1);
    int cols = coplanar ? 2 : 3;

    // build A (rows x cols) and b (rows x 1)
    // stored row-major
    std::vector<double> A(rows * cols, 0.0);
    std::vector<double> b(rows, 0.0);

    for (int i = 0; i < rows; ++i)
    {
        const auto& pi = anchors[i + 1].position;
        double disq = anchors[i + 1].distance * anchors[i + 1].distance;

        A[i * cols + 0] = 2.0 * (pi.x - p0.x);
        A[i * cols + 1] = 2.0 * (pi.y - p0.y);
        if (!coplanar)
            A[i * cols + 2] = 2.0 * (pi.z - p0.z);

        b[i] = (d0sq - disq)
             + (pi.x * pi.x - p0.x * p0.x)
             + (pi.y * pi.y - p0.y * p0.y)
             + (pi.z * pi.z - p0.z * p0.z);
    }

    // solve via normal equations: (A^T A) * p = A^T b
    // AtA is cols x cols, Atb is cols x 1
    std::vector<double> AtA(cols * cols, 0.0);
    std::vector<double> Atb(cols, 0.0);

    for (int i = 0; i < cols; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < rows; ++k)
                sum += A[k * cols + i] * A[k * cols + j];
            AtA[i * cols + j] = sum;
        }
        double sum = 0.0;
        for (int k = 0; k < rows; ++k)
            sum += A[k * cols + i] * b[k];
        Atb[i] = sum;
    }

    // solve the small system with gaussian elimination
    // augmented matrix [AtA | Atb]
    std::vector<double> aug(cols * (cols + 1));
    for (int i = 0; i < cols; ++i)
    {
        for (int j = 0; j < cols; ++j)
            aug[i * (cols + 1) + j] = AtA[i * cols + j];
        aug[i * (cols + 1) + cols] = Atb[i];
    }

    // forward elimination with partial pivoting
    for (int col = 0; col < cols; ++col)
    {
        // find pivot
        int maxRow = col;
        double maxVal = std::abs(aug[col * (cols + 1) + col]);
        for (int row = col + 1; row < cols; ++row)
        {
            double v = std::abs(aug[row * (cols + 1) + col]);
            if (v > maxVal) { maxVal = v; maxRow = row; }
        }

        if (maxVal < 1e-12) return std::nullopt; // singular, cant solve

        // swap rows
        if (maxRow != col)
        {
            for (int j = 0; j <= cols; ++j)
                std::swap(aug[col * (cols + 1) + j], aug[maxRow * (cols + 1) + j]);
        }

        // eliminate below
        for (int row = col + 1; row < cols; ++row)
        {
            double factor = aug[row * (cols + 1) + col] / aug[col * (cols + 1) + col];
            for (int j = col; j <= cols; ++j)
                aug[row * (cols + 1) + j] -= factor * aug[col * (cols + 1) + j];
        }
    }

    // back substitution
    std::vector<double> result(cols, 0.0);
    for (int i = cols - 1; i >= 0; --i)
    {
        double sum = aug[i * (cols + 1) + cols];
        for (int j = i + 1; j < cols; ++j)
            sum -= aug[i * (cols + 1) + j] * result[j];
        result[i] = sum / aug[i * (cols + 1) + i];
    }

    Vec3 pos;
    pos.x = result[0];
    pos.y = result[1];
    pos.z = (cols == 3) ? result[2] : 0.0;

    // compute residual (RMS of distance errors)
    double sumSqErr = 0.0;
    for (const auto& a : anchors)
    {
        double dx = pos.x - a.position.x;
        double dy = pos.y - a.position.y;
        double dz = pos.z - a.position.z;
        double computed = std::sqrt(dx * dx + dy * dy + dz * dz);
        double err = computed - a.distance;
        sumSqErr += err * err;
    }
    double residual = std::sqrt(sumSqErr / static_cast<double>(n));

    return std::pair{pos, residual};
}

} // namespace uwbp::uwb
