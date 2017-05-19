enum EnumGraphTypes { kGraphInflated, kGraphDeflated };

struct TypeProperties {
  TypeProperties() : graph(NULL), color(0) { };
  TypeProperties(TGraphErrors *g, int c) : graph(g), color(c) { }

  TGraphErrors *graph;
  int color;
};

struct GraphProperties {
  GraphProperties() : type(kGraphInflated), title("UNKNOWN"), priority(-1) { }
  GraphProperties(EnumGraphTypes ty, TString ti, int p)
    : type(ty), title(ti), priority(p) { }

  EnumGraphTypes type;
  TString title;
  int priority;
};

void FillPropsMap(std::map<TString, GraphProperties> *props_map) {
  (*props_map)["root-inflated"] =
   GraphProperties(kGraphInflated, "ROOT (inflated)", 0);
  (*props_map)["root-deflated"] =
    GraphProperties(kGraphDeflated, "ROOT (compressed)", 1);
  (*props_map)["avro-inflated"] =
    GraphProperties(kGraphInflated, "Avro (inflated)", 9);
  (*props_map)["avro-deflated"] =
    GraphProperties(kGraphDeflated, "Avro (compressed)", 10);
  (*props_map)["parquet-inflated"] =
    GraphProperties(kGraphInflated, "Parquet (inflated)", 7);
  (*props_map)["parquet-deflated"] =
    GraphProperties(kGraphDeflated, "Parquet (compressed)", 8);
  (*props_map)["protobuf-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf (inflated)", 2);
  (*props_map)["protobuf-deflated"]
    = GraphProperties(kGraphDeflated, "Protobuf (compressed)", 3);
  (*props_map)["h5row"] = GraphProperties(kGraphInflated, "HDF5 (row-wise)", 5);
  (*props_map)["h5column"] =
    GraphProperties(kGraphInflated, "HDF5 (column-wise)", 6);
  (*props_map)["sqlite"] =
    GraphProperties(kGraphInflated, "SQlite", 4);
}

void FillGraphMap(std::map<EnumGraphTypes, TypeProperties> *graph_map) {
  (*graph_map)[kGraphInflated] = TypeProperties(new TGraphErrors(), 40);
  (*graph_map)[kGraphDeflated] = TypeProperties(new TGraphErrors(), 46);
}

const float kBarSpacing = 1.3;
