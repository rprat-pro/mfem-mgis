/*!
 * \file   SmallStrainMechanicalBehaviourIntegrator.ixx
 * \brief
 * \author Thomas Helfer
 * \date   13/10/2020
 */

#ifndef LIB_MFEM_MGIS_SMALLSTRAINMECHANICALBEHAVIOURINTEGRATOR_IXX
#define LIB_MFEM_MGIS_SMALLSTRAINMECHANICALBEHAVIOURINTEGRATOR_IXX

#include <utility>
#include "mfem/fem/eltrans.hpp"
#include "MGIS/Raise.hxx"
#include "MFEMMGIS/PartialQuadratureSpace.hxx"

namespace mfem_mgis {

  template <Hypothesis H>
  const mfem::IntegrationRule &
  SmallStrainMechanicalBehaviourIntegrator<H>::getIntegrationRule(
      const mfem::FiniteElement &el, const mfem::ElementTransformation &Trans) {
    const auto order = 2 * Trans.OrderGrad(&el);
    if constexpr ((H == Hypothesis::AXISYMMETRICALGENERALISEDPLANESTRAIN) ||
                  (H == Hypothesis::AXISYMMETRICALGENERALISEDPLANESTRESS) ||
                  (H == Hypothesis::AXISYMMETRICAL)) {
      const auto& ir = mfem::IntRules.Get(el.GetGeomType(), order + 1);
      return ir;
    } else {
      const auto &ir = mfem::IntRules.Get(el.GetGeomType(), order);
      return ir;
    }
  }  // end of getIntegrationRule

  template <Hypothesis H>
  std::shared_ptr<const PartialQuadratureSpace>
  SmallStrainMechanicalBehaviourIntegrator<H>::buildQuadratureSpace(
      const mfem::FiniteElementSpace &fs, const size_type m) {
    auto selector = [](const mfem::FiniteElement &e,
                       const mfem::ElementTransformation &tr)
        -> const mfem::IntegrationRule & {
      return getIntegrationRule(e, tr);
    };  // end of selector
    return std::make_shared<PartialQuadratureSpace>(fs, m, selector);
  }  // end of buildQuadratureSpace

  template <Hypothesis H>
  SmallStrainMechanicalBehaviourIntegrator<H>::
      SmallStrainMechanicalBehaviourIntegrator(
          const mfem::FiniteElementSpace &fs,
          const size_type m,
          std::unique_ptr<const Behaviour> b_ptr)
      : StandardBehaviourIntegratorCRTPBase<
            SmallStrainMechanicalBehaviourIntegrator>(
            buildQuadratureSpace(fs, m), std::move(b_ptr)) {
    this->checkHypotheses(H);
  }  // end of SmallStrainMechanicalBehaviourIntegrator

  template <Hypothesis H>
  SmallStrainMechanicalBehaviourIntegrator<
      H>::~SmallStrainMechanicalBehaviourIntegrator() = default;

  // inline implementations

  template <Hypothesis H>
  void SmallStrainMechanicalBehaviourIntegrator<H>::updateGradients(
      mgis::span<real>& g,
      const mfem::Vector &u,
      const mfem::DenseMatrix &dN,
      const size_type ni) {
    constexpr const auto icste = 0.70710678118654752440;
    static_assert((H == Hypothesis::TRIDIMENSIONAL) ||
                      (H == Hypothesis::PLANESTRAIN) ||
                      (H == Hypothesis::PLANESTRESS),
                  "unsupported hypothesis");
    if constexpr ((H == Hypothesis::PLANESTRAIN) ||
                  (H == Hypothesis::PLANESTRESS)) {
      const auto nnodes = dN.NumRows();
      const auto nx = ni;
      const auto ny = ni + nnodes;
      const auto ux = u[nx];
      const auto uy = u[ny];
      g[0] += ux * dN(nx, 0);                             // xx
      g[1] += uy * dN(ny, 1);                             // yy
      g[2] += 0;                                          // zz
      g[3] += (ux * dN(nx, 1) + uy * dN(ny, 0)) * icste;  // xy
    } else if constexpr (H == Hypothesis::TRIDIMENSIONAL) {
      const auto nnodes = dN.NumRows();
      const auto ux = u[ni];
      const auto uy = u[ni + nnodes];
      const auto uz = u[ni + 2 * nnodes];
      g[0] += ux * dN(ni, 0);                             // xx
      g[1] += uy * dN(ni, 1);                             // yy
      g[2] += uz * dN(ni, 2);                             // zz
      g[3] += (ux * dN(ni, 1) + uy * dN(ni, 0)) * icste;  // xy
      g[4] += (ux * dN(ni, 2) + uz * dN(ni, 0)) * icste;  // xz
      g[5] += (uy * dN(ni, 2) + uz * dN(ni, 1)) * icste;  // yz
    }
  }  // end of updateGradients

