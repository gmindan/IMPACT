//
//  Copyright@2013, Illinois Rocstar LLC. All rights reserved.
//
//  See LICENSE file included with this source or
//  (opensource.org/licenses/NCSA) for license information.
//

#include "Element_accessors.hpp"
#include "Generic_element_2.h"
#include "Rocsurf.h"

SURF_BEGIN_NAMESPACE

/** Adpator for accessing constant scalar node-centered data.
 *  T is typically Real.
 */
template <class T>
class Nodal_scalar_const_2d {
 public:
  typedef T Base_type;
  typedef const T Value;
  typedef T Value_nonconst;
  typedef const T Value_const;
  typedef unsigned int Size;

  /// Constructor
  Nodal_scalar_const_2d(int i, int j) : nc(i), c(j) {}
  void set_col(int j) { c = j; }

  /// Dimension of the dataitem.
  Size dimension() const { return 1; }

  /// Get the ith data item. Here the index i uses Fortran convention.
  T get_value(const Base_type *buf, int r) const {
    assert(r >= 1);
    return buf[(r - 1) * nc + c];
  }

 private:
  const int nc;
  int c;
};

/** Adpator for element-wise data container.
 *  It provides an interface to access nodal dataitems using
 *  element-wise local indices. Here _Cont is usually Nodal_scalar_const_2d.
 */
template <class _Cont>
class Field {
 public:
  typedef typename _Cont::Base_type Base_type;
  typedef typename _Cont::Value Value;
  typedef typename _Cont::Value_nonconst Value_nonconst;
  typedef typename _Cont::Value_const Value_const;
  typedef unsigned int Size;

 public:
  /** Constructor.
   * \param cont is a wrapper for accessing nodal data.
   * \param p points to the pane-wise array of the nodal data.
   * \param ids is a node-ID enumerator for the element.
   */
  Field(_Cont &cont, Value *p, const Element_node_enumerator &ids)
      : _cont(cont), _p(p), _enum(ids) {}
  /// Dimension of each dataitem.
  Size dimension() const { return _cont.dimension(); }
  /// Obtain a const reference to the nodal data associated
  /// with ith node of the element.
  Value_const operator[](int i) const { return _cont.get_value(_p, _enum[i]); }
  /// Obtain a non-const reference to the nodal data associated
  /// with ith node of the element.
  Value_nonconst operator[](int i) { return _cont.get_value(_p, _enum[i]); }

 private:
  _Cont &_cont;
  Value *_p;
  const Element_node_enumerator &_enum;
};

// Interpolate node-centered data x to element-centered data z.
// Now support only double precision data.
void Rocsurf::interpolate_to_centers(const COM::DataItem *x, COM::DataItem *z) {
  assert(x->is_nodal() && z->is_elemental() &&
         x->size_of_components() == z->size_of_components());
  assert(x->data_type() == COM_DOUBLE ||
         x->data_type() == COM_DOUBLE_PRECISION);
  assert(z->data_type() == COM_DOUBLE ||
         z->data_type() == COM_DOUBLE_PRECISION);

  std::vector<const COM::Pane *> xpanes;
  std::vector<COM::Pane *> zpanes;

  x->window()->panes(xpanes);
  z->window()->panes(zpanes);

  std::vector<COM::Pane *>::iterator zit, zend;
  std::vector<const COM::Pane *>::const_iterator xit;

  const int ndim = x->size_of_components();
  // Loop through all the panes.
  for (zit = zpanes.begin(), zend = zpanes.end(), xit = xpanes.begin();
       zit != zend; ++zit, ++xit) {
    // Loop for each dimension.
    for (int i = 0; i < ndim; ++i) {
      COM::DataItem *z_pa =
          (*zit)->dataitem(z->id() + ((ndim > 1) ? i + 1 : 0));
      Real *zval = reinterpret_cast<Real *>(z_pa->pointer());
      int zstrd = z_pa->stride();

      const COM::DataItem *x_pa =
          (*xit)->dataitem(x->id() + ((ndim > 1) ? i + 1 : 0));
      const Real *xval = reinterpret_cast<const Real *>(x_pa->pointer());

      Nodal_scalar_const_2d<Real> X(x_pa->stride(), 0);
      Element_node_enumerator ene(*xit, 1);
      Field<Nodal_scalar_const_2d<Real> > f(X, xval, ene);

      for (int j = (*xit)->size_of_elements(); j > 0;
           --j, ene.next(), zval += zstrd)
        Generic_element_2(ene.size_of_edges(), ene.size_of_nodes())
            .interpolate_to_center(f, zval);
    }
  }
}

SURF_END_NAMESPACE
