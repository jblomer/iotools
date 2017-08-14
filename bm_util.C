enum EnumGraphTypes { kGraphInflated, kGraphDeflated };

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
   GraphProperties(kGraphInflated, "ROOT (inflated)", 0);
  (*props_map)["root-inflated~treereader"] =
   GraphProperties(kGraphInflated, "ROOT / TTreeReader (inflated)", 1);
  (*props_map)["root-inflated~dataframe"] =
   GraphProperties(kGraphInflated, "ROOT / TDataFrame (inflated)", 2);
  (*props_map)["root-inflated~dataframemt"] =
   GraphProperties(kGraphInflated, "ROOT / TDataFrameMT (inflated)", 3);
  (*props_map)["root-deflated"] =
    GraphProperties(kGraphDeflated, "ROOT (zlib)", 10);
  (*props_map)["root-deflated~treereader"] =
    GraphProperties(kGraphDeflated, "ROOT / TTreeReader (zlib)", 11);
  (*props_map)["root-deflated~dataframe"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrame (zlib)", 12);
  (*props_map)["root-deflated~dataframemt"] =
    GraphProperties(kGraphDeflated, "ROOT / TDataFrameMT (zlib)", 13);
  (*props_map)["root-lz4"] =
    GraphProperties(kGraphDeflated, "ROOT (LZ4)", 20);
  (*props_map)["root-lzma"] =
    GraphProperties(kGraphDeflated, "ROOT (LZMA)", 22);
  (*props_map)["rootrow-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (inflated, row-wise)", 25);
  (*props_map)["rootautosplit-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (inflated, auto-split)", 26);
  (*props_map)["rootdeepsplit-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (inflated, deep split)", 27);
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
  (*props_map)["avro-inflated"] =
    GraphProperties(kGraphInflated, "Avro (inflated)", 110);
  (*props_map)["avro-deflated"] =
    GraphProperties(kGraphDeflated, "Avro (zlib)", 120);
  (*props_map)["parquet-inflated"] =
    GraphProperties(kGraphInflated, "Parquet (inflated)", 90);
  (*props_map)["parquet-deflated"] =
    GraphProperties(kGraphDeflated, "Parquet (zlib)", 100);
  (*props_map)["protobuf-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf (inflated)", 40);
  (*props_map)["protobuf-deflated"]
    = GraphProperties(kGraphDeflated, "Protobuf (gzip)", 50);
  (*props_map)["h5row"] =
    GraphProperties(kGraphInflated, "HDF5 (row-wise)", 70);
  (*props_map)["h5column"] =
    GraphProperties(kGraphInflated, "HDF5 (column-wise)", 80);
  (*props_map)["sqlite"] =
    GraphProperties(kGraphInflated, "SQlite", 60);
}

void FillGraphMap(std::map<EnumGraphTypes, TypeProperties> *graph_map) {
  (*graph_map)[kGraphInflated] = TypeProperties(new TGraphErrors(), 40);
  (*graph_map)[kGraphDeflated] = TypeProperties(new TGraphErrors(), 46);
}

const float kBarSpacing = 1.3;

void SetStyle() {
  gStyle->SetLegendTextSize(0.03);
  gStyle->SetLabelSize(0.04, "xyz");
  gStyle->SetEndErrorSize(6);
}
