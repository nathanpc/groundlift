/**
 * Exceptions/GroundLift.cpp
 * GroundLift exception C++ wrapper.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "GroundLift.h"

#include <utils/logging.h>

using namespace GroundLift;

/**
 * Frees up any resources allocated for the exception.
 */
Exception::~Exception() {
	gl_error_free(err);
}
