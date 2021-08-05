/**
 * @file core/data/load_csv.hpp
 * @author ThamNgapWei
 * @author Conrad Sanderson
 * @author Gopi M. Tatiraju
 *
 * This csv parser is designed by taking reference from armadillo's csv parser.
 * In this mlpack's version, all the arma dependencies were removed or replaced
 * accordingly, making the parser totally independent of armadillo.
 *
 * https://gitlab.com/conradsnicta/armadillo-code/-/blob/10.5.x/include/armadillo_bits/diskio_meat.hpp
 * Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
 * Copyright 2008-2016 National ICT Australia (NICTA)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ------------------------------------------------------------------------
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MLPACK_CORE_DATA_LOAD_CSV_HPP
#define MLPACK_CORE_DATA_LOAD_CSV_HPP

#include <mlpack/core/util/log.hpp>
#include <set>
#include <string>

#include "string_algorithms.hpp"
#include "extension.hpp"
#include "format.hpp"
#include "dataset_mapper.hpp"
#include "types.hpp"

namespace mlpack {
namespace data {

/**
 * Load the csv file.This class use boost::spirit
 * to implement the parser, please refer to following link
 * http://theboostcpplibraries.com/boost.spirit for quick review.
 */
class LoadCSV
{
 public:

  char delim;
  // Do nothing, just a place holder, to be removed later.
  LoadCSV()
  {
  } 

  /**
   * Construct the LoadCSV object on the given file.  This will construct the
   * rules necessary for loading and attempt to open the file.
   */
  LoadCSV(const std::string& file) :
    extension(Extension(file)),
    filename(file),
    inFile(file)
  {
    if(extension == "csv")
    {
      delim = ',';
    }
    else if(extension == "tsv")
    {
      delim = '\t';
    }
    else if(extension == "txt")
    {
      // Can we have a case where number
      // of spaces is more than 1
      delim = ' ';
    }

    CheckOpen();
  }

  /**
   * Convert the given string token to assigned datatype and assign
   * this value to the given address. The address here will be a
   * matrix location.
   * 
   * Token is always read as a string, if the given token is +/-INF or NAN
   * it converts them to infinity and NAN using numeric_limits.
   *
   * @param val Token's value will be assigned to this address
   * @param token Value which should be assigned
   */
  template<typename MatType>
  bool ConvertToken(typename MatType::elem_type& val, const std::string& token);

  /**
   * Returns a bool value showing whether data was loaded successfully or not.
   *
   * Parses a csv file and loads the data into a given matrix. In the first pass,
   * the function will determine the number of cols and rows in the given file.
   * Once the rows and cols are fixed we initialize the matrix with zeros. In 
   * the second pass, the function converts each value to required datatype
   * and sets it equal to val. 
   *
   * This function uses MatType as template parameter in order to provide
   * support for any type of matrices from any linear algebra library. 
   *
   * @param x Matrix in which data will be loaded
   * @param f File stream to access the data file
   */
  template<typename MatType>
  bool LoadCSVFile(MatType& x, std::fstream& f);

  /**
   * Load the file into the given matrix with the given DatasetMapper object.
   * Throws exceptions on errors.
   *
   * @param inout Matrix to load into.
   * @param infoSet DatasetMapper to use while loading.
   * @param transpose If true, the matrix should be transposed on loading
   *     (default).
   */
  template<typename T, typename PolicyType>
  void Load(arma::Mat<T> &inout,
            DatasetMapper<PolicyType> &infoSet,
            const bool transpose = true)
  {
    CheckOpen();

    if (transpose)
      TransposeParse(inout, infoSet);
    else
      NonTransposeParse(inout, infoSet);
  }

