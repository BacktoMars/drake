#include "PiecewisePolynomial.h"
#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <complex>

using namespace std;
using namespace Eigen;

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType>::PiecewisePolynomial(std::vector<PolynomialMatrix> const& polynomials, std::vector<double> const& segment_times) :
    PiecewisePolynomialBase(segment_times),
    polynomials(polynomials)
{
  assert(segment_times.size() == (polynomials.size() + 1));
  for (int i = 1; i < getNumberOfSegments(); i++) {
    if (polynomials[i].rows() != polynomials[0].rows())
      throw std::runtime_error("The polynomial matrix for each segment must have the same number of rows.");
    if (polynomials[i].cols() != polynomials[0].cols())
      throw std::runtime_error("The polynomial matrix for each segment must have the same number of columns.");
  }
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType>::PiecewisePolynomial(std::vector<PolynomialType> const& polynomials, std::vector<double> const& segment_times) :
    PiecewisePolynomialBase(segment_times)
{
  assert(segment_times.size() == (polynomials.size() + 1));

  for (int i = 0; i < polynomials.size(); i++) {
    PolynomialMatrix matrix(1, 1);
    matrix(0, 0) = polynomials[i];
    this->polynomials.push_back(matrix);
  }
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType>::PiecewisePolynomial() {
  // empty
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType> PiecewisePolynomial<CoefficientType>::derivative(int derivative_order) const {
  PiecewisePolynomial ret = *this;
  for (auto it = ret.polynomials.begin(); it != ret.polynomials.end(); ++it) {
    PolynomialMatrix& matrix = *it;
    for (DenseIndex row = 0; row < rows(); row++) {
      for (DenseIndex col = 0; col < cols(); col++) {
        matrix(row, col) = matrix(row, col).derivative(derivative_order);
      }
    }
  }
  return ret;
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType> PiecewisePolynomial<CoefficientType>::integral(double value_at_start_time) const {
  CoefficientMatrix matrix_value_at_start_time = CoefficientMatrix::Constant(rows(), cols(), value_at_start_time);
  return integral(matrix_value_at_start_time);
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType> PiecewisePolynomial<CoefficientType>::integral(const Eigen::Ref<PiecewisePolynomial<CoefficientType>::CoefficientMatrix> & value_at_start_time) const {
  PiecewisePolynomial ret = *this;
  for (int segment_index = 0; segment_index < getNumberOfSegments(); segment_index++) {
    PolynomialMatrix& matrix = ret.polynomials[segment_index];
    for (DenseIndex row = 0; row < rows(); row++) {
      for (DenseIndex col = 0; col < cols(); col++) {
        if (segment_index == 0) {
          matrix(row, col) = matrix(row, col).integral(value_at_start_time(row, col));
        }
        else {
          matrix(row, col) = matrix(row, col).integral(ret.segmentValueAtGlobalAbscissa(segment_index - 1, getStartTime(segment_index), row, col));
        }
      }
    }
  }
  return ret;
}

template <typename CoefficientType>
double PiecewisePolynomial<CoefficientType>::value(double t, Eigen::DenseIndex row, Eigen::DenseIndex col) {
  int segment_index = getSegmentIndex(t);
  return segmentValueAtGlobalAbscissa(segment_index, t, row, col);
}

template <typename CoefficientType>
Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> PiecewisePolynomial<CoefficientType>::matrixValue(double t) {
  int segment_index = getSegmentIndex(t);
  Eigen::Matrix<double, PolynomialMatrix::RowsAtCompileTime, PolynomialMatrix::ColsAtCompileTime> ret(rows(), cols());
  for (DenseIndex row = 0; row < rows(); row++) {
    for (DenseIndex col = 0; col < cols(); col++) {
      ret(row, col) = segmentValueAtGlobalAbscissa(segment_index, t, row, col);
    }
  }
  return ret;
}

template <typename CoefficientType>
const Polynomial<CoefficientType>& PiecewisePolynomial<CoefficientType>::getPolynomial(int segment_index, Eigen::DenseIndex row, Eigen::DenseIndex col) const {
  segmentNumberRangeCheck(segment_index);
  return polynomials[segment_index](row, col);
}

template <typename CoefficientType>
int PiecewisePolynomial<CoefficientType>::getSegmentPolynomialDegree(int segment_index, Eigen::DenseIndex row, Eigen::DenseIndex col) const {
  segmentNumberRangeCheck(segment_index);
  return polynomials[segment_index](row, col).getDegree();
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType>& PiecewisePolynomial<CoefficientType>::operator+=(const PiecewisePolynomial<CoefficientType>& other) {
  if (!segmentTimesEqual(other, 1e-10))
    throw runtime_error("Addition not yet implemented when segment times are not equal");
  for (int i = 0; i < polynomials.size(); i++)
    polynomials[i] += other.polynomials[i];
  return *this;
}

template <typename CoefficientType>
PiecewisePolynomial<CoefficientType>& PiecewisePolynomial<CoefficientType>::operator*=(const PiecewisePolynomial<CoefficientType>& other) {
  if (!segmentTimesEqual(other, 1e-10))
    throw runtime_error("Multiplication not yet implemented when segment times are not equal");
  for (int i = 0; i < polynomials.size(); i++)
    polynomials[i] *= other.polynomials[i];
  return *this;
}

template <typename CoefficientType>
const PiecewisePolynomial<CoefficientType> PiecewisePolynomial<CoefficientType>::operator+(const PiecewisePolynomial<CoefficientType> &other) const {
  PiecewisePolynomial<CoefficientType> ret = *this;
  ret += other;
  return ret;
}

template <typename CoefficientType>
const PiecewisePolynomial<CoefficientType> PiecewisePolynomial<CoefficientType>::operator*(const PiecewisePolynomial<CoefficientType> &other) const {
  PiecewisePolynomial<CoefficientType> ret = *this;
  ret *= other;
  return ret;
}

template <typename CoefficientType>
bool PiecewisePolynomial<CoefficientType>::isApprox(const PiecewisePolynomial<CoefficientType>& other, double tol) const {
  if (rows() != other.rows() || cols() != other.cols())
    return false;

  if (!segmentTimesEqual(other, tol))
    return false;

  for (int segment_index = 0; segment_index < getNumberOfSegments(); segment_index++) {
    const PolynomialMatrix& matrix = polynomials[segment_index];
    const PolynomialMatrix& other_matrix = other.polynomials[segment_index];
    for (DenseIndex row = 0; row < rows(); row++) {
      for (DenseIndex col = 0; col < cols(); col++) {
      if (!matrix(row, col).isApprox(other_matrix(row, col), tol))
        return false;
      }
    }
  }
  return true;
}

template <typename CoefficientType>
double PiecewisePolynomial<CoefficientType>::segmentValueAtGlobalAbscissa(int segment_index, double t, Eigen::DenseIndex row, Eigen::DenseIndex col) const {
  return polynomials[segment_index](row, col).value(t - getStartTime(segment_index));
}

template <typename CoefficientType>
Eigen::DenseIndex PiecewisePolynomial<CoefficientType>::rows() const
{
  if (polynomials.size() > 0)
    return polynomials[0].rows();
  else
    throw std::runtime_error("PiecewisePolynomial has no segments. Number of rows is undefined.");
}

template <typename CoefficientType>
Eigen::DenseIndex PiecewisePolynomial<CoefficientType>::cols() const
{
    if (polynomials.size() > 0)
    return polynomials[0].cols();
  else
    throw std::runtime_error("PiecewisePolynomial has no segments. Number of columns is undefined.");
}

template class DLLEXPORT PiecewisePolynomial<double>;
//template class DLLEXPORT PiecewisePolynomial<std::complex<double>>; // doesn't work yet
