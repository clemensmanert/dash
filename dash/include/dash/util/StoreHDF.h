#ifdef DASH_ENABLE_HDF5
#ifndef DASH__UTIL__HDF5_H__
#define DASH__UTIL__HDF5_H__

#include <dash/internal/Config.h>
#include <dash/Exception.h>
#include <dash/Init.h>
#include <dash/Array.h>
#include <dash/Matrix.h>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <array>

namespace dash {
namespace util {

class StoreHDF 
{
public:
  friend void dash::init(int *argc, char ***argv);

public:
	/**
   * Store all array values in an HDF5 file
	 */
	template<
		typename value_t,
		typename index_t,
		class pattern_t >
	static void write(
	dash::Array<value_t, index_t, pattern_t> &array,
	std::string filename,
	std::string table)
	{

	auto		globalsize = array.size();
	auto		pattern		 = array.pattern();
	auto		pat_dims	 = pattern.ndim();
	auto 		localsize	 = pattern.local_size();
	auto		tilesize	 = pattern.blocksize(0);
	auto		numunits	 = pattern.num_units();
	auto		lbegindex	 = pattern.lbegin();
	// global distance between two local tiles in elements
	auto		tiledist	 = numunits * tilesize;
	// Map native types to HDF5 types
	auto h5datatype = _convertType(array[0]);


	// Currently only works for 1-dimensional tiling
	DASH_ASSERT_EQ(
		pat_dims,
		1,
		"Array pattern has to be one-dimensional for HDF5 storage");

	/* HDF5 definition */
	hid_t		file_id;
	hid_t		dataset;
	hid_t 	internal_type;
	hid_t		plist_id; // property list identifier
	hid_t 	filespace;
	hid_t		memspace;
	/* global or file data dims */
	hsize_t	data_dimsf[] = {(hsize_t) globalsize};
	/* local data dims */
	hsize_t data_dimsm[] = {(hsize_t) localsize};

	hid_t		attr_id;
	herr_t	status;

	// Hyperslab definition
	hsize_t	count[1];
	hsize_t stride[1];
	hsize_t	offset[1];
	hsize_t block[1];

	// setup mpi access
	plist_id = H5Pcreate(H5P_FILE_ACCESS);
	H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);

	// HD5 create file
	file_id = H5Fcreate( filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id );

	// close property list
	H5Pclose(plist_id);

	// Create dataspace
	filespace			= H5Screate_simple(1, data_dimsf, NULL);
	memspace			= H5Screate_simple(1, data_dimsm, NULL);
	internal_type	= H5Tcopy(h5datatype);
	
	// Create dataset
	dataset = H5Dcreate(file_id, table.c_str(), internal_type, filespace,
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	// Close global dataspace
	H5Sclose(filespace);

	filespace = H5Dget_space(dataset);

	if(pat_dims == 1){
		// Select Hyperslabs in file
		count[0]	= (hsize_t) (localsize/tilesize); // tiles per node
		stride[0]	= (hsize_t) tiledist;	 // space between two tiles of same node
		offset[0] = (hsize_t) lbegindex; // offset in positions
		block[0]	= (hsize_t) tilesize;	 // size of a tile

		H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

		// Create property list for collective writes
		plist_id = H5Pcreate(H5P_DATASET_XFER);
		H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

		// Write data
		H5Dwrite(dataset, internal_type, memspace, filespace,
						plist_id, array.lbegin());
	} else {
		// Store each block separate
		for(int dim=0; dim<pat_dims; dim++){
			// TODO
			//count[0]	= pattern.local_extend(dim);
			//stride[0] = 
			//DASH_LOG_DEBUG("ext ",dim ,count[0]);
			//stride[0] = 0;
			//offset[0] = 0;
			//block[0]	= blocksize(dim)
		}
	}

	// Add Attributes
	hid_t attrspace = H5Screate(H5S_SCALAR);
	long attr = (long) tilesize;
	hid_t attribute_id = H5Acreate(
		dataset, "DASH_TILESIZE", H5T_NATIVE_LONG,
		attrspace, H5P_DEFAULT, H5P_DEFAULT);
	H5Awrite(attribute_id, H5T_NATIVE_LONG, &attr);
	H5Aclose(attribute_id);
	H5Sclose(attrspace);

	// Close all
	H5Dclose(dataset);
	H5Sclose(filespace);
	H5Sclose(memspace);
	H5Tclose(internal_type);
	H5Fclose(file_id);
	}

