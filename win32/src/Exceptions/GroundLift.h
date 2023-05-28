/**
 * Exceptions/GroundLift.h
 * GroundLift exception C++ wrapper.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_EXCEPTIONS_GROUNDLIFT_H
#define _WINGL_EXCEPTIONS_GROUNDLIFT_H

#if _MSC_VER > 1000
#pragma once
#endif	// _MSC_VER > 1000

#include <stdexcept>
#include <groundlift/error.h>

namespace GroundLift {

/**
 * GroundLift exception C++ wrapper.
 */
class Exception : public std::exception {
private:
	gl_err_t *err;

public:
	Exception(gl_err_t *err) :
		std::exception(err->msg) {
		this->err = err;
	};
	virtual ~Exception();
};

}  // namespace GroundLift

#endif	// _WINGL_EXCEPTIONS_GROUNDLIFT_H