  /**
   * Peek at the file to determine the number of rows and columns in the matrix,
   * assuming a non-transposed matrix.  This will also take a first pass over
   * the data for DatasetMapper, if MapPolicy::NeedsFirstPass is true.  The info
   * object will be re-initialized with the correct dimensionality.
   *
   * @param rows Variable to be filled with the number of rows.
   * @param cols Variable to be filled with the number of columns.
   * @param info DatasetMapper object to use for first pass.
   */
  template<typename T, typename MapPolicy>
  void GetMatrixSize(size_t& rows, size_t& cols, DatasetMapper<MapPolicy>& info)
  {
    // Take a pass through the file.  If the DatasetMapper policy requires it,
    // we will pass everything string through MapString().  This might be useful
    // if, e.g., the MapPolicy needs to find which dimensions are numeric or
    // categorical.

    // Reset to the start of the file.
    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    rows = 0;
    cols = 0;

    // First, count the number of rows in the file (this is the dimensionality).
    std::string line;
    while (std::getline(inFile, line))
    {
       ++rows;
    }

    // Reset the DatasetInfo object, if needed.
    if (info.Dimensionality() == 0)
    {
      info.SetDimensionality(rows);
    }
    else if (info.Dimensionality() != rows)
    {
      std::ostringstream oss;
      oss << "data::LoadCSV(): given DatasetInfo has dimensionality "
          << info.Dimensionality() << ", but data has dimensionality "
          << rows;
      throw std::invalid_argument(oss.str());
    }

    // Reset the DatasetInfo object, if needed.
    if (info.Dimensionality() == 0)
    {
      info.SetDimensionality(rows);
    }
    else if (info.Dimensionality() != rows)
    {
      std::ostringstream oss;
      oss << "data::LoadCSV(): given DatasetInfo has dimensionality "
          << info.Dimensionality() << ", but data has dimensionality "
          << rows;
      throw std::invalid_argument(oss.str());
    }

    // Now, jump back to the beginning of the file.
    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    rows = 0;
    
    while (std::getline(inFile, line))
    {
      trim(line);
      ++rows;
      if (rows == 1)
      {
        // Extract the number of columns.
        std::pair<size_t, size_t> dimen = GetNonNumericMatSize(inFile, delim);
        cols = dimen.second;
      }

      // I guess this is technically a second pass, but that's ok... still the
      // same idea...
      if (MapPolicy::NeedsFirstPass)
      {
        // In this case we must pass everything we parse to the MapPolicy.
	std::string str(line.begin(), line.end());
      
        std::stringstream line_stream;
        std::string token;

        if(line.size() == 0)
	{
	  break;
        }

       line_stream.clear();
       line_stream.str(line);

       while(line_stream.good())
       {
         std::getline(line_stream, token, delim);
	 trim(token);

	 /*size_t found = token.find('"');

	 if(found != std::string::npos)
	 {
	   std::string firstPart = token + ",";
	   std::string secondPart;

	   std::getline(line_stream, secondPart, delim);
	   token = firstPart + secondPart;
	 }
	 */

        if(token[0] == '"' && token[token.size() - 1] != '"')
        {
	  /*
          token += delim;
          std::string part;
          std::getline(line_stream, part, delim);
          token += part;
	  */
          std::string tok = token;

	  while(token[token.size() - 1] != '"')
	  {
	    tok += delim;
	    std::getline(line_stream, token, delim);
	    tok += token;
	  }

	  token = tok;
	}

	 info.template MapFirstPass<T>(std::move(token), rows - 1);
       }
      }
    }
  }

