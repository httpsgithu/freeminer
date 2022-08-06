/*
util/pointer.h
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
*/

/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "irrlichttypes.h"
#include "debug.h" // For assert()
#include <cstring>
#include <memory> // std::shared_ptr


template<typename T> class ConstSharedPtr {
public:
	ConstSharedPtr(T *ptr) : ptr(ptr) {}
	ConstSharedPtr(const std::shared_ptr<T> &ptr) : ptr(ptr) {}

	const T* get() const noexcept { return ptr.get(); }
	const T& operator*() const noexcept { return *ptr.get(); }
	const T* operator->() const noexcept { return ptr.get(); }

private:
	std::shared_ptr<T> ptr;
};

template <typename T>
class Buffer
{
public:
	Buffer()
	{
		m_size = 0;
		data = nullptr;
	}
	Buffer(unsigned int size)
	{
		m_size = size;
		if(size != 0)
			data = new T[size];
		else
			data = nullptr;
	}

	// Disable class copy
	Buffer(const Buffer &) = delete;
	Buffer &operator=(const Buffer &) = delete;

	Buffer(Buffer &&buffer)
	{
		m_size = buffer.m_size;
		if(m_size != 0)
		{
			data = buffer.data;
			buffer.data = nullptr;
			buffer.m_size = 0;
		}
		else
			data = nullptr;
	}
	// Copies whole buffer
	Buffer(const T *t, unsigned int size)
	{
		m_size = size;
		if(size != 0)
		{
			data = new T[size];
			memcpy(data, t, size);
		}
		else
			data = nullptr;
	}

	~Buffer()
	{
		drop();
	}

	Buffer& operator=(Buffer &&buffer)
	{
		if(this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		if(m_size != 0)
		{
			data = buffer.data;
			buffer.data = nullptr;
			buffer.m_size = 0;
		}
		else
			data = nullptr;
		return *this;
	}

	void copyTo(Buffer &buffer) const
	{
		buffer.drop();
		buffer.m_size = m_size;
		if (m_size != 0) {
			buffer.data = new T[m_size];
			memcpy(buffer.data, data, m_size);
		} else {
			buffer.data = nullptr;
		}
	}

	T & operator[](unsigned int i) const
	{
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}

	unsigned int getSize() const
	{
		return m_size;
	}

private:
	void drop()
	{
		delete[] data;
	}
	T *data;
	unsigned int m_size;
};

/************************************************
 *           !!!  W A R N I N G  !!!            *
 *                                              *
 * This smart pointer class is NOT thread safe. *
 * ONLY use in a single-threaded context!       *
 *                                              *
 ************************************************/
template <typename T>
class SharedBuffer
{
public:
	SharedBuffer()
	{
		m_size = 0;
		data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	SharedBuffer(unsigned int size)
	{
		m_size = size;
		if(m_size != 0)
		{
			data = new T[m_size];
			memset(data,0,sizeof(T)*m_size);
		}
		else
			data = nullptr;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	SharedBuffer(const SharedBuffer &buffer)
	{
		m_size = buffer.m_size;
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
	}
	SharedBuffer & operator=(const SharedBuffer & buffer)
	{
		if(this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
		return *this;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const T *t, unsigned int size)
	{
		m_size = size;
		if(m_size != 0)
		{
			data = new T[m_size];
			memcpy(data, t, m_size);
		}
		else
			data = nullptr;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const Buffer<T> &buffer)
	{
		m_size = buffer.getSize();
		if (m_size != 0) {
				data = new T[m_size];
				memcpy(data, *buffer, buffer.getSize());
		}
		else
			data = nullptr;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	~SharedBuffer()
	{
		drop();
	}
	T & operator[](unsigned int i) const
	{
		assert(i < m_size);
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}
	unsigned int getSize() const
	{
		return m_size;
	}
	operator Buffer<T>() const
	{
		return Buffer<T>(data, m_size);
	}
private:
	void drop()
	{
		if((*refcount) == 0)
			return; //whoops
		(*refcount)--;
		if(*refcount == 0)
		{
			delete[] data;
			delete refcount;
		}
	}
	T *data;
	unsigned int m_size;
	unsigned int *refcount;
};

template<class T>
class sloppy {}; 

// convert between T** and const T** 
template<class T>
class sloppy<T**>
{
    T** t;
    public: 
    sloppy(T** mt) : t(mt) {}
    sloppy(const T** mt) : t(const_cast<T**>(mt)) {}

    operator T** () const { return t; }
    operator const T** () const { return const_cast<const T**>(t); }
};
