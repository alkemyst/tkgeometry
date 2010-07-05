#include <rootweb.hh>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <TError.h>
#include <time.h>
#include <boost/filesystem/operations.hpp>

//*******************************************//
// RootWTable                                //
//*******************************************//
RootWTable::RootWTable() {
  serialRow_ = 0;
  serialCol_ = 0;
}

ostream& RootWTable::dump(ostream& output) {
  RootWTable myRootWTable = (*this);

  int minRow, maxRow;
  int minCol, maxCol;
  bool firstNotFound = true;

  pair<int, int> myIndex;
  rootWTableContent::iterator tableContentIt;
  rootWTableContent myTableContent = myRootWTable.tableContent_;

  for (tableContentIt = myTableContent.begin();
       tableContentIt != myTableContent.end(); ++tableContentIt) {
    myIndex = (*tableContentIt).first;
    if (firstNotFound) {
      firstNotFound = false;
      minRow = myIndex.first;
      maxRow = myIndex.first;
      minCol = myIndex.second;
      maxCol = myIndex.second;
    } else {
      minRow = ( myIndex.first < minRow ) ? myIndex.first : minRow;
      maxRow = ( myIndex.first > maxRow ) ? myIndex.first : maxRow;
      minCol = ( myIndex.first < minCol ) ? myIndex.first : minCol;
      maxCol = ( myIndex.first > maxCol ) ? myIndex.first : maxCol;
    }
  }

  if (firstNotFound) return output;

  output << "<table>";
  for (int iRow = minRow; iRow<=maxRow; ++iRow) { 
    output << "<tr>";
    for (int iCol = minCol; iCol<=maxCol; ++iCol) {
      output << "<td>" << myTableContent[make_pair(iRow, iCol)]  << "</td>" << " ";
    }
    output << "</tr>";
  }
  output << "</table>" << endl;
  
  return output;
}

void RootWTable::setContent(int row, int column, string content) {
  tableContent_[make_pair(row, column)] = content;
}

void RootWTable::setContent(int row, int column, int number) {
  stringstream myNum_;
  myNum_.clear();
  myNum_ << dec << number;
  tableContent_[make_pair(row, column)] = myNum_.str();
}

void RootWTable::setContent(int row, int column, double number, int precision) {
  stringstream myNum_;
  myNum_.clear();
  myNum_ << dec << fixed << setprecision(precision) << number;
  tableContent_[make_pair(row, column)] = myNum_.str();
}

pair<int, int> RootWTable::addContent(string myContent) {
  setContent(serialRow_, serialCol_++, myContent);
  return make_pair(serialRow_, serialCol_);
}

pair<int, int> RootWTable::addContent(int number) {
  stringstream myNum_;
  myNum_.clear();
  myNum_ << dec << number;
  return(addContent(myNum_.str()));
}

pair<int, int> RootWTable::addContent(double number, int precision) {
  stringstream myNum_;
  myNum_.clear();
  myNum_ << dec << fixed << setprecision(precision) << number;
  return(addContent(myNum_.str()));
}

pair<int, int> RootWTable::newLine() {
  serialRow_++;
  serialCol_=0;
  return make_pair(serialRow_, serialCol_);
}

//*******************************************//
// RootWImage                                //
//*******************************************//

RootWImage::RootWImage() {
  myCanvas_ = NULL;
  zoomedWidth_ = 0; zoomedHeight_ = 0;
  relativeHtmlDirectory_ = "";
  targetDirectory_ = "";
  comment_ = "";
  allowedExtensions_ = DEFAULTALLOWEDEXTENSIONS;
  setDefaultExtensions();
}

RootWImage::RootWImage(TCanvas* myCanvas, int witdh, int height) {
  setCanvas(myCanvas);
  setZoomedSize(witdh, height);
  relativeHtmlDirectory_ = "";
  targetDirectory_ = "";
  comment_ = "";
  allowedExtensions_ = DEFAULTALLOWEDEXTENSIONS;
  setDefaultExtensions();
}

RootWImage::RootWImage(TCanvas* myCanvas, int witdh, int height, string relativehtmlDirectory) {
  setCanvas(myCanvas);
  setZoomedSize(witdh, height);
  setRelativeHtmlDirectory(relativehtmlDirectory);
  targetDirectory_ = "";
  comment_ = "";
  allowedExtensions_ = DEFAULTALLOWEDEXTENSIONS;
  setDefaultExtensions();
}

void RootWImage::setDefaultExtensions() {
   addExtension("C");
   addExtension("pdf");
}

void RootWImage::setComment(string newComment) {
  comment_ = newComment;
}

