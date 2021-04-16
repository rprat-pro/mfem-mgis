/*!
 * \file   src/NonLinearEvolutionProblemImplementation.cxx
 * \brief
 * \author Thomas Helfer
 * \date   11/12/2020
 */

#include "MGIS/Raise.hxx"
#include "MFEMMGIS/Parameters.hxx"
#include "MFEMMGIS/SolverUtilities.hxx"
#include "MFEMMGIS/LinearSolverFactory.hxx"
#include "MFEMMGIS/NewtonSolver.hxx"
#include "MFEMMGIS/PostProcessing.hxx"
#include "MFEMMGIS/PostProcessingFactory.hxx"
#include "MFEMMGIS/FiniteElementDiscretization.hxx"
#include "MFEMMGIS/MultiMaterialNonLinearIntegrator.hxx"
#include "MFEMMGIS/NonLinearEvolutionProblemImplementation.hxx"

namespace mfem_mgis {

  /*!
   * \brief post-processing defined by an std::function
   * \tparam parallel: boolean stating if the computations are performed in
   * parallel.
   */
  template <bool parallel>
  struct StdFunctionPostProcessing final : PostProcessing<parallel> {
    /*!
     * \brief constructor
     * \param[in] fct: function executing the postprocessing
     */
    StdFunctionPostProcessing(
        const std::function<void(const real, const real)>& fct)
        : f(fct) {}  // end of StdFunctionPostProcessing
    //
    void execute(NonLinearEvolutionProblemImplementation<parallel>&,
                 const real t,
                 const real dt) override {
      this->f(t, dt);
    }  // end of execute
    //! \brief destructor
    ~StdFunctionPostProcessing() override = default;

   private:
    //! \brief function executing the post-processing
    std::function<void(const real, const real)> f;
  };  // end of struct StdFunctionPostProcessing

#ifdef MFEM_USE_MPI

  NonLinearEvolutionProblemImplementation<true>::
      NonLinearEvolutionProblemImplementation(
          std::shared_ptr<FiniteElementDiscretization> fed,
          const Hypothesis h,
          const Parameters& p)
      : NonLinearEvolutionProblemImplementationBase(fed, h, p),
        mfem::ParNonlinearForm(&(fed->getFiniteElementSpace<true>())) {
    if (this->fe_discretization->getMesh<true>().Dimension() !=
        mgis::behaviour::getSpaceDimension(h)) {
      raise(
          "NonLinearEvolutionProblemImplementationBase::"
          "NonLinearEvolutionProblemImplementationBase: "
          "modelling hypothesis is not consistent with the spatial dimension "
          "of the mesh");
    }
    this->solver = std::make_unique<NewtonSolver>(*this);
    if (this->mgis_integrator != nullptr) {
      this->AddDomainIntegrator(this->mgis_integrator);
    }
  }  // end of NonLinearEvolutionProblemImplementation

  void NonLinearEvolutionProblemImplementation<true>::addPostProcessing(
      std::unique_ptr<PostProcessing<true>> p) {
    this->postprocessings.push_back(std::move(p));
  }  // end of addPostProcessing

  void NonLinearEvolutionProblemImplementation<true>::addPostProcessing(
      std::string_view n, const Parameters& p) {
    const auto& f = PostProcessingFactory<true>::getFactory();
    this->postprocessings.push_back(f.generate(n, *this, p));
  }  // end of addPostProcessing

  void NonLinearEvolutionProblemImplementation<true>::setLinearSolver(
      std::string_view n, const Parameters& p) {
    const auto& f = LinearSolverFactory<true>::getFactory();
    this->updateLinearSolver(f.generate(n, *this, p));
  }  // end of setLinearSolver

  void NonLinearEvolutionProblemImplementation<true>::addPostProcessing(
      const std::function<void(const real, const real)>& p) {
    this->addPostProcessing(
        std::make_unique<StdFunctionPostProcessing<true>>(p));
  }  // end of addPostProcessing

