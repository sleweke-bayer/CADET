// =============================================================================
//  CADET - The Chromatography Analysis and Design Toolkit
//  
//  Copyright © 2008-2017: The CADET Authors
//            Please see the AUTHORS and CONTRIBUTORS file.
//  
//  All rights reserved. This program and the accompanying materials
//  are made available under the terms of the GNU Public License v3.0 (or, at
//  your option, any later version) which accompanies this distribution, and
//  is available at http://www.gnu.org/licenses/gpl.html
// =============================================================================

#ifndef HDF5WRITER_HPP_
#define HDF5WRITER_HPP_

#include <vector>
#include <string>
#include <cstring>

#include "cadet/cadetCompilerInfo.hpp"
#include "common/CompilerSpecific.hpp"
#include "HDF5Base.hpp"

namespace cadet
{

namespace io
{

class HDF5Writer : public HDF5Base
{
public:
	/// \brief Constructor
	HDF5Writer();

	/// \brief Destructor
	~HDF5Writer() CADET_NOEXCEPT;

	/// \brief Write data from C-array to a dataset
	template <typename T>
	void write(const std::string& dataSetName, const size_t rank, const size_t* dims, const T* buffer);

	/// \brief Write data from C-array to a dataset
	template <typename T>
	void write(const std::string& dataSetName, const size_t rank, const size_t* dims, const T* buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing tensors from C-array
	template <typename T>
	void tensor(const std::string& dataSetName, const size_t rank, const size_t* dims, const T* buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing tensors from std::vector
	template <typename T>
	void tensor(const std::string& dataSetName, const size_t rank, const size_t* dims, const std::vector<T>& buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing matrices from C-array
	template <typename T>
	void matrix(const std::string& dataSetName, const size_t rows, const size_t cols, const T* buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing matrices from std::vector
	template <typename T>
	void matrix(const std::string& dataSetName, const size_t rows, const size_t cols, const std::vector<T>& buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing vectors from C-array
	template <typename T>
	void vector(const std::string& dataSetName, const size_t length, const T* buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing vectors from std::vector
	template <typename T>
	void vector(const std::string& dataSetName, const std::vector<T>& buffer, const size_t stride = 1);

	/// \brief Convenience wrapper for writing scalars
	template <typename T>
	void scalar(const std::string& dataSetName, const T buffer);

	/// \brief Removes an existing group from the file
	inline void unlinkGroup(const std::string& groupName);

	/// \brief Removes an existing dataset from the current group
	inline void unlinkDataset(const std::string& dsName);

	/// \brief Enable/disable compression for tensors of 2nd order and above
	inline void compressFields(bool setCompression) {_writeCompressed = setCompression;}

	/// \brief Tensors of 2nd order (matrices) and above are written as extendible fields
	///        (maxsize = unlimited, chunked layout), when set to true.
	inline void extendibleFields(bool setExtendible) {_writeExtendible = setExtendible;}

private:

	void writeWork(const std::string& dataSetName, hid_t memType, hid_t fileType, const size_t rank, const size_t* dims, const void* buffer, const size_t stride);

	bool                    _writeScalar;
	bool                    _writeExtendible;
	bool                    _writeCompressed;
	hsize_t*                _maxDims;
	hsize_t*                _chunks;
	double                  _chunkFactor;
};


HDF5Writer::HDF5Writer() :
		_writeScalar(false),
		_writeExtendible(true),
		_writeCompressed(false),
		_maxDims(NULL),
		_chunks(NULL),
		_chunkFactor(1.5)
{}

HDF5Writer::~HDF5Writer() CADET_NOEXCEPT { }


// ============================================================================================================
//   Template specializations of member function write() for diffenet data types
// ============================================================================================================
template <>
void HDF5Writer::write<double>(const std::string& dataSetName, const size_t rank, const size_t* dims, const double* buffer, const size_t stride)
{
	writeWork(dataSetName, H5T_NATIVE_DOUBLE, H5T_IEEE_F64LE, rank, dims, buffer, stride);
}

template <>
void HDF5Writer::write<int>(const std::string& dataSetName, const size_t rank, const size_t* dims, const int* buffer, const size_t stride)
{
	writeWork(dataSetName, H5T_NATIVE_INT, H5T_STD_I32LE, rank, dims, buffer, stride);
}

template <>
void HDF5Writer::write<uint64_t>(const std::string& dataSetName, const size_t rank, const size_t* dims, const uint64_t* buffer, const size_t stride)
{
	writeWork(dataSetName, H5T_NATIVE_UINT64, H5T_STD_I32LE, rank, dims, buffer, stride);
}

template <>
void HDF5Writer::write<std::string>(const std::string& dataSetName, const size_t rank, const size_t* dims, const std::string* buffer, const size_t stride)
{
	hid_t dataType = H5Tcopy(H5T_C_S1);
	H5Tset_size(dataType, H5T_VARIABLE);

	size_t bufSize = 1;
	for (size_t i = 0; i < rank; ++i)
	{
		bufSize *= dims[i];
	}

	// Copy strings to char*
	char** strBuffer = new char*[bufSize];
	for (size_t i = 0; i < bufSize; ++i)
	{
		strBuffer[i] = new char[buffer[i * stride].size() + 1]; // +1 for null-terminating character
		strcpy(strBuffer[i], buffer[i * stride].c_str());
	}

	writeWork(dataSetName, dataType, dataType, rank, dims, strBuffer, 1);

	H5Tclose(dataType);

	// Memory cleanup
	for (size_t i = 0; i < bufSize; ++i)
		delete[] strBuffer[i];
	delete[] strBuffer;
}

// Template that matches on every unsupported type and throws an exception
template <typename T>
void HDF5Writer::write(const std::string& dataSetName, const size_t rank, const size_t* dims, const T* buffer)
{
	throw IOException("You may not try to write an unsupported type");
}

template <typename T>
void HDF5Writer::write(const std::string& dataSetName, const size_t rank, const size_t* dims, const T* buffer, const size_t stride)
{
	throw IOException("You may not try to write an unsupported type");
}
// ============================================================================================================


// ============================================================================================================
//   Convenience wrappers
// ============================================================================================================
template <typename T>
void HDF5Writer::tensor(const std::string& dataSetName, const size_t rank, const size_t* dims, const std::vector<T>& buffer, const size_t stride)
{
#ifdef DEBUG
	size_t bufSize = 1;
	for (size_t i = 0; i < rank; ++i)
		bufSize *= dims[i];
	cadet_assert(bufSize <= buffer.size());
#endif
	write<T>(dataSetName, rank, dims, buffer.data());
}

template <typename T>
void HDF5Writer::tensor(const std::string& dataSetName, const size_t rank, const size_t* dims, const T* buffer, const size_t stride)
{
	write<T>(dataSetName, rank, dims, buffer, stride);
}

template <typename T>
void HDF5Writer::matrix(const std::string& dataSetName, const size_t rows, const size_t cols, const T* buffer, const size_t stride)
{
	size_t dims[2] = {rows, cols};
	write<T>(dataSetName, 2, dims, buffer, stride);
}

template <typename T>
void HDF5Writer::matrix(const std::string& dataSetName, const size_t rows, const size_t cols, const std::vector<T>& buffer, const size_t stride)
{
	cadet_assert(rows*cols <= buffer.size());
	size_t dims[2] = {rows, cols};
	write<T>(dataSetName, 2, dims, buffer.data(), stride);
}

template <typename T>
void HDF5Writer::vector(const std::string& dataSetName, const size_t length, const T* buffer, const size_t stride)
{
	write<T>(dataSetName, 1, &length, buffer, stride);
}

template <typename T>
void HDF5Writer::vector(const std::string& dataSetName, const std::vector<T>& buffer, const size_t stride)
{
	size_t length = buffer.size() / stride;
	write<T>(dataSetName, 1, &length, buffer.data(), stride);
}

template <typename T>
void HDF5Writer::scalar(const std::string& dataSetName, const T buffer)
{
	_writeScalar = true;
	vector<T>(dataSetName, 1, &buffer);
}
// ============================================================================================================


void HDF5Writer::unlinkGroup(const std::string& groupName)
{
	H5Ldelete(_file, groupName.c_str(), H5P_DEFAULT);
}


void HDF5Writer::unlinkDataset(const std::string& dsName)
{
	bool wasOpen = !_groupsOpened.empty();

	if (!wasOpen)
		openGroup(false);

	H5Ldelete(_groupsOpened.top(), dsName.c_str(), H5P_DEFAULT);

	if (!wasOpen)
		closeGroup();
}


void HDF5Writer::writeWork(const std::string& dataSetName, hid_t memType, hid_t fileType, const size_t rank, const size_t* dims, const void* buffer, const size_t stride)
{
	hid_t propList = H5Pcreate(H5P_DATASET_CREATE);
	hid_t dataSpace;
	if (!_writeScalar)
	{
		if (_writeExtendible || _writeCompressed) // we need chunking
		{
			_chunks  = new hsize_t[rank];
			for (size_t i = 0; i < rank; ++i)
				_chunks[i] = (_writeExtendible) ? static_cast<hsize_t>(dims[i] * _chunkFactor) : dims[i]; // leave some space in all dims, if extendible

			H5Pset_chunk(propList, rank, _chunks);
			delete[] _chunks;
		}

		_maxDims = new hsize_t[rank];
		if (_writeExtendible) // we set maxdims unlimited
		{
			for (size_t i = 0; i < rank; ++i)
				_maxDims[i] = H5S_UNLIMITED;
		}
		else // we set maxdims to dims of the buffer
		{
			for (size_t i = 0; i < rank; ++i)
				_maxDims[i] = dims[i];
		}

		hsize_t* convDims = new hsize_t[rank];
		for (size_t i = 0; i < rank; ++i)
		{
			convDims[i] = dims[i];
		}

		dataSpace = H5Screate_simple(rank, convDims, _maxDims);
		delete[] convDims;
		delete[] _maxDims;

		if (_writeCompressed) // enable compression
			H5Pset_deflate(propList, 9);
	}
	else // reset _writeScalar
	{
		// init dataspace as scalar and reset state variable
		dataSpace = H5Screate(H5S_SCALAR);
		_writeScalar = false;
	}

	// Create dataset
	openGroup(true);
	const hid_t dataSet = H5Dcreate2(_groupsOpened.top(), dataSetName.c_str(), fileType, dataSpace, H5P_DEFAULT, propList, H5P_DEFAULT);
	closeGroup();

	// Write data
	if (stride <= 1)
		H5Dwrite(dataSet, memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);
	else
	{
		// Create strided memory data space
		const hsize_t clampedStride = (stride < 1) ? 1 : stride;
		const hsize_t numElem = H5Sget_simple_extent_npoints(dataSpace);

		// We need the actual array size (not just the number of elements to be written)
		const hsize_t spaceExtent = numElem * clampedStride;
		const hid_t memSpace = H5Screate_simple(1, &spaceExtent, nullptr);

		const hsize_t start = 0;
		H5Sselect_hyperslab(memSpace, H5S_SELECT_SET, &start, &clampedStride, &numElem, nullptr);

		H5Dwrite(dataSet, memType, memSpace, H5S_ALL, H5P_DEFAULT, buffer);
		H5Sclose(memSpace);
	}

	H5Dclose(dataSet);
	H5Sclose(dataSpace);
	H5Pclose(propList);
}

}  // namespace io
}  // namespace cadet

#endif /* HDF5WRITER_HPP_ */
