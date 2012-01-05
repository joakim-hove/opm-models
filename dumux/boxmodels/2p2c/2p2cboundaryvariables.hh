// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2010 by Katherina Baber, Klaus Mosthaf                    *
 *   Copyright (C) 2008-2009 by Bernd Flemisch, Andreas Lauser               *
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
 *        all fluxes of the fluid phase over the boundary of a finite volume.
 *
 * This means pressure and temperature gradients, phase densities at
 * the integration point of the boundary, etc.
 */
#ifndef DUMUX_2P2C_BOUNDARY_VARIABLES_HH
#define DUMUX_2P2C_BOUNDARY_VARIABLES_HH

#include <dumux/common/math.hh>

#include "2p2cproperties.hh"

namespace Dumux
{

/*!
 * \ingroup TwoPTwoCModel
 * \brief This template class contains the data which is required to
 *        calculate the fluxes of the fluid phases over the boundary of a
 *        finite volume for the 2p2c model.
 *
 * This means pressure and velocity gradients, phase density and viscosity at
 * the integration point of the boundary, etc.
 */
template <class TypeTag>
class TwoPTwoCBoundaryVariables
{
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Problem)) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(VolumeVariables)) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(ElementVolumeVariables)) ElementVolumeVariables;
    enum { numPhases = GET_PROP_VALUE(TypeTag, PTAG(NumPhases)) };

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(TwoPTwoCIndices)) Indices;
    enum {
        lPhaseIdx = Indices::lPhaseIdx,
        gPhaseIdx = Indices::gPhaseIdx,
        lCompIdx = Indices::lCompIdx,
        gCompIdx = Indices::gCompIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    enum { dim = GridView::dimension };

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    typedef Dune::FieldVector<Scalar, dim> Vector;
    typedef Dune::FieldMatrix<Scalar, dim, dim> Tensor;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FVElementGeometry)) FVElementGeometry;
    typedef typename FVElementGeometry::BoundaryFace BoundaryFace;

public:
    TwoPTwoCBoundaryVariables(const Problem &problem,
                     const Element &element,
                     const FVElementGeometry &elemGeom,
                     int boundaryFaceIdx,
                     const ElementVolumeVariables &elemDat,
                     int scvIdx)
        : fvElemGeom_(elemGeom),
          scvIdx_(scvIdx)
    {
        boundaryFace_ = &fvElemGeom_.boundaryFace[boundaryFaceIdx];

        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            densityAtIP_[phaseIdx] = Scalar(0);
            pressureAtIP_[phaseIdx] = Scalar(0);
            molarDensityAtIP_[phaseIdx] = Scalar(0);
            potentialGrad_[phaseIdx] = Scalar(0);
            concentrationGrad_[phaseIdx] = Scalar(0);
            molarConcGrad_[phaseIdx] = Scalar(0);
            KmvpNormal_[phaseIdx] = Scalar(0);
            porousDiffCoeff_[phaseIdx] = Scalar(0);
        }

        calculateBoundaryValues_(problem, element, elemDat);
    };

    Scalar KmvpNormal(int phaseIdx) const
    { return KmvpNormal_[phaseIdx]; }

    /*!
     * \brief The binary diffusion coefficient for each fluid phase.
     */
    Scalar porousDiffCoeff(int phaseIdx) const
    { return porousDiffCoeff_[phaseIdx]; };

    /*!
     * \brief Return density \f$\mathrm{[kg/m^3]}\f$ of a phase at the integration
     *        point.
     */
    Scalar densityAtIP(int phaseIdx) const
    { return densityAtIP_[phaseIdx]; }

    /*!
     * \brief Return molar density \f$\mathrm{[mol/m^3]}\f$ of a phase at the integration
     *        point.
     */
    Scalar molarDensityAtIP(int phaseIdx) const
    { return molarDensityAtIP_[phaseIdx]; }

    /*!
     * \brief The concentration gradient of a component in a phase.
     */
    const Vector &concentrationGrad(int phaseIdx) const
    { return concentrationGrad_[phaseIdx]; };

    /*!
     * \brief The molar concentration gradient of a component in a phase.
     */
    const Vector &molarConcGrad(int phaseIdx) const
    { return molarConcGrad_[phaseIdx]; };

    const FVElementGeometry &fvElemGeom() const
    { return fvElemGeom_; }

    const BoundaryFace& boundaryFace() const
    { return *boundaryFace_; }

