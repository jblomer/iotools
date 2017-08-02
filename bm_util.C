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
  (*props_map)["root-deflated"] =
    GraphProperties(kGraphDeflated, "ROOT (zlib)", 10);
  (*props_map)["root-lz4"] =
    GraphProperties(kGraphDeflated, "ROOT (LZ4)", 20);
  (*props_map)["rootrow-inflated"] =
    GraphProperties(kGraphInflated, "ROOT (inflated, row-wise)", 25);
  (*props_map)["avro-inflated"] =
    GraphProperties(kGraphInflated, "Avro (inflated)", 100);
  (*props_map)["avro-deflated"] =
    GraphProperties(kGraphDeflated, "Avro (zlib)", 110);
  (*props_map)["parquet-inflated"] =
    GraphProperties(kGraphInflated, "Parquet (inflated)", 80);
  (*props_map)["parquet-deflated"] =
    GraphProperties(kGraphDeflated, "Parquet (zlib)", 90);
  (*props_map)["protobuf-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf (inflated)", 30);
  (*props_map)["protobuf-deflated"]
    = GraphProperties(kGraphDeflated, "Protobuf (gzip)", 40);
  (*props_map)["h5row"] =
    GraphProperties(kGraphInflated, "HDF5 (row-wise)", 60);
  (*props_map)["h5column"] =
    GraphProperties(kGraphInflated, "HDF5 (column-wise)", 70);
  (*props_map)["sqlite"] =
    GraphProperties(kGraphInflated, "SQlite", 50);
}

void FillGraphMap(std::map<EnumGraphTypes, TypeProperties> *graph_map) {
  (*graph_map)[kGraphInflated] = TypeProperties(new TGraphErrors(), 40);
  (*graph_map)[kGraphDeflated] = TypeProperties(new TGraphErrors(), 46);
}

const float kBarSpacing = 1.3;

void SetStyle() {
  gStyle->SetLegendTextSize(0.04);
  gStyle->SetLabelSize(0.04, "xyz");
  gStyle->SetEndErrorSize(6);
}