  void NonLinearEvolutionProblemImplementation<true>::executePostProcessings(
      const real t, const real dt) {
    for (auto& p : this->postprocessings) {
      p->execute(*this, t, dt);
    }
  }  // end of executePostProcessings

  const FiniteElementSpace<true>&
  NonLinearEvolutionProblemImplementation<true>::getFiniteElementSpace() const {
    return this->fe_discretization->getFiniteElementSpace<true>();
  }  // end of getFiniteElementSpace

  FiniteElementSpace<true>&
  NonLinearEvolutionProblemImplementation<true>::getFiniteElementSpace() {
    return this->fe_discretization->getFiniteElementSpace<true>();
  }  // end of getFiniteElementSpace

  bool NonLinearEvolutionProblemImplementation<true>::integrate(
      const mfem::Vector& u, const IntegrationType it) {
    if (this->mgis_integrator == nullptr) {
      return true;
    }
    const auto& pu = this->Prolongate(u);
    const auto& fespace = this->getFiniteElementSpace();
    mfem::Array<int> vdofs;
    mfem::Vector ue;
    bool noerror = true;
    for (size_type i = 0; noerror && (i != fespace.GetNE()); ++i) {
      const auto& e = *(fespace.GetFE(i));
      auto& tr = *(fespace.GetElementTransformation(i));
      fespace.GetElementVDofs(i, vdofs);
      pu.GetSubVector(vdofs, ue);
      noerror = this->mgis_integrator->integrate(e, tr, ue, it);
    }
    MPI_Allreduce(MPI_IN_PLACE, &noerror, 1, MPI_C_BOOL, MPI_LAND,
                  MPI_COMM_WORLD);
    return noerror;
  }  // end of integrate

  void NonLinearEvolutionProblemImplementation<true>::
      markDegreesOfFreedomHandledByDirichletBoundaryConditions(
          std::vector<size_type> dofs) {
    auto tmp = mfem::Array<size_type>(dofs.data(), dofs.size());
    this->SetEssentialTrueDofs(tmp);
  }  // end of markDegreesOfFreedomHandledByDirichletBoundaryConditions

  bool NonLinearEvolutionProblemImplementation<true>::solvePredictionProblem(
      mfem::Vector&, const real, const real) {
    return false;
  }  // end of solvePredictionProblem

  NonLinearEvolutionProblemImplementation<
      true>::~NonLinearEvolutionProblemImplementation() = default;

#endif /* MFEM_USE_MPI */

  NonLinearEvolutionProblemImplementation<false>::
      NonLinearEvolutionProblemImplementation(
          std::shared_ptr<FiniteElementDiscretization> fed,
          const Hypothesis h,
          const Parameters& p)
      : NonLinearEvolutionProblemImplementationBase(fed, h, p),
        mfem::NonlinearForm(&(fed->getFiniteElementSpace<false>())) {
    this->solver = std::make_unique<NewtonSolver>(*this);
    if (this->fe_discretization->getMesh<false>().Dimension() !=
        mgis::behaviour::getSpaceDimension(h)) {
      raise(
          "NonLinearEvolutionProblemImplementationBase::"
          "NonLinearEvolutionProblemImplementationBase: "
          "modelling hypothesis is not consistent with the spatial dimension "
          "of the mesh");
    }
    if (this->mgis_integrator != nullptr) {
      this->AddDomainIntegrator(this->mgis_integrator);
    }
  }  // end of NonLinearEvolutionProblemImplementation

  void NonLinearEvolutionProblemImplementation<false>::addPostProcessing(
      std::unique_ptr<PostProcessing<false>> p) {
    this->postprocessings.push_back(std::move(p));
  }  // end of addPostProcessing

  void NonLinearEvolutionProblemImplementation<false>::addPostProcessing(
      const std::function<void(const real, const real)>& p) {
    this->addPostProcessing(
        std::make_unique<StdFunctionPostProcessing<false>>(p));
  }  // end of addPostProcessing

  void NonLinearEvolutionProblemImplementation<false>::addPostProcessing(
      std::string_view n, const Parameters& p) {
    const auto& f = PostProcessingFactory<false>::getFactory();
    this->postprocessings.push_back(f.generate(n, *this, p));
  }  // end of addPostProcessing