  /**
   * Peek at the file to determine the number of rows and columns in the matrix,
   * assuming a transposed matrix.  This will also take a first pass over the
   * data for DatasetMapper, if MapPolicy::NeedsFirstPass is true.  The info
   * object will be re-initialized with the correct dimensionality.
   *
   * @param rows Variable to be filled with the number of rows.
   * @param cols Variable to be filled with the number of columns.
   * @param info DatasetMapper object to use for first pass.
   */
  template<typename T, typename MapPolicy>
  void GetTransposeMatrixSize(size_t& rows,
                              size_t& cols,
                              DatasetMapper<MapPolicy>& info)
  {
    // Take a pass through the file.  If the DatasetMapper policy requires it,
    // we will pass everything string through MapString().  This might be useful
    // if, e.g., the MapPolicy needs to find which dimensions are numeric or
    // categorical.

    // Reset to the start of the file.
    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    rows = 0;
    cols = 0;

    std::string line;
    while (std::getline(inFile, line))
    {
      ++cols;
      
      trim(line);

      if (cols == 1)
      {
        // Extract the number of dimensions.
        std::pair<size_t, size_t> dimen = GetNonNumericMatSize(inFile, delim);
        rows = dimen.second;

        // Reset the DatasetInfo object, if needed.
        if (info.Dimensionality() == 0)
        {
          info.SetDimensionality(rows);
        }
        else if (info.Dimensionality() != rows)
        {
          std::ostringstream oss;
          oss << "data::LoadCSV(): given DatasetInfo has dimensionality "
              << info.Dimensionality() << ", but data has dimensionality "
              << rows;
          throw std::invalid_argument(oss.str());
        }
      }

      // If we need to do a first pass for the DatasetMapper, do it.
      if (MapPolicy::NeedsFirstPass)
      {
        size_t dim = 0;

	std::stringstream line_stream;
	std::string token;

	if(line.size() == 0)
	{
	  break;
	}

	line_stream.clear();
	line_stream.str(line);

	while(line_stream.good())
        {
          std::getline(line_stream, token, delim);
	  trim(token);

	  /*size_t found = token.find('"');

	  if(found != std::string::npos)
	  {
	    std::string firstPart = token + ",";
	    std::string secondPart;

	    std::getline(line_stream, secondPart, delim);
	    token = firstPart + secondPart;
	  }
	  */
  
          if(token[0] == '"' && token[token.size() - 1] != '"')
          {
	    /*
            token += delim;
            std::string part;
            std::getline(line_stream, part, delim);
            token += part;
	    */

	    std::string tok = token;

	    while(token[token.size() - 1] != '"')
	    {
	      tok += delim;
	      std::getline(line_stream, token, delim);
	      tok += token;
            }

	    token = tok;
          }

          info.template MapFirstPass<T>(std::move(token), dim++);
        }
      }
    }
  }

 private:
  /**
   * Check whether or not the file has successfully opened; throw an exception
   * if not.
   */
  void CheckOpen()
  {
    if (!inFile.is_open())
    {
      std::ostringstream oss;
      oss << "Cannot open file '" << filename << "'. " << std::endl;
      throw std::runtime_error(oss.str());
    }

    inFile.unsetf(std::ios::skipws);
  }

  /**
   * Parse a non-transposed matrix.
   *
   * @param inout Matrix to load into.
   * @param infoSet DatasetMapper object to load with.
   */
  template<typename T, typename PolicyType>
  void NonTransposeParse(arma::Mat<T>& inout,
                         DatasetMapper<PolicyType>& infoSet)
  {
    // Get the size of the matrix.
    size_t rows, cols;
    GetMatrixSize<T>(rows, cols, infoSet);

    // Set up output matrix.
    inout.set_size(rows, cols);
    size_t row = 0;
    size_t col = 0;

    // Reset file position.
    std::string line;
    inFile.clear();
    inFile.seekg(0, std::ios::beg);

    while (std::getline(inFile, line))
    {
      
      trim(line);

      const bool canParse = true;
      std::stringstream line_stream;
      std::string token;

      if(line.size() == 0)
      {
        break;
      }

      line_stream.clear();
      line_stream.str(line);

      while(line_stream.good())
      {
	if(token == "\t")
	{
	  token.clear();
	}

        std::getline(line_stream, token, delim);
        trim(token);

	/*size_t found = token.find('"');

	if(found != std::string::npos)
	{
	  std::string firstPart = token + ",";
	  std::string secondPart;

	  std::getline(line_stream, secondPart, delim);
	  token = firstPart + secondPart;
	}
	*/

        if(token[0] == '"' && token[token.size() - 1] != '"')
        {
	  /*
          token += delim;
          std::string part;
          std::getline(line_stream, part, delim);
          token += part;
	  */

	  std::string tok = token;

	  while(token[token.size() - 1] != '"')
	  {
	    tok += delim;
	    std::getline(line_stream, token, delim);
	    tok += token;
          }

	  token = tok;
        }

	inout(row, col++) = infoSet.template MapString<T>(std::move(token), row);
      }      
      // Make sure we got the right number of rows.
      if (col != cols)
      {
        std::ostringstream oss;
        oss << "LoadCSV::NonTransposeParse(): wrong number of dimensions ("
            << col << ") on line " << row << "; should be " << cols
            << " dimensions.";
        throw std::runtime_error(oss.str());
      }

      // I am not able to understand when can we enter this case.
      // I am looking into it, if anyone can give me some hint
      // it might help, currently I've assigned canParse as true
      // by default
      if (!canParse)
      {
        std::ostringstream oss;
        oss << "LoadCSV::NonTransposeParse(): parsing error on line " << col
            << "!";
        throw std::runtime_error(oss.str());
      }

      ++row; col = 0;
    }
  }

