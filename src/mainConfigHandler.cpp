#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>

#include <sys/types.h>
#include <regex.h>

#include <mainConfigHandler.h>

using namespace std;
using namespace boost;

bool mainConfigHandler::checkDirectory(string dirName) {
  if (! filesystem::exists(dirName)) {
    cout << "Directory '" << dirName << "' does not exist!" << endl;
    return false;
  }
  if (! filesystem::is_directory(dirName) ) {
    cout << "Directory '" << dirName << "' is not a directory!" << endl;
    return false;    
  }

  return true;
}

bool mainConfigHandler::createConfigurationFileFromQuestions(string& configFileName,  string& styleDirectory, string& layoutDirectory) {
  // Clear screen
  cout << "\033[2J"; // Clears the screen
  cout << "\033[1;1H"; // Places cursor on line 1

  // I have no configuration, so I must create it
  cout << "Could not find the configuration file "  << configFileName 
       << " maybe this is the first time you run with the new system." << endl;
  cout << "Answer to the following questions to have your configuration file automatically created." << endl;
  cout << "You will be later able to edit it manually, or you can just delete it and answer these questions again." << endl;
  cout << endl;
  cout << "*** What is the tkLayout installation style directory?" << endl
       << "    This directory must be visible by the web server" << endl
       << "    if you want the pages to be readable from the web"<< endl
       << "    ( Usually this directory is called 'style' and it is in" << endl
       << "    the main program's directory, but you can copy it anywhere you like.)" << endl
       << "    Example: /home/username/tkLayout/style : ";
  cin >> styleDirectory ;
  if (!checkDirectory(styleDirectory)) return false;
  cout << endl;

  cout << "*** What is the web server directory where you want to" << endl
       << "    place your output?" << endl
       << "    Example: /home/username/www/layouts : ";
  cin >> layoutDirectory;
  if (!checkDirectory(layoutDirectory)) return false;
  cout << endl;

  ofstream configFile;
  configFile.open(configFileName.c_str(), ifstream::out);
  if (!configFile.good()) {
    cout << "Could not open " << configFile << " for writing. I quit."  << endl;
    configFile.close();
    return false;
  } else {
    configFile << STYLEDIRECTORYDEFINITION << " = \"" <<  styleDirectory << "\"" << endl;
    configFile << LAYOUTDIRECTORYDEFINITION << " = \"" << layoutDirectory << "\"" << endl;
    configFile.close();
  }

  return true;
}


bool mainConfigHandler::parseLine(const char* codeLine, string& parameter, string& value) {
  cmatch what;
  regex parseLineExpression("^[ \\t]*([a-zA-Z0-9]*)[ \\t]*=[ \\t]*\"([^\"]+)\".*");
  regex parseLineEmpty("^[ \\t]*");

  if (regex_match(codeLine, what, parseLineExpression)) {
    // what[1] contains the parameter name
    // what[2] contains the parameter value
    parameter = what[1];
    value = what[2];
    return true;
  } else if (regex_match(codeLine, what, parseLineEmpty)) {
    // Empty line, ok
    parameter="";
    value="";
    return false;
  } else {
    // Wrong formatting
    cerr << "Cannot understand line: '" << codeLine << "' in the configuration file " << CONFIGURATIONFILENAME << endl;
    return false;
  }
}


bool mainConfigHandler::readConfigurationFile(ifstream& configFile, string& styleDirectory, string& layoutDirectory) {
  char myLine[1024];
  string parameter, value;
  bool styleFound=false;
  bool layoutFound=false;

  // Parsing all the lines of the configuration file
  while (configFile.good()) {
    configFile.getline(myLine, 1024);
    if (parseLine(myLine, parameter, value)) {
      std::transform(parameter.begin(), parameter.end(), parameter.begin(), ::tolower);
      // If he's defining the style directory, then we map it to styleDirectory
      if (parameter==STYLEDIRECTORYDEFINITIONLOWERCASE) {
	styleDirectory = value;
	styleFound=true;
      } else if (parameter==LAYOUTDIRECTORYDEFINITIONLOWERCASE) {
	layoutDirectory = value;
	layoutFound = true;
      } else {
	cerr << "Unknown parameter " << parameter << " in the configuration file " << CONFIGURATIONFILENAME << endl;
      }
    }
  }

  return (styleFound&&layoutFound);
}

bool mainConfigHandler::getConfiguration(string& styleDirectory, string& layoutDirectory) {
  ifstream configFile;

  string homeDirectory = string(getenv(HOMEDIRECTORY));
  string configFileName = homeDirectory+"/"+CONFIGURATIONFILENAME;
  bool goodConfig;

  configFile.open(configFileName.c_str(), ifstream::in);
  if (!configFile.good()) {
    configFile.close();
    goodConfig = createConfigurationFileFromQuestions(configFileName, styleDirectory, layoutDirectory);
  } else {
    // I will read the configuration out of that
    goodConfig = readConfigurationFile(configFile, styleDirectory, layoutDirectory);
    configFile.close();
    if (goodConfig) {
      if (!checkDirectory(styleDirectory)) {
	cout << "You probably need to edit or delete the configuration file " << CONFIGURATIONFILENAME << endl;
	return false;
      }
      if (!checkDirectory(layoutDirectory)) {
	cout << "You probably need to edit or delete the configuration file " << CONFIGURATIONFILENAME << endl;
	return false;
      }
    } else { // not good config read
      cout << "Configuration file '" << configFileName << "' not properly formatted. You probably need to edit or delete it" << endl;
      return false;
    }
  }

  return goodConfig;
}