  void NonLinearEvolutionProblemImplementation<false>::executePostProcessings(
      const real t, const real dt) {
    for (auto& p : this->postprocessings) {
      p->execute(*this, t, dt);
    }
  }  // end of executePostProcessings

  const FiniteElementSpace<false>& NonLinearEvolutionProblemImplementation<
      false>::getFiniteElementSpace() const {
    return this->fe_discretization->getFiniteElementSpace<false>();
  }  // end of getFiniteElementSpace

  FiniteElementSpace<false>&
  NonLinearEvolutionProblemImplementation<false>::getFiniteElementSpace() {
    return this->fe_discretization->getFiniteElementSpace<false>();
  }  // end of getFiniteElementSpace

  void NonLinearEvolutionProblemImplementation<false>::setLinearSolver(
      std::string_view n, const Parameters& p) {
    const auto& f = LinearSolverFactory<false>::getFactory();
    this->updateLinearSolver(f.generate(n, *this, p));
  }  // end of setLinearSolver

  bool NonLinearEvolutionProblemImplementation<false>::integrate(
      const mfem::Vector& u, const IntegrationType it) {
    if (this->mgis_integrator == nullptr) {
      return true;
    }
    const auto& pu = this->Prolongate(u);
    const auto& fespace = this->getFiniteElementSpace();
    mfem::Array<int> vdofs;
    mfem::Vector ue;
    for (size_type i = 0; i != fespace.GetNE(); ++i) {
      const auto& e = *(fespace.GetFE(i));
      auto& tr = *(fespace.GetElementTransformation(i));
      fespace.GetElementVDofs(i, vdofs);
      pu.GetSubVector(vdofs, ue);
      if (!this->mgis_integrator->integrate(e, tr, ue, it)) {
        return false;
      }
    }
    return true;
  }  // end of integrate

  void NonLinearEvolutionProblemImplementation<false>::
      markDegreesOfFreedomHandledByDirichletBoundaryConditions(
          std::vector<size_type> dofs) {
    auto tmp = mfem::Array<size_type>(dofs.data(), dofs.size());
    this->SetEssentialTrueDofs(tmp);
  }  // end of markDegreesOfFreedomHandledByDirichletBoundaryConditions

  bool NonLinearEvolutionProblemImplementation<false>::solvePredictionProblem(
      mfem::Vector& du, const real t, const real dt) {
    //     mfem::Vector r(this->u1.Size());
    //     r = 0.;
    //     //
    //     for (const auto& bc : this->dirichlet_boundary_conditions) {
    //       bc->setImposedValuesIncrements(r, t, t + dt);
    //     }
    //     // unmarked Dirichlet boundary conditions
    //     this->markDegreesOfFreedomHandledByDirichletBoundaryConditions({});
    //     // solve the tangent problem
    //     const auto ddofs = this->getEssentialDegreesOfFreedom();
    //     auto& K = this->GetGradient(this->u0);
    //     auto Kh = mfem::OperatorHandle(K, false);
    //     // eliminiate boundary conditions
    //     auto tmp = mfem::Array<size_type>(ddofs.data(), ddofs.size());
    //
    //
    //     //
    //     this->linear_solver->SetOperator(Kh);
    //     this->linear_solver->Mult(r, du);
    //     // check convergence of the linear solver
    //     const auto converged = [this] {
    //       const auto* const isolver =
    //           dynamic_cast<const
    //           mfem::IterativeSolver*>(this->linear_solver.get());
    //       if (isolver != nullptr) {
    //         return isolver.GetConverged();
    //       }
    //       return true;
    //     }();
    //     // reset Dirichlet boundary conditions
    //     this->markDegreesOfFreedomHandledByDirichletBoundaryConditions(ddofs);
    //     return converged;
    return false;
  }  // end of solvePredictionProblem

  NonLinearEvolutionProblemImplementation<
      false>::~NonLinearEvolutionProblemImplementation() = default;

}  // end of namespace mfem_mgis
