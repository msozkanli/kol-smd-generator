/**
 * ______________________________________________________
 * This file is part of ko-smd-generator project.
 * 
 * @author       Mustafa Kemal Gılor <mustafagilor@gmail.com> (2016)
 * .
 * SPDX-License-Identifier:	MIT
 * ______________________________________________________
 */

#pragma once
#include <atomic>
#include "types.h"


class ReferenceObject
{
public:
	ReferenceObject();

	void IncRef();

	void DecRef();

	virtual ~ReferenceObject() {}
   
private:
	mutable std::atomic<uint32> m_refCount;
};