/**
 * ______________________________________________________
 * This file is part of ko-smd-generator project.
 * 
 * @author       Mustafa Kemal Gılor <mustafagilor@gmail.com> (2016)
 * .
 * SPDX-License-Identifier:	MIT
 * ______________________________________________________
 */

#include "stdafx.h"
#include "ReferenceObject.h"

ReferenceObject::ReferenceObject() : m_refCount(0)
{
	IncRef();
}

void ReferenceObject::IncRef() 
{
	m_refCount.fetch_add(1, std::memory_order_relaxed);
}

void ReferenceObject::DecRef()
{
	if (m_refCount.fetch_sub(1, std::memory_order_release) == 1) 
	{
		std::atomic_thread_fence(std::memory_order_acquire);
		delete this;
	}
}