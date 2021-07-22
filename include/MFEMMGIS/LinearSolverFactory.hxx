/*!
 * \file   include/MFEMMGIS/LinearSolverFactory.hxx
 * \brief
 * \author Thomas Helfer
 * \date   24/03/2021
 */

#ifndef LIB_MFEM_MGIS_LINEARSOLVERFACTORY_HXX
#define LIB_MFEM_MGIS_LINEARSOLVERFACTORY_HXX

#include <map>
#include <memory>
#include <functional>
#include "MFEMMGIS/Config.hxx"

namespace mfem_mgis {

  // forward declaration
  struct Parameters;
  // forward declaration
  template <bool parallel>
  struct NonLinearEvolutionProblemImplementation;

  /*!
   * \brief result from the linear solver factories
   */
  struct LinearSolverHandler {
    std::unique_ptr<LinearSolver> linear_solver;
    std::unique_ptr<LinearSolverPreconditioner> preconditioner;
  };  // end of LinearSolverHandler

  /*!
   * \brief an abstract factory for behaviour integrators
   * \tparam parallel: boolean stating if parallel post-processing are
   * considered
   */
  template <bool parallel>
  struct LinearSolverFactory;

#ifdef MFEM_USE_MPI

  //! \brief partial specialisation in parallel
  template <>
  struct MFEM_MGIS_EXPORT LinearSolverFactory<true> {
    //! a simple alias
    using Generator = std::function<LinearSolverHandler(
        NonLinearEvolutionProblemImplementation<true>&, const Parameters&)>;
    //! \return the unique instance of the class
    static LinearSolverFactory& getFactory();
    /*!
     * \brief register a new post-processing
     * \param[in] n: name of the post-processing
     * \param[in] g: generator of the post-processing
     */
    void add(std::string_view, Generator);
    /*!
     * \return the requested post-processing
     * \param[in] n: name of the post-processing
     * \param[in] p: problem to be solved
     * \param[in] params: parameters passed to the post-processing
     */
    LinearSolverHandler generate(std::string_view,
                                 NonLinearEvolutionProblemImplementation<true>&,
                                 const Parameters&) const;

   private:
    //! \brief default destructor
    LinearSolverFactory();
    //! \brief destructor
    ~LinearSolverFactory();
    //! \brief registred factories
    std::map<std::string, Generator, std::less<>> generators;
  };  // end of struct LinearSolverFactory

#endif /* MFEM_USE_MPI */

  //! \brief partial specialisation in sequential
  template <>
  struct MFEM_MGIS_EXPORT LinearSolverFactory<false> {
    //! a simple alias
    using Generator = std::function<LinearSolverHandler(
        NonLinearEvolutionProblemImplementation<false>&, const Parameters&)>;
    //! \return the unique instance of the class
    static LinearSolverFactory& getFactory();
    /*!
     * \brief register a new post-processing
     * \param[in] n: name of the post-processing
     * \param[in] g: generator of the post-processing
     */
    void add(std::string_view, Generator);
    /*!
     * \return the requested post-processing
     * \param[in] n: name of the post-processing
     * \param[in] p: problem to be solved
     * \param[in] params: parameters passed to the post-processing
     */
    LinearSolverHandler generate(
        std::string_view,
        NonLinearEvolutionProblemImplementation<false>&,
        const Parameters&) const;

   private:
    //! \brief default destructor
    LinearSolverFactory();
    //! \brief destructor
    ~LinearSolverFactory();
    //! \brief registred factories
    std::map<std::string, Generator, std::less<>> generators;
  };  // end of struct LinearSolverFactory

}  // end of namespace mfem_mgis

#endif /* LIB_MFEM_MGIS_LINEARSOLVERFACTORY_HXX */
