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



#if !defined(LOOS_DCDWRITER_HPP)
#define LOOS_DCDWRITER_HPP

#include <ios>
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <exception>
#include <vector>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <unistd.h>

#include <loos_defs.hpp>

#include <AtomicGroup.hpp>
#include <dcd.hpp>
#include <trajwriter.hpp>

namespace loos {

  //! A <I>very</I> lightweight class for writing simple DCDs
  class DCDWriter : public TrajectoryWriter {

    // Use a union to convert data to appropriate type...
    typedef union { unsigned int ui; int i; char c[4]; float f; } DataOverlay; 

  public:

    static pTrajectoryWriter create(const std::string& s, const bool append = false) {
      return(pTrajectoryWriter(new DCDWriter(s, append)));
    }

    //! Setup for writing to a file named by \a s
    /** You can opt to write the header explicitly, i.e.
     * \code
     DCDWriter dcd("output.dcd");
     dcd.setHeader(500, 10, 1e-3, no);
     dcd.setTitles("MY TITLE HERE");
     dcd.writeHeader();
     dcd.writeFrame(frame);
     \endcode
     * Or you can let dcdwriter create the header from the first frame written...
     */
    explicit DCDWriter(const std::string& s, const bool append = false) :
      TrajectoryWriter(s, append),
      _natoms(0), _nsteps(0),
      _timestep(0.001), _current(0),
      _has_box(false),
      _header_written(false)
    {
      if (appending_)
	prepareToAppend();
      else
        _titles.push_back("AUTO GENERATED BY LOOS");
    }

    //! Setup for writing to a stream
    explicit DCDWriter(std::iostream& fs, const bool append = false) : 
      TrajectoryWriter(&fs, append),
      _natoms(0), _nsteps(0), _timestep(0.001), _current(0),
      _has_box(false), _header_written(false)
    {
      if (appending_)
	prepareToAppend();
      else
	_titles.push_back("AUTO GENERATED BY LOOS");
    }

    //! Writes the coordinates of \a grps to file \a s as a DCD.
    DCDWriter(const std::string& s, const std::vector<AtomicGroup>& grps, const bool append = false) :
      TrajectoryWriter(s, append),
      _natoms(grps[0].size()),
      _nsteps(grps.size()),
      _timestep(1e-3),
      _current(0),
      _has_box(grps[0].isPeriodic()),
      _header_written(false)
    {
      if (appending_)
	prepareToAppend();
      else
        _titles.push_back("AUTO GENERATED BY LOOS");
    
      writeHeader();
      writeFrames(grps);
    }

    //! Writes coordinates of \a grps adding \a comment as a TITLE record
    DCDWriter(const std::string& s, const std::vector<AtomicGroup>& grps, const std::string& comment, const bool append = false) :
      TrajectoryWriter(s, append),
      _natoms(grps[0].size()),
      _nsteps(grps.size()),
      _timestep(1e-3),
      _current(0),
      _has_box(grps[0].isPeriodic()),
      _header_written(false)
    {
      if (appending_)
	prepareToAppend();
      else
        _titles.push_back(comment);
    
      writeHeader();
      writeFrames(grps);
    }

    //! Writes coordinates of \a grps adding \a comments as TITLE records
    DCDWriter(const std::string& s, const std::vector<AtomicGroup>& grps, const std::vector<std::string>& comments, const bool append = false) :
      TrajectoryWriter(s, append),
      _natoms(grps[0].size()),
      _nsteps(grps.size()),
      _timestep(1e-3),
      _current(0),
      _has_box(grps[0].isPeriodic()),
      _header_written(false)
    {
      _titles = comments;

      writeHeader();
      writeFrames(grps);
    }

    ~DCDWriter() {
    }


    //! Sets header parameters
    /** These must be set prior to writing a header or frame, but it is
     *  not an error to not do so...
     *  Arguments:
     *  \arg \c na Number of atoms
     *  \arg \c ns Number of steps (total frames)
     *  \arg \c ts Timestep of each frame
     *  \arg \c bf Flag for whether or not each frame will include periodic box info as Xtal data
     */
    void setHeader(const int na, const int ns, const greal ts, const bool bf) {
      if (_header_written)
        throw(std::logic_error("Cannot set header after having written it"));

      _natoms = na;
      _nsteps = ns;
      _timestep = ts;
      _has_box = bf;
    }

    void setTitles(const std::vector<std::string>& titles) {
      if (_header_written)
        throw(std::logic_error("Cannot set header after having written it"));
      _titles = titles;
    }

    void setTitle(const std::string& s) {
      if (_header_written)
        throw(std::logic_error("Cannot set header after having written it"));
      _titles.clear(); addTitle(s);
    }

    void addTitle(const std::string& s) {
      if (_header_written)
        throw(std::logic_error("Cannot set header after having written it"));
      _titles.push_back(s);
    }

    void setComments(const std::vector<std::string>& comments) { setTitles(comments); }
    bool hasComments() const { return(true); }
    
    //! Writes a frame to a growing DCD
    /** writeFrame() will automatically extend the DCD for you if you
     *  write past the initially specified number of frames.
     *  Alternatively, you can just begin writing frames without
     *  explicitly writing a header and let writeFrame() handle it for
     *  you.  As the DCD grows, writeFrame() will automatically update
     *  the header information for you.
     */
    void writeFrame(const AtomicGroup& grp);


    //! Same as writeFrame(), but writes out the vector of frames...
    void writeFrames(const std::vector<AtomicGroup>& grps);

    void writeHeader(void);

    uint framesWritten(void) const { return(_current); }

  private:
    void writeF77Line(const char* const data, const unsigned int len); 
    std::string fixStringSize(const std::string& s, const unsigned int size);
    void writeBox(const GCoord& box);

    void prepareToAppend();


  private:
    uint _natoms, _nsteps;
    greal _timestep;
    uint _current;
    bool _has_box;
    bool _header_written;
    std::vector<std::string> _titles;
  };

}


#endif