void RootWImage::setCanvas(TCanvas* myCanvas) {
  myCanvas_ = myCanvas;
}

void RootWImage::setZoomedSize(int witdh, int height) {
  zoomedWidth_ = witdh;
  zoomedHeight_ = height;
}

void RootWImage::setRelativeHtmlDirectory(string newDirectory) {
  relativeHtmlDirectory_ = newDirectory;
}

void RootWImage::setTargetDirectory(string newDirectory) {
  targetDirectory_ = newDirectory;
}

string RootWImage::saveFiles(int smallWidth, int smallHeight, int largeWidth, int largeHeight) {
  setZoomedSize(largeWidth, largeHeight);
  return saveFiles(smallWidth, smallHeight);
}

RootWImageSize RootWImage::makeSizeCode(int sw, int sh, int lw, int lh) {
  stringstream aString;
  aString << sw <<"x" << sh << "-" << lw <<"x" << lh;
  return aString.str();
}

string RootWImage::saveFiles(int smallWidth, int smallHeight) {
  RootWImageSize myImageSize;
  myImageSize = makeSizeCode(smallWidth,smallHeight,zoomedWidth_,zoomedHeight_);
  if (fileSaved_[myImageSize]) {
    return myText_[myImageSize];
  }

  stringstream thisText;  
  thisText.clear();
  if (relativeHtmlDirectory_ == "") relativeHtmlDirectory_ = ".";
  if (targetDirectory_ == "" ) targetDirectory_ = ".";

  if (!myCanvas_) return "";

  string canvasName = myCanvas_->GetName();
  string smallCanvasFileName = canvasName + "-" + myImageSize + "_small.png";
  string largeCanvasFileBaseName = canvasName + "-" + myImageSize;

  double wScale = double(smallWidth) / double(zoomedWidth_);
  double hScale = double(smallHeight) / double(zoomedHeight_);
  double myScale;

  myScale =  (hScale < wScale) ? hScale : wScale;
  int imgW = int (zoomedWidth_ * myScale);
  int imgH = int (zoomedHeight_ * myScale);
  int canW = int(imgW * thumb_compression_);
  int canH = int(imgH * thumb_compression_);
  
  string smallCanvasCompleteFileName = targetDirectory_+"/"+smallCanvasFileName;
  string largeCanvasCompleteFileName = targetDirectory_+"/"+largeCanvasFileBaseName + ".png";

  gErrorIgnoreLevel = 1500;

  myCanvas_->SetCanvasSize(canW, canH);
  myCanvas_->SaveAs(smallCanvasCompleteFileName.c_str());
  myCanvas_->SetCanvasSize(zoomedWidth_, zoomedHeight_);
  myCanvas_->SaveAs(largeCanvasCompleteFileName.c_str());

  string fileTypeList;
  for (vector<string>::iterator it=fileTypeV_.begin(); it!=fileTypeV_.end(); ++it) {
    largeCanvasCompleteFileName = targetDirectory_+"/"+largeCanvasFileBaseName + "." + (*it);
    myCanvas_->SaveAs(largeCanvasCompleteFileName.c_str());
    if (it!=fileTypeV_.begin()) fileTypeList+="|";
    fileTypeList+=(*it);
  }
  
  thisText << "<img class='wleftzoom' width='"<< imgW<<"' height='"<<imgH<<"' src='"<< relativeHtmlDirectory_ << "/" << smallCanvasFileName <<"' alt='"<< comment_ <<"' onclick=\"popupDiv('" << relativeHtmlDirectory_ << "/" << largeCanvasFileBaseName << "', "<< zoomedWidth_<<", "<< zoomedHeight_ << " , '"<< comment_ << "','"<<fileTypeList << "');\" />";
  
  myText_[myImageSize] = thisText.str();
  fileSaved_[myImageSize] = true;
  lastSize_ = myImageSize;
  return thisText.str();
}

ostream& RootWImage::dump(ostream& output) {
  output << myText_[lastSize_];
  return output;
}

bool RootWImage::addExtension(string myExtension) {
  // First check if the extension has not a "|" inside, which
  // would invalidate the search
  if (myExtension.find("|")!=string::npos) {
    cerr << "In bool RootWImage::addExtension(string myExtension) extension contains '|', which is not allowed" << endl;
    return false;
  }
  bool result = false;

  // Then create the real search pattern
  string mySearchExtension="|"+myExtension+"|";

  // Last: check if the extension is in the list of allowed extensions
  if (allowedExtensions_.find(mySearchExtension)!=string::npos) {
    // Try to find if the extension is already registered
    vector<string>::iterator anExtension = find(fileTypeV_.begin(), fileTypeV_.end(), myExtension);
    if (anExtension == fileTypeV_.end()) {
      fileTypeV_.push_back(myExtension);
      result=true;
    }
  } else {
    cerr << "Extension '" << myExtension << "' is not allowed for canvas saving: new file type not added." << endl;
  }

  return result;
}


