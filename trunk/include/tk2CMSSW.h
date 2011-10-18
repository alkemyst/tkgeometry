// 
// File:   tk2CMSSW.h
// Author: ndemaio
//
// Created on August 28, 2009, 5:23 PM
//

/**
 * @file tk2CMSSW.h
 * @brief This is the header file for the CMSSW XML translator class
 */

#ifndef _TK2CMSSW_H
#define	_TK2CMSSW_H

#include <tk2CMSSW_datatypes.h>
#include <tk2CMSSW_strings.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <Extractor.h>
#include <XMLWriter.h>
#include <MaterialTable.h>
#include <MaterialBudget.h>
#include <boost/filesystem.hpp>

/**
 * A shorter alias for the filesystem library namespace
 */
namespace bfs = boost::filesystem;
namespace insur {
    /**
     * @class tk2CMSSW
     * @brief This class is the main translator interface for generating XML output for CMSSW from an existing material budget and table.
     *
     * It deals directly with setting up output paths and buffers for output files, while it delegates analysis and XML formatting of the material 
     * budget to an internal instance of <i>Extractor</i> and <i>XMLWriter</i>, respectively. Existing files are not overwritten until analysis
     * and XML formatting have been completed successfully. Should any of them fail, any existing older files are restored; preliminary
     * results for the new files are discarded.
     */
    class tk2CMSSW {
    public:
        tk2CMSSW() {}
        virtual ~tk2CMSSW() {}
        void translate(MaterialTable& mt, MaterialBudget& mb, std::string outsubdir = "", bool wt = false);
    protected:
        CMSSWBundle data;
        Extractor ex;
        XMLWriter wr;
    private:
        void print();
    };
}
#endif	/* _TK2CMSSW_H */