protected:
    void calculateBoundaryValues_(const Problem &problem,
                             const Element &element,
                             const ElementVolumeVariables &elemDat)
    {
        Vector tmp(0.0);

        // calculate gradients and secondary variables at IPs of the boundary
        for (int idx = 0;
             idx < fvElemGeom_.numVertices;
             idx++) // loop over adjacent vertices
        {
            // FE gradient at vertex idx
            const Vector& feGrad = boundaryFace_->grad[idx];

            // compute sum of pressure gradients for each phase
            for (int phaseIdx = 0; phaseIdx < numPhases; phaseIdx++)
            {
                // the pressure gradient
                tmp = feGrad;
                tmp *= elemDat[idx].pressure(phaseIdx);
                potentialGrad_[phaseIdx] += tmp;
            }

            // the concentration gradient of the non-wetting
            // component in the wetting phase
            tmp = feGrad;
            tmp *= elemDat[idx].fluidState().massFraction(lPhaseIdx, gCompIdx);
            concentrationGrad_[lPhaseIdx] += tmp;

            tmp = feGrad;
            tmp *= elemDat[idx].fluidState().moleFraction(lPhaseIdx, gCompIdx);
            molarConcGrad_[lPhaseIdx] += tmp;

            // the concentration gradient of the wetting component
            // in the non-wetting phase
            tmp = feGrad;
            tmp *= elemDat[idx].fluidState().massFraction(gPhaseIdx, lCompIdx);
            concentrationGrad_[gPhaseIdx] += tmp;

            tmp = feGrad;
            tmp *= elemDat[idx].fluidState().moleFraction(gPhaseIdx, lCompIdx);
            molarConcGrad_[gPhaseIdx] += tmp;

            for (int phaseIdx=0; phaseIdx < numPhases; phaseIdx++)
            {
                pressureAtIP_[phaseIdx] += elemDat[idx].pressure(phaseIdx)*boundaryFace_->shapeValue[idx];
                densityAtIP_[phaseIdx] += elemDat[idx].density(phaseIdx)*boundaryFace_->shapeValue[idx];
                molarDensityAtIP_[phaseIdx] += elemDat[idx].molarDensity(phaseIdx)*boundaryFace_->shapeValue[idx];
            }
        }

        for (int phaseIdx=0; phaseIdx < numPhases; phaseIdx++)
        {
            if (GET_PARAM(TypeTag, bool, EnableGravity)) {
                tmp = problem.gravity();
                tmp *= densityAtIP_[phaseIdx];
            }

            // calculate the potential gradient
            potentialGrad_[phaseIdx] -= tmp;

            Scalar k = problem.spatialParameters().intrinsicPermeability(element, fvElemGeom_, scvIdx_);
            Tensor K(0);
            K[0][0] = K[1][1] = k;
            Vector Kmvp;
            K.mv(potentialGrad_[phaseIdx], Kmvp);
            KmvpNormal_[phaseIdx] = - (Kmvp*boundaryFace_->normal);

            const VolumeVariables &vertDat = elemDat[scvIdx_];

            if (vertDat.saturation(phaseIdx) <= 0)
                porousDiffCoeff_[phaseIdx] = 0.0;
            else
            {
                Scalar tau = 1.0/(vertDat.porosity()*vertDat.porosity())*
                    pow(vertDat.porosity()*vertDat.saturation(phaseIdx), 7.0/3);

                porousDiffCoeff_[phaseIdx] = vertDat.porosity()*vertDat.saturation(phaseIdx)*tau*vertDat.diffCoeff(phaseIdx);
            }
        }
    }

    const FVElementGeometry &fvElemGeom_;
    const BoundaryFace *boundaryFace_;

    // gradients
    Vector potentialGrad_[numPhases];
    Vector concentrationGrad_[numPhases];
    Vector molarConcGrad_[numPhases];

    // density of each face at the integration point
    Scalar pressureAtIP_[numPhases];
    Scalar densityAtIP_[numPhases];
    Scalar molarDensityAtIP_[numPhases];

    // intrinsic permeability times pressure potential gradient
    // projected on the face normal
    Scalar KmvpNormal_[numPhases];

    // the diffusion coefficient for the porous medium
    Scalar porousDiffCoeff_[numPhases];

    int scvIdx_;
};

} // end namespace

#endif
