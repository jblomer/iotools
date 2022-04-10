#include "cms_ttree.h"
#include "util_arrow.h"

#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>

#include <cassert>
#include <cstdio>
#include <memory>
#include <unistd.h>
#include <unordered_map>

static const std::unordered_map<std::string, parquet::Compression::type> g_name2codec{
  {"none", parquet::Compression::UNCOMPRESSED},
  {"zstd", parquet::Compression::ZSTD},
};

/// Chunk size for `parquet::arrow::FileWriter::WriteTable()` defaults to the old RNTuple entries per cluster
constexpr size_t kDefaultWriteTableChunkSize = 64000;

static void Usage(char *progname) {
  printf("Usage: %s -i <input ROOT file> -o <output parquet file> [-c none|zstd] [-s <WriteTable() chunk size>]\n", progname);
}

std::shared_ptr<arrow::Schema> InitSchema() {
  using namespace arrow;
  return schema({
      field("run", int32()),
      field("luminosityBlock", uint32()),
      field("event", uint64()),
      field("HLT_IsoMu24_eta2p1", boolean()),
      field("HLT_IsoMu24", boolean()),
      field("HLT_IsoMu17_eta2p1_LooseIsoPFTau20", boolean()),
      field("PV_npvs", int32()),
      field("PV_x", float32()),
      field("PV_y", float32()),
      field("PV_z", float32()),
      //field("nMuon", int32()), // `arrow::list` should encode this info
      field("Muon_pt", arrow::list(float32())),
      field("Muon_eta", arrow::list(float32())),
      field("Muon_phi", arrow::list(float32())),
      field("Muon_mass", arrow::list(float32())),
      field("Muon_charge", arrow::list(int32())),
      field("Muon_pfRelIso03_all", arrow::list(float32())),
      field("Muon_pfRelIso04_all", arrow::list(float32())),
      field("Muon_tightId", arrow::list(boolean())),
      field("Muon_softId", arrow::list(boolean())),
      field("Muon_dxy", arrow::list(float32())),
      field("Muon_dxyErr", arrow::list(float32())),
      field("Muon_dz", arrow::list(float32())),
      field("Muon_dzErr", arrow::list(float32())),
      field("Muon_jetIdx", arrow::list(int32())),
      field("Muon_genPartIdx", arrow::list(int32())),
      //field("nElectron", int32()),
      field("Electron_pt", arrow::list(float32())),
      field("Electron_eta", arrow::list(float32())),
      field("Electron_phi", arrow::list(float32())),
      field("Electron_mass", arrow::list(float32())),
      field("Electron_charge", arrow::list(int32())),
      field("Electron_pfRelIso03_all", arrow::list(float32())),
      field("Electron_dxy", arrow::list(float32())),
      field("Electron_dxyErr", arrow::list(float32())),
      field("Electron_dz", arrow::list(float32())),
      field("Electron_dzErr", arrow::list(float32())),
      field("Electron_cutBasedId", arrow::list(boolean())),
      field("Electron_pfId", arrow::list(boolean())),
      field("Electron_jetIdx", arrow::list(int32())),
      field("Electron_genPartIdx", arrow::list(int32())),
      //field("nTau", int32()),
      field("Tau_pt", arrow::list(float32())),
      field("Tau_eta", arrow::list(float32())),
      field("Tau_phi", arrow::list(float32())),
      field("Tau_mass", arrow::list(float32())),
      field("Tau_charge", arrow::list(int32())),
      field("Tau_decayMode", arrow::list(int32())),
      field("Tau_relIso_all", arrow::list(float32())),
      field("Tau_jetIdx", arrow::list(int32())),
      field("Tau_genPartIdx", arrow::list(int32())),
      field("Tau_idDecayMode", arrow::list(boolean())),
      field("Tau_idIsoRaw", arrow::list(float32())),
      field("Tau_idIsoVLoose", arrow::list(boolean())),
      field("Tau_idIsoLoose", arrow::list(boolean())),
      field("Tau_idIsoMedium", arrow::list(boolean())),
      field("Tau_idIsoTight", arrow::list(boolean())),
      field("Tau_idAntiEleLoose", arrow::list(boolean())),
      field("Tau_idAntiEleMedium", arrow::list(boolean())),
      field("Tau_idAntiEleTight", arrow::list(boolean())),
      field("Tau_idAntiMuLoose", arrow::list(boolean())),
      field("Tau_idAntiMuMedium", arrow::list(boolean())),
      field("Tau_idAntiMuTight", arrow::list(boolean())),
      field("MET_pt", float32()),
      field("MET_phi", float32()),
      field("MET_sumet", float32()),
      field("MET_significance", float32()),
      field("MET_CovXX", float32()),
      field("MET_CovXY", float32()),
      field("MET_CovYY", float32()),
      //field("nJet", int32()),
      field("Jet_pt", arrow::list(float32())),
      field("Jet_eta", arrow::list(float32())),
      field("Jet_phi", arrow::list(float32())),
      field("Jet_mass", arrow::list(float32())),
      field("Jet_puId", arrow::list(boolean())),
      field("Jet_btag", arrow::list(float32())),
      //field("nGenPart", int32()),
      field("GenPart_pt", arrow::list(float32())),
      field("GenPart_eta", arrow::list(float32())),
      field("GenPart_phi", arrow::list(float32())),
      field("GenPart_mass", arrow::list(float32())),
      field("GenPart_pdgId", arrow::list(int32())),
      field("GenPart_status", arrow::list(int32())),
    });
}

