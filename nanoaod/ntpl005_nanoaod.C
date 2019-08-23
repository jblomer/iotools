/// \file
/// \ingroup tutorial_ntuple
/// \notebook
/// Convert LHCb run 1 open data from a TTree to RNTuple.
/// This tutorial illustrates data conversion for a simple, tabular data model.
/// For reading, the tutorial shows the use of an ntuple View, which selectively accesses specific fields.
/// If a view is used for reading, there is no need to define the data model as an RNTupleModel first.
/// The advantage of a view is that it directly accesses RNTuple's data buffers without making an additional
/// memory copy.
///
/// \macro_image
/// \macro_code
///
/// \date April 2019
/// \author The ROOT Team

// NOTE: The RNTuple classes are experimental at this point.
// Functionality, interface, and data format is still subject to changes.
// Do not use for real data!

#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>

#include <TBranch.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

// Import classes from experimental namespace for the time being
using RFieldBase = ROOT::Experimental::Detail::RFieldBase;

constexpr char const* kDatasetName = "Events";
constexpr char const* kTreeFileName = "/data/cms/nanoaod.root";
constexpr char const* kNTupleFileName = "ntpl005_nanoaod.root";

struct ClassDecl {
   std::string fClassName;
   std::vector<std::string> fMembers;
   explicit ClassDecl(const std::string &className) : fClassName(className) {}
   bool operator== (const ClassDecl &other) const { return fClassName == other.fClassName; }
};

struct BranchDef {
   std::string fBranchName;
   std::string fInClass;
   std::string fTypeName;
   bool fIsCollection = false;
};

std::vector<ClassDecl> classes;
std::vector<BranchDef> branches;
std::vector<std::string> model;
std::vector<std::string> branchAddresses;

void CodegenField(const std::string &fldName, const std::string &fldType, bool isCollection)
{
   std::string type = isCollection ? "std::vector<" + fldType + ">" : fldType;
   model.emplace_back("model->MakeField<" + type + ">(\"" + fldName + "\");");
}

void CodegenMember(const std::string &className, const std::string &memberName, const std::string &typeName)
{
   auto classItr = std::find(classes.begin(), classes.end(), ClassDecl(className));
   if (classItr == classes.end()) {
      classes.emplace_back(ClassDecl(className));
      classItr = classes.begin() + (classes.size() - 1);
   }
   classItr->fMembers.emplace_back(typeName + " " + memberName + ";");
}

void CodegenClasses(std::ostream &output = std::cout)
{
   for (auto c : classes) {
      output << "struct " << c.fClassName << " {" << std::endl;
      for (auto m : c.fMembers) {
         output << "   " << m << std::endl;
      }
      output << "};" << std::endl;
   }
}

void CodegenLinkdef(std::ostream &output = std::cout)
{
   output << "#ifdef __CINT__" << std::endl;
   output << "#pragma link off all globals;" << std::endl;
   output << "#pragma link off all classes;" << std::endl;
   output << "#pragma link off all functions;" << std::endl;
   for (auto c : classes) {
      output << "#pragma link C++ class " << c.fClassName << "+;" << std::endl;
   }
   output << "#endif" << std::endl;
}

void CodegenModel(std::ostream &output = std::cout)
{
   output << "std::unique_ptr<RNTupleModel> MakeModel() {" << std::endl;
   output << "   auto model = RNTupleModel::Create();" << std::endl;
   for (auto f : model) {
      output << "   " << f << std::endl;
   }
   output << "   return model;" << std::endl;
   output << "}" << std::endl;
}

void CodegenMakefile(std::ostream &output = std::cout)
{
   output << "CXX = g++" << std::endl;
   output << "CXXFLAGS = $(shell root-config --cflags) -Wall -pthread -g -O2" << std::endl;
   output << "LDFLAGS = $(shell root-config --libs) -lROOTNTuple" << std::endl;
   output << ".PHONY = clean" << std::endl;
   output << "all: convert libClasses.so" << std::endl;
   output << "classes.cxx: classes.hxx linkdef.h" << std::endl;
   output << "\trootcling -f $@ $^" << std::endl;
   output << "libClasses.so: classes.cxx" << std::endl;
   output << "\t$(CXX) -shared -fPIC $(CXXFLAGS) -o$@ $< $(LDFLAGS)" << std::endl;
   output << "convert: convert.cxx classes.hxx" << std::endl;
   output << "\t$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)" << std::endl;
   output << "clean:" << std::endl;
   output << "\trm -f classes.cxx libClasses.so convert" << std::endl;
}

