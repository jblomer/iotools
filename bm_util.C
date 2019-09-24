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
  (*props_map)["root-inflated"] =
   GraphProperties(kGraphInflated, "TTree", 3);
  (*props_map)["root-inflated~treereader"] =
   GraphProperties(kGraphInflated, "ROOT / TTreeReader", 1);
  (*props_map)["root-inflated~dataframe"] =
   GraphProperties(kGraphInflated, "ROOT / TDataFrame", 2);
  (*props_map)["root-inflated~dataframemt"] =
   GraphProperties(kGraphInflated, "ROOT / TDataFrameMT", 4);
  (*props_map)["root-inflated~dataframenoht"] =
   GraphProperties(kGraphInflated, "ROOT / TDataFrameMT/no-HT", 3);

  (*props_map)["ntuple-inflated"] =
   GraphProperties(kGraphInflated, "RNTuple", 0);
  (*props_map)["ntuple-inflated~view"] =
   GraphProperties(kGraphInflated, "RNTuple / Views", 1);

   (*props_map)["ntuple-deflated"] =
   GraphProperties(kGraphDeflated, "RNTuple (zlib)", 5);
   (*props_map)["ntuple-deflated~view"] =
   GraphProperties(kGraphDeflated, "RNTuple / Views (zlib)", 6);

  (*props_map)["root-inflated+times10"] =
   GraphProperties(kGraphInflated, "ROOTx10 (inflated)", 0);
  (*props_map)["root-inflated~treereader+times10"] =
   GraphProperties(kGraphInflated, "ROOTx10 / TTreeReader (inflated)", 1);
  (*props_map)["root-inflated~dataframe+times10"] =
   GraphProperties(kGraphInflated, "ROOTx10 / TDataFrame (inflated)", 2);
  (*props_map)["root-inflated~dataframemt+times10"] =
   GraphProperties(kGraphInflated, "ROOTx10 / TDataFrameMT (inflated)", 3);
  (*props_map)["root-inflated~dataframenoht+times10"] =
   GraphProperties(kGraphInflated,
                   "ROOTx10 / TDataFrameMT/no-HT (inflated)", 4);

  (*props_map)["root-deflated"] =
    GraphProperties(kGraphDeflated, "TTree (zlib)", 10);
  (*props_map)["root-deflated~treereader"] =
    GraphProperties(kGraphDeflated, "ROOT / TTreeReader (zlib)", 11);
  (*props_map)["root-deflated~dataframe"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrame (zlib)", 12);
  (*props_map)["root-deflated~dataframemt"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrameMT (zlib)", 14);
    (*props_map)["root-deflated~dataframenoht"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrameMT/no-HT (zlib)", 13);
  (*props_map)["root-lz4"] =
    GraphProperties(kGraphDeflated, "ROOT (LZ4)", 20);
  (*props_map)["root-lz4~treereader"] =
    GraphProperties(kGraphDeflated, "ROOT / TTreeReader (lz4)", 21);
  (*props_map)["root-lz4~dataframe"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrame (lz4)", 22);
  (*props_map)["root-lz4~dataframemt"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrameMT (lz4)", 23);
    (*props_map)["root-lz4~dataframenoht"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrameMT/no-HT (lz4)", 24);
  (*props_map)["root-lzma"] =
    GraphProperties(kGraphDeflated, "ROOT (LZMA)", 22);
  (*props_map)["rootrow-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (row-wise)", 1);
  (*props_map)["rootrow-inflated~unfixed"] =
    GraphProperties(kGraphInflated, "ROOT (row-wise)", 25);
  (*props_map)["rootrow-inflated~fixed"] =
    GraphProperties(kGraphInflated, "ROOT / Fixed (row-wise)", 26);
  (*props_map)["rootautosplit-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (inflated, auto-split)", 26);

  (*props_map)["rootdeepsplit-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (inflated, deep split)", 27);
  (*props_map)["rootdeepsplit-deflated"] =
    GraphProperties(kGraphDeflated, "ROOT (zlip, deep split)", 28);
  (*props_map)["rootdeepsplit-lz4"] =
    GraphProperties(kGraphDeflated, "ROOT (LZ4, deep split)", 29);

  (*props_map)["rootautosplit-inflated~treereader"] =
    GraphProperties(kGraphInflated, "ROOT / TTreeReader (inflated, auto-split)",
                    27);
  (*props_map)["rootautosplit-inflated~dataframe"] =
    GraphProperties(kGraphInflated, "ROOT / TDataFrame (inflated, auto-split)",
                    28);
  (*props_map)["rootautosplit-inflated~dataframemt"] =
    GraphProperties(kGraphInflated,
                    "ROOT / TDataFrameMT (inflated, auto-split)", 29);
  (*props_map)["rootautosplit-deflated"] =
    GraphProperties(kGraphDeflated, "ROOT (zlib, auto-split)", 30);
  (*props_map)["rootautosplit-deflated~treereader"] =
    GraphProperties(kGraphDeflated, "ROOT / TTreeReader (zlib, auto-split)",
                    31);
  (*props_map)["rootautosplit-deflated~dataframe"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrame (zlib, auto-split)",
                    32);
  (*props_map)["rootautosplit-deflated~dataframemt"] =
    GraphProperties(kGraphDeflated,
                    "ROOT / TDataFrameMT (zlib, auto-split)", 33);
  (*props_map)["rootautosplit-deflated~dataframenoht"] =
    GraphProperties(kGraphDeflated,
                    "ROOT / TDataFrameMT/no-HT (zlib, auto-split)", 34);
  (*props_map)["avro-inflated"] =
    GraphProperties(kGraphInflated, "Avro", 110);
  (*props_map)["avro-inflated~java"] =
    GraphProperties(kGraphInflated, "Avro-Java", 111);
  (*props_map)["avro-deflated"] =
    GraphProperties(kGraphDeflated, "Avro (zlib)", 120);
  (*props_map)["avro-deflated~java"] =
    GraphProperties(kGraphDeflated, "Avro-Java (zlib)", 121);
  (*props_map)["parquet-inflated"] =
    GraphProperties(kGraphInflated, "Parquet", 90);
  (*props_map)["parquet-deflated"] =
    GraphProperties(kGraphDeflated, "Parquet (zlib)", 100);
  (*props_map)["protobuf-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf", 40);
  (*props_map)["protobufdeep-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf (deep)", 41);
  (*props_map)["protobuf-deflated"]
    = GraphProperties(kGraphDeflated, "Protobuf (gzip)", 50);
  (*props_map)["h5"] =
    GraphProperties(kGraphInflated, "HDF5", 70);
  (*props_map)["h5row"] =
    GraphProperties(kGraphInflated, "HDF5 (row-wise)", 70);
  (*props_map)["h5column"] =
    GraphProperties(kGraphInflated, "HDF5 (column-wise)", 80);
  (*props_map)["sqlite"] =
    GraphProperties(kGraphInflated, "SQlite", 60);


  //  DETAIL PLOTS


  (*props_map)["serialization-root-read"] =
    GraphProperties(kGraphRead, "ROOT", 1);
  (*props_map)["serialization-rootrow~unfixed-read"] =
    GraphProperties(kGraphRead, "ROOT (row-wise)", 10);
  (*props_map)["serialization-rootrow~fixed-read"] =
    GraphProperties(kGraphRead, "ROOT / Fixed (row-wise)", 20);
  (*props_map)["serialization-protobuf-read"] =
    GraphProperties(kGraphRead, "Protobuf", 30);
  (*props_map)["serialization-root-write"] =
    GraphProperties(kGraphWrite, "ROOT", 100);
  (*props_map)["serialization-rootrow~unfixed-write"] =
    GraphProperties(kGraphWrite, "ROOT (row-wise)", 110);
  (*props_map)["serialization-rootrow~fixed-write"] =
    GraphProperties(kGraphWrite, "ROOT / Fixed (row-wise)", 120);
  (*props_map)["serialization-protobuf-write"] =
    GraphProperties(kGraphWrite, "Protobuf", 130);


  (*props_map)["flavor-root-inflated"] =
   GraphProperties(kGraphInflated, "SetBranchAddress", 0);
  (*props_map)["flavor-root-inflated~treereader"] =
   GraphProperties(kGraphInflated, "TTreeReader", 1);
  (*props_map)["flavor-root-inflated~dataframe"] =
   GraphProperties(kGraphInflated, "TDataFrame", 2);
  (*props_map)["flavor-root-inflated~dataframemt"] =
   GraphProperties(kGraphInflated, "TDataFrameMT", 4);
  (*props_map)["flavor-root-inflated~dataframenoht"] =
   GraphProperties(kGraphInflated, "TDataFrameMT no-HT", 3);

  (*props_map)["flavor-root-deflated"] =
    GraphProperties(kGraphDeflated, "SetBranchAddress", 10);
  (*props_map)["flavor-root-deflated~treereader"] =
    GraphProperties(kGraphDeflated, "TTreeReader", 11);
  (*props_map)["flavor-root-deflated~dataframe"] =
    GraphProperties(kGraphDeflated, "TDataFrame", 12);
  (*props_map)["flavor-root-deflated~dataframemt"] =
    GraphProperties(kGraphDeflated, "TDataFrameMT", 14);
    (*props_map)["flavor-root-deflated~dataframenoht"] =
    GraphProperties(kGraphDeflated, "TDataFrameMT no-HT", 13);

  (*props_map)["split-root-inflated"] =
   GraphProperties(kGraphInflated, "ROOT / Manual Branching", 0);
  (*props_map)["split-rootautosplit-inflated"] =
    GraphProperties(kGraphInflated, "ROOT / Auto Split", 2);
  (*props_map)["split-rootdeepsplit-inflated"] =
    GraphProperties(kGraphInflated, "ROOT / Deep Split", 3);
  (*props_map)["split-protobuf-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf", 10);
  (*props_map)["split-protobufdeep-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf / DeepEvent", 11);
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

