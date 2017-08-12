#include "bm_util.C"

struct Interval {
  Interval() : from(0), to(0) { }
  Interval(unsigned f, unsigned t) : from(f), to(t) { }

  bool operator <(const Interval &other) const {
    return this->from < other.from;
  }

  unsigned from;
  unsigned to;
};

void DrawSegment(unsigned from, unsigned to, unsigned max,
                 unsigned slot, unsigned nslots,
                 Int_t color,
                 TString caption = "")
{
  double slot_height = 1. / double(nslots);
  double padding = 0.05 * slot_height;
  double ymin = (double)slot * slot_height;
  double ymax = ymin + slot_height;
  ymax -= slot_height / 2.0;

  double xmin = (double)from / (double)max;
  double xmax = (double)to / (double)max;

  //printf("New box %f %f %f %f\n", xmin, ymin, xmax, ymax);
  TBox *box = new TBox(xmin, ymin + padding, xmax, ymax - padding);
  box->SetFillColor(color);
  box->Draw();

  if (caption != "") {
    TText *text = new TText(0, ymax + padding, caption);
    text->SetTextSize(0.20 / (double)nslots);
    text->Draw();
  }
}

void bm_iopattern(TString dataSets=
  "result_iopattern.root-inflated.txt result_iopattern.root-deflated.txt",
  TString output_path = "graph_iopattern.root")
{
  std::map<TString, GraphProperties> props_map;
  FillPropsMap(&props_map);

  vector<TString> format_vec;
  map<TString, vector<Interval> *> interval_map;
  map<TString, unsigned> total_size_map;
  unsigned max_size = 0;

  TObjArray *formats = dataSets.Tokenize(" ");
  for (unsigned i = 0; i < formats->GetEntries(); ++i) {
    TString format =
      reinterpret_cast<TObjString *>(formats->At(i))->CopyString();
    if (format == "")
      continue;

    std::ifstream file_pattern(format);
    TString dummy;
    TString format_name;
    unsigned total_size;

    file_pattern >> dummy >> format_name >> total_size;
    format_vec.push_back(format_name);
    total_size_map[format_name] = total_size;
    max_size = std::max(max_size, total_size);

    unsigned offset;
    unsigned nbytes;
    vector<Interval> raw_intervals;
    while (file_pattern >> offset >> nbytes) {
      raw_intervals.push_back(Interval(offset, offset + nbytes));
    }
    cout << "processed " << raw_intervals.size() << " Fuse read events "
         << "for " << format_name << endl;

    cout << "   ... merging intervals" << endl;

    vector<Interval> *merged_intervals = new vector<Interval>();


    for (unsigned i = 1; i < raw_intervals.size(); ++i) {
      unsigned from = raw_intervals[i].from;
      unsigned to = raw_intervals[i].to;

      auto iter_succ = std::lower_bound(
        merged_intervals->begin(), merged_intervals->end(), raw_intervals[i]);

      bool got_merged = false;

      if (iter_succ != merged_intervals->begin()) {
        auto iter_prior = iter_succ - 1;
        if (iter_prior->to >= from) {
          iter_prior->to = to;
          got_merged = true;
        }
      }

      if (iter_succ != merged_intervals->end()) {
        if (iter_succ->from <= to) {
          iter_succ->from = from;
          got_merged = true;
        }
      }

      if (!got_merged)
        merged_intervals->insert(iter_succ, raw_intervals[i]);
    }
    cout << "   --> " << merged_intervals->size() << " intervals" << endl;

    interval_map[format_name] = merged_intervals;

  }

  SetStyle();
  TCanvas *canvas = new TCanvas();

  /*TArrow *scale = new TArrow(0.02, 0.98, 0.98, 0.98, 0.005, "|>");
  scale->SetLineWidth(2);
  scale->Draw();*/

  // sort the formats
  for (unsigned i = 0; i < format_vec.size(); ++i) {
    unsigned idx_min = i;
    for (unsigned j = i + 1; j < format_vec.size(); ++j) {
      if (props_map[format_vec[idx_min]].priority <
          props_map[format_vec[j]].priority)
      {
        idx_min = j;
      }
    }
    if (idx_min != i) {
      std::swap(format_vec[i], format_vec[idx_min]);
    }
  }

  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TString format = format_vec[i];
    DrawSegment(0, total_size_map[format], max_size,
                i, format_vec.size(),
                38, props_map[format].title);
    vector<Interval> *intervals = interval_map[format];
    for (unsigned j = 0; j < intervals->size(); ++j) {
      DrawSegment((*intervals)[j].from, (*intervals)[j].to, max_size,
                  i, format_vec.size(),
                  46);
    }
    cout << "Drew " << intervals->size() << " intervals for " << format << endl;
  }

  TFile * output =
    TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