void CodegenPreamble(std::ostream &output = std::cout)
{
   output << "#include <ROOT/RNTuple.hxx>" << std::endl;
   output << "#include <ROOT/RNTupleModel.hxx>" << std::endl;
   output << "#include <TBranch.h>" << std::endl;
   output << "#include <TCollection.h>" << std::endl;
   output << "#include <TFile.h>" << std::endl;
   output << "#include <TSystem.h>" << std::endl;
   output << "#include <TTree.h>" << std::endl;
   output << "#include <iostream>" << std::endl;
   output << "#include <memory>" << std::endl;
   output << "#include <utility>" << std::endl;
   output << "#include <vector>" << std::endl;
   output << "#include \"classes.hxx\"" << std::endl;
   output << "using RNTupleModel = ROOT::Experimental::RNTupleModel;" << std::endl;
   output << "using RNTupleReader = ROOT::Experimental::RNTupleReader;" << std::endl;
   output << "using RNTupleWriter = ROOT::Experimental::RNTupleWriter;" << std::endl;
}

void CodegenConvert(std::ostream &output = std::cout)
{
   output << "void Convert(TTree *tree, std::unique_ptr<RNTupleModel> model) {" << std::endl;

   for (auto b : branches) {
      if (b.fInClass.empty() && !b.fIsCollection) {
         output << "   {" << std::endl;
         output << "      void *fieldDataPtr = model->GetDefaultEntry()->GetValue(\""
                << b.fBranchName + "\").GetRawPtr();" << std::endl;
         output << "      tree->SetBranchAddress(\"" << b.fBranchName << "\", fieldDataPtr);" << std::endl;
         output << "   }" << std::endl;
      }
      if (b.fIsCollection) {
         output << "   unsigned int num" << b.fBranchName << ";" << std::endl;
         output << "   tree->SetBranchAddress(\"" << b.fBranchName << "\", &num" << b.fBranchName << ");" << std::endl;
         output << "   auto fld" << b.fBranchName << " = model->Get<std::vector<" << b.fBranchName << ">>(\""
                << b.fBranchName + "\");" << std::endl;
      }
      if (!b.fInClass.empty()) {
         output << "   " << b.fTypeName << " *arr" << b.fBranchName << " = new "
                << b.fTypeName << "[10000];" << std::endl;
         output << "   tree->SetBranchAddress(\"" << b.fBranchName << "\", arr" << b.fBranchName
                << ");" << std::endl;
      }
   }
   output << "   auto ntuple = RNTupleWriter::Recreate(std::move(model), \"NTuple\", \"ntuple.root\");" << std::endl;
   output << "   auto nEntries = tree->GetEntries();" << std::endl;
   output << "   for (decltype(nEntries) i = 0; i < nEntries; ++i) {" << std::endl;
   output << "      tree->GetEntry(i);" << std::endl;
   for (auto v : branches) {
      if (!v.fIsCollection)
         continue;
      output << "      fld" << v.fBranchName << "->resize(num" << v.fBranchName << ");" << std::endl;
      output << "      for (unsigned int j = 0; j < num" << v.fBranchName << "; ++j) {" << std::endl;
      for (auto l : branches) {
         if (l.fInClass != v.fBranchName)
            continue;
         output << "         (*fld" << v.fBranchName << ")[j]." << l.fBranchName << " = arr"
                << l.fBranchName << "[j];" << std::endl;
      }
      output << "      }" << std::endl;
   }
   output << "      ntuple->Fill();" << std::endl;
   output << "      if (i && i % 1000 == 0)" << std::endl;
   output << "         std::cout << \"Wrote \" << i << \" entries\" << std::endl;" << std::endl;
   output << "   }" << std::endl;
   output << "}" << std::endl;
}


