#ifndef PARQUET_SCHEMA_HH_
#define PARQUET_SCHEMA_HH_

#include <memory>

#include <parquet/stream_reader.h>
#include <parquet/stream_writer.h>

#include "lhcb_event.h"

std::shared_ptr<parquet::schema::GroupNode> InitParquetSchema();

parquet::StreamWriter& operator<<(parquet::StreamWriter& os, const LhcbEvent& v)
{
  os << v.B_FlightDistance << v.B_VertexChi2
     << v.H1_PX << v.H1_PY << v.H1_PZ << v.H1_ProbK << v.H1_ProbPi << v.H1_Charge << v.H1_isMuon << v.H1_IpChi2
     << v.H2_PX << v.H2_PY << v.H2_PZ << v.H2_ProbK << v.H2_ProbPi << v.H2_Charge << v.H2_isMuon << v.H2_IpChi2
     << v.H3_PX << v.H3_PY << v.H3_PZ << v.H3_ProbK << v.H3_ProbPi << v.H3_Charge << v.H3_isMuon << v.H3_IpChi2
     << parquet::EndRow;
  return os;
}

parquet::StreamReader& operator>>(parquet::StreamReader& os, LhcbEvent& v)
{
#ifdef SKIP_UNUSED_COLUMNS
  os.SkipColumns(2);

  os >> v.H1_PX >> v.H1_PY >> v.H1_PZ >> v.H1_ProbK >> v.H1_ProbPi;
  os.SkipColumns(1);
  os >> v.H1_isMuon;
  os.SkipColumns(1);

  os >> v.H2_PX >> v.H2_PY >> v.H2_PZ >> v.H2_ProbK >> v.H2_ProbPi;
  os.SkipColumns(1);
  os >> v.H2_isMuon;
  os.SkipColumns(1);

  os >> v.H3_PX >> v.H3_PY >> v.H3_PZ >> v.H3_ProbK >> v.H3_ProbPi;
  os.SkipColumns(1);
  os >> v.H3_isMuon;
  os.SkipColumns(1);

  os.EndRow();
#else
  os >> v.B_FlightDistance >> v.B_VertexChi2
     >> v.H1_PX >> v.H1_PY >> v.H1_PZ >> v.H1_ProbK >> v.H1_ProbPi >> v.H1_Charge >> v.H1_isMuon >> v.H1_IpChi2
     >> v.H2_PX >> v.H2_PY >> v.H2_PZ >> v.H2_ProbK >> v.H2_ProbPi >> v.H2_Charge >> v.H2_isMuon >> v.H2_IpChi2
     >> v.H3_PX >> v.H3_PY >> v.H3_PZ >> v.H3_ProbK >> v.H3_ProbPi >> v.H3_Charge >> v.H3_isMuon >> v.H3_IpChi2
     >> parquet::EndRow;
#endif
  return os;
}

#endif /* PARQUET_SCHEMA_HH_ */