//*******************************************//
// RootWContent                              //
//*******************************************//

RootWContent::RootWContent() {
  title_ = "Untitled";
  targetDirectory_ = "";
}

RootWContent::RootWContent(string title) {
  title_ = title;
  targetDirectory_ = "";
}

void RootWContent::setTargetDirectory(string newTargetDirectory) {
  targetDirectory_ = newTargetDirectory;
}

RootWContent::~RootWContent() {
  RootWItem* myItem;

  while(!itemList_.empty()) {
    myItem = (*itemList_.begin());
    if (myItem!=NULL) {
      //delete myItem;
      myItem = NULL;
    }
    itemList_.erase(itemList_.begin());
  }
}

void RootWContent::addItem(RootWItem* newItem) {
  if (newItem) {
    itemList_.push_back(newItem);
  }
}

void RootWContent::setTitle(string newTitle) {
  title_ = newTitle;
}

void RootWContent::addParagraph(string parText) {
  RootWText* newText = new RootWText;
  //newText->addText("<p>");
  newText->addText(parText);
  newText->addText("<br/>\n");
  //newText->addText("</p>\n");
  
  
  addItem(newText);
}

ostream& RootWContent::dump(ostream& output) {
  vector<RootWItem*>::iterator it;
  RootWItem* myItem;
  RootWImage* myImage;
  RootWFile* myFile;
  output << "<h2 class=\"hidingTitle\">"<<title_<<"</h2>" << endl;
  output << "<div class=\"hideable\"> ";
  for (it = itemList_.begin(); it != itemList_.end(); ++it) {
    myItem =(*it);
    if (myItem->isImage()) {
      if ( (myImage=dynamic_cast<RootWImage*>(myItem)) ) {
	myImage->setTargetDirectory(targetDirectory_);
	myImage->saveFiles(THUMBSMALLSIZE, THUMBSMALLSIZE);
      } else {
	cout << "WARNING: this should never happen. contact the author immediately!" << endl;
      }
    }
    if (myItem->isFile()) {
      if ( (myFile=dynamic_cast<RootWFile*>(myItem)) ) {
        myFile->setTargetDirectory(targetDirectory_);
      } else {
        cout << "WARNING: this should never happen. contact the author immediately!" << endl;
      }
    }
    myItem->dump(output);
  }
  output << "<div class=\"clearer\">&nbsp;</div>";
  output << "</div>" << endl;
  return output;
}

//*******************************************//
// RootWPage                                 //
//*******************************************//

RootWPage::RootWPage() {
  address_ = "";
  title_ = "Untitled";
  site_ = NULL;
  targetDirectory_ = "";
}

RootWPage::RootWPage(string title) {
  setTitle(title);
  site_ = NULL;
  targetDirectory_ = "";
}

RootWPage::~RootWPage() {
}

void RootWPage::setTargetDirectory(string newTargetDirectory) {
  targetDirectory_ = newTargetDirectory;
}

void RootWPage::setTitle(string newTitle) {
  title_ = newTitle;
  if (address_=="") address_ = title_+".html";
}

string RootWPage::getTitle() {
  return title_;
}

void RootWPage::setAddress(string newAddress) {
  address_ = newAddress;
}

string RootWPage::getAddress() {
  return address_;
}

void RootWPage::setSite(RootWSite* newSite) {
  site_ = newSite;
}


void RootWPage::addContent(RootWContent* newContent) {
  if (newContent) {
    contentList_.push_back(newContent);
  }
}

ostream& RootWPage::dump(ostream& output) {
  bool fakeSite=false;
  if (site_==NULL) {
    fakeSite = true;
    site_ = new RootWSite();
    site_->addPage(this);
  }
  
  // Header standard part, with the menu list, etc...
  // up to opening the primaryContent div
  site_->dumpHeader(output, this);
  
  vector<RootWContent*>::iterator it;
  RootWContent* myContent;
  for (it = contentList_.begin(); it != contentList_.end(); ++it) {
    myContent =(*it);
    myContent->setTargetDirectory(targetDirectory_);
    myContent->dump(output);
  }
  
  // Header standard part, with copyright, etc...
  // starting from closing the primaryContent div
  site_->dumpFooter(output);
  
  if (fakeSite) {
    delete site_;
    site_=NULL;
  }
  
  return output;
  
}

