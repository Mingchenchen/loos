/*
  AG_numerical.cpp

  Numerical methods for AtomicGroup class
*/

/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2008, Tod D. Romo, Alan Grossfield
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <ios>
#include <sstream>
#include <iomanip>

#include <assert.h>
#include <vector>

#include <algorithm>

#include <boost/random.hpp>

#include <loos/AtomicGroup.hpp>
#include <loos/utils.hpp>


namespace loos {

  // Bounding box for all atoms in this group
  std::vector<GCoord> AtomicGroup::boundingBox(void) const {
    greal min[3] = {0,0,0}, max[3] = {0,0,0};
    const_iterator i;
    int j;
    std::vector<GCoord> res(2);
    GCoord c;

    if (atoms.size() == 0) {
      res[0] = c;
      res[1] = c;
      return(res);
    }

    for (j=0; j<3; j++)
      min[j] = max[j] = (atoms[0]->coords())[j];

    for (i=atoms.begin()+1; i != atoms.end(); i++)
      for (j=0; j<3; j++) {
        if (max[j] < ((*i)->coords())[j])
          max[j] = ((*i)->coords())[j];
        if (min[j] > ((*i)->coords())[j])
          min[j] = ((*i)->coords())[j];
      }

    c.set(min[0], min[1], min[2]);
    res[0] = c;
    c.set(max[0], max[1], max[2]);
    res[1] = c;

    return(res);
  }

  // Geometric center of the group
  GCoord AtomicGroup::centroid(void) const {
    GCoord c(0,0,0);
    const_iterator i;

    // Optimization for groups containing only one atom (such as a
    // water heavy-atom)
    if (atoms.size() == 1)
      return(atoms[0]->coords());

    for (i = atoms.begin(); i != atoms.end(); i++)
      c += (*i)->coords();

    c /= atoms.size();
    return(c);
  }


  GCoord AtomicGroup::centerOfMass(void) const {
    GCoord c(0,0,0);
    const_iterator i;

    // Optimization for groups containing only one atom (such as a
    // water heavy-atom)
    if (atoms.size() == 1) {
      return(atoms[0]->coords());
    }

    for (i=atoms.begin(); i != atoms.end(); i++) {
      c += (*i)->mass() * (*i)->coords();
    }
    c /= totalMass();
    return(c);
  }

  //* Note: this code implicitly assumes the group is neutral,
  //  since it sums based on atomic number
  GCoord AtomicGroup::centerOfElectrons(void) const {
    GCoord c(0,0,0);
    const_iterator i;
    int electrons = 0;

    for (i=atoms.begin(); i != atoms.end(); i++) {
      int an = (*i)->atomic_number();  
      c += an * (*i)->coords();
      electrons += an;
    }
    c /= electrons;
    return(c);
  }

  GCoord AtomicGroup::dipoleMoment(void) const {
    GCoord center = centroid();
    GCoord moment(0,0,0);
    const_iterator i;
    for (i=atoms.begin(); i != atoms.end(); i++) {
      moment += (*i)->charge() * ((*i)->coords() - center);
    }
    return(moment);
  }

  greal AtomicGroup::totalCharge(void) const {
    const_iterator i;
    greal charge = 0.0;

    for (i = atoms.begin(); i != atoms.end(); i++)
      charge += (*i)->charge();

    return(charge);
  }

  greal AtomicGroup::totalMass(void) const {
    const_iterator i;
    greal mass = 0.0;

    for (i = atoms.begin(); i != atoms.end(); i++)
      mass += (*i)->mass();

    return(mass);
  }

  // Geometric max radius of the group (relative to the centroid)
  greal AtomicGroup::radius(const bool use_atom_as_reference) const {
    GCoord c;
    if (use_atom_as_reference) {
      c = atoms[0]->coords();
    }
    else {
      c = centroid();
    }
    greal radius = 0.0;
    const_iterator i;

    for (i=atoms.begin(); i != atoms.end(); i++) {
      greal d = c.distance2((*i)->coords());
      if (d > radius)
        radius = d;
    }

    radius = sqrt(radius);
    return(radius);
  }



  greal AtomicGroup::radiusOfGyration(void) const {
    GCoord c = centerOfMass();
    greal radius = 0;
    const_iterator i;

    for (i = atoms.begin(); i != atoms.end(); i++)
      radius += c.distance2((*i)->coords());

    radius = sqrt(radius / atoms.size());
    return(radius);
  }


  greal AtomicGroup::rmsd(const AtomicGroup& v) {
  
    if (size() != v.size())
      throw(LOOSError("Cannot compute RMSD between groups with different sizes"));


    int n = size();
    double d = 0.0;
    for (int i = 0; i < n; i++) {
      GCoord x = atoms[i]->coords();
      GCoord y = v.atoms[i]->coords();
      d += x.distance2(y);
    }
  
    d = sqrt(d/n);

    return(d);
  }



  std::vector<GCoord> AtomicGroup::getTransformedCoords(const XForm& M) const {
    std::vector<GCoord> crds(atoms.size());
    const_iterator i;
    GMatrix W = M.current();
    int j = 0;

    for (i = atoms.begin(); i != atoms.end(); i++) {
      GCoord res = W * (*i)->coords();
      crds[j++] = res;
    }

    return(crds);
  }


  void AtomicGroup::translate(const GCoord & v) {
      iterator i;
      for (i = atoms.begin(); i != atoms.end(); i++)
          (*i)->coords() += v;
  }

  void AtomicGroup::applyTransform(const XForm& M) {
    iterator i;
    GMatrix W = M.current();

    for (i = atoms.begin(); i != atoms.end(); i++)
      (*i)->coords() = W * (*i)->coords();

  }


  void AtomicGroup::rotate(const GCoord& axis, const greal angle_in_degrees) {
    XForm M;

    GCoord center = centroid();
    M.translate(center);
    M.rotate(axis, -angle_in_degrees);
    M.translate(-center);
    applyTransform(M);
  }

  // Returns a newly allocated array of double coords in row-major
  // order...
  double* AtomicGroup::coordsAsArray(void) const {
    double *A;
    int n = size();

    A = new double[n*3];
    int k = 0;
    int i;
    for (i=0; i<n; i++) {
      A[k++] = atoms[i]->coords().x();
      A[k++] = atoms[i]->coords().y();
      A[k++] = atoms[i]->coords().z();
    }

    return(A);
  }

  // Returns a newly allocated array of double coords in row-major order
  // transformed by the current transformation.
  double* AtomicGroup::transformedCoordsAsArray(const XForm& M) const {
    double *A;
    GCoord x;
    int n = size();
    GMatrix W = M.current();

    A = new double[n*3];
    int k = 0;
    int i;
    for (i=0; i<n; i++) {
      x = W * atoms[i]->coords();
      A[k++] = x.x();
      A[k++] = x.y();
      A[k++] = x.z();
    }

    return(A);
  }



  GCoord AtomicGroup::centerAtOrigin(void) {
    GCoord c = centroid();
    iterator i;

    for (i = atoms.begin(); i != atoms.end(); i++)
      (*i)->coords() -= c;

    return(c);
  }



  void AtomicGroup::perturbCoords(const greal rms) {
    int i, n = size();
    GCoord r;

    for (i=0; i<n; i++) {
      r.random();
      r *= rms;
      atoms[i]->coords() += r;
    }
  }


}