  /**
   * Parse a transposed matrix.
   *
   * @param inout Matrix to load into.
   * @param infoSet DatasetMapper to load with.
   */
  template<typename T, typename PolicyType>
  void TransposeParse(arma::Mat<T>& inout, DatasetMapper<PolicyType>& infoSet)
  {
    // Get matrix size.  This also initializes infoSet correctly.
    size_t rows, cols;
    GetTransposeMatrixSize<T>(rows, cols, infoSet);

    // Set the matrix size.
    inout.set_size(rows, cols);

    // Initialize auxiliary variables.
    size_t row = 0;
    size_t col = 0;
    std::string line;
    inFile.clear();
    inFile.seekg(0, std::ios::beg);

    while (std::getline(inFile, line))
    {
      trim(line);
      // Reset the row we are looking at.  (Remember this is transposed.)
      row = 0;
      const bool canParse = true;
      std::stringstream line_stream;
      std::string token;

      if(line.size() == 0)
      {
        break;
      }
      
      line_stream.clear();
      line_stream.str(line);

      while(line_stream.good())
      {
        std::getline(line_stream, token, delim);
	trim(token);

	/*size_t found = token.find('"');

	if(found != std::string::npos)
	{
	  std::string firstPart = token + ",";
	  std::string secondPart;

	  std::getline(line_stream, secondPart, delim);
	  token = firstPart + secondPart;
	}
	*/

        if(token[0] == '"' && token[token.size() - 1] != '"')
        {

            /*
	    token += delim;
            std::string part;
            std::getline(line_stream, part, delim);
            token += part;
	    */
	  // first part of the string
	  std::string tok = token;

	  while(token[token.size() - 1] != '"')
	  {
	    tok += delim;
	    std::getline(line_stream, token, delim);
	    tok += token;
	  }

	  token = tok;
        }

        inout(row, col) = infoSet.template MapString<T>(std::move(token), row);
	row++;
      }
      
      // Make sure we got the right number of rows.
      if (row != rows)
      {
        std::ostringstream oss;
        oss << "LoadCSV::TransposeParse(): wrong number of dimensions (" << row
            << ") on line " << col << "; should be " << rows << " dimensions.";
        throw std::runtime_error(oss.str());
      }
      
      // I am not able to understand when can we enter this case.
      // I am looking into it, if anyone can give me some hint
      // it might help, currently I've assigned canParser as true
      // by default
      if (!canParse)
      {
        std::ostringstream oss;
        oss << "LoadCSV::TransposeParse(): parsing error on line " << col
            << "!";
        throw std::runtime_error(oss.str());
      }

      // Increment the column index.
      ++col;
    }
  }

  inline std::pair<size_t, size_t> GetMatSize(std::fstream& f, const char delim);

  inline std::pair<size_t, size_t> GetNonNumericMatSize(std::ifstream& f, const char delim);
  //! Extension (type) of file.
  std::string extension;
  //! Name of file.
  std::string filename;
  //! Opened stream for reading.
  std::ifstream inFile;
};

} // namespace data
} // namespace mlpack

#include "load_csv_impl.hpp"

#endif
