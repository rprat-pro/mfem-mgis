/*!
 * \file   include/MFEMMGIS/StandardBehaviourIntegratorCRTPBase.ixx
 * \brief
 * \author Thomas Helfer
 * \date   14/12/2020
 */

#ifndef LIB_MFEM_MGIS_STANDARDBEHAVIOURINTEGRATORCRTPBASE_IXX
#define LIB_MFEM_MGIS_STANDARDBEHAVIOURINTEGRATORCRTPBASE_IXX

#include "mfem/fem/fe.hpp"
#include "mfem/fem/eltrans.hpp"
#include "MFEMMGIS/PartialQuadratureSpace.hxx"

namespace mfem_mgis {

  template <typename Child>
  void StandardBehaviourIntegratorCRTPBase<Child>::implementComputeInnerForces(
      mfem::Vector &Fe,
      const mfem::FiniteElement &e,
      mfem::ElementTransformation &tr,
      const mfem::Vector &u) {
#ifdef MFEM_THREAD_SAFE
    DenseMatrix dshape(e.GetDof(), e.GetDim());
#else
    dshape.SetSize(e.GetDof(), e.GetDim());
#endif
    // element offset
    const auto nnodes = e.GetDof();
    const auto gsize = this->s1.gradients_stride;
    const auto thsize = this->s1.thermodynamic_forces_stride;
    const auto eoffset = this->quadrature_space->getOffset(tr.ElementNo);
    Fe.SetSize(e.GetDof() * e.GetDim());
    Fe = 0.;
    const auto &ir = Child::getIntegrationRule(e, tr);
    for (size_type i = 0; i < ir.GetNPoints(); ++i) {
      // get the gradients of the shape functions
      const auto &ip = ir.IntPoint(i);
      tr.SetIntPoint(&ip);
      e.CalcPhysDShape(tr, dshape);
      // get the weights associated to point ip
      const auto w = ip.weight * tr.Weight();
      // offset of the integration point
      const auto o = eoffset + i;
      auto g = this->s1.gradients.subspan(o * gsize, gsize);
      std::copy(this->macroscopic_gradients.begin(),
                this->macroscopic_gradients.end(), g.begin());
      for (size_type ni = 0; ni != nnodes; ++ni) {
        static_cast<Child *>(this)->updateGradients(g, u, dshape, ni);
      }
      this->integrate(eoffset + i);
      const auto s = this->s1.thermodynamic_forces.subspan(o * thsize, thsize);
      // assembly of the inner forces
      for (size_type ni = 0; ni != nnodes; ++ni) {
        static_cast<const Child *>(this)->updateInnerForces(Fe, s, dshape, w,
                                                            ni);
      }
    }
  }  // end of implementComputeInnerForces

  template <typename Child>
  void
  StandardBehaviourIntegratorCRTPBase<Child>::implementComputeStiffnessMatrix(
      mfem::DenseMatrix &Ke,
      const mfem::FiniteElement &e,
      mfem::ElementTransformation &tr) {
#ifdef MFEM_THREAD_SAFE
    DenseMatrix dshape(e.GetDof(), e.GetDim());
#else
    dshape.SetSize(e.GetDof(), e.GetDim());
#endif
    // element offset
    const auto nnodes = e.GetDof();
    const auto gsize = this->s1.gradients_stride;
    const auto thsize = this->s1.thermodynamic_forces_stride;
    const auto eoffset = this->quadrature_space->getOffset(tr.ElementNo);
    Ke.SetSize(e.GetDof() * e.GetDim(), e.GetDof() * e.GetDim());
    Ke = 0.;
    const auto &ir = Child::getIntegrationRule(e, tr);
    for (size_type i = 0; i < ir.GetNPoints(); ++i) {
      // get the gradients of the shape functions
      const auto &ip = ir.IntPoint(i);
      tr.SetIntPoint(&ip);
      e.CalcPhysDShape(tr, dshape);
      // get the weights associated to point ip
      const auto w = ip.weight * tr.Weight();
      // offset of the integration point
      const auto o = eoffset + i;
      const auto Kip = this->K.subspan(o * gsize * thsize, gsize * thsize);
      // assembly of the stiffness matrix
      for (size_type ni = 0; ni != nnodes; ++ni) {
        static_cast<const Child *>(this)->updateStiffnessMatrix(Ke, Kip, dshape,
                                                                w, ni);
      }
    }
  }  // end of implementComputeStiffnessMatrix

  template <typename Child>
  StandardBehaviourIntegratorCRTPBase<
      Child>::~StandardBehaviourIntegratorCRTPBase() = default;

}  // end of namespace mfem_mgis

#endif /* LIB_MFEM_MGIS_STANDARDBEHAVIOURINTEGRATORCRTPBASE_IXX */