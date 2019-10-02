enum EnumGraphTypes { kGraphTreeOpt, kGraphNtupleOpt,
                      kGraphTreeRdf, kGraphNtupleRdf,
                      kGraphRatio /* must be last */ };

enum EnumCompression { kZipNone, kZipLz4, kZipZlib, kZipLzma };
const char *kCompressionNames[] = {"uncompressed", "lz4", "zlib", "lzma"};

struct TypeProperties {
  TypeProperties() : graph(NULL), color(0) { };
  TypeProperties(TGraphErrors *g, int c) : graph(g), color(c) { }

  TGraphErrors *graph;
  int color;
};

struct GraphProperties {
  GraphProperties()
    : type(kGraphTreeOpt), compression(kZipNone), priority(-1), size(0.0) { }
  GraphProperties(EnumGraphTypes ty, EnumCompression c)
    : type(ty)
    , compression(c)
    , priority(kGraphRatio * compression + type)
    , size(0.0) { }

  EnumGraphTypes type;
  EnumCompression compression;
  int priority;
  float size;
};

void FillPropsMap(std::map<TString, GraphProperties> *props_map) {
  (*props_map)["root-none"] =
   GraphProperties(kGraphTreeOpt, kZipNone);
  (*props_map)["root-lz4"] =
   GraphProperties(kGraphTreeOpt, kZipLz4);
  (*props_map)["root-zlib"] =
   GraphProperties(kGraphTreeOpt, kZipZlib);
  (*props_map)["root-lzma"] =
   GraphProperties(kGraphTreeOpt, kZipLzma);

  (*props_map)["ntuple-none"] =
   GraphProperties(kGraphNtupleOpt, kZipNone);
  (*props_map)["ntuple-lz4"] =
   GraphProperties(kGraphNtupleOpt, kZipLz4);
  (*props_map)["ntuple-zlib"] =
   GraphProperties(kGraphNtupleOpt, kZipZlib);
  (*props_map)["ntuple-lzma"] =
   GraphProperties(kGraphNtupleOpt, kZipLzma);
}

void FillGraphMap(std::map<EnumGraphTypes, TypeProperties> *graph_map) {
  (*graph_map)[kGraphTreeOpt] =
    TypeProperties(new TGraphErrors(), kBlue);
  (*graph_map)[kGraphNtupleOpt] =
    TypeProperties(new TGraphErrors(), kRed);
  (*graph_map)[kGraphRatio] =
    TypeProperties(new TGraphErrors(), kOrange + 2);
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

const float kBarSpacing = 1.3;

void SetStyle() {
  gStyle->SetEndErrorSize(6);
  gStyle->SetOptTitle(0);
  gStyle->SetOptStat(0);
}

