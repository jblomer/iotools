enum EnumGraphTypes { kGraphTreeDirect, kGraphNtupleDirect,
                      kGraphTreeRdf, kGraphNtupleRdf,
                      kGraphRatioDirect, kGraphRatioRdf,
                      kNumGraphs };

enum EnumCompression { kZipNone, kZipLz4, kZipZstd, kZipZlib, kZipLzma };
const char *kCompressionNames[] = {"uncompressed", "lz4", "zstd", "zlib", "lzma"};

struct TypeProperties {
  TypeProperties() : graph(NULL), color(0), shade(0) { };
  TypeProperties(TGraphErrors *g, int c, int s, bool r, bool d)
    : graph(g), color(c), shade(s), is_ratio(r), is_direct(d) { }

  TGraphErrors *graph;
  int color;
  int shade;
  bool is_ratio = false;
  bool is_direct = false;
};

struct GraphProperties {
  GraphProperties()
    : type(kGraphTreeDirect), compression(kZipNone), priority(-1), size(0.0) { }
  GraphProperties(EnumGraphTypes ty, EnumCompression c)
    : type(ty)
    , compression(c)
    , priority(kNumGraphs * compression + type)
    , size(0.0) { }

  EnumGraphTypes type;
  EnumCompression compression;
  int priority;
  float size;
};

void FillPropsMap(std::map<TString, GraphProperties> *props_map) {
  (*props_map)["root-none"] =
   GraphProperties(kGraphTreeDirect, kZipNone);
  (*props_map)["root-lz4"] =
   GraphProperties(kGraphTreeDirect, kZipLz4);
  (*props_map)["root-zlib"] =
   GraphProperties(kGraphTreeDirect, kZipZlib);
  (*props_map)["root-lzma"] =
   GraphProperties(kGraphTreeDirect, kZipLzma);
  (*props_map)["root-zstd"] =
   GraphProperties(kGraphTreeDirect, kZipZstd);

  (*props_map)["root+direct-none"] =
   GraphProperties(kGraphTreeDirect, kZipNone);
  (*props_map)["root+direct-lz4"] =
   GraphProperties(kGraphTreeDirect, kZipLz4);
  (*props_map)["root+direct-zlib"] =
   GraphProperties(kGraphTreeDirect, kZipZlib);
  (*props_map)["root+direct-lzma"] =
   GraphProperties(kGraphTreeDirect, kZipLzma);
  (*props_map)["root+direct-zstd"] =
   GraphProperties(kGraphTreeDirect, kZipZstd);

  (*props_map)["root+rdf-none"] =
   GraphProperties(kGraphTreeRdf, kZipNone);
  (*props_map)["root+rdf-lz4"] =
   GraphProperties(kGraphTreeRdf, kZipLz4);
  (*props_map)["root+rdf-zlib"] =
   GraphProperties(kGraphTreeRdf, kZipZlib);
  (*props_map)["root+rdf-lzma"] =
   GraphProperties(kGraphTreeRdf, kZipLzma);
  (*props_map)["root+rdf-zstd"] =
   GraphProperties(kGraphTreeRdf, kZipZstd);

  (*props_map)["ntuple-none"] =
   GraphProperties(kGraphNtupleDirect, kZipNone);
  (*props_map)["ntuple-lz4"] =
   GraphProperties(kGraphNtupleDirect, kZipLz4);
  (*props_map)["ntuple-zlib"] =
   GraphProperties(kGraphNtupleDirect, kZipZlib);
  (*props_map)["ntuple-lzma"] =
   GraphProperties(kGraphNtupleDirect, kZipLzma);
  (*props_map)["ntuple-zstd"] =
   GraphProperties(kGraphNtupleDirect, kZipZstd);

  (*props_map)["ntuple+direct-none"] =
   GraphProperties(kGraphNtupleDirect, kZipNone);
  (*props_map)["ntuple+direct-lz4"] =
   GraphProperties(kGraphNtupleDirect, kZipLz4);
  (*props_map)["ntuple+direct-zlib"] =
   GraphProperties(kGraphNtupleDirect, kZipZlib);
  (*props_map)["ntuple+direct-lzma"] =
   GraphProperties(kGraphNtupleDirect, kZipLzma);
  (*props_map)["ntuple+direct-zstd"] =
   GraphProperties(kGraphNtupleDirect, kZipZstd);

  (*props_map)["ntuple+rdf-none"] =
   GraphProperties(kGraphNtupleRdf, kZipNone);
  (*props_map)["ntuple+rdf-lz4"] =
   GraphProperties(kGraphNtupleRdf, kZipLz4);
  (*props_map)["ntuple+rdf-zlib"] =
   GraphProperties(kGraphNtupleRdf, kZipZlib);
  (*props_map)["ntuple+rdf-lzma"] =
   GraphProperties(kGraphNtupleRdf, kZipLzma);
  (*props_map)["ntuple+rdf-zstd"] =
   GraphProperties(kGraphNtupleRdf, kZipZstd);
}

void FillGraphMap(std::map<EnumGraphTypes, TypeProperties> *graph_map) {
  (*graph_map)[kGraphTreeDirect] =
    TypeProperties(new TGraphErrors(), kBlue, 1001, false, true);
  (*graph_map)[kGraphNtupleDirect] =
    TypeProperties(new TGraphErrors(), kRed, 1001, false, true);
  (*graph_map)[kGraphTreeRdf] =
    TypeProperties(new TGraphErrors(), kBlue, 3001, false, false);
  (*graph_map)[kGraphNtupleRdf] =
    TypeProperties(new TGraphErrors(), kRed, 3001, false, false);
  (*graph_map)[kGraphRatioDirect] =
    TypeProperties(new TGraphErrors(), kGreen + 2, 1001, true, false);
  (*graph_map)[kGraphRatioRdf] =
    TypeProperties(new TGraphErrors(), kGreen + 2, 3001, true, false);
}

TString GetPhysicalFormat(TString format) {
  TObjArray *tokens = format.Tokenize("~");
  TString physical_format =
    reinterpret_cast<TObjString *>(tokens->At(0))->CopyString();
  delete tokens;
  return physical_format;
}

float GetBloatFactor(TString format) {
  if (format.EndsWith("times10"))
    return 10.0;
  return 1.0;
}

void GetStats(double *vals, int nval, double &mean, double &error, double &median) {
  assert(nval > 1);
  mean = 0.0;
  for (int i = 0; i < nval; ++i)
    mean += vals[i];
  mean /= nval;
  double s2 = 0.0;
  for (int i = 0; i < nval; ++i)
    s2 += (vals[i] - mean) * (vals[i] - mean);
  s2 /= nval - 1;
  double s = sqrt(s2);
  double t = abs(ROOT::Math::tdistribution_quantile(0.05 / 2., nval - 1));
  error = t * s / sqrt(nval);

  std::vector<double> v;
  for (int i = 0; i < nval; ++i)
    v.push_back(vals[i]);
  std::sort(v.begin(), v.end());
  median = v[nval/2];
}

void SetStyle() {
  gStyle->SetEndErrorSize(6);
  gStyle->SetOptTitle(1);
  gStyle->SetOptStat(0);
  //gStyle->SetTitleFontSize(30);

  Int_t ci = 1179;      // for color index setting
  new TColor(ci, 1, 0, 0, " ", 0.);
}

Int_t GetTransparentColor() {
  return 1179;
}
