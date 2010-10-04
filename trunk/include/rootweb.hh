#ifndef _ROOTWEB_HH_
#define _ROOTWEB_HH_

#define USING_ROOTWEB // TODO: remove this everywhere

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <TCanvas.h>

using namespace std;

#define THUMBSMALLSIZE 120
#define DEFAULTPROGRAMNAME "tkGeometry"
#define DEFAULTPROGRAMSITE "http://code.google.com/p/tkgeometry/"
// The following is a list of allowed file etensions for TCanvas::SaveAs
// It should be separated, start and end with '|'
#define DEFAULTALLOWEDEXTENSIONS "|C|png|gif|svg|root|eps|pdf|ps|"

class RootWItem {
public:
  virtual ~RootWItem() {};
  RootWItem() {taken=false;};
  virtual bool isTable() {return false;};
  virtual bool isImage() {return false;};
  virtual bool isText() {return false;};
  virtual bool isFile() {return false;};
  virtual ostream& dump(ostream& output) {return output;};
  bool taken;
};

class RootWText: public RootWItem {
public:
  RootWText() {myText_.clear();};
  RootWText(string newText) {myText_.str(newText); };
  ~RootWText() {};
  void addText(string newText) { myText_ << newText; };
  ostream& dump(ostream& output) {output << myText_.str(); return output;};
  bool isText() {return true;};
protected:
  stringstream myText_;
};

class RootWInfo: public RootWItem {
public:
  RootWInfo() {description_="missing_description";value_="missing_value";};
  RootWInfo(string description) {description_=description; value_="missing_value";};
  RootWInfo(string description, string value) {description_=description;value_=value;};
  ~RootWInfo() {};
  string setDescription(string newText) { description_ = newText; return newText; };
  string setValue(string newText);
  string setValue(int number);
  string setValue(double number, int precision);
  string addValueText(string newText) {value_+=newText; return value_;};
  ostream& dump(ostream& output);
protected:
  string description_;
  string value_;
};

typedef std::map<pair<int,int>, string> rootWTableContent;
class RootWTable : public RootWItem {
public:
  ~RootWTable() {};
  RootWTable();
  void setContent(int row, int column, string content);
  void setContent(int row, int column, int number);
  void setContent(int row, int column, double number, int precision);
  ostream& dump(ostream& output);
  pair<int, int> addContent(string content);
  pair<int, int> addContent(int number);
  pair<int, int> addContent(double number, int precision);
  pair<int, int> newLine();
  bool isTable() {return true;};
private:
  rootWTableContent tableContent_;
  int serialRow_, serialCol_;
};

typedef string RootWImageSize;

class RootWImage : public RootWItem {
public:
  ~RootWImage();
  RootWImage();
  RootWImage(TCanvas* myCanvas, int witdh, int height);
  RootWImage(TCanvas* myCanvas, int witdh, int height, string relativeHtmlDirectory); // TODO: is this used for real?
  void setCanvas(TCanvas* myCanvas);
  void setComment(string newComment);
  void setZoomedSize(int witdh, int height);
  void setRelativeHtmlDirectory(string newDirectory); 
  void setTargetDirectory(string newDirectory); 
  string saveFiles(int smallWidth, int smallHeight);
  string saveFiles(int smallWidth, int smallHeight, int largeWidth, int largeHeight);
  ostream& dump(ostream& output);
  bool isImage() {return true;};
  bool addExtension(string newExt);
private:
  TCanvas* myCanvas_;
  int zoomedWidth_;
  int zoomedHeight_;
  string relativeHtmlDirectory_;
  string targetDirectory_;
  map<RootWImageSize, string> myText_;
  map<RootWImageSize, bool> fileSaved_;
  RootWImageSize lastSize_;
  string comment_;
  RootWImageSize makeSizeCode(int sw, int sh, int lw, int lh);
  vector<string> fileTypeV_;

  static const double thumb_compression_ = 2.;
  string allowedExtensions_; // Will be initialized in the constructor
  void setDefaultExtensions();
};

class RootWFile : public RootWItem {
protected:
  string fileName_; // The destination file name
  string description_;
  string targetDirectory_;
public:
  RootWFile() {fileName_="aFile.txt";};
  ~RootWFile() {};
  RootWFile(string newFileName) { setFileName(newFileName); setDescription(""); };
  RootWFile(string newFileName, string newDescription) {setFileName(newFileName); setDescription(newDescription); };
  void setFileName(string newFileName) { fileName_ = newFileName;};
  void setDescription(string newDescription) { description_=newDescription; };
  string getFileName() { return fileName_ ; };
  string getDescription() { return description_ ; };
  void setTargetDirectory(string newTargetDirectory) {targetDirectory_ = newTargetDirectory; };
  bool isFile() {return true;};
};