//*******************************************//
// RootWSite                                 //
//*******************************************//

RootWSite::RootWSite() {
  title_ = "Untitled";
  comment_ = "";
  programName_ = DEFAULTPROGRAMNAME;
  programSite_ = DEFAULTPROGRAMSITE;
  revision_="";
  targetDirectory_ = ".";
  styleDirectory_ = ".";
}

RootWSite::RootWSite(string title) {
  title_ = title;
  comment_ = "";
  programName_ = DEFAULTPROGRAMNAME;
  programSite_ = DEFAULTPROGRAMSITE;
  revision_="";
  targetDirectory_ = ".";
  styleDirectory_ = ".";
}

RootWSite::RootWSite(string title, string comment) {
  title_ = title;
  comment_ = comment;
  programName_ = DEFAULTPROGRAMNAME;
  programSite_ = DEFAULTPROGRAMSITE;
  revision_="";
  targetDirectory_ = ".";
  styleDirectory_ = ".";
}

RootWSite::~RootWSite() {
}

void RootWSite::setTitle(string newTitle) {
  title_ = newTitle;
}

void RootWSite::setComment(string newComment) {
  comment_ = newComment;
}

string RootWSite::getTitle() {
  return title_;
}

string RootWSite::getComment() {
  return comment_;
}

string RootWSite::getRevision() {
  return revision_;
}

void RootWSite::setRevision(string newRevision) {
  revision_ = newRevision;
}

void RootWSite::addPage(RootWPage* newPage) {
  pageList_.push_back(newPage);
  newPage->setSite(this);
}

void RootWSite::addAuthor(string newAuthor) {
  authorList_.push_back(newAuthor);
}

ostream& RootWSite::dumpFooter(ostream& output) {
  output  << "          </div>" << endl
	  << "        </div>" << endl
	  << "        <div class=\"clear\"></div>" << endl
	  << "      </div>" << endl
	  << "      <div id=\"footer\">" << endl;

  // Add the list of authors if any
  bool firstAuthorFound=false;
  for (vector<string>::iterator it= authorList_.begin();
       it != authorList_.end(); ++it) {
    if (!firstAuthorFound) {
      firstAuthorFound = true;
      output << "        <p>&copy; ";
      output << (*it);
    } else {
      output << ", " << (*it);
    }
  }
  if (firstAuthorFound) output <<"</p>" << endl;

  time_t rawtime;
  time ( &rawtime );
  output << "        <p>Page created on "<< asctime(gmtime ( &rawtime )) << " GMT</p>" << endl
	 << "        <p>by <a href=\""<< programSite_ <<"\">"<<programName_<<"</a>";
  if (revision_!="")
    output << " revision " << revision_;
  output << "</p>" << endl
	 << "      </div>" << endl
	 << "    </div>" << endl
	 << "  </body>" << endl
	 << "</html>" << endl;
  return output;
}

ostream& RootWSite::dumpHeader(ostream& output, RootWPage* thisPage) {
  output << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl
	 << "  <head>" << endl
	 << "    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />" << endl
	 << "    <title>"<< title_ << " - " << thisPage->getTitle() <<"</title>" << endl
	 << "    <meta name=\"keywords\" content=\"CERN CMS tracker upgrade\" />" << endl
	 << "    <meta name=\"description\" content=\"CMS Tracker upgrade summary page\" />" << endl
	 << "    <link href=\"style/default.css\" rel=\"stylesheet\" type=\"text/css\" />" << endl
	 << "    <link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"style/images/favicon.ico\">" << endl
	 << " </head>" << endl
	 << "  <script type=\"text/javascript\" src=\"style/functions.js\"></script>" << endl
	 << "  <body onload=\"preparePageGoodies();\">" << endl
	 << "    <div id=\"outer\">" << endl
	 << "      <div id=\"header\">" << endl
	 << "        <h1>" << endl
	 << "    <a href=\"index.html\">" << title_ << "</a>" << endl
	 << "        </h1>" << endl
	 << "    <h2>"<<comment_<<"</h2>" << endl
	 << "      </div>" << endl
	 << "      <div id=\"menu\">" << endl
	 << "        <ul>" << endl;
  for (vector<RootWPage*>::iterator it = pageList_.begin();
       it !=pageList_.end(); ++it) {
    output << "          <li";
    if ((*it)==(thisPage)) output << " class=\"current\"";
    output << ">" << "    <a href=\"" << (*it)->getAddress() << "\" >"<< (*it)->getTitle() <<"</a>" << "          </li>" << endl;
  }
  output << "        </ul>" << endl
	 << "      </div>" << endl
	 << "      <div id=\"content\">" << endl
	 << "        <div id=\"primaryContentContainer\">" << endl
	 << "          <div id=\"primaryContent\">" << endl;
  
  return output;
  
}

