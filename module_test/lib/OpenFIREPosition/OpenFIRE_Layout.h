/*!
 * @file OpenFIRE_Square.h
 * @brief Light Gun library for 4 LED setup
 * @n CPP file for Samco Light Gun 4 LED setup
 *
 * @copyright Samco, https://github.com/samuelballantyne, 2024
 * @copyright GNU Lesser General Public License
 *
 * @author [Sam Ballantyne](samuelballantyne@hotmail.com)
 * @version V1.0
 * @date 2024
 */

#ifndef _OpenFIRE_Layout_h_
#define _OpenFIRE_Layout_h_

#include <stdint.h>

#include "OpenFIREConst.h"

class OpenFIRE_Layout {
   public:
    /// @brief Main function to calculate X, Y, and H
    virtual void         begin(const int* px, const int* py, unsigned int seen) = 0;
    virtual int          X(int index) const                                     = 0;
    virtual int          Y(int index) const                                     = 0;
    virtual unsigned int testSee(int index) const                               = 0;
    virtual int          testMedianX() const                                    = 0;
    virtual int          testMedianY() const                                    = 0;

    /// @brief Height
    virtual float H() const = 0;

    /// @brief Height
    virtual float W() const = 0;

    /// @brief Angle
    virtual float Ang() const = 0;

    /// @brief Bit mask of positions the camera saw
    virtual unsigned int seen() const = 0;
};

#endif