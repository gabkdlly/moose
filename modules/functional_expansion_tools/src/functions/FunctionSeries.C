//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include <numeric> // Provides accumulate()

#include "FunctionalBasisInterface.h" // Provides _domain_options
#include "FunctionSeries.h"
#include "Cartesian.h"
#include "CylindricalDuo.h"

template <>
InputParameters
validParams<FunctionSeries>()
{
  InputParameters params = validParams<MutableCoefficientsFunctionInterface>();

  params.addClassDescription("This function uses a convolution of functional series (functional "
                             "expansion or FX) to create a 1D, 2D, or 3D function");

  // The available composite series types.
  //   Cartesian:      1D, 2D, or 3D, depending on which of x, y, and z are present
  //   CylindricalDuo: planar disc expansion and axial expansion
  MooseEnum series_types("Cartesian CylindricalDuo");
  MooseEnum single_series_types_1D("Legendre");
  MooseEnum single_series_types_2D("Zernike");

  params.addRequiredParam<MooseEnum>(
      "series_type", series_types, "The type of function series to construct.");

  /*
   * This needs to use `unsigned int` instead of `std::size_t` because otherwise MOOSE errors at
   * runtime
   */
  params.addRequiredParam<std::vector<unsigned int>>("orders",
                                                     "The order of each series. These must be "
                                                     "defined as \"x y z\" for Cartesian, and \"z "
                                                     "disc\" for CylindricalDuo.");

  params.addParam<std::vector<Real>>("physical_bounds",
                                     "The physical bounds of the function series. These must be "
                                     "defined as \"x_min x_max y_min y_max z_min z_max\" for "
                                     "Cartesian, and \"axial_min axial_max disc_center1 "
                                     "disc_center2 radius\" for CylindricalDuo");

  params.addParam<MooseEnum>("x", single_series_types_1D, "The series to use for the x-direction.");
  params.addParam<MooseEnum>("y", single_series_types_1D, "The series to use for the y-direction.");
  params.addParam<MooseEnum>("z", single_series_types_1D, "The series to use for the z-direction.");

  params.addParam<MooseEnum>("disc",
                             single_series_types_2D,
                             "The series to use for the disc. Its direction is determined by "
                             "orthogonality to the declared direction of the axis.");
  return params;
}

FunctionSeries::FunctionSeries(const InputParameters & parameters)
  : MutableCoefficientsFunctionInterface(this, parameters),
    _orders(convertOrders(getParam<std::vector<unsigned int>>("orders"))),
    _physical_bounds(getParam<std::vector<Real>>("physical_bounds")),
    _series_type_name(getParam<MooseEnum>("series_type")),
    _x(getParam<MooseEnum>("x")),
    _y(getParam<MooseEnum>("y")),
    _z(getParam<MooseEnum>("z")),
    _disc(getParam<MooseEnum>("disc"))
{
  std::vector<MooseEnum> domains;
  std::vector<MooseEnum> types;

  if (_series_type_name == "Cartesian")
  {
    /*
     * For Cartesian series, at least one of 'x', 'y', and 'z' must be specified.
     *
     * The individual series are always stored in x, y, z order (independent of the order in which
     * they appear in the input file). Hence, the 'orders' and 'physical_bounds' vectors must always
     * be specified in x, y, z order.
     */
    if (isParamValid("x"))
    {
      domains.push_back(FunctionalBasisInterface::_domain_options = "x");
      types.push_back(_x);
    }
    if (isParamValid("y"))
    {
      domains.push_back(FunctionalBasisInterface::_domain_options = "y");
      types.push_back(_y);
    }
    if (isParamValid("z"))
    {
      domains.push_back(FunctionalBasisInterface::_domain_options = "z");
      types.push_back(_z);
    }
    if (types.size() == 0)
      mooseError("Must specify one of 'x', 'y', or 'z' for 'Cartesian' series!");
    _series_type = libmesh_make_unique<Cartesian>(domains, _orders, types, name());
  }
  else if (_series_type_name == "CylindricalDuo")
  {
    /*
     * CylindricalDuo represents a disc-axial expansion, where the disc is described by a single
     * series, such as Zernike (as opposed to a series individually representing r and a second
     * series independently representing theta. For CylindricalDuo series, the series are always
     * stored in the axial, planar order, independent of which order the series appear in the input
     * file. Therefore, the _orders and _physical_bounds vectors must always appear in axial, planar
     * order. The first entry in _domains is interpreted as the axial direction, and the following
     * two as the planar.
     */
    if (isParamValid("x"))
    {
      domains = {FunctionalBasisInterface::_domain_options = "x",
                 FunctionalBasisInterface::_domain_options = "y",
                 FunctionalBasisInterface::_domain_options = "z"};
      types.push_back(_x);
    }
    if (isParamValid("y"))
    {
      domains = {FunctionalBasisInterface::_domain_options = "y",
                 FunctionalBasisInterface::_domain_options = "x",
                 FunctionalBasisInterface::_domain_options = "z"};
      types.push_back(_y);
    }
    if (isParamValid("z"))
    {
      domains = {FunctionalBasisInterface::_domain_options = "z",
                 FunctionalBasisInterface::_domain_options = "x",
                 FunctionalBasisInterface::_domain_options = "y"};
      types.push_back(_z);
    }

    if (types.size() == 0)
      mooseError("Must specify one of 'x', 'y', or 'z' for 'CylindricalDuo' series!");

    if (types.size() > 1)
      mooseError("Cannot specify more than one of 'x', 'y', or 'z' for 'CylindricalDuo' series!");

    types.push_back(_disc);
    _series_type = libmesh_make_unique<CylindricalDuo>(domains, _orders, types, name());
  }
  else
    mooseError("Unknown functional series type \"", _series_type_name, "\"");

  // Set the physical bounds of each of the single series if defined
  if (isParamValid("physical_bounds"))
    _series_type->setPhysicalBounds(_physical_bounds);

  // Resize the coefficient array as needed
  enforceSize(false), resize(getNumberOfTerms(), 0.0), enforceSize(true);
  setCharacteristics(_orders);
}

