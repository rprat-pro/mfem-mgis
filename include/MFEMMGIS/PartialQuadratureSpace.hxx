/*!
 * \file   PartialQuadratureSpace.hxx
 * \brief
 * \author Thomas Helfer
 * \date   11/06/2020
 */

#ifndef LIB_MFEM_MGIS_PARTIALQUADRATURESPACE_HXX
#define LIB_MFEM_MGIS_PARTIALQUADRATURESPACE_HXX

#include <memory>
#include <variant>
#include <functional>
#include <unordered_map>
#include "MFEMMGIS/Config.hxx"

namespace mfem_mgis {

  // forward declaration
  struct FiniteElementDiscretization;

  /*!
   * \brief a space on quadrature points defined on a material
   */
  struct MFEM_MGIS_EXPORT PartialQuadratureSpace {
    /*!
     * \brief throw an exception in case of invalid offset
     * \param[in] id: material identifier
     * \param[in] i: element number
     */
    [[noreturn]] static void treatInvalidOffset(const size_type,
                                                const size_type);
    /*!
     * \brief constructor
     * \param[in] fed: finite element discretization.
     * \param[in] m: material attribute.
     * \param[in] irs: function returning the order of quadrature for the
     * considered finite element.
     */
    PartialQuadratureSpace(const FiniteElementDiscretization &,
                           const size_type,
                           const std::function<const mfem::IntegrationRule &(
                               const mfem::FiniteElement &,
                               const mfem::ElementTransformation &)> &);
    //! \return the finite element discretization
    const FiniteElementDiscretization &getFiniteElementDiscretization() const;
    /*!
     * \return the integration ruel associated with the given finite element and
     * element transformation
     * \param[in] e: finite element
     * \param[in] tr: element transformation
     */
    const mfem::IntegrationRule &getIntegrationRule(
        const mfem::FiniteElement &, const mfem::ElementTransformation &) const;
    /*!
     * \brief return the number of element associated with this material
     * identifier
     */
    size_type getNumberOfElements() const;
    //! \brief return the number of integration points
    size_type getNumberOfIntegrationPoints() const;
    /*!
     * \brief return the offset associated with an element
     * \param[in] i: element number (global numbering)
     */
    size_type getOffset(const size_type) const;
    //! \return the material id
    size_type getId() const;
    //! \brief destructor
    ~PartialQuadratureSpace();

   private:
    //! \brief underlying finite element discretization
    const FiniteElementDiscretization &fe_discretization;
    /*!
     * \brief function returning the order of quadrature for the
     * considered finite element.
     */
    std::function<const mfem::IntegrationRule &(
        const mfem::FiniteElement &, const mfem::ElementTransformation &)>
        integration_rule_selector;
    //! \brief offsets associated with elements
    std::unordered_map<size_type,  // element number (global numbering)
                       size_type>  // offset
        offsets;
    //! \brief material identifier
    size_type id;
    //! \brief number of integration points
    size_type ng;
  };  // end of struct QuadratureSpace

}  // namespace mfem_mgis

#include "MFEMMGIS/PartialQuadratureSpace.ixx"

#endif /* LIB_MFEM_MGIS_PARTIALQUADRATURESPACE_HXX */