int main(int argc, char **argv) {
  std::string inputPath;
  std::string outputPath;
  std::string compression{"none"};
  std::size_t chunkSize = kDefaultWriteTableChunkSize;

  int c;
  while ((c = getopt(argc, argv, "hvi:o:c:s:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
        break;
      case 'o':
        outputPath = optarg;
        break;
      case 'c':
        compression = optarg;
        break;
      case 's':
        chunkSize = std::atoi(optarg);
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }
  assert(!inputPath.empty() && !outputPath.empty());
  printf("Converting %s --> %s\n", inputPath.c_str(), outputPath.c_str());

  EventReaderRoot reader;
  reader.Open(inputPath);

  parquet::WriterProperties::Builder props;
  props.data_pagesize(64 * 1024); // match RNTuple defaults for page/cluster size 
  props.max_row_group_length(50 * 1000 * 1000);
  props.encoding(parquet::Encoding::PLAIN);
  props.disable_dictionary();
  props.compression(g_name2codec.at(compression));

  auto schema = InitSchema();

  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(outfile,
			  arrow::io::FileOutputStream::Open(outputPath));
  std::unique_ptr<parquet::arrow::FileWriter> writer;
  PARQUET_THROW_NOT_OK(parquet::arrow::FileWriter::Open(*schema, arrow::default_memory_pool(), outfile, props.build(),
							&writer));

  ArrowTableBuilder<
    /*run*/arrow::Int32Builder,
    /*luminosityBlock*/arrow::UInt32Builder,
    /*event*/arrow::UInt64Builder,
    /*HLT_IsoMu24_eta2p1*/arrow::BooleanBuilder,
    /*HLT_IsoMu24*/arrow::BooleanBuilder,
    /*HLT_IsoMu17_eta2p1_LooseIsoPFTau20*/arrow::BooleanBuilder,
    /*PV_npvs*/arrow::Int32Builder,
    /*PV_x*/arrow::FloatBuilder,
    /*PV_y*/arrow::FloatBuilder,
    /*PV_z*/arrow::FloatBuilder,
    /*nMuon*/
    /*Muon_pt*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_eta*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_phi*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_mass*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_charge*/ListAdapter<arrow::Int32Builder>,
    /*Muon_pfRelIso03_all*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_pfRelIso04_all*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_tightId*/ListAdapter<arrow::BooleanBuilder>,
    /*Muon_softId*/ListAdapter<arrow::BooleanBuilder>,
    /*Muon_dxy*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_dxyErr*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_dz*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_dzErr*/ListAdapter<arrow::FloatBuilder>,
    /*Muon_jetIdx*/ListAdapter<arrow::Int32Builder>,
    /*Muon_genPartIdx*/ListAdapter<arrow::Int32Builder>,
    /*nElectron*/
    /*Electron_pt*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_eta*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_phi*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_mass*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_charge*/ListAdapter<arrow::Int32Builder>,
    /*Electron_pfRelIso03_all*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_dxy*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_dxyErr*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_dz*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_dzErr*/ListAdapter<arrow::FloatBuilder>,
    /*Electron_cutBasedId*/ListAdapter<arrow::BooleanBuilder>,
    /*Electron_pfId*/ListAdapter<arrow::BooleanBuilder>,
    /*Electron_jetIdx*/ListAdapter<arrow::Int32Builder>,
    /*Electron_genPartIdx*/ListAdapter<arrow::Int32Builder>,
    /*nTau*/
    /*Tau_pt*/ListAdapter<arrow::FloatBuilder>,
    /*Tau_eta*/ListAdapter<arrow::FloatBuilder>,
    /*Tau_phi*/ListAdapter<arrow::FloatBuilder>,
    /*Tau_mass*/ListAdapter<arrow::FloatBuilder>,
    /*Tau_charge*/ListAdapter<arrow::Int32Builder>,
    /*Tau_decayMode*/ListAdapter<arrow::Int32Builder>,
    /*Tau_relIso_all*/ListAdapter<arrow::FloatBuilder>,
    /*Tau_jetIdx*/ListAdapter<arrow::Int32Builder>,
    /*Tau_genPartIdx*/ListAdapter<arrow::Int32Builder>,
    /*Tau_idDecayMode*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idIsoRaw*/ListAdapter<arrow::FloatBuilder>,
    /*Tau_idIsoVLoose*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idIsoLoose*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idIsoMedium*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idIsoTight*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idAntiEleLoose*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idAntiEleMedium*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idAntiEleTight*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idAntiMuLoose*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idAntiMuMedium*/ListAdapter<arrow::BooleanBuilder>,
    /*Tau_idAntiMuTight*/ListAdapter<arrow::BooleanBuilder>,
    /*MET_pt*/arrow::FloatBuilder,
    /*MET_phi*/arrow::FloatBuilder,
    /*MET_sumet*/arrow::FloatBuilder,
    /*MET_significance*/arrow::FloatBuilder,
    /*MET_CovXX*/arrow::FloatBuilder,
    /*MET_CovXY*/arrow::FloatBuilder,
    /*MET_CovYY*/arrow::FloatBuilder,
    /*nJet*/
    /*Jet_pt*/ListAdapter<arrow::FloatBuilder>,
    /*Jet_eta*/ListAdapter<arrow::FloatBuilder>,
    /*Jet_phi*/ListAdapter<arrow::FloatBuilder>,
    /*Jet_mass*/ListAdapter<arrow::FloatBuilder>,
    /*Jet_puId*/ListAdapter<arrow::BooleanBuilder>,
    /*Jet_btag*/ListAdapter<arrow::FloatBuilder>,
    /*nGenPart*/
    /*GenPart_pt*/ListAdapter<arrow::FloatBuilder>,
    /*GenPart_eta*/ListAdapter<arrow::FloatBuilder>,
    /*GenPart_phi*/ListAdapter<arrow::FloatBuilder>,
    /*GenPart_mass*/ListAdapter<arrow::FloatBuilder>,
    /*GenPart_pdgId*/ListAdapter<arrow::Int32Builder>,
    /*GenPart_status*/ListAdapter<arrow::Int32Builder>
    > tblBuilder(schema,
		 chunkSize,
		 [&writer,chunkSize](std::shared_ptr<arrow::Table> table) { writer->WriteTable(*table, chunkSize); });

  size_t nEvent = 0;
  while (reader.NextEvent()) {
    const auto &row = reader.fEvent;
    tblBuilder << std::make_tuple(row.run,
				  row.luminosityBlock,
				  row.event,
				  row.HLT_IsoMu24_eta2p1,
				  row.HLT_IsoMu24,
				  row.HLT_IsoMu17_eta2p1_LooseIsoPFTau20,
				  row.PV_npvs,
				  row.PV_x,
				  row.PV_y,
				  row.PV_z,
				  std::make_pair(row.Muon_pt, row.nMuon),
				  std::make_pair(row.Muon_eta, row.nMuon),
				  std::make_pair(row.Muon_phi, row.nMuon),
				  std::make_pair(row.Muon_mass, row.nMuon),
				  std::make_pair(row.Muon_charge, row.nMuon),
				  std::make_pair(row.Muon_pfRelIso03_all, row.nMuon),
				  std::make_pair(row.Muon_pfRelIso04_all, row.nMuon),
				  std::make_pair(row.Muon_tightId, row.nMuon),
				  std::make_pair(row.Muon_softId, row.nMuon),
				  std::make_pair(row.Muon_dxy, row.nMuon),
				  std::make_pair(row.Muon_dxyErr, row.nMuon),
				  std::make_pair(row.Muon_dz, row.nMuon),
				  std::make_pair(row.Muon_dzErr, row.nMuon),
				  std::make_pair(row.Muon_jetIdx, row.nMuon),
				  std::make_pair(row.Muon_genPartIdx, row.nMuon),
				  std::make_pair(row.Electron_pt, row.nElectron),
				  std::make_pair(row.Electron_eta, row.nElectron),
				  std::make_pair(row.Electron_phi, row.nElectron),
				  std::make_pair(row.Electron_mass, row.nElectron),
				  std::make_pair(row.Electron_charge, row.nElectron),
				  std::make_pair(row.Electron_pfRelIso03_all, row.nElectron),
				  std::make_pair(row.Electron_dxy, row.nElectron),
				  std::make_pair(row.Electron_dxyErr, row.nElectron),
				  std::make_pair(row.Electron_dz, row.nElectron),
				  std::make_pair(row.Electron_dzErr, row.nElectron),
				  std::make_pair(row.Electron_cutBasedId, row.nElectron),
				  std::make_pair(row.Electron_pfId, row.nElectron),
				  std::make_pair(row.Electron_jetIdx, row.nElectron),
				  std::make_pair(row.Electron_genPartIdx, row.nElectron),
				  std::make_pair(row.Tau_pt, row.nTau),
				  std::make_pair(row.Tau_eta, row.nTau),
				  std::make_pair(row.Tau_phi, row.nTau),
				  std::make_pair(row.Tau_mass, row.nTau),
				  std::make_pair(row.Tau_charge, row.nTau),
				  std::make_pair(row.Tau_decayMode, row.nTau),
				  std::make_pair(row.Tau_relIso_all, row.nTau),
				  std::make_pair(row.Tau_jetIdx, row.nTau),
				  std::make_pair(row.Tau_genPartIdx, row.nTau),
				  std::make_pair(row.Tau_idDecayMode, row.nTau),
				  std::make_pair(row.Tau_idIsoRaw, row.nTau),
				  std::make_pair(row.Tau_idIsoVLoose, row.nTau),
				  std::make_pair(row.Tau_idIsoLoose, row.nTau),
				  std::make_pair(row.Tau_idIsoMedium, row.nTau),
				  std::make_pair(row.Tau_idIsoTight, row.nTau),
				  std::make_pair(row.Tau_idAntiEleLoose, row.nTau),
				  std::make_pair(row.Tau_idAntiEleMedium, row.nTau),
				  std::make_pair(row.Tau_idAntiEleTight, row.nTau),
				  std::make_pair(row.Tau_idAntiMuLoose, row.nTau),
				  std::make_pair(row.Tau_idAntiMuMedium, row.nTau),
				  std::make_pair(row.Tau_idAntiMuTight, row.nTau),
				  row.MET_pt,
				  row.MET_phi,
				  row.MET_sumet,
				  row.MET_significance,
				  row.MET_CovXX,
				  row.MET_CovXY,
				  row.MET_CovYY,
				  std::make_pair(row.Jet_pt, row.nJet),
				  std::make_pair(row.Jet_eta, row.nJet),
				  std::make_pair(row.Jet_phi, row.nJet),
				  std::make_pair(row.Jet_mass, row.nJet),
				  std::make_pair(row.Jet_puId, row.nJet),
				  std::make_pair(row.Jet_btag, row.nJet),
				  std::make_pair(row.GenPart_pt, row.nGenPart),
				  std::make_pair(row.GenPart_eta, row.nGenPart),
				  std::make_pair(row.GenPart_phi, row.nGenPart),
				  std::make_pair(row.GenPart_mass, row.nGenPart),
				  std::make_pair(row.GenPart_pdgId, row.nGenPart),
				  std::make_pair(row.GenPart_status, row.nGenPart)
				  );

    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);
  return 0;
}