FunctionSeries &
FunctionSeries::checkAndConvertFunction(Function & function,
                                        const std::string & typeName,
                                        const std::string & objectName)
{
  FunctionSeries * test = dynamic_cast<FunctionSeries *>(&function);
  if (!test)
    ::mooseError("In ",
                 typeName,
                 "-type object \"",
                 objectName,
                 "\": the named Function \"",
                 function.name(),
                 "\" must be a FunctionSeries-type object.");

  return *test;
}

Real
FunctionSeries::getStandardizedFunctionVolume() const
{
  return _series_type->getStandardizedFunctionVolume();
}

std::size_t
FunctionSeries::getNumberOfTerms() const
{
  return _series_type->getNumberOfTerms();
}

const std::vector<size_t> &
FunctionSeries::getOrders() const
{
  return _orders;
}

/*
 * getAllOrthonormal() is defined in the FunctionalBasisInterface, which calls the pure virtual
 * evaluateOrthonormal() method of the CompositeSeriesBasisInterface class, which then calls the
 * getAllOrthonormal() method of each of the single series.
 */
const std::vector<Real> &
FunctionSeries::getOrthonormal()
{
  return _series_type->getAllOrthonormal();
}

/*
 * getAllStandard() is defined in the FunctionalBasisInterface, which calls the pure virtual
 * evaluateStandard() method of the CompositeSeriesBasisInterface class, which then calls the
 * getAllStandard() method of each of the single series.
 */
const std::vector<Real> &
FunctionSeries::getStandard()
{
  return _series_type->getAllStandard();
}

/*
 * isInPhysicalBounds() is a pure virtual method of the FunctionalBasisInterface that is defined in
 * the CompositeSeriesBasisInterface class because it is agnostic to the underlying types of the
 * single series.
 */
bool
FunctionSeries::isInPhysicalBounds(const Point & point) const
{
  return _series_type->isInPhysicalBounds(point);
}

void
FunctionSeries::setLocation(const Point & point)
{
  _series_type->setLocation(point);
}

Real
FunctionSeries::evaluateValue(Real, const Point & point)
{
  // Check that the point is within the physical bounds of the series
  if (!isInPhysicalBounds(point))
    return 0.0;

  // Set the location at which to evaluate the series
  setLocation(point);

  return expand();
}

Real
FunctionSeries::expand()
{
  return expand(_coefficients);
}

Real
FunctionSeries::expand(const std::vector<Real> & coefficients)
{
  // Evaluate all of the terms in the series
  const std::vector<Real> & terms = getStandard();

  return std::inner_product(terms.begin(), terms.end(), coefficients.begin(), 0.0);
}

std::ostream &
operator<<(std::ostream & stream, const FunctionSeries & me)
{
  stream << "\n\n"
         << "FunctionSeries: " << me.name() << "\n"
         << "         Terms: " << me.getNumberOfTerms() << "\n";
  me._series_type->formatCoefficients(stream, me._coefficients);
  stream << "\n\n";

  return stream;
}

std::vector<std::size_t>
FunctionSeries::convertOrders(const std::vector<unsigned int> & orders)
{
  return std::vector<std::size_t>(orders.begin(), orders.end());
}