  template <Hypothesis H>
  void SmallStrainMechanicalBehaviourIntegrator<H>::updateInnerForces(
      mfem::Vector &Fe,
      const mgis::span<const real> &s,
      const mfem::DenseMatrix &dN,
      const real w,
      const size_type ni) const {
    constexpr const auto icste = 0.70710678118654752440;
    static_assert(H == Hypothesis::TRIDIMENSIONAL, "unsupported hypothesis");
    if constexpr (H == Hypothesis::TRIDIMENSIONAL) {
      const auto nnodes = dN.NumRows();
      const auto nx = ni;
      const auto ny = ni + nnodes;
      const auto nz = ni + 2 * nnodes;
      real B[6][3] = {{dN(ni, 0), 0, 0},                          //
                      {0, dN(ni, 1), 0},                          //
                      {0, 0, dN(ni, 2)},                          //
                      {dN(ni, 1) * icste, dN(ni, 0) * icste, 0},  // xy
                      {dN(ni, 2) * icste, 0, dN(ni, 0) * icste},  // xz
                      {0, dN(ni, 2) * icste, dN(ni, 1) * icste}}; // yz
      Fe[nx] += w * (B[0][0] * s[0] + B[1][0] * s[1] + B[2][0] * s[2] +
                     B[3][0] * s[3] + B[4][0] * s[4] + B[5][0] * s[5]);
      Fe[ny] += w * (B[0][1] * s[0] + B[1][1] * s[1] + B[2][1] * s[2] +
                     B[3][1] * s[3] + B[4][1] * s[4] + B[5][1] * s[5]);
      Fe[nz] += w * (B[0][2] * s[0] + B[1][2] * s[1] + B[2][2] * s[2] +
                     B[3][2] * s[3] + B[4][2] * s[4] + B[5][2] * s[5]);
    }
  }  // end of updateInnerForces

  template <Hypothesis H>
  void SmallStrainMechanicalBehaviourIntegrator<H>::updateStiffnessMatrix(
      mfem::DenseMatrix &Ke,
      const mgis::span<const real>& Kip,
      const mfem::DenseMatrix &dN,
      const real w,
      const size_type ni) const {
    constexpr const auto icste = 0.70710678118654752440;
    static_assert(H == Hypothesis::TRIDIMENSIONAL, "unsupported hypothesis");
    if constexpr (H == Hypothesis::TRIDIMENSIONAL) {
      const auto nnodes = dN.NumRows();
      real Bi[6][3] = {{dN(ni, 0), 0, 0},                          //
                       {0, dN(ni, 1), 0},                          //
                       {0, 0, dN(ni, 2)},                          //
                       {dN(ni, 1) * icste, dN(ni, 0) * icste, 0},  //
                       {dN(ni, 2) * icste, 0, dN(ni, 0) * icste},  // xz
                       {0, dN(ni, 2) * icste, dN(ni, 1) * icste}};
      for (size_type nj = 0; nj != nnodes; ++nj) {
        real Bj[6][3] = {{dN(nj, 0), 0, 0},                          //
                         {0, dN(nj, 1), 0},                          //
                         {0, 0, dN(nj, 2)},                          //
                         {dN(nj, 1) * icste, dN(nj, 0) * icste, 0},  //
                         {dN(nj, 2) * icste, 0, dN(nj, 0) * icste},  //
                         {0, dN(nj, 2) * icste, dN(nj, 1) * icste}};
        real KB[6][3];
        for (size_type i = 0; i != 6; ++i) {
          for (size_type j = 0; j != 3; ++j) {
            KB[i][j] = real{};
            for (size_type k = 0; k != 6; ++k) {
              KB[i][j] += Kip[i * 6 + k] * Bj[k][j];
            }
          }
        }
        for (size_type i = 0; i != 3; ++i) {
          for (size_type j = 0; j != 3; ++j) {
            auto tBKB = real{};
            for (size_type k = 0; k != 6; ++k) {
              tBKB += Bi[k][i] * KB[k][j];
            }
            Ke(ni + i * nnodes, nj + j * nnodes) += w * tBKB;
          }
        }
      } // end of for (size_type nj = 0; nj != nnodes; ++nj)
    } // end of if constexpr (H == Hypothesis::TRIDIMENSIONAL)
  }   // end of updateStiffnessMatrix

  /* Methods that must be explicitely instanciated in a source file */

  template <>
  void SmallStrainMechanicalBehaviourIntegrator<Hypothesis::TRIDIMENSIONAL>::
      computeInnerForces(mfem::Vector &,
                         const mfem::FiniteElement &,
                         mfem::ElementTransformation &,
                         const mfem::Vector &);

  template <>
  void SmallStrainMechanicalBehaviourIntegrator<Hypothesis::TRIDIMENSIONAL>::
      computeStiffnessMatrix(mfem::DenseMatrix &,
                             const mfem::FiniteElement &,
                             mfem::ElementTransformation &,
                             const mfem::Vector &);

}  // end of namespace mfem_mgis

#endif /* LIB_MFEM_MGIS_SMALLSTRAINMECHANICALBEHAVIOURINTEGRATOR_IXX */