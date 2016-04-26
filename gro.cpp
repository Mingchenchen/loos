/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2009, Tod D. Romo, Alan Grossfield
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



#include <gro.hpp>
#include <utils.hpp>
#include <Fmt.hpp>



namespace loos {
  
  void Gromacs::read(std::istream& ifs) {
    std::string buf;

    getline(ifs, title_);

    // Get the # of atoms;;;
    getline(ifs, buf);
    int natoms = parseStringAs<int>(buf);
    while (natoms-- > 0) {
      getline(ifs, buf);
      
      int resid = parseStringAs<int>(buf, 0, 5);
      std::string resname = parseStringAs<std::string>(buf, 5, 5);
      std::string name = parseStringAs<std::string>(buf, 10, 5);
      int atomid = parseStringAs<int>(buf, 15, 5);
      float x = parseStringAs<float>(buf, 20, 8) * 10.0;
      float y = parseStringAs<float>(buf, 28, 8) * 10.0;
      float z = parseStringAs<float>(buf, 36, 8) * 10.0;
      
      // We ignore velocities...
      pAtom pa(new Atom);
      pa->index(_max_index++);
      pa->resid(resid);
      pa->id(atomid);
      pa->resname(resname);
      pa->name(name);
      pa->coords(GCoord(x,y,z));

      append(pa);
    }

    // Now process box...
    getline(ifs, buf);
    std::istringstream iss(buf);
    GCoord box;
    if (!(iss >> box[0] >> box[1] >> box[2]))
      throw(FileReadError(_filename, "Cannot parse box '" + buf + "'"));
    periodicBox(box * 10.0);

    // Since the atomic field in .gro files is only 5-chars wide, it can
    // overflow.  if there are enough atoms to cause an overflow, manually
    // renumber everything...
    if (atoms.size() >= 100000)
      renumber();
  }

  std::string Gromacs::atomAsString(const pAtom p) const {
    std::ostringstream s;

    // Float formatter for coords
    Fmt crdfmt(3);
    crdfmt.width(8);
    crdfmt.right();
    crdfmt.trailingZeros(true);
    crdfmt.fixed();

    // Float formatter for velocities
    Fmt velfmt(4);
    velfmt.width(8);
    velfmt.right();
    velfmt.trailingZeros(true);
    velfmt.fixed();

    s << std::setw(5) << p->resid();
    s << std::left << std::setw(5) << p->resname();
    s << std::right << std::setw(5) << p->name();
    s << std::setw(5) << p->id();
    s << crdfmt(p->coords().x()/10.);
    s << crdfmt(p->coords().y()/10.);
    s << crdfmt(p->coords().z()/10.);
    s << velfmt(0.0);
    s << velfmt(0.0);
    s << velfmt(0.0);

    return(s.str());
    
  }

  //! Build a GRO from an AtomicGroup
  //*
  //  Note: if the AtomicGroup doesn't have periodicity information,
  //        the GRO file gets a large generic periodic box.  This is 
  //        to facilitate writing GRO files, where the box line seems 
  //        to be mandatory.
  //
  Gromacs Gromacs::fromAtomicGroup(const AtomicGroup& g) {
    Gromacs p(g);
    
    // GRO files need a periodicity line
    if (!g.isPeriodic()) {
        p.periodicBox(9999., 9999., 9999.);
    }

    p.title_ = std::string("Generic title inserted here");

    return(p);
  }

  

  //! Output the group as a GRO...

  std::ostream& operator<<(std::ostream& os, const Gromacs& g) {
      AtomicGroup::const_iterator i;
      
      os << g.title() << std::endl;
      os << g.size() << std::endl;
      for (i=g.atoms.begin(); i != g.atoms.end(); ++i) {
          os << g.atomAsString(*i) << std::endl;
      }

      GCoord box = g.periodicBox();
      box /= 10.0;
      os << box.x() << "  " << box.y() << "  " << box.z() << std::endl;
      
          
      return(os);
  }
}
