// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Copyright (C) 2008-2009 by Melanie Darcis                               *
 *   Copyright (C) 2008-2009 by Klaus Mosthaf                                *
 *   Copyright (C) 2008-2009 by Bernd Flemisch                               *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief This file contains the data which is required to calculate
 *        all fluxes (mass of components and energy) over a face of a finite volume.
 *
 * This means pressure, concentration and temperature gradients, phase
 * densities at the integration point, etc.
 */
#ifndef DUMUX_2P2CNI_FLUX_VARIABLES_HH
#define DUMUX_2P2CNI_FLUX_VARIABLES_HH

#include <dumux/common/math.hh>
#include <dumux/boxmodels/2p2c/2p2cfluxvariables.hh>

namespace Dumux
{

/*!
 * \ingroup TwoPTwoCNIModel
 * \ingroup BoxFluxVariables
 * \brief This template class contains the data which is required to
 *        calculate all fluxes (mass of components and energy) over a face of a finite
 *        volume for the non-isothermal two-phase, two-component model.
 *
 * This means pressure and concentration gradients, phase densities at
 * the integration point, etc.
 */
template <class TypeTag>
class TwoPTwoCNIFluxVariables : public TwoPTwoCFluxVariables<TypeTag>
{
    typedef TwoPTwoCFluxVariables<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Problem)) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(ElementVolumeVariables)) ElementVolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FVElementGeometry)) FVElementGeometry;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    enum { dimWorld = GridView::dimensionworld };

    typedef typename GridView::ctype CoordScalar;
    typedef Dune::FieldVector<CoordScalar, dimWorld> Vector;

public:
    /*
     * \brief The constructor
     *
     * \param problem The problem
     * \param element The finite element
     * \param elemGeom The finite-volume geometry in the box scheme
     * \param faceIdx The local index of the SCV (sub-control-volume) face
     * \param elemDat The volume variables of the current element
     */
    TwoPTwoCNIFluxVariables(const Problem &problem,
                       const Element &element,
                       const FVElementGeometry &elemGeom,
                       int scvfIdx,
                       const ElementVolumeVariables &elemDat)
        : ParentType(problem, element, elemGeom, scvfIdx, elemDat)
    {
        // calculate temperature gradient using finite element
        // gradients
        Vector temperatureGrad(0);
        Vector tmp(0.0);
        for (int vertIdx = 0; vertIdx < elemGeom.numVertices; vertIdx++)
        {
            tmp = elemGeom.subContVolFace[scvfIdx].grad[vertIdx];
            tmp *= elemDat[vertIdx].temperature();
            temperatureGrad += tmp;
        }

        // The spatial parameters calculates the actual heat flux vector
        problem.spatialParameters().matrixHeatFlux(tmp,
                                                   *this,
                                                   elemDat,
                                                   temperatureGrad,
                                                   element,
                                                   elemGeom,
                                                   scvfIdx);
        // project the heat flux vector on the face's normal vector
        normalMatrixHeatFlux_ = tmp*elemGeom.subContVolFace[scvfIdx].normal;
    }

    /*!
     * \brief The total heat flux \f$\mathrm{[J/s]}\f$ due to heat conduction
     *        of the rock matrix over the sub-control volume's face in
     *        direction of the face normal.
     */
    Scalar normalMatrixHeatFlux() const
    { return normalMatrixHeatFlux_; }

private:
    Scalar normalMatrixHeatFlux_;
};

} // end namepace

#endif