	/**
   * Store DASH::Matrix in an HDF5 file
	 */
	template<
		typename value_t,
		dim_t ndim,
		typename index_t,
		class pattern_t >
	static void write(
	dash::Matrix<value_t, ndim, index_t, pattern_t> &array,
	std::string filename,
	std::string table)
	{

	auto pattern		= array.pattern();
	auto pat_dims		= pattern.ndim();
	// Map native types to HDF5 types
	auto h5datatype = _convertType(*array.lbegin());

	// leave team if unit is not in team
	//if(!pattern.team().is_member(dash::myid())){
	//	return;
	//}
	// Currently only works for 1-dimensional tiling
	DASH_ASSERT_EQ(
		pat_dims,
		ndim,
		"Pattern dimension has to match matrix dimension");

	/* HDF5 definition */
	hid_t		file_id;
	hid_t		dataset;
	hid_t 	internal_type;
	hid_t		plist_id; // property list identifier
	hid_t 	filespace;
	hid_t		memspace;
	hid_t		attr_id;
	herr_t	status;

	/* global or file data dims */
	hsize_t	data_dimsf[ndim];
	/* local data dims */
	hsize_t data_dimsm[ndim];

	// Hyperslab definition
	hsize_t	count[ndim];
	hsize_t stride[ndim];
	hsize_t	offset[ndim];
	hsize_t block[ndim];

  // setup extends per dimension
	for(int i=0; i<ndim; i++){
		data_dimsf[i] = pattern.extent(i);
		data_dimsm[i] = pattern.local_extent(i);
		// number of tiles in this dimension
		// works also for underfilled tiles
		count[i]			= (data_dimsm[i]-1) / pattern.blocksize(i) + 1;
		offset[i]			= pattern.local_block(0).offset(i);
		block[i]			= pattern.blocksize(i);

		// calculate stride
		stride[i] = pattern.teamspec().extent(i)*block[i];
	
		DASH_LOG_DEBUG("COUNT",i, count[i]);
		DASH_LOG_DEBUG("OFFSET",i, offset[i]);
		DASH_LOG_DEBUG("BLOCK",i, block[i]);
		DASH_LOG_DEBUG("STRIDE",i, stride[i]);
	}

	// setup mpi access
	plist_id = H5Pcreate(H5P_FILE_ACCESS);
	H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);

	// HD5 create file
	file_id = H5Fcreate( filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id );

	// close property list
	H5Pclose(plist_id);

	// Create dataspace
	filespace			= H5Screate_simple(ndim, data_dimsf, NULL);
	memspace			= H5Screate_simple(ndim, data_dimsm, NULL);
	internal_type	= H5Tcopy(h5datatype);
	
	// Create dataset
	dataset = H5Dcreate(file_id, table.c_str(), internal_type, filespace,
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	// Close global dataspace
	H5Sclose(filespace);

	filespace = H5Dget_space(dataset);

	H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

	// Create property list for collective writes
	plist_id = H5Pcreate(H5P_DATASET_XFER);
	H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

	// Write data
	H5Dwrite(dataset, internal_type, memspace, filespace,
						plist_id, array.lbegin());

	// Add Attributes
	long pattern_spec[ndim*4];
	// Structure is 
	// sizespec, teamspec, blockspec, blocksize
	for(int i=0; i<ndim; i++){
		pattern_spec[i]	  				= pattern.sizespec().extent(i);
		pattern_spec[i+ndim] 			= pattern.teamspec().extent(i);
		pattern_spec[i+(ndim*2)] 	= pattern.blockspec().extent(i);
		pattern_spec[i+(ndim*3)]  = pattern.blocksize(i);
	}

	hsize_t attr_len[] = { ndim*4 };
	hid_t attrspace = H5Screate_simple(1,
											attr_len, 
											NULL);
	hid_t attribute_id = H5Acreate(
		dataset, "DASH_PATTERN", H5T_NATIVE_LONG,
		attrspace, H5P_DEFAULT, H5P_DEFAULT);
	H5Awrite(attribute_id, H5T_NATIVE_LONG, &pattern_spec);
	H5Aclose(attribute_id);
	H5Sclose(attrspace);

	// Close all
	H5Dclose(dataset);
	H5Sclose(filespace);
	H5Sclose(memspace);
	H5Tclose(internal_type);
	H5Fclose(file_id);
	}


