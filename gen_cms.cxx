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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <unistd.h>

#include "util.h"

// Import classes from experimental namespace for the time being
using RFieldBase = ROOT::Experimental::Detail::RFieldBase;

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
   output << "\trm -f classes.cxx libClasses.so convert classes_rdict.pcm" << std::endl;
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
   output << "#include <TROOT.h>" << std::endl;
   output << "#include <iostream>" << std::endl;
   output << "#include <memory>" << std::endl;
   output << "#include <utility>" << std::endl;
   output << "#include <vector>" << std::endl;
   output << "#include \"classes.hxx\"" << std::endl;
   output << "using RNTupleModel = ROOT::Experimental::RNTupleModel;" << std::endl;
   output << "using RNTupleReader = ROOT::Experimental::RNTupleReader;" << std::endl;
   output << "using RNTupleWriter = ROOT::Experimental::RNTupleWriter;" << std::endl;
}

void CodegenConvert(std::string ntupleFile, bool imt, unsigned bloatFactor = 1, std::ostream &output = std::cout)
{
   output << "void Convert(TTree *tree, std::unique_ptr<RNTupleModel> model, int compression) {" << std::endl;
   if (imt)
      output << "  ROOT::EnableImplicitMT();" << std::endl;

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
   output << "   ROOT::Experimental::RNTupleWriteOptions options;" << std::endl;
   output << "   options.SetCompression(compression);" << std::endl;
   output << "   options.SetNEntriesPerCluster(5000);" << std::endl;
   output << "   auto ntuple = RNTupleWriter::Recreate(std::move(model), \"NTuple\", \"" << ntupleFile
          << "\", options);" << std::endl;
   output << "   auto nEntries = tree->GetEntries();" << std::endl;
   output << "   for (unsigned bl = 0; bl < " << bloatFactor << "; ++bl) {" << std::endl;
   output << "      for (decltype(nEntries) i = 0; i < nEntries; ++i) {" << std::endl;
   output << "         tree->GetEntry(i);" << std::endl;
   for (auto v : branches) {
      if (!v.fIsCollection)
         continue;
      output << "         fld" << v.fBranchName << "->resize(num" << v.fBranchName << ");" << std::endl;
      output << "         for (unsigned int j = 0; j < num" << v.fBranchName << "; ++j) {" << std::endl;
      for (auto l : branches) {
         if (l.fInClass != v.fBranchName)
            continue;
         output << "            (*fld" << v.fBranchName << ")[j]." << l.fBranchName << " = arr"
                << l.fBranchName << "[j];" << std::endl;
      }
      output << "         }" << std::endl;
   }
   output << "         ntuple->Fill();" << std::endl;
   output << "         if (i && i % 1000 == 0)" << std::endl;
   output << "            std::cout << \"Wrote \" << i << \" entries\" << std::endl;" << std::endl;
   output << "      }" << std::endl;
   output << "   }" << std::endl;
   output << "}" << std::endl;
}


void CodegenVerify(std::string ntupleFile, std::ostream &output = std::cout)
{
   output << "void Verify() {" << std::endl;
   output << "   auto ntuple = RNTupleReader::Open(\"NTuple\", \"" << ntupleFile
          << "\");" << std::endl;
   output << "   ntuple->PrintInfo();" << std::endl;
   output << "}" << std::endl;
}

void CodegenMain(const std::string &treeFileName, const std::string &treeName, int compression,
                 std::ostream &output = std::cout)
{
   output << "int main(int argc, char **argv) {" << std::endl;
   output << "   int compression = " << compression << ";" << std::endl;
   output << "   gSystem->Load(\"./libClasses.so\");" << std::endl;
   output << "   auto model = MakeModel();" << std::endl;
   output << "   std::unique_ptr<TFile> f(TFile::Open(\"" + treeFileName + "\"));" << std::endl;
   output << "   auto tree = f->Get<TTree>(\"" + treeName + "\");" << std::endl;
   output << "   Convert(tree, std::move(model), compression);" << std::endl;
   output << "   Verify();" << std::endl;
   output << "}" << std::endl;
}

static void Usage(char *progname)
{
   std::cout << "Usage: " << progname << " -i <ttjet_13tev_june2019.root> -o <ntuple-path> -c <compression> "
             << "-H <header path> -m(t) -b <bloat factor>"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile = "ttjet_13tev_june2019.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";
   std::string headers;
   unsigned bloatFactor = 1;
   bool imt = false;

   int c;
   while ((c = getopt(argc, argv, "hvi:o:c:H:b:m")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         inputFile = optarg;
         break;
      case 'o':
         outputPath = optarg;
         break;
      case 'c':
         compressionSettings = GetCompressionSettings(optarg);
         compressionShorthand = optarg;
         break;
      case 'H':
         headers = optarg;
         break;
      case 'm':
         imt = true;
         break;
      case 'b':
         bloatFactor = std::stoi(optarg);
         assert(bloatFactor > 0);
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   std::string dsName = "ttjet_13tev_june2019";
   if (bloatFactor > 1) {
      std::cout << " ... bloat factor x" << bloatFactor << std::endl;
      dsName += "X" + std::to_string(bloatFactor);
   }
   std::string outputFile = outputPath + "/" + dsName + "~" + compressionShorthand + ".ntuple";
   std::string makePath = headers.empty() ? "_make_" + dsName + "~" + compressionShorthand : headers;
   std::cout << "Converting " << inputFile << " --> " << outputFile << std::endl;

   std::unique_ptr<TFile> f(TFile::Open(inputFile.c_str()));
   assert(f && ! f->IsZombie());

   auto tree = f->Get<TTree>("Events");
   for (auto b : TRangeDynCast<TBranch>(*tree->GetListOfBranches())) {
      // The dynamic cast to TBranch should never fail for GetListOfBranches()
      assert(b);
      assert(b->GetNleaves() == 1);
      auto l = static_cast<TLeaf*>(b->GetListOfLeaves()->First());

      auto field = RFieldBase::Create(l->GetName(), l->GetTypeName()).Unwrap();
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
         auto field = RFieldBase::Create(l->GetName(), l->GetTypeName()).Unwrap();
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
         auto field = RFieldBase::Create(l->GetName(), l->GetTypeName()).Unwrap();
         CodegenField(field->GetName(), field->GetType(), false);
         branchDef.fTypeName = field->GetType();
      }
      branches.emplace_back(branchDef);
   }

   gSystem->MakeDirectory(makePath.c_str());
   std::ofstream fclasses(makePath + "/classes.hxx", std::ofstream::out | std::ofstream::trunc);
   CodegenClasses(fclasses);
   fclasses.close();

   std::ofstream flinkdef(makePath + "/linkdef.h", std::ofstream::out | std::ofstream::trunc);
   CodegenLinkdef(flinkdef);
   flinkdef.close();

   if (headers.empty()) {
      std::ofstream fmain(makePath + "/convert.cxx", std::ofstream::out | std::ofstream::trunc);
      CodegenPreamble(fmain);
      CodegenModel(fmain);
      CodegenConvert(outputFile, imt, bloatFactor, fmain);
      CodegenVerify(outputFile, fmain);
      CodegenMain(inputFile, "Events", compressionSettings, fmain);
      fmain.close();

      std::ofstream fmakefile(makePath + "/Makefile", std::ofstream::out | std::ofstream::trunc);
      CodegenMakefile(fmakefile);
      fmakefile.close();

      chdir(makePath.c_str());
      system("make -j");
      system("./convert");
   }
}