void CodegenVerify(std::ostream &output = std::cout)
{
   output << "void Verify() {" << std::endl;
   output << "   auto ntuple = RNTupleReader::Open(\"NTuple\", \"ntuple.root\");" << std::endl;
   output << "   ntuple->PrintInfo();" << std::endl;
   output << "}" << std::endl;
}

void CodegenMain(const std::string &treeFileName, const std::string &treeName, std::ostream &output = std::cout)
{
   output << "int main() {" << std::endl;
   output << "   gSystem->Load(\"./libClasses.so\");" << std::endl;
   output << "   auto model = MakeModel();" << std::endl;
   output << "   std::unique_ptr<TFile> f(TFile::Open(\"" + treeFileName + "\"));" << std::endl;
   output << "   auto tree = f->Get<TTree>(\"" + treeName + "\");" << std::endl;
   output << "   Convert(tree, std::move(model));" << std::endl;
   output << "   Verify();" << std::endl;
   output << "}" << std::endl;
}

void MakeNTuple(const std::string &treeFileName, const std::string &treeName)
{
   std::unique_ptr<TFile> f(TFile::Open(kTreeFileName));
   assert(f && ! f->IsZombie());

   auto tree = f->Get<TTree>(treeName.c_str());
   for (auto b : TRangeDynCast<TBranch>(*tree->GetListOfBranches())) {
      // The dynamic cast to TBranch should never fail for GetListOfBranches()
      assert(b);
      assert(b->GetNLeaves() == 1);
      auto l = static_cast<TLeaf*>(b->GetListOfLeaves()->First());

      auto field = std::unique_ptr<RFieldBase>(RFieldBase::Create(l->GetName(), l->GetTypeName()));
      auto szLeaf = l->GetLeafCount();
      if (szLeaf) {
         CodegenMember(szLeaf->GetName(), field->GetName(), field->GetType());
      }
   }

   for (auto b : TRangeDynCast<TBranch>(*tree->GetListOfBranches())) {
      assert(b);
      auto l = static_cast<TLeaf*>(b->GetListOfLeaves()->First());
      BranchDef branchDef;
      branchDef.fBranchName = b->GetName();
      if (l->GetLeafCount()) {
         auto field = std::unique_ptr<RFieldBase>(RFieldBase::Create(l->GetName(), l->GetTypeName()));
         branchDef.fInClass = l->GetLeafCount()->GetName();
         branchDef.fTypeName = field->GetType();
         branches.emplace_back(branchDef);
         continue;
      }

      bool isCollection = std::find(classes.begin(), classes.end(), ClassDecl(l->GetName())) != classes.end();
      branchDef.fIsCollection = isCollection;

      if (isCollection) {
         CodegenField(l->GetName(), l->GetName(), true);
         branchDef.fTypeName = l->GetName();
      } else {
         auto field = std::unique_ptr<RFieldBase>(RFieldBase::Create(l->GetName(), l->GetTypeName()));
         CodegenField(field->GetName(), field->GetType(), false);
         branchDef.fTypeName = field->GetType();
      }
      branches.emplace_back(branchDef);
   }

   gSystem->MakeDirectory("ntuple");
   std::ofstream fclasses("ntuple/classes.hxx", std::ofstream::out | std::ofstream::trunc);
   CodegenClasses(fclasses);
   fclasses.close();

   std::ofstream flinkdef("ntuple/linkdef.h", std::ofstream::out | std::ofstream::trunc);
   CodegenLinkdef(flinkdef);
   flinkdef.close();

   std::ofstream fmain("ntuple/convert.cxx", std::ofstream::out | std::ofstream::trunc);
   CodegenPreamble(fmain);
   CodegenModel(fmain);
   CodegenConvert(fmain);
   CodegenVerify(fmain);
   CodegenMain(treeFileName, treeName, fmain);
   fmain.close();

   std::ofstream fmakefile("ntuple/Makefile", std::ofstream::out | std::ofstream::trunc);
   CodegenMakefile(fmakefile);
   fmakefile.close();
}


void ntpl005_nanoaod()
{
   MakeNTuple(kTreeFileName, kDatasetName);
   return;
}