	/**
   * Read an HDF5 table into an DASH array.
   */
	template<typename value_t>
	static void read(
		dash::Array<value_t> &array,
		std::string filename,
		std::string table)
	{

	long		globalsize;
	long 		localsize;
	long		tilesize;
	int			numunits;
	long		lbegindex;
	long		tiledist;// global distance between two local tiles

	/* HDF5 definition */
	hid_t		file_id;
	hid_t		dataset;
	hid_t 	internal_type;
	hid_t		plist_id; // property list identifier
	hid_t 	filespace;
	hid_t		memspace;
	/* global data dims */
	hsize_t	data_dimsf[1];
	/* local data dims */
	hsize_t data_dimsm[1];

	herr_t	status;
	int			rank;
	// Map native types to HDF5 types
	hid_t h5datatype;
	
	// Hyperslab definition
	hsize_t	count[1];
	hsize_t stride[1];
	hsize_t	offset[1];
	hsize_t block[1];

	// setup mpi access
	plist_id = H5Pcreate(H5P_FILE_ACCESS);
	H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);

	// HD5 create file
	file_id = H5Fopen(filename.c_str(), H5P_DEFAULT, plist_id );

	// close property list
	H5Pclose(plist_id);

	// Create dataset
	dataset = H5Dopen(file_id, table.c_str(), H5P_DEFAULT);

	// Get dimensions of data
	filespace 		= H5Dget_space(dataset);
	rank      		= H5Sget_simple_extent_ndims(filespace);
	
	DASH_ASSERT_EQ(rank, 1, "Data dimension of HDF5 table is not 1");

	status    		= H5Sget_simple_extent_dims(filespace, data_dimsf, NULL);

	// Initialize DASH Array
	// no explicit pattern specified / try to load pattern from hdf5 file
	if(H5Aexists(dataset, "DASH_TILESIZE")){
		hid_t attrspace			= H5Screate(H5S_SCALAR);
		hid_t attribute_id  = H5Aopen(dataset, "DASH_TILESIZE", H5P_DEFAULT);
		H5Aread(attribute_id, H5T_NATIVE_LONG, &tilesize);
		H5Aclose(attribute_id);
		H5Sclose(attrspace);
	} else {
		tilesize = 1; // dash::CYCLIC
	}

	// Allocate DASH Array
	array.allocate(data_dimsf[0], dash::TILE(tilesize));
	h5datatype 		= _convertType(array[0]); // hack

	// Calculate chunks
	globalsize		= array.size();
	localsize			= array.pattern().local_size();
	tilesize			= array.pattern().blocksize(0);
	numunits			=	array.pattern().num_units();
	lbegindex			= array.pattern().lbegin();
	tiledist			= numunits * tilesize;
	data_dimsm[0]	= localsize;

	// Create HDF5 memspace
	memspace  		= H5Screate_simple(1,data_dimsm,NULL);
	internal_type  = H5Tcopy(h5datatype);

	// Select Hyperslabs in file
	count[0]	= (hsize_t) (localsize/tilesize); // tiles per node
	stride[0]	= (hsize_t) tiledist;	 // space between two tiles of same node
	offset[0] = (hsize_t) lbegindex; // offset in positions
	block[0]	= (hsize_t) tilesize;	 // size of a tile

	H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

	// Create property list for collective reads
	plist_id = H5Pcreate(H5P_DATASET_XFER);
	H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

	// read data
	H5Dread(dataset, internal_type, memspace, filespace,
		plist_id, array.lbegin());

	// Close all
	H5Dclose(dataset);
	H5Sclose(filespace);
	H5Sclose(memspace);
	H5Tclose(internal_type);
	H5Fclose(file_id);
	}


	/**
   * Import a HDF5 n-dimensional matrix into a DASH::Matrix
	 */
	template<
		typename value_t,
		dim_t ndim>
	static void read(
		dash::Matrix<
						value_t,
						ndim> &matrix,
		std::string filename,
		std::string table)
	{
	typedef dash::TilePattern<ndim> pattern_t;

	/* HDF5 definition */
	hid_t		file_id;
	hid_t		dataset;
	hid_t 	internal_type;
	hid_t		plist_id; // property list identifier
	hid_t 	filespace;
	hid_t		memspace;
	/* global data dims */
	hsize_t	data_dimsf[ndim];
	/* local data dims */
	hsize_t data_dimsm[ndim];

	herr_t	status;
	int			rank;
	// Map native types to HDF5 types
	hid_t h5datatype;
	
	// Hyperslab definition
	hsize_t	count[ndim];
	hsize_t stride[ndim];
	hsize_t	offset[ndim];
	hsize_t block[ndim];

	// setup mpi access
	plist_id = H5Pcreate(H5P_FILE_ACCESS);
	H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);

	// HD5 create file
	file_id = H5Fopen(filename.c_str(), H5P_DEFAULT, plist_id );

	// close property list
	H5Pclose(plist_id);

