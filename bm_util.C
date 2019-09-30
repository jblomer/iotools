enum EnumGraphTypes { kGraphInflated, kGraphDeflated,
                      kGraphRead, kGraphWrite };

struct TypeProperties {
  TypeProperties() : graph(NULL), color(0) { };
  TypeProperties(TGraphErrors *g, int c) : graph(g), color(c) { }

  TGraphErrors *graph;
  int color;
};

struct GraphProperties {
  GraphProperties()
    : type(kGraphInflated), title("UNKNOWN"), priority(-1), size(0.0) { }
  GraphProperties(EnumGraphTypes ty, TString ti, int p)
    : type(ty), title(ti), priority(p), size(0.0) { }

  EnumGraphTypes type;
  TString title;
  int priority;
  float size;
};

void FillPropsMap(std::map<TString, GraphProperties> *props_map) {
  (*props_map)["root-none"] =
   GraphProperties(kGraphInflated, "TTree (uncompressed)", 10);
  (*props_map)["root-zlib"] =
   GraphProperties(kGraphDeflated, "TTree (zlib)", 12);
  (*props_map)["root-lz4"] =
   GraphProperties(kGraphDeflated, "TTree (lz4)", 14);
  (*props_map)["root-lzma"] =
   GraphProperties(kGraphDeflated, "TTree (lzma)", 16);

  (*props_map)["ntuple-none"] =
   GraphProperties(kGraphInflated, "RNTuple (uncompressed)", 11);
  (*props_map)["ntuple-zlib"] =
   GraphProperties(kGraphDeflated, "RNTuple (zlib)", 13);
  (*props_map)["ntuple-lz4"] =
   GraphProperties(kGraphDeflated, "RNTuple (lz4)", 15);
  (*props_map)["ntuple-lzma"] =
   GraphProperties(kGraphDeflated, "RNTuple (lzma)", 17);
}

void FillGraphMap(std::map<EnumGraphTypes, TypeProperties> *graph_map) {
  (*graph_map)[kGraphInflated] =
    TypeProperties(new TGraphErrors(), kBlue - 9);
  (*graph_map)[kGraphDeflated] =
    TypeProperties(new TGraphErrors(), kRed - 9);

  (*graph_map)[kGraphRead] = TypeProperties(new TGraphErrors(), 38);
  (*graph_map)[kGraphWrite] = TypeProperties(new TGraphErrors(), 33);
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
  gStyle->SetLegendTextSize(0.03);
  gStyle->SetLabelSize(0.04, "xyz");
  gStyle->SetEndErrorSize(6);
}