bool RootWSite::makeSite() {
  ofstream myPageFile;
  RootWPage* myPage;
  string myPageFileName;
  string targetStyleDirectory = targetDirectory_ + "/style";


  // Check if the directory already exists
  if (boost::filesystem::exists( targetDirectory_ )) {
    if (! boost::filesystem::is_directory(targetDirectory_) ) {
      cerr << "A file named " << targetDirectory_ << " already exists. Cannot create target directory" << endl;
      return false;
    }
  } else {
    // If the directory does not exist, then try to create it
    if (!boost::filesystem::create_directory(targetDirectory_)) {
      cerr << "Couldn't create directory " << targetDirectory_ << endl;
      return false;
    }
  }
  
  // Recreate the style symlink
  if (boost::filesystem::exists( targetStyleDirectory )) {
    boost::filesystem::remove_all( targetStyleDirectory );
  }
  boost::filesystem::create_symlink(styleDirectory_, targetStyleDirectory);
  
  vector<RootWPage*>::iterator it;
  for (it=pageList_.begin(); it!=pageList_.end(); it++) {
    myPage = (*it);
    myPageFileName = targetDirectory_+"/"+myPage->getAddress();
    myPageFile.open(myPageFileName.c_str(), ios::out);
    myPage->setTargetDirectory(targetDirectory_);
    myPage->dump(myPageFile);
    myPageFile.close();
  }

  return true;
}

//*******************************************//
// RootWItemCollection                       //
//*******************************************//


RootWItem* RootWItemCollection::getItem(string itemName) {
  RootWItem* result;
  result=itemCollection_[itemName];
  if (result!=NULL) result->taken=true;
  return result;
}

void RootWItemCollection::addItem(RootWItem* anItem, string itemName) {
  RootWItem* findItem;
  findItem=itemCollection_[itemName];
  if (findItem!=NULL) {
    cerr << "Warning in RootWItemCollection::addItem(): item named " << itemName << " already exists! Overwriting it." << endl;
  }
  itemCollection_[itemName]=anItem;
}

vector<RootWItem*> RootWItemCollection::getOtherItems() {
  vector<RootWItem*> result;
  map<string, RootWItem*>::iterator itItem=itemCollection_.begin();
  map<string, RootWItem*>::iterator endItem=itemCollection_.end();
  for (; itItem!=endItem; ++itItem) {
    if (!(*itItem).second->taken) result.push_back((*itItem).second);
  }
  return result;
}

//*******************************************//
// RootWTextFile                             //
//*******************************************//

ostream& RootWTextFile::dump(ostream& output) {

  if (fileName_=="") {
    cerr << "Warning: RootWTextFile::dump() was called without prior setting the destination file name" << endl;
    return output;
  }

  std::ofstream outputFile;
  string destinationFileName = targetDirectory_ +"/" + fileName_;
  outputFile.open(destinationFileName.c_str());
  // TODO: add a check heer if the file was correctly opened
  //outputFile << myText_.str();
  //myText_ >> outputFile;
  outputFile << myText_.str() << endl;
  outputFile.close();

  output << "<b>" << description_ << ":</b> <a href=\""
         << fileName_ << "\">"
         << fileName_ << "</a></tt><br/>";
  return output;
};


//*******************************************//
// RootWBinaryFile                           //
//*******************************************//

ostream& RootWBinaryFile::dump(ostream& output) {
  if (originalFile_=="") {
    cerr << "Warning: RootWBinaryFile::dump() was called without prior setting the original file name" << endl;
    return output;
  } 
  if (fileName_=="") {
    cerr << "Warning: RootWBinaryFile::dump() was called without prior setting the destination file name" << endl;
    return output;
  }

  std::ofstream outputFile;
  string destinationFileName = targetDirectory_ +"/" + fileName_;

  if (boost::filesystem::exists(originalFile_)) {
    try {
      boost::filesystem::copy_file(configFileName, destConfigFile);
    } catch (...) {
      cerr << "Problem copying the file '" << originalFile_ << "' to '" << destinationFileName << "'" << endl;
    }
  }

  outputFile.open(destinationFileName.c_str());
  // TODO: add a check heer if the file was correctly opened
  //outputFile << myText_.str();
  //myText_ >> outputFile;
  outputFile << myText_.str() << endl;
  outputFile.close();

  output << "<b>" << description_ << ":</b> <a href=\""
         << fileName_ << "\">"
         << fileName_ << "</a></tt><br/>";
  return output;
};