	// Create dataset
	dataset = H5Dopen(file_id, table.c_str(), H5P_DEFAULT);

	// Get dimensions of data
	filespace 		= H5Dget_space(dataset);
	rank      		= H5Sget_simple_extent_ndims(filespace);
	
	DASH_ASSERT_EQ(rank, ndim, 
			"Data dimension of HDF5 table does not match matrix dimension");

	status    		= H5Sget_simple_extent_dims(filespace, data_dimsf, NULL);

	// Initialize DASH Array
	// no explicit pattern specified / try to load pattern from hdf5 file

	std::array<size_t, ndim>						 size_extents;
	std::array<size_t, ndim> 						 team_extents;
	std::array<dash::Distribution, ndim> dist_extents;
	long hdf_dash_pattern[ndim*4];

	// delayed initialisation not possible
	for(int i=0; i<ndim; i++){
			size_extents[i] = data_dimsf[i];
			if(i==0){
				team_extents[i] = dash::Team::All().size();
			} else {
				team_extents[i] = 1;
			}
		}
		dash::SizeSpec<ndim> size_spec(size_extents);
		dash::TeamSpec<ndim> team_spec(team_extents);
		team_spec.balance_extents();

		pattern_t pattern = dash::make_pattern <
                dash::summa_pattern_partitioning_constraints,
     		        dash::summa_pattern_mapping_constraints,
           			dash::summa_pattern_layout_constraints >(
             			size_spec,
             			team_spec);

	// Check if file contains DASH metadata and recreate the pattern
	if(H5Aexists(dataset, "DASH_PATTERN")){
		hsize_t attr_len[]  = { ndim*4};
		hid_t attrspace			= H5Screate_simple(1, attr_len, NULL);
		hid_t attribute_id  = H5Aopen(dataset, "DASH_PATTERN", H5P_DEFAULT);
		H5Aread(attribute_id, H5T_NATIVE_LONG, hdf_dash_pattern);
		H5Aclose(attribute_id);
		H5Sclose(attrspace);
		for(int i=0; i<ndim; i++){
			size_extents[i]  = static_cast<size_t> (hdf_dash_pattern[i]);
			team_extents[i]  = static_cast<size_t> (hdf_dash_pattern[i+ndim]);
			dist_extents[i]  = dash::TILE(hdf_dash_pattern[i+(ndim*3)]);
		}
		// instantiate pattern
		pattern = pattern_t(
								dash::SizeSpec<ndim>(size_extents),
								dash::DistributionSpec<ndim>(dist_extents),
								dash::TeamSpec<ndim>(team_extents));
	}

	DASH_LOG_DEBUG("Pattern", pattern);

	// Allocate DASH Matrix
	
	matrix.allocate(pattern);
	h5datatype 		= _convertType(*matrix.lbegin()); // hack

  // setup extends per dimension
	for(int i=0; i<ndim; i++){
		data_dimsm[i] = pattern.local_extent(i);
		// number of tiles in this dimension
		// works also for underfilled tiles
		count[i]			= (data_dimsm[i]-1) / pattern.blocksize(i) + 1;
		offset[i]			= pattern.local_block(0).offset(i);
		block[i]			= pattern.blocksize(i);

		// calculate stride
		stride[i] = pattern.teamspec().extent(i)*block[i];
	
		DASH_LOG_DEBUG("COUNT",i, count[i]);
		DASH_LOG_DEBUG("OFFSET",i, offset[i]);
		DASH_LOG_DEBUG("BLOCK",i, block[i]);
		DASH_LOG_DEBUG("STRIDE",i, stride[i]);
	}

	// Create dataspace
	memspace			= H5Screate_simple(ndim, data_dimsm, NULL);
	internal_type	= H5Tcopy(h5datatype);

	H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

	// Create property list for collective reads
	plist_id = H5Pcreate(H5P_DATASET_XFER);
	H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

	// read data
	H5Dread(dataset, internal_type, memspace, filespace,
		plist_id, matrix.lbegin());

	// Close all
	H5Dclose(dataset);
	H5Sclose(filespace);
	H5Sclose(memspace);
	H5Tclose(internal_type);
	H5Fclose(file_id);

	}

private:
	static hid_t _convertType(int t){
		return H5T_NATIVE_INT;
	}
	static hid_t _convertType(long t){
		return H5T_NATIVE_LONG;
	}
	static hid_t _convertType(float t){
		return H5T_NATIVE_FLOAT;
	}
	static hid_t _convertType(double t){
		return H5T_NATIVE_DOUBLE;
	}

};
} // namespace util
} // namespace dash

#endif // DASH__UTIL__HDF5_H_
#endif