/***************************************************************
FronTier is a set of libraries that implements different types of 
Front Traking algorithms. Front Tracking is a numerical method for 
the solution of partial differential equations whose solutions have 
discontinuities.  

Copyright (C) 1999 by The University at Stony Brook. 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
****************************************************************/

/*! \file uapi.h
    
    \brief The uapi.h contains the functions used as utilities.
 */

/*! \defgroup SAMPLE   Sampling Functions
 **/

#include <util/cdecs.h>

                /* Utility Function Prototypes*/

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*! \defgroup DEBUG    Utility of Debugging Functions
 **/

/*! \fn void U_SetExcutableName(const char *exname)
 *  \ingroup INITIALIZATION
    \brief Initialize strings for debugging option of the problem,
     read the input file.
    \param exname @b in The name of the excutable
 */
   IMPORT  void U_SetExcutableName(const char *exname);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
