#ifndef VIENNACL_LINALG_TOEPLITZ_MATRIX_OPERATIONS_HPP_
#define VIENNACL_LINALG_TOEPLITZ_MATRIX_OPERATIONS_HPP_

/* =========================================================================
   Copyright (c) 2010-2013, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at
               
   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

/** @file toeplitz_matrix_operations.hpp
    @brief Implementations of operations using toeplitz_matrix. Experimental.
*/

#include "viennacl/forwards.h"
#include "viennacl/ocl/device.hpp"
#include "viennacl/ocl/handle.hpp"
#include "viennacl/ocl/kernel.hpp"
#include "viennacl/scalar.hpp"
#include "viennacl/vector.hpp"
#include "viennacl/tools/tools.hpp"
#include "viennacl/fft.hpp"

namespace viennacl
{
  namespace linalg
  {
    
    
    // A * x
    
    /** @brief Carries out matrix-vector multiplication with a toeplitz_matrix
    *
    * Implementation of the convenience expression result = prod(mat, vec);
    *
    * @param mat    The matrix
    * @param vec    The vector
    * @param result The result vector
    */
    template<class SCALARTYPE, unsigned int ALIGNMENT>
    void prod_impl(const viennacl::toeplitz_matrix<SCALARTYPE, ALIGNMENT> & mat, 
                   const viennacl::vector_base<SCALARTYPE> & vec,
                         viennacl::vector_base<SCALARTYPE> & result)
    {
      assert(mat.size1() == result.size());
      assert(mat.size2() == vec.size());
      
      viennacl::vector<SCALARTYPE> tmp(vec.size() * 4); tmp.clear();
      viennacl::vector<SCALARTYPE> tmp2(vec.size() * 4);
      
      viennacl::vector<SCALARTYPE> tep(mat.elements().size() * 2);
      viennacl::detail::fft::real_to_complex(mat.elements(), tep, mat.elements().size());


      
      copy(vec, tmp);
      viennacl::detail::fft::real_to_complex(tmp, tmp2, vec.size() * 2);
      viennacl::linalg::convolve(tep, tmp2, tmp);
      viennacl::detail::fft::complex_to_real(tmp, tmp2, vec.size() * 2);
      copy(tmp2.begin(), tmp2.begin() + vec.size(), result.begin());
    }

  } //namespace linalg



    /** @brief Implementation of the operation v1 = A * v2, where A is a matrix
    *
    * @param proxy  An expression template proxy class.
    */
    template <typename SCALARTYPE, unsigned int ALIGNMENT>
    template <unsigned int MAT_ALIGNMENT>
    viennacl::vector<SCALARTYPE, ALIGNMENT> & 
    viennacl::vector<SCALARTYPE, ALIGNMENT>::operator=(const viennacl::vector_expression< const toeplitz_matrix<SCALARTYPE, MAT_ALIGNMENT>,
                                                                                          const viennacl::vector_base<SCALARTYPE>,
                                                                                          viennacl::op_prod> & proxy) 
    {
      // check for the special case x = A * x
      if (viennacl::traits::handle(proxy.rhs()) == viennacl::traits::handle(*this))
      {
        viennacl::vector<SCALARTYPE, ALIGNMENT> result(proxy.rhs().size());
        viennacl::linalg::prod_impl(proxy.lhs(), proxy.rhs(), result);
        *this = result;
      }
      else
      {
        viennacl::linalg::prod_impl(proxy.lhs(), proxy.rhs(), *this);
      }
      return *this;
    }

    //v += A * x
    /** @brief Implementation of the operation v1 += A * v2, where A is a matrix
    *
    * @param proxy  An expression template proxy class.
    */
    template <typename SCALARTYPE, unsigned int ALIGNMENT>
    template <unsigned int MAT_ALIGNMENT>
    viennacl::vector<SCALARTYPE, ALIGNMENT> & 
    viennacl::vector<SCALARTYPE, ALIGNMENT>::operator+=(const vector_expression< const toeplitz_matrix<SCALARTYPE, MAT_ALIGNMENT>,
                                                                                 const vector_base<SCALARTYPE>,
                                                                                 op_prod> & proxy) 
    {
      vector<SCALARTYPE, ALIGNMENT> result(proxy.lhs().size1());
      viennacl::linalg::prod_impl(proxy.lhs(), proxy.rhs(), result);
      *this += result;
      return *this;
    }

    /** @brief Implementation of the operation v1 -= A * v2, where A is a matrix
    *
    * @param proxy  An expression template proxy class.
    */
    template <typename SCALARTYPE, unsigned int ALIGNMENT>
    template <unsigned int MAT_ALIGNMENT>
    viennacl::vector<SCALARTYPE, ALIGNMENT> & 
    viennacl::vector<SCALARTYPE, ALIGNMENT>::operator-=(const vector_expression< const toeplitz_matrix<SCALARTYPE, MAT_ALIGNMENT>,
                                                                                 const vector_base<SCALARTYPE>,
                                                                                 op_prod> & proxy) 
    {
      vector<SCALARTYPE, ALIGNMENT> result(proxy.get_lhs().size1());
      viennacl::linalg::prod_impl(proxy.lhs(), proxy.rhs(), result);
      *this -= result;
      return *this;
    }
    
    
    //free functions:
    /** @brief Implementation of the operation 'result = v1 + A * v2', where A is a matrix
    *
    * @param proxy  An expression template proxy class.
    */
    template <typename SCALARTYPE, unsigned int ALIGNMENT>
    template <unsigned int MAT_ALIGNMENT>
    viennacl::vector<SCALARTYPE, ALIGNMENT> 
    viennacl::vector<SCALARTYPE, ALIGNMENT>::operator+(const vector_expression< const toeplitz_matrix<SCALARTYPE, MAT_ALIGNMENT>,
                                                                                const vector_base<SCALARTYPE>,
                                                                                op_prod> & proxy) 
    {
      assert(proxy.get_lhs().size1() == base_type::size());
      vector<SCALARTYPE, ALIGNMENT> result(base_type::size());
      viennacl::linalg::prod_impl(proxy.lhs(), proxy.rhs(), result);
      result += *this;
      return result;
    }

    /** @brief Implementation of the operation 'result = v1 - A * v2', where A is a matrix
    *
    * @param proxy  An expression template proxy class.
    */
    template <typename SCALARTYPE, unsigned int ALIGNMENT>
    template <unsigned int MAT_ALIGNMENT>
    viennacl::vector<SCALARTYPE, ALIGNMENT> 
    viennacl::vector<SCALARTYPE, ALIGNMENT>::operator-(const vector_expression< const toeplitz_matrix<SCALARTYPE, MAT_ALIGNMENT>,
                                                                                const vector_base<SCALARTYPE>,
                                                                                op_prod> & proxy) 
    {
      assert(proxy.get_lhs().size1() == base_type::size());
      vector<SCALARTYPE, ALIGNMENT> result(base_type::size());
      viennacl::linalg::prod_impl(proxy.lhs(), proxy.rhs(), result);
      result = *this - result;
      return result;
    }

} //namespace viennacl


#endif
