/*!
 * \file   include/MFEMMGIS/Config.hxx
 * \brief
 * \author Thomas Helfer
 * \date   19/06/2018
 */

#ifndef LIB_MFEM_MGIS_CONFIG_HXX
#define LIB_MFEM_MGIS_CONFIG_HXX

#include "MGIS/Config.hxx"
#include "MFEMMGIS/MGISForward.hxx"
#include "MFEMMGIS/MFEMForward.hxx"

#define MFEM_MGIS_VISIBILITY_LOCAL MGIS_VISIBILITY_LOCAL

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
#if defined MFEMMGIS_EXPORTS
#define MFEM_MGIS_EXPORT MGIS_VISIBILITY_EXPORT
#else /* defined MFEMMGIS_EXPORTS */
#ifndef MFEM_MGIS_STATIC_BUILD
#define MFEM_MGIS_EXPORT MGIS_VISIBILITY_IMPORT
#else /* MFEM_MGIS_STATIC_BUILD */
#define MFEM_MGIS_EXPORT
#endif /* MFEM_MGIS_STATIC_BUILD */
#endif /* defined MFEMMGIS_EXPORTS */
#else  /* defined _WIN32 || defined _WIN64 || defined __CYGWIN__ */
#define MFEM_MGIS_EXPORT MGIS_VISIBILITY_EXPORT
#endif /* */

namespace mfem_mgis {

  //! a simple alias
  using size_type = int;

  //! alias to the numeric type used
  using real = mgis::real;

  /*!
   * \brief this function can be called to report that the parallel computation
   * are not supported.
   */
  MFEM_MGIS_EXPORT [[noreturn]] void reportUnsupportedParallelComputations();

  //! \brief a simple alias
  using MainFunctionArguments = char**;

  /*!
   * \brief function that must be called to initialize `mfem-mgis`.
   * \param[in] argc: number of arguments
   * \param[in] argv: arguments
   *
   * In parallel, this function calls the `MPI_Init` function.
   * It is safe to call this function multiple time.
   */
  MFEM_MGIS_EXPORT void initialize(int&, MainFunctionArguments&);
  /*!
   * \brief function that must be called to end `mfem-mgis`
   * This call is optional if the code exits normally.
   */
  MFEM_MGIS_EXPORT void finalize();
  /*!
   * \brief function that must be called if one MPI process detect an fatal error.
   */
  MFEM_MGIS_EXPORT [[noreturn]] void abort(int error = EXIT_FAILURE);

}  // namespace mfem_mgis

#endif /* LIB_MFEM_MGIS_CONFIG_HXX */
