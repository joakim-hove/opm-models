// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Copyright (C) 2008-2009 by Klaus Mosthaf                                *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
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
#ifndef DUMUX_OBSTACLE_SPATIAL_PARAMS_HH
#define DUMUX_OBSTACLE_SPATIAL_PARAMS_HH

#include <dumux/material/spatialparameters/boxspatialparameters.hh>
#include <dumux/material/fluidmatrixinteractions/2p/linearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedlinearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedbrookscorey.hh>
#include <dumux/material/fluidmatrixinteractions/2p/efftoabslaw.hh>

#include <dumux/boxmodels/mpnc/mpncmodel.hh>
#include <dumux/material/fluidmatrixinteractions/mp/mplinearmaterial.hh>
#include <dumux/material/fluidmatrixinteractions/mp/2padapter.hh>

namespace Dumux
{

//forward declaration
template<class TypeTag>
class ObstacleSpatialParameters;

namespace Properties
{
// The spatial parameters TypeTag
NEW_TYPE_TAG(ObstacleSpatialParameters);

// Set the spatial parameters
SET_TYPE_PROP(ObstacleSpatialParameters, SpatialParameters, Dumux::ObstacleSpatialParameters<TypeTag>);

// Set the material Law
SET_PROP(ObstacleSpatialParameters, MaterialLaw)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    enum {
        lPhaseIdx = FluidSystem::lPhaseIdx
    };
    // define the material law
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    //    typedef RegularizedBrooksCorey<Scalar> EffMaterialLaw;
        typedef RegularizedLinearMaterial<Scalar> EffMaterialLaw;
        typedef EffToAbsLaw<EffMaterialLaw> TwoPMaterialLaw;
public:
    typedef TwoPAdapter<lPhaseIdx, TwoPMaterialLaw> type;
};
}

/**
 * \ingroup MPNCModel
 * \ingroup BoxTestProblems
 * \brief Definition of the spatial params properties for the obstacle problem
 *
 */
template<class TypeTag>
class ObstacleSpatialParameters : public BoxSpatialParameters<TypeTag>
{
    typedef BoxSpatialParameters<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    typedef typename Grid::ctype CoordScalar;
    enum {
        dim=GridView::dimension,
        dimWorld=GridView::dimensionworld
    };

    enum {
        lPhaseIdx = FluidSystem::lPhaseIdx
    };

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef Dune::FieldVector<CoordScalar,dimWorld> GlobalPosition;

public:
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename MaterialLaw::Params MaterialLawParams;

    ObstacleSpatialParameters(const GridView &gv)
        : ParentType(gv)
    {
        // intrinsic permeabilities
        coarseK_ = 1e-12;
        fineK_ = 1e-15;

        // the porosity
        porosity_ = 0.3;

        // residual saturations
        fineMaterialParams_.setSwr(0.0);
        fineMaterialParams_.setSnr(0.0);
        coarseMaterialParams_.setSwr(0.0);
        coarseMaterialParams_.setSnr(0.0);

        // parameters for the linear law, i.e. minimum and maximum
        // pressures
        fineMaterialParams_.setEntryPC(0.0);
        coarseMaterialParams_.setEntryPC(0.0);
        fineMaterialParams_.setMaxPC(0.0);
        coarseMaterialParams_.setMaxPC(0.0);

        /*
        // entry pressures for Brooks-Corey
        fineMaterialParams_.setPe(5e3);
        coarseMaterialParams_.setPe(1e3);

        // Brooks-Corey shape parameters
        fineMaterialParams_.setLambda(2);
        coarseMaterialParams_.setLambda(2);
        */
    }

    ~ObstacleSpatialParameters()
    {}

    /*!
     * \brief Update the spatial parameters with the flow solution
     *        after a timestep.
     *
     * \param globalSol The current solution vector
     */
    void update(const SolutionVector &globalSol)
    {
    };

    /*!
     * \brief Returns the intrinsic permeability tensor.
     *
     * \param element       The current finite element
     * \param fvElemGeom    The current finite volume geometry of the element
     * \param scvIdx        The index sub-control volume where the
     *                      intrinsic permeability is given.
     */
    using ParentType::intrinsicPermeability;
    Scalar intrinsicPermeability(const Element &element,
                                 const FVElementGeometry &fvElemGeom,
                                 int scvIdx) const
    {
        if (isFineMaterial_(fvElemGeom.subContVol[scvIdx].global))
            return fineK_;
        return coarseK_;
    }

    /*!
     * \brief Define the porosity \f$[-]\f$ of the soil
     *
     * \param element     The finite element
     * \param fvElemGeom  The finite volume geometry
     * \param scvIdx      The local index of the sub-control volume where
     *                    the porosity needs to be defined
     */
    using ParentType::porosity;
    Scalar porosity(const Element &element,
                    const FVElementGeometry &fvElemGeom,
                    int scvIdx) const
    {
        return porosity_;
    }

    /*!
     * \brief Function for defining the parameters needed by constitutive relationships (kr-Sw, pc-Sw, etc.).
     *
     * \param pos The global position of the sub-control volume.
     * \return the material parameters object
     */
    const MaterialLawParams &materialLawParamsAtPos(const GlobalPosition &pos) const
    {
        if (isFineMaterial_(pos))
            return fineMaterialParams_;
        else
            return coarseMaterialParams_;
    }


    /*!
     * \brief Returns the volumetric heat capacity [J/(K m^3)] of the
     *        solid phase with no pores in a sub-control volume.
     */
    using ParentType::heatCapacitySolid;
    Scalar heatCapacitySolid(const Element &element,
                             const FVElementGeometry &fvElemGeom,
                             int scvIdx) const
    {
        return
            2700.0 // density of granite [kg/m^3] 
            * 790.0 ;  // specific heat capacity of granite [J / (kg K)]
    }

    /*!
     * \brief Returns the thermal conductivity [W / (K m)] of the solid
     *        phase disregarding the pores in a sub-control volume.
     */
    using ParentType::thermalConductivitySolid;
    Scalar thermalConductivitySolid(const Element &element,
                                    const FVElementGeometry &fvElemGeom,
                                    int scvIdx) const
    {
        return 2.8; // conductivity of granite [W / (m K ) ]
    }

private:
    /*!
     * \brief Returns whether a given global position is in the
     *        fine-permeability region or not.
     */
    static bool isFineMaterial_(const GlobalPosition &pos)
    {
        return
            10 <= pos[0] && pos[0] <= 20 &&
            0 <= pos[1] && pos[1] <= 35;
    };

    Scalar coarseK_;
    Scalar fineK_;
    Scalar porosity_;
    MaterialLawParams fineMaterialParams_;
    MaterialLawParams coarseMaterialParams_;
};

}

#endif