class RootWTextFile : public RootWFile {
private:
  stringstream myText_;
public:
  RootWTextFile() {};
  ~RootWTextFile() {};
  RootWTextFile(string newFileName) {setFileName(newFileName); setDescription(""); };
  RootWTextFile(string newFileName, string newDescription) {setFileName(newFileName); setDescription(newDescription); };
  void addText(string newText) {myText_ << newText ; };
  void addText(stringstream& newText) {myText_ << newText.str() ; };
  ostream& dump(ostream& output);
};

class RootWBinaryFile : public RootWFile {
private:
  string originalFileName_;
public:
  RootWBinaryFile() {};
  ~RootWBinaryFile() {};
  RootWBinaryFile(string newFileName) {setFileName(newFileName); setDescription(""); setOriginalFile(""); };
  RootWBinaryFile(string newFileName, string newDescription) {setFileName(newFileName); setDescription(newDescription); setOriginalFile("");};
  RootWBinaryFile(string newFileName, string newDescription, string newOriginalFile) {
    setFileName(newFileName); setDescription(newDescription); setOriginalFile(newOriginalFile);};
  void setOriginalFile(string newFile) {originalFileName_ = newFile ; };
  ostream& dump(ostream& output);
};

class RootWContent {
public:
  ~RootWContent();
  RootWContent();
  RootWContent(string title, bool visible=true);
  void setTargetDirectory(string newTargetDirectory);
  void addParagraph(string parText) ;
  void setTitle(string newTitle) ;
  void addItem(RootWItem* newItem);
  ostream& dump(ostream& output);
  RootWText& addText();
  RootWText& addText(string newText);
  RootWInfo& addInfo();
  RootWInfo& addInfo(string description);
  RootWInfo& addInfo(string description, string value);
  RootWTable& addTable();
  RootWImage& addImage();
  RootWImage& addImage(TCanvas* myCanvas, int witdh, int height);
  RootWImage& addImage(TCanvas* myCanvas, int witdh, int height, string relativeHtmlDirectory); // TODO: is this used for real?
  RootWTextFile& addTextFile();
  RootWTextFile& addTextFile(string newFileName);
  RootWTextFile& addTextFile(string newFileName, string newDescription);
  RootWBinaryFile& addBinaryFile();
  RootWBinaryFile& addBinaryFile(string newFileName);
  RootWBinaryFile& addBinaryFile(string newFileName, string newDescription);
  RootWBinaryFile& addBinaryFile(string newFileName, string newDescription, string newOriginalFile);
private:
  bool visible_;
  string title_;
  //void setDefaultParameters();
  vector<RootWItem*> itemList_;
  string targetDirectory_;
};

class RootWPage;

class RootWSite {
protected:
private:
  vector<RootWPage*> pageList_;
  string title_;
  string comment_;
  vector<string> authorList_;
  string programName_;
  string programSite_;
  string revision_;
  string targetDirectory_;
  //string styleDirectory_;

public:
  ~RootWSite();
  RootWSite();
  RootWSite(string title);
  RootWSite(string title, string comment);
  void setTitle(string newTitle);
  void setComment(string newComment);
  string getTitle();
  string getComment();
  string getRevision();
  void setRevision (string newRevision);
  ostream& dumpHeader(ostream& output, RootWPage* thisPage);
  ostream& dumpFooter(ostream& output);
  void addPage(RootWPage* newPage); // TODO: deprecate this
  RootWPage& addPage(string title);
  void addAuthor(string newAuthor);
  void setTargetDirectory(string newTargetDirectory) {targetDirectory_ = newTargetDirectory; };
  //void setStyleDirectory(string newStyleDirectory) {styleDirectory_ = newStyleDirectory; } ;
  bool makeSite();
};

class RootWPage {
private:
  string title_;
  string address_;
  vector<RootWContent*> contentList_;
  RootWSite* site_;
  string targetDirectory_;

public:
  ~RootWPage();
  RootWPage();
  RootWPage(string title);
  void setTargetDirectory(string newTargetDirectory);
  void setTitle(string newTitle);
  string getTitle();
  void setAddress(string newAddress);
  string getAddress();
  void setSite(RootWSite* newSite);
  ostream& dump(ostream& output);
  void addContent(RootWContent* newContent);
  RootWContent& addContent(string title, bool visible=true);
};

class RootWItemCollection {
private:
  map<string, RootWItem*> itemCollection_;
public:
  RootWItemCollection() {};
  ~RootWItemCollection() {};
  RootWItem* getItem(string itemName);
  void  addItem(RootWItem* anItem, string itemName);
  vector<RootWItem*> getOtherItems();
};


  
#endif
